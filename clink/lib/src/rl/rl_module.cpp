// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "rl_module.h"
#include "line_buffer.h"

#include <core/base.h>
#include <core/log.h>
#include <terminal/ecma48_iter.h>
#include <terminal/printer.h>
#include <terminal/terminal_in.h>

extern "C" {
#include <readline/readline.h>
#include <readline/rldefs.h>
#include <readline/xmalloc.h>
}

//------------------------------------------------------------------------------
static FILE*        null_stream = (FILE*)1;
void                show_rl_help(Printer&);
extern "C" int32    wcwidth(int32);
extern "C" char*    tgetstr(char*, char**);
static const int32  RL_MORE_INPUT_STATES = ~(
                        RL_STATE_CALLBACK|
                        RL_STATE_INITIALIZED|
                        RL_STATE_OVERWRITE|
                        RL_STATE_VICMDONCE);

extern "C" {
extern void         (*rl_fwrite_function)(FILE*, const char*, int32);
extern void         (*rl_fflush_function)(FILE*);
extern char*        _rl_comment_begin;
extern int32        _rl_convert_meta_chars_to_ascii;
extern int32        _rl_output_meta_chars;
#if defined(PLATFORM_WINDOWS)
extern int32        _rl_vis_botlin;
extern int32        _rl_last_c_pos;
extern int32        _rl_last_v_pos;
#endif
} // extern "C"



//------------------------------------------------------------------------------
static void load_user_inputrc()
{
#if defined(PLATFORM_WINDOWS)
    // Remember to update clink_info() if anything changes in here.

    const char* env_vars[] = {
        "clink_inputrc",
        "userprofile",
        "localappdata",
        "appdata",
        "home"
    };

    for (const char* env_var : env_vars)
    {
        Str<MAX_PATH> path;
        int32 path_length = GetEnvironmentVariable(env_var, path.data(), path.size());
        if (!path_length || path_length > int32(path.size()))
            continue;

        path << "\\.inputrc";

        for (int32 j = 0; j < 2; ++j)
        {
            if (!rl_read_init_file(path.c_str()))
            {
                static bool once = false;
                if (!once)
                {
                    once = true;
                    LOG("Found Readline inputrc at '%s'", path.c_str());
                }
                break;
            }

            int32 dot = path.last_of('.');
            if (dot >= 0)
                path.data()[dot] = '_';
        }
    }
#endif // PLATFORM_WINDOWS
}



//------------------------------------------------------------------------------
enum {
    bind_id_input,
    bind_id_more_input,
    bind_id_rl_help,
};



//------------------------------------------------------------------------------
static int32 terminal_read_thunk(FILE* stream)
{
    if (stream == null_stream)
        return 0;

    TerminalIn* term = (TerminalIn*)stream;
    return term->read();
}

//------------------------------------------------------------------------------
static void terminal_write_thunk(FILE* stream, const char* chars, int32 char_count)
{
    if (stream == stderr || stream == null_stream)
        return;

    Printer* printer = (Printer*)stream;
    printer->print(chars, char_count);
}



//------------------------------------------------------------------------------
RlModule::RlModule(const char* shell_name)
: _rl_buffer(nullptr)
, _prev_group(-1)
{
    rl_getc_function = terminal_read_thunk;
    rl_fwrite_function = terminal_write_thunk;
    rl_fflush_function = [] (FILE*) {};
    rl_instream = null_stream;
    rl_outstream = null_stream;

    rl_readline_name = shell_name;
    rl_catch_signals = 0;
    _rl_comment_begin = savestring("::"); // this will do...

    // Readline needs a tweak of it's handling of 'meta' (i.e. IO bytes >=0x80)
    // so that it handles UTF-8 correctly (convert=Input, output=output)
    _rl_convert_meta_chars_to_ascii = 0;
    _rl_output_meta_chars = 1;

    // Disable completion and match display.
    rl_completion_entry_function = [](const char*, int32) -> char* { return nullptr; };
    rl_completion_display_matches_hook = [](char**, int32, int32) {};

    // Bind extended keys so editing follows Windows' conventions.
    static const char* ext_key_binds[][2] = {
        { "\\e[1;5D", "backward-word" },           // ctrl-left
        { "\\e[1;5C", "forward-word" },            // ctrl-right
        { "\\e[F",    "end-of-line" },             // end
        { "\\e[H",    "beginning-of-line" },       // home
        { "\\e[3~",   "delete-char" },             // del
        { "\\e[1;5F", "kill-line" },               // ctrl-end
        { "\\e[1;5H", "backward-kill-line" },      // ctrl-home
        { "\\e[5~",   "history-search-backward" }, // pgup
        { "\\e[6~",   "history-search-forward" },  // pgdn
        { "\\e[M",    "revert-line" },             // esc
        { "\\C-z",    "undo" },
        { "\\C-w",    "unix-filename-rubout" },
    };

    for (int32 i = 0; i < sizeof_array(ext_key_binds); ++i)
        rl_bind_keyseq(ext_key_binds[i][0], rl_named_function(ext_key_binds[i][1]));

    load_user_inputrc();
}

//------------------------------------------------------------------------------
RlModule::~RlModule()
{
    free(_rl_comment_begin);
    _rl_comment_begin = nullptr;
}

//------------------------------------------------------------------------------
void RlModule::bind_input(Binder& binder)
{
    int32 default_group = binder.get_group();
    binder.bind(default_group, "", bind_id_input);
    binder.bind(default_group, "\\M-h", bind_id_rl_help);

    _catch_group = binder.create_group("readline");
    binder.bind(_catch_group, "", bind_id_more_input);
}

//------------------------------------------------------------------------------
void RlModule::on_begin_line(const Context& context)
{
    rl_outstream = (FILE*)(TerminalOut*)(&context.printer);

    // Readline needs to be told about parts of the prompt that aren't visible
    // by enclosing them in a pair of 0x01/0x02 chars.
    Str<128> rl_prompt;

    Ecma48State state;
    Ecma48Iter iter(context.prompt, state);
    while (const Ecma48Code& code = iter.next())
    {
        bool c1 = (code.get_type() == Ecma48Code::type_c1);
        if (c1) rl_prompt.concat("\x01", 1);
                rl_prompt.concat(code.get_pointer(), code.get_length());
        if (c1) rl_prompt.concat("\x02", 1);
    }

    auto handler = [] (char* line) { RlModule::get()->done(line); };
    rl_callback_handler_install(rl_prompt.c_str(), handler);

    _done = false;
    _eof = false;
    _prev_group = -1;
}

//------------------------------------------------------------------------------
void RlModule::on_end_line()
{
    if (_rl_buffer != nullptr)
    {
        rl_line_buffer = _rl_buffer;
        _rl_buffer = nullptr;
    }

    // This prevents any partial Readline state leaking from one line to the next
    rl_readline_state &= ~RL_MORE_INPUT_STATES;
}

//------------------------------------------------------------------------------
void RlModule::on_matches_changed(const Context& context)
{
}

//------------------------------------------------------------------------------
void RlModule::on_input(const Input& Input, Result& result, const Context& context)
{
    if (Input.id == bind_id_rl_help)
    {
        show_rl_help(context.printer);
        result.redraw();
        return;
    }

    // Setup the terminal.
    struct : public TerminalIn
    {
        virtual void    begin() override   {}
        virtual void    end() override     {}
        virtual void    select() override  {}
        virtual int32   read() override    { return *(uint8*)(data++); }
        const char*     data;
    } term_in;

    term_in.data = Input.keys;
    rl_instream = (FILE*)(&term_in);

    // Call Readline's until there's no characters left.
    int32 is_inc_searching = rl_readline_state & RL_STATE_ISEARCH;
    while (*term_in.data && !_done)
    {
        rl_callback_read_char();

        // Internally Readline tries to resend escape characters but it doesn't
        // work with how Clink uses Readline. So we do it here instead.
        if (term_in.data[-1] == 0x1b && is_inc_searching)
        {
            --term_in.data;
            is_inc_searching = 0;
        }
    }

    if (_done)
    {
        result.done(_eof);
        return;
    }

    // Check if Readline want's more Input or if we're done.
    if (rl_readline_state & RL_MORE_INPUT_STATES)
    {
        if (_prev_group < 0)
            _prev_group = result.set_bind_group(_catch_group);
    }
    else if (_prev_group >= 0)
    {
        result.set_bind_group(_prev_group);
        _prev_group = -1;
    }
}

//------------------------------------------------------------------------------
void RlModule::done(const char* line)
{
    _done = true;
    _eof = (line == nullptr);

    // Readline will reset the line state on returning from this call. Here we
    // trick it into reseting something else so we can use rl_line_buffer later.
    static char dummy_buffer = 0;
    _rl_buffer = rl_line_buffer;
    rl_line_buffer = &dummy_buffer;

    rl_callback_handler_remove();
}

//------------------------------------------------------------------------------
void RlModule::on_terminal_resize(int32 columns, int32 rows, const Context& context)
{
#if 1
    rl_reset_screen_size();
    rl_redisplay();
#else
    static int32 prev_columns = columns;

    int32 remaining = prev_columns;
    int32 line_count = 1;

    auto measure = [&] (const char* Input, int32 length) {
        Ecma48State state;
        Ecma48Iter iter(Input, state, length);
        while (const Ecma48Code& code = iter.next())
        {
            switch (code.get_type())
            {
            case Ecma48Code::type_chars:
                for (StrIter i(code.get_pointer(), code.get_length()); i.more(); )
                {
                    int32 n = wcwidth(i.next());
                    remaining -= n;
                    if (remaining > 0)
                        continue;

                    ++line_count;

                    remaining = prev_columns - ((remaining < 0) << 1);
                }
                break;

            case Ecma48Code::type_c0:
                switch (code.get_code())
                {
                case Ecma48Code::c0_lf:
                    ++line_count;
                    /* fallthrough */

                case Ecma48Code::c0_cr:
                    remaining = prev_columns;
                    break;

                case Ecma48Code::c0_ht:
                    if (int32 n = 8 - ((prev_columns - remaining) & 7))
                        remaining = max(remaining - n, 0);
                    break;

                case Ecma48Code::c0_bs:
                    remaining = min(remaining + 1, prev_columns); // doesn't consider full-width
                    break;
                }
                break;
            }
        }
    };

    measure(context.prompt, -1);

    const LineBuffer& buffer = context.buffer;
    const char* buffer_ptr = buffer.get_buffer();
    measure(buffer_ptr, buffer.get_cursor());
    int32 cursor_line = line_count - 1;

    buffer_ptr += buffer.get_cursor();
    measure(buffer_ptr, -1);

    static const char* const termcap_up    = tgetstr("ku", nullptr);
    static const char* const termcap_down  = tgetstr("kd", nullptr);
    static const char* const termcap_cr    = tgetstr("cr", nullptr);
    static const char* const termcap_clear = tgetstr("ce", nullptr);

    auto& printer = context.printer;

    // Move cursor to bottom line.
    for (int32 i = line_count - cursor_line; --i;)
        printer.print(termcap_down, 64);

    printer.print(termcap_cr, 64);
    do
    {
        printer.print(termcap_clear, 64);

        if (--line_count)
            printer.print(termcap_up, 64);
    }
    while (line_count);

    printer.print(context.prompt, 1024);
    printer.print(buffer.get_buffer(), 1024);

    prev_columns = columns;
#endif
}
