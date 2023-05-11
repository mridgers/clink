// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "win_terminal_in.h"

#include <core/base.h>
#include <core/settings.h>
#include <core/str.h>

#include <Windows.h>

//------------------------------------------------------------------------------
static SettingEnum g_escape_remap(
    "input.esc",
    "Remaps the escape key",
    "Changes how the escape key is interpreted. A value of 1 translates escape\n"
    "into a Ctrl-C. Option 2 reverts the line (standard Windows behaviour).",
    "raw,ctrl_c,revert_line",
    2);

//------------------------------------------------------------------------------
#define CSI(x) "\x1b[" #x
#define SS3(x) "\x1bO" #x
namespace terminfo { //                       Shf        Alt        AtlShf     Ctl        CtlShf     CtlAlt     CtlAltShf
static const char* const kcuu1[] = { CSI(A),  CSI(1;2A), CSI(1;3A), CSI(1;4A), CSI(1;5A), CSI(1;6A), CSI(1;7A), CSI(1;8A) }; // up
static const char* const kcud1[] = { CSI(B),  CSI(1;2B), CSI(1;3B), CSI(1;4B), CSI(1;5B), CSI(1;6B), CSI(1;7B), CSI(1;8B) }; // down
static const char* const kcub1[] = { CSI(D),  CSI(1;2D), CSI(1;3D), CSI(1;4D), CSI(1;5D), CSI(1;6D), CSI(1;7D), CSI(1;8D) }; // left
static const char* const kcuf1[] = { CSI(C),  CSI(1;2C), CSI(1;3C), CSI(1;4C), CSI(1;5C), CSI(1;6C), CSI(1;7C), CSI(1;8C) }; // right
static const char* const kich1[] = { CSI(2~), CSI(2;2~), CSI(2;3~), CSI(2;4~), CSI(2;5~), CSI(2;6~), CSI(2;7~), CSI(2;8~) }; // insert
static const char* const kdch1[] = { CSI(3~), CSI(3;2~), CSI(3;3~), CSI(3;4~), CSI(3;5~), CSI(3;6~), CSI(3;7~), CSI(3;8~) }; // delete
static const char* const khome[] = { CSI(H),  CSI(1;2H), CSI(1;3H), CSI(1;4H), CSI(1;5H), CSI(1;6H), CSI(1;7H), CSI(1;8H) }; // home
static const char* const kend[]  = { CSI(F),  CSI(1;2F), CSI(1;3F), CSI(1;4F), CSI(1;5F), CSI(1;6F), CSI(1;7F), CSI(1;8F) }; // end
static const char* const kpp[]   = { CSI(5~), CSI(5;2~), CSI(5;3~), CSI(5;4~), CSI(5;5~), CSI(5;6~), CSI(5;7~), CSI(5;8~) }; // pgup
static const char* const knp[]   = { CSI(6~), CSI(6;2~), CSI(6;3~), CSI(6;4~), CSI(6;5~), CSI(6;6~), CSI(6;7~), CSI(6;8~) }; // pgdn
static const char* const kcbt    = CSI(Z); // back tab key
static const char* const kdl1    = CSI(M); // delete line key
static const char* const kfx[]   = {
    // kf1-12 : Fx unmodified
    SS3(P),     SS3(Q),     SS3(R),     SS3(S),
    CSI(15~),   CSI(17~),   CSI(18~),   CSI(19~),
    CSI(20~),   CSI(21~),   CSI(23~),   CSI(24~),

    // kf13-24 : shift
    CSI(1;2P),  CSI(1;2Q),  CSI(1;2R),  CSI(1;2S),
    CSI(15;2~), CSI(17;2~), CSI(18;2~), CSI(19;2~),
    CSI(20;2~), CSI(21;2~), CSI(23;2~), CSI(24;2~),

    // kf25-36 : ctrl
    CSI(1;5P),  CSI(1;5Q),  CSI(1;5R),  CSI(1;5S),
    CSI(15;5~), CSI(17;5~), CSI(18;5~), CSI(19;5~),
    CSI(20;5~), CSI(21;5~), CSI(23;5~), CSI(24;5~),

    // kf37-48 : ctrl-shift
    CSI(1;6P),  CSI(1;6Q),  CSI(1;6R),  CSI(1;6S),
    CSI(15;6~), CSI(17;6~), CSI(18;6~), CSI(19;6~),
    CSI(20;6~), CSI(21;6~), CSI(23;6~), CSI(24;6~),
};
} // namespace terminfo
#undef SS3
#undef CSI



//------------------------------------------------------------------------------
enum : uint8
{
    input_abort_byte    = 0xff,
    input_none_byte     = 0xfe,
    input_timeout_byte  = 0xfd,
};



//------------------------------------------------------------------------------
static uint32 get_dimensions()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    auto cols = int16(csbi.dwSize.X);
    auto rows = int16(csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
    return (cols << 16) | rows;
}

//------------------------------------------------------------------------------
static void set_cursor_visibility(bool state)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(handle, &info);
    info.bVisible = BOOL(state);
    SetConsoleCursorInfo(handle, &info);
}

//------------------------------------------------------------------------------
static void adjust_cursor_on_resize(COORD prev_position)
{
    // Windows will move the cursor onto a new line when it gets clipped on
    // buffer resize. Other terminals clamp along the X axis.

    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(handle , &csbi);
    if (prev_position.X < csbi.dwSize.X)
        return;

    COORD fix_position = {
        int16(csbi.dwSize.X - 1),
        int16(csbi.dwCursorPosition.Y - 1)
    };
    SetConsoleCursorPosition(handle, fix_position);
}



//------------------------------------------------------------------------------
void WinTerminalIn::begin()
{
    _buffer_count = 0;
    _stdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD prev_mode;
    GetConsoleMode(_stdin, &prev_mode);
    _prev_mode = prev_mode;
    set_cursor_visibility(false);
}

//------------------------------------------------------------------------------
void WinTerminalIn::end()
{
    set_cursor_visibility(true);
    SetConsoleMode(_stdin, _prev_mode);
    _stdin = nullptr;
}

//------------------------------------------------------------------------------
void WinTerminalIn::select()
{
    if (!_buffer_count)
        read_console();
}

//------------------------------------------------------------------------------
int32 WinTerminalIn::read()
{
    uint32 dimensions = get_dimensions();
    if (dimensions != _dimensions)
    {
        _dimensions = dimensions;
        return TerminalIn::input_terminal_resize;
    }

    if (!_buffer_count)
        return TerminalIn::input_none;

    uint8 c = pop();
    switch (c)
    {
    case input_none_byte:       return TerminalIn::input_none;
    case input_timeout_byte:    return TerminalIn::input_timeout;
    case input_abort_byte:      return TerminalIn::input_abort;
    default:                    return c;
    }
}

//------------------------------------------------------------------------------
void WinTerminalIn::read_console()
{
    // Clear 'processed input' flag so key presses such as Ctrl-C and Ctrl-S
    // aren't swallowed. We also want events about window size changes.
    struct ModeScope {
        HANDLE  handle;
        DWORD   prev_mode;

        ModeScope(HANDLE handle) : handle(handle)
        {
            GetConsoleMode(handle, &prev_mode);
            SetConsoleMode(handle, ENABLE_WINDOW_INPUT);
        }

        ~ModeScope()
        {
            SetConsoleMode(handle, prev_mode);
        }
    } _ms(_stdin);

    // Hide the cursor unless we're accepting input so we don't have to see it
    // jump around as the screen's drawn.
    struct CursorScope {
        CursorScope()   { set_cursor_visibility(true); }
        ~CursorScope() { set_cursor_visibility(false); }
    } _cs;

    // Conhost restarts the cursor blink when writing to the console. It restarts
    // hidden which means that if you Type faster than the blink the cursor turns
    // invisible. Fortunately, moving the cursor restarts the blink on visible.
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(stdout_handle, &csbi);
    SetConsoleCursorPosition(stdout_handle, csbi.dwCursorPosition);

    // Read input records sent from the terminal (aka conhost) until some
    // input has beeen buffered.
    uint32 buffer_count = _buffer_count;
    while (buffer_count == _buffer_count)
    {
        DWORD count;
        INPUT_RECORD record;
        if (!ReadConsoleInputW(_stdin, &record, 1, &count))
        {
            // Handle's probably invalid if ReadConsoleInput() failed.
            _buffer_count = 1;
            _buffer[0] = input_abort_byte;
            return;
        }

        switch (record.EventType)
        {
        case KEY_EVENT:
            {
                auto& key_event = record.Event.KeyEvent;

                // Some times conhost can send through ALT codes, with the
                // resulting Unicode code point in the Alt key-up event.
                if (!key_event.bKeyDown
                    && key_event.wVirtualKeyCode == VK_MENU
                    && key_event.uChar.UnicodeChar)
                {
                    key_event.bKeyDown = TRUE;
                    key_event.dwControlKeyState = 0;
                }

                if (key_event.bKeyDown)
                    process_input(key_event);
            }
            break;

        case WINDOW_BUFFER_SIZE_EVENT:
            // Windows will move the cursor onto a new line when it gets clipped
            // on buffer resize. Other terminals
            adjust_cursor_on_resize(csbi.dwCursorPosition);
            return;
        }
    }
}

//------------------------------------------------------------------------------
void WinTerminalIn::process_input(KEY_EVENT_RECORD const& record)
{
    static const int32 CTRL_PRESSED = LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED;
    static const int32 ALT_PRESSED = LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED;

    int32 key_char = record.uChar.UnicodeChar;
    int32 key_vk = record.wVirtualKeyCode;
    int32 key_sc = record.wVirtualScanCode;
    int32 key_flags = record.dwControlKeyState;

    // We filter out Alt key presses unless they generated a character.
    if (key_vk == VK_MENU)
    {
        if (key_char)
            push(key_char);

        return;
    }

    // Early out of unaccompanied ctrl/shift key presses.
    if (key_vk == VK_CONTROL || key_vk == VK_SHIFT)
        return;

    // If the input was formed using AltGr or LeftAlt-LeftCtrl then things get
    // tricky. But there's always a Ctrl bit set, even if the user didn't press
    // a ctrl key. We can use this and the knowledge that Ctrl-modified keys
    // aren't printable to clear appropriate AltGr Flags.
    if (key_char > 0x1f && (key_flags & CTRL_PRESSED))
    {
        key_flags &= ~CTRL_PRESSED;
        if (key_flags & RIGHT_ALT_PRESSED)
            key_flags &= ~RIGHT_ALT_PRESSED;
        else
            key_flags &= ~LEFT_ALT_PRESSED;
    }

    // Special case for shift-tab (aka. back-tab or kcbt).
    if (key_char == '\t' && !_buffer_count && (key_flags & SHIFT_PRESSED))
        return push(terminfo::kcbt);

    // Function keys (kf1-kf48 from xterm+pcf2)
    uint32 key_func = key_vk - VK_F1;
    if (key_func <= (VK_F12 - VK_F1))
    {
        if (key_flags & ALT_PRESSED)
            push(0x1b);

        int32 kfx_group = !!(key_flags & SHIFT_PRESSED);
        kfx_group |= !!(key_flags & CTRL_PRESSED) << 1;
        push((terminfo::kfx + (12 * kfx_group) + key_func)[0]);

        return;
    }

    // Turn ESC into a "delete line" key press
    if (key_vk == VK_ESCAPE)
    {
        switch (g_escape_remap.get())
        {
        case 1:  return push("\x03"); // ctrl-c
        case 2:  return push(terminfo::kdl1);
        default: break;
        }
    }

    // Include an ESC character in the input stream if Alt is pressed.
    if (key_char)
    {
        if (key_flags & ALT_PRESSED)
            push(0x1b);

        return push(key_char);
    }

    // The numpad keys such as PgUp, End, etc. don't come through with the
    // ENHANCED_KEY flag set so we'll infer it here.
    static const int32 enhanced_vks[] = {
        VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_HOME, VK_END,
        VK_INSERT, VK_DELETE, VK_PRIOR, VK_NEXT,
    };

    for (int32 i = 0; i < sizeof_array(enhanced_vks); ++i)
    {
        if (key_vk == enhanced_vks[i])
        {
            key_flags |= ENHANCED_KEY;
            break;
        }
    }

    // Convert enhanced keys to normal mode xterm compatible escape sequences.
    if (key_flags & ENHANCED_KEY)
    {
        static const struct {
            int32               code;
            const char* const*  seqs;
        } sc_map[] = {
            { 'H', terminfo::kcuu1, }, // up
            { 'P', terminfo::kcud1, }, // down
            { 'K', terminfo::kcub1, }, // left
            { 'M', terminfo::kcuf1, }, // right
            { 'R', terminfo::kich1, }, // insert
            { 'S', terminfo::kdch1, }, // delete
            { 'G', terminfo::khome, }, // home
            { 'O', terminfo::kend, },  // end
            { 'I', terminfo::kpp, },   // pgup
            { 'Q', terminfo::knp, },   // pgdn
        };

        // Calculate Xterm's modifier number.
        int32 i = 0;
        i |= !!(key_flags & SHIFT_PRESSED);
        i |= !!(key_flags & ALT_PRESSED) << 1;
        i |= !!(key_flags & CTRL_PRESSED) << 2;

        for (const auto& iter : sc_map)
        {
            if (iter.code != key_sc)
                continue;

            push(iter.seqs[i]);
            break;
        }

        return;
    }

    // This builds Ctrl-<key> c0 codes. Some of these actually come though in
    // key_char and some don't.
    if (key_flags & CTRL_PRESSED)
    {
        #define CONTAINS(l, r) (uint32)(key_vk - l) <= (r - l)
             if (CONTAINS('A', 'Z'))    key_vk -= 'A' - 1;
        else if (CONTAINS(0xdb, 0xdd))  key_vk -= 0xdb - 0x1b;
        else if (key_vk == 0x32)        key_vk = 0;
        else if (key_vk == 0x36)        key_vk = 0x1e;
        else if (key_vk == 0xbd)        key_vk = 0x1f;
        else                            return;
        #undef CONTAINS

        if (key_flags & ALT_PRESSED)
            push(0x1b);

        push(key_vk);
    }
}

//------------------------------------------------------------------------------
void WinTerminalIn::push(const char* seq)
{
    static const uint32 mask = sizeof_array(_buffer) - 1;

    if (_buffer_count >= sizeof_array(_buffer))
        return;

    int32 index = _buffer_head + _buffer_count;
    for (; _buffer_count <= mask && *seq; ++_buffer_count, ++index, ++seq)
        _buffer[index & mask] = *seq;
}

//------------------------------------------------------------------------------
void WinTerminalIn::push(uint32 value)
{
    static const uint32 mask = sizeof_array(_buffer) - 1;

    if (_buffer_count >= sizeof_array(_buffer))
        return;

    int32 index = _buffer_head + _buffer_count;

    if (value < 0x80)
    {
        _buffer[index & mask] = value;
        ++_buffer_count;
        return;
    }

    wchar_t wc[2] = { (wchar_t)value, 0 };
    char utf8[mask + 1];
    uint32 n = to_utf8(utf8, sizeof_array(utf8), wc);
    if (n <= uint32(mask - _buffer_count))
        for (uint32 i = 0; i < n; ++i, ++index)
            _buffer[index & mask] = utf8[i];

    _buffer_count += n;
}

//------------------------------------------------------------------------------
uint8 WinTerminalIn::pop()
{
    if (!_buffer_count)
        return input_none_byte;

    uint8 value = _buffer[_buffer_head];

    --_buffer_count;
    _buffer_head = (_buffer_head + 1) & (sizeof_array(_buffer) - 1);

    return value;
}
