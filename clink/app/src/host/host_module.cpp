// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "host_module.h"

#include <core/path.h>
#include <core/settings.h>
#include <core/str.h>
#include <lib/line_buffer.h>
#include <terminal/printer.h>

//------------------------------------------------------------------------------
static SettingEnum g_paste_crlf(
    "clink.paste_crlf",
    "Strips CR and LF chars on paste",
    "Setting this to a value >0 will make Clink strip CR and LF characters\n"
    "from text pasted into the current line. Set this to 'delete' to strip all\n"
    "newline characters to replace them with a space.",
    "delete,space",
    1);

static SettingStr g_key_paste(
    "keybind.paste",
    "Inserts clipboard contents",
    "^v");

static SettingStr g_key_ctrlc(
    "keybind.ctrlc",
    "Cancels current line edit",
    "^c");

static SettingStr g_key_copy_line(
    "keybind.copy_line",
    "Copies line to clipboard",
    "\\M-C-c");

static SettingStr g_key_copy_cwd(
    "keybind.copy_cwd",
    "Copies current directory",
    "\\M-C");

static SettingStr g_key_up_dir(
    "keybind.up_dir",
    "Goes up a directory (default = Ctrl-PgUp)",
    "\\e[5;5~");

static SettingStr g_key_dotdot(
    "keybind.dotdot",
    "Inserts ..\\",
    "\\M-a");

static SettingStr g_key_expand_env(
    "keybind.expand_env",
    "Expands %envvar% under cursor",
    "\\M-C-e");



//------------------------------------------------------------------------------
static void ctrl_c(
    EditorModule::Result& result,
    const EditorModule::Context& context)
{
    context.buffer.reset();
    context.printer.print("\n^C\n");
    result.redraw();
}

//------------------------------------------------------------------------------
static void strip_crlf(char* line)
{
    int32 setting = g_paste_crlf.get();

    int32 prev_was_crlf = 0;
    char* write = line;
    const char* read = line;
    while (*read)
    {
        char c = *read;
        if (c != '\n' && c != '\r')
        {
            prev_was_crlf = 0;
            *write = c;
            ++write;
        }
        else if (setting > 0 && !prev_was_crlf)
        {
            prev_was_crlf = 1;
            *write = ' ';
            ++write;
        }

        ++read;
    }

    *write = '\0';
}

//------------------------------------------------------------------------------
static void paste(LineBuffer& buffer)
{
    if (OpenClipboard(nullptr) == FALSE)
        return;

    HANDLE clip_data = GetClipboardData(CF_UNICODETEXT);
    if (clip_data != nullptr)
    {
        Str<1024> utf8;
        to_utf8(utf8, (wchar_t*)clip_data);

        strip_crlf(utf8.data());
        buffer.insert(utf8.c_str());
    }

    CloseClipboard();
}

//------------------------------------------------------------------------------
static void copy_impl(const char* value, int32 length)
{
    int32 size = (length + 4) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (mem == nullptr)
        return;

    wchar_t* data = (wchar_t*)GlobalLock(mem);
    length = to_utf16((wchar_t*)data, length + 1, value);
    GlobalUnlock(mem);

    if (OpenClipboard(nullptr) == FALSE)
        return;

    SetClipboardData(CF_TEXT, nullptr);
    SetClipboardData(CF_UNICODETEXT, mem);
    CloseClipboard();
}

//------------------------------------------------------------------------------
static void copy_line(const LineBuffer& buffer)
{
    copy_impl(buffer.get_buffer(), buffer.get_length());
}

//------------------------------------------------------------------------------
static void copy_cwd(const LineBuffer& buffer)
{
    Str<270> cwd;
    uint32 length = GetCurrentDirectory(cwd.size(), cwd.data());
    if (length < cwd.size())
    {
        cwd << "\\";
        path::normalise(cwd);
        copy_impl(cwd.c_str(), cwd.length());
    }
}

//------------------------------------------------------------------------------
static void up_directory(EditorModule::Result& result, LineBuffer& buffer)
{
    buffer.begin_undo_group();
    buffer.remove(0, ~0u);
    buffer.insert(" cd ..");
    buffer.end_undo_group();
    result.done();
}

//------------------------------------------------------------------------------
static void get_word_bounds(const LineBuffer& buffer, int32* left, int32* right)
{
    const char* str = buffer.get_buffer();
    uint32 cursor = buffer.get_cursor();

    // Determine the word delimiter depending on whether the word's quoted.
    int32 delim = 0;
    for (uint32 i = 0; i < cursor; ++i)
    {
        char c = str[i];
        delim += (c == '\"');
    }

    // Search outwards from the cursor for the delimiter.
    delim = (delim & 1) ? '\"' : ' ';
    *left = 0;
    for (int32 i = cursor - 1; i >= 0; --i)
    {
        char c = str[i];
        if (c == delim)
        {
            *left = i + 1;
            break;
        }
    }

    const char* post = strchr(str + cursor, delim);
    if (post != nullptr)
        *right = int32(post - str);
    else
        *right = int32(strlen(str));
}

//------------------------------------------------------------------------------
static void expand_env_vars(LineBuffer& buffer)
{
    // Extract the word under the cursor.
    int32 word_left, word_right;
    get_word_bounds(buffer, &word_left, &word_right);

    Str<1024> in;
    in << (buffer.get_buffer() + word_left);
    in.truncate(word_right - word_left);

    // Do the environment variable expansion.
    Str<1024> out;
    if (!ExpandEnvironmentStrings(in.c_str(), out.data(), out.size()))
        return;

    // Update Readline with the resulting expansion.
    buffer.begin_undo_group();
    buffer.remove(word_left, word_right);
    buffer.set_cursor(word_left);
    buffer.insert(out.c_str());
    buffer.end_undo_group();
}

//------------------------------------------------------------------------------
static void insert_dot_dot(LineBuffer& buffer)
{
    if (uint32 cursor = buffer.get_cursor())
    {
        char last_char = buffer.get_buffer()[cursor - 1];
        if (last_char != ' ' && !path::is_separator(last_char))
            buffer.insert("\\");
    }

    buffer.insert("..\\");
}



//------------------------------------------------------------------------------
enum
{
    bind_id_paste,
    bind_id_ctrlc,
    bind_id_copy_line,
    bind_id_copy_cwd,
    bind_id_up_dir,
    bind_id_expand_env,
    bind_id_dotdot,
};



//------------------------------------------------------------------------------
HostModule::HostModule(const char* host_name)
: _host_name(host_name)
{
}

//------------------------------------------------------------------------------
void HostModule::bind_input(Binder& binder)
{
    int32 default_group = binder.get_group();
    binder.bind(default_group, g_key_paste.get(), bind_id_paste);
    binder.bind(default_group, g_key_ctrlc.get(), bind_id_ctrlc);
    binder.bind(default_group, g_key_copy_line.get(), bind_id_copy_line);
    binder.bind(default_group, g_key_copy_cwd.get(), bind_id_copy_cwd);
    binder.bind(default_group, g_key_up_dir.get(), bind_id_up_dir);
    binder.bind(default_group, g_key_dotdot.get(), bind_id_dotdot);

    if (stricmp(_host_name, "cmd.exe") == 0)
        binder.bind(default_group, g_key_expand_env.get(), bind_id_expand_env);
}

//------------------------------------------------------------------------------
void HostModule::on_begin_line(const Context& context)
{
}

//------------------------------------------------------------------------------
void HostModule::on_end_line()
{
}

//------------------------------------------------------------------------------
void HostModule::on_matches_changed(const Context& context)
{
}

//------------------------------------------------------------------------------
void HostModule::on_input(const Input& Input, Result& result, const Context& context)
{
    switch (Input.id)
    {
    case bind_id_paste:         paste(context.buffer);                break;
    case bind_id_ctrlc:         ctrl_c(result, context);              break;
    case bind_id_copy_line:     copy_line(context.buffer);            break;
    case bind_id_copy_cwd:      copy_cwd(context.buffer);             break;
    case bind_id_up_dir:        up_directory(result, context.buffer); break;
    case bind_id_expand_env:    expand_env_vars(context.buffer);      break;
    case bind_id_dotdot:        insert_dot_dot(context.buffer);       break;
    };
}

//------------------------------------------------------------------------------
void HostModule::on_terminal_resize(int32 columns, int32 rows, const Context& context)
{
}
