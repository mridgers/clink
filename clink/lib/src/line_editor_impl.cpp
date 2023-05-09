// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "line_editor_impl.h"
#include "line_buffer.h"
#include "match_generator.h"
#include "match_pipeline.h"

#include <core/base.h>
#include <core/os.h>
#include <core/path.h>
#include <core/str_iter.h>
#include <core/str_tokeniser.h>
#include <terminal/terminal_in.h>
#include <terminal/terminal_out.h>

//------------------------------------------------------------------------------
inline char get_closing_quote(const char* quote_pair)
{
    return quote_pair[1] ? quote_pair[1] : quote_pair[0];
}



//------------------------------------------------------------------------------
LineEditor* line_editor_create(const LineEditor::Desc& desc)
{
    // Check there's at least a Terminal.
    if (desc.input == nullptr)
        return nullptr;

    if (desc.output == nullptr)
        return nullptr;

    return new LineEditorImpl(desc);
}

//------------------------------------------------------------------------------
void line_editor_destroy(LineEditor* editor)
{
    delete editor;
}



//------------------------------------------------------------------------------
LineEditorImpl::LineEditorImpl(const Desc& desc)
: m_module(desc.shell_name)
, m_desc(desc)
, m_printer(*desc.output)
{
    if (m_desc.quote_pair == nullptr)
        m_desc.quote_pair = "";

    add_module(m_module);
}

//------------------------------------------------------------------------------
void LineEditorImpl::initialise()
{
    if (check_flag(flag_init))
        return;

    struct : public EditorModule::Binder {
        virtual int get_group(const char* name) const override
        {
            return binder->get_group(name);
        }

        virtual int create_group(const char* name) override
        {
            return binder->create_group(name);
        }

        virtual bool bind(unsigned int group, const char* chord, unsigned char key) override
        {
            return binder->bind(group, chord, *module, key);
        }

        ::Binder*       binder;
        EditorModule*   module;
    } binder_impl;

    binder_impl.binder = &m_binder;
    for (auto* module : m_modules)
    {
        binder_impl.module = module;
        module->bind_input(binder_impl);
    }

    set_flag(flag_init);
}

//------------------------------------------------------------------------------
void LineEditorImpl::begin_line()
{
    clear_flag(~flag_init);
    set_flag(flag_editing);

    m_bind_resolver.reset();
    m_command_offset = 0;
    m_keys_size = 0;
    m_prev_key = ~0u;

    MatchPipeline pipeline(m_matches);
    pipeline.reset();

    m_desc.input->begin();
    m_desc.output->begin();
    m_buffer.begin_line();

    LineState line = get_linestate();
    EditorModule::Context context = get_context(line);
    for (auto module : m_modules)
        module->on_begin_line(context);
}

//------------------------------------------------------------------------------
void LineEditorImpl::end_line()
{
    for (auto i = m_modules.rbegin(), n = m_modules.rend(); i != n; ++i)
        i->on_end_line();

    m_buffer.end_line();
    m_desc.output->end();
    m_desc.input->end();

    clear_flag(flag_editing);
}

//------------------------------------------------------------------------------
bool LineEditorImpl::add_module(EditorModule& module)
{
    EditorModule** slot = m_modules.push_back();
    return (slot != nullptr) ? *slot = &module, true : false;
}

//------------------------------------------------------------------------------
bool LineEditorImpl::add_generator(MatchGenerator& generator)
{
    MatchGenerator** slot = m_generators.push_back();
    return (slot != nullptr) ? *slot = &generator, true : false;
}

//------------------------------------------------------------------------------
bool LineEditorImpl::get_line(char* out, int out_size)
{
    if (check_flag(flag_editing))
        end_line();

    if (check_flag(flag_eof))
        return false;

    const char* line = m_buffer.get_buffer();
    StrBase(out, out_size).copy(line);
    return true;
}

//------------------------------------------------------------------------------
bool LineEditorImpl::edit(char* out, int out_size)
{
    // Update first so the init state goes through.
    while (update())
        m_desc.input->select();

    return get_line(out, out_size);
}

//------------------------------------------------------------------------------
bool LineEditorImpl::update()
{
    if (!check_flag(flag_init))
        initialise();

    if (!check_flag(flag_editing))
    {
        begin_line();
        update_internal();
        return true;
    }

    update_input();

    if (!check_flag(flag_editing))
        return false;

    update_internal();
    return true;
}

//------------------------------------------------------------------------------
void LineEditorImpl::update_input()
{
    int key = m_desc.input->read();

    if (key == TerminalIn::input_terminal_resize)
    {
        int columns = m_desc.output->get_columns();
        int rows = m_desc.output->get_rows();
        LineState line = get_linestate();
        EditorModule::Context context = get_context(line);
        for (auto* module : m_modules)
            module->on_terminal_resize(columns, rows, context);
    }

    if (key == TerminalIn::input_abort)
    {
        m_buffer.reset();
        end_line();
        return;
    }

    if (key < 0)
        return;

    if (!m_bind_resolver.step(key))
        return;

    struct ResultImpl : public EditorModule::Result
    {
        enum
        {
            flag_pass       = 1 << 0,
            flag_done       = 1 << 1,
            flag_eof        = 1 << 2,
            flag_redraw     = 1 << 3,
            flag_append_lcd = 1 << 4,
        };

        virtual void    pass() override                           { flags |= flag_pass; }
        virtual void    done(bool eof) override                   { flags |= flag_done|(eof ? flag_eof : 0); }
        virtual void    redraw() override                         { flags |= flag_redraw; }
        virtual void    append_match_lcd() override               { flags |= flag_append_lcd; }
        virtual void    accept_match(unsigned int index) override { match = index; }
        virtual int     set_bind_group(int id) override           { int t = group; group = id; return t; }
        int             match;  // = -1;  <!
        unsigned short  group;  //        <! MSVC bugs; see connect
        unsigned char   flags;  // = 0;   <! issues about C2905
    };

    while (auto Binding = m_bind_resolver.next())
    {
        // Binding found, dispatch it off to the module.
        ResultImpl result;
        result.match = -1;
        result.flags = 0;
        result.group = m_bind_resolver.get_group();

        Str<16> chord;
        EditorModule* module = Binding.get_module();
        unsigned char id = Binding.get_id();
        Binding.get_chord(chord);

        LineState line = get_linestate();
        EditorModule::Context context = get_context(line);
        EditorModule::Input input = { chord.c_str(), id };
        module->on_input(input, result, context);

        m_bind_resolver.set_group(result.group);

        // Process what ResultImpl has collected.
        if (result.flags & ResultImpl::flag_pass)
            continue;

        Binding.claim();

        if (result.flags & ResultImpl::flag_done)
        {
            end_line();

            if (result.flags & ResultImpl::flag_eof)
                set_flag(flag_eof);
        }

        if (!check_flag(flag_editing))
            return;

        if (result.flags & ResultImpl::flag_redraw)
            m_buffer.redraw();

        if (result.match >= 0)
            accept_match(result.match);
        else if (result.flags & ResultImpl::flag_append_lcd)
            append_match_lcd();
    }

    m_buffer.draw();
}

//------------------------------------------------------------------------------
void LineEditorImpl::find_command_bounds(const char*& start, int& length)
{
    const char* line_buffer = m_buffer.get_buffer();
    unsigned int line_cursor = m_buffer.get_cursor();

    start = line_buffer;
    length = line_cursor;

    if (m_desc.command_delims == nullptr)
        return;

    StrIter token_iter(start, length);
    StrTokeniser tokens(token_iter, m_desc.command_delims);
    tokens.add_quote_pair(m_desc.quote_pair);
    while (tokens.next(start, length));

    // We should expect to reach the cursor. If not then there's a trailing
    // separator and we'll just say the command starts at the cursor.
    if (start + length != line_buffer + line_cursor)
    {
        start = line_buffer + line_cursor;
        length = 0;
    }
}

//------------------------------------------------------------------------------
void LineEditorImpl::collect_words()
{
    m_words.clear();

    const char* line_buffer = m_buffer.get_buffer();
    unsigned int line_cursor = m_buffer.get_cursor();

    const char* command_start;
    int command_length;
    find_command_bounds(command_start, command_length);

    m_command_offset = int(command_start - line_buffer);

    StrIter token_iter(command_start, command_length);
    StrTokeniser tokens(token_iter, m_desc.word_delims);
    tokens.add_quote_pair(m_desc.quote_pair);
    while (1)
    {
        int length = 0;
        const char* start = nullptr;
        StrToken token = tokens.next(start, length);
        if (!token)
            break;

        // Add the word.
        unsigned int offset = unsigned(start - line_buffer);
        m_words.push_back();
        *(m_words.back()) = { offset, unsigned(length), 0, token.delim };
    }

    // Add an empty word if the cursor is at the beginning of one.
    Word* end_word = m_words.back();
    if (!end_word || end_word->offset + end_word->length < line_cursor)
    {
        unsigned char delim = 0;
        if (line_cursor)
            delim = line_buffer[line_cursor - 1];

        m_words.push_back();
        *(m_words.back()) = { line_cursor, 0, 0, delim };
    }

    // Adjust for quotes.
    for (Word& word : m_words)
    {
        if (word.length == 0)
            continue;

        const char* start = line_buffer + word.offset;

        int start_quoted = (start[0] == m_desc.quote_pair[0]);
        int end_quoted = 0;
        if (word.length > 1)
            end_quoted = (start[word.length - 1] == get_closing_quote(m_desc.quote_pair));

        word.offset += start_quoted;
        word.length -= start_quoted + end_quoted;
        word.quoted = !!start_quoted;
    }

    // The last word is truncated to the longest length returned by the match
    // generators. This is a little clunky but works well enough.
    LineState line = get_linestate();
    end_word = m_words.back();
    int prefix_length = 0;
    const char* word_start = line_buffer + end_word->offset;
    for (const auto* generator : m_generators)
    {
        int i = generator->get_prefix_length(line);
        prefix_length = max(prefix_length, i);
    }
    end_word->length = min<unsigned int>(prefix_length, end_word->length);
}

//------------------------------------------------------------------------------
void LineEditorImpl::accept_match(unsigned int index)
{
    if (index >= m_matches.get_match_count())
        return;

    const char* match = m_matches.get_match(index);
    if (!*match)
        return;

    Word end_word = *(m_words.back());
    int word_start = end_word.offset;
    int word_end = end_word.offset + end_word.length;

    const char* buf_ptr = m_buffer.get_buffer();

    Str<288> to_insert;
    if (!m_matches.is_prefix_included())
        to_insert.concat(buf_ptr + word_start, end_word.length);
    to_insert << match;

    // TODO: This has not place here and should be done somewhere else.
    // Clean the word if it is a valid file system path.
    if (os::get_path_type(to_insert.c_str()) != os::path_type_invalid)
        path::normalise(to_insert);

    // Does the selected match need quoting?
    bool needs_quote = end_word.quoted;
    for (const char* c = match; *c && !needs_quote; ++c)
        needs_quote = (strchr(m_desc.word_delims, *c) != nullptr);

    // Clear the word.
    m_buffer.remove(word_start, m_buffer.get_cursor());
    m_buffer.set_cursor(word_start);

    // Read the word plus the match.
    if (needs_quote && !end_word.quoted)
    {
        char quote[2] = { m_desc.quote_pair[0] };
        m_buffer.insert(quote);
    }
    m_buffer.insert(to_insert.c_str());

    // Use a suffix if one's associated with the match, otherwise derive it.
    char match_suffix = m_matches.get_suffix(index);
    char suffix = match_suffix;
    if (!suffix)
    {
        unsigned int match_length = unsigned(strlen(match));

        Word match_word = { 0, match_length };
        Array<Word> match_words(&match_word, 1);
        LineState match_line = { match, match_length, 0, match_words };

        int prefix_length = 0;
        for (const auto* generator : m_generators)
        {
            int i = generator->get_prefix_length(match_line);
            prefix_length = max(prefix_length, i);
        }

        if (prefix_length != match_length)
            suffix = m_desc.word_delims[0];
    }

    // If this match doesn't make a new partial word, close it off
    if (suffix)
    {
        // Add a closing quote on the end if required and only if the suffix
        // did not come from the match.
        if (needs_quote && !match_suffix)
        {
            char quote[2] = { get_closing_quote(m_desc.quote_pair) };
            m_buffer.insert(quote);
        }

        // Just move the cursor if the suffix will duplicate the character under
        // the cursor.
        unsigned int cursor = m_buffer.get_cursor();
        if (cursor >= m_buffer.get_length() || m_buffer.get_buffer()[cursor] != suffix)
        {
            char suffix_str[2] = { suffix };
            m_buffer.insert(suffix_str);
        }
        else
            m_buffer.set_cursor(cursor + 1);
    }
}

//------------------------------------------------------------------------------
void LineEditorImpl::append_match_lcd()
{
    Str<288> lcd;
    m_matches.get_match_lcd(lcd);

    unsigned int lcd_length = lcd.length();
    if (!lcd_length)
        return;

    unsigned int cursor = m_buffer.get_cursor();

    Word end_word = *(m_words.back());
    int word_end = end_word.offset;
    if (!m_matches.is_prefix_included())
        word_end += end_word.length;

    int dx = lcd_length - (cursor - word_end);
    if (dx < 0)
    {
        m_buffer.remove(cursor + dx, cursor);
        m_buffer.set_cursor(cursor + dx);
    }
    else if (dx > 0)
    {
        int start = end_word.offset;
        if (!m_matches.is_prefix_included())
            start += end_word.length;

        m_buffer.remove(start, cursor);
        m_buffer.set_cursor(start);
        m_buffer.insert(lcd.c_str());
    }

    // Prefix a quote if required.
    bool needs_quote = false;
    for (const char* c = lcd.c_str(); *c && !needs_quote; ++c)
        needs_quote = (strchr(m_desc.word_delims, *c) != nullptr);

    for (int i = 0, n = m_matches.get_match_count(); i < n && !needs_quote; ++i)
    {
        const char* match = m_matches.get_match(i) + lcd_length;
        if (match[0])
            needs_quote = (strchr(m_desc.word_delims, match[0]) != nullptr);
    }

    if (needs_quote && !end_word.quoted)
    {
        char quote[2] = { m_desc.quote_pair[0] };
        int cursor = m_buffer.get_cursor();
        m_buffer.set_cursor(end_word.offset);
        m_buffer.insert(quote);
        m_buffer.set_cursor(cursor + 1);
    }
}

//------------------------------------------------------------------------------
LineState LineEditorImpl::get_linestate() const
{
    return {
        m_buffer.get_buffer(),
        m_buffer.get_cursor(),
        m_command_offset,
        m_words,
    };
}

//------------------------------------------------------------------------------
EditorModule::Context LineEditorImpl::get_context(const LineState& line) const
{
    auto& buffer = const_cast<RlBuffer&>(m_buffer);
    auto& pter = const_cast<Printer&>(m_printer);
    return { m_desc.prompt, pter, buffer, line, m_matches };
}

//------------------------------------------------------------------------------
void LineEditorImpl::set_flag(unsigned char flag)
{
    m_flags |= flag;
}

//------------------------------------------------------------------------------
void LineEditorImpl::clear_flag(unsigned char flag)
{
    m_flags &= ~flag;
}

//------------------------------------------------------------------------------
bool LineEditorImpl::check_flag(unsigned char flag) const
{
    return ((m_flags & flag) != 0);
}

//------------------------------------------------------------------------------
void LineEditorImpl::update_internal()
{
    collect_words();

    const Word& end_word = *(m_words.back());

    union key_t {
        struct {
            unsigned int word_offset : 11;
            unsigned int word_length : 10;
            unsigned int cursor_pos  : 11;
        };
        unsigned int value;
    };

    key_t next_key = { end_word.offset, end_word.length };

    key_t prev_key;
    prev_key.value = m_prev_key;
    prev_key.cursor_pos = 0;

    // Should we generate new matches?
    if (next_key.value != prev_key.value)
    {
        LineState line = get_linestate();
        MatchPipeline pipeline(m_matches);
        pipeline.reset();
        pipeline.generate(line, m_generators);
        pipeline.fill_info();
    }

    next_key.cursor_pos = m_buffer.get_cursor();
    prev_key.value = m_prev_key;

    // Should we sort and select matches?
    if (next_key.value != prev_key.value)
    {
        Str<64> needle;
        int needle_start = end_word.offset;
        if (!m_matches.is_prefix_included())
            needle_start += end_word.length;

        const char* buf_ptr = m_buffer.get_buffer();
        needle.concat(buf_ptr + needle_start, next_key.cursor_pos - needle_start);

        if (!needle.empty() && end_word.quoted)
        {
            int i = needle.length();
            if (needle[i - 1] == get_closing_quote(m_desc.quote_pair))
                needle.truncate(i - 1);
        }

        MatchPipeline pipeline(m_matches);
        pipeline.select(needle.c_str());
        pipeline.sort();

        m_prev_key = next_key.value;

        // Tell all the modules that the matches changed.
        LineState line = get_linestate();
        EditorModule::Context context = get_context(line);
        for (auto module : m_modules)
            module->on_matches_changed(context);
    }
}
