// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "win_terminal_out.h"

#include <core/base.h>
#include <core/str_iter.h>

#include <Windows.h>

//------------------------------------------------------------------------------
void WinTerminalOut::begin()
{
    DWORD prev_mode;
    _stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(_stdout, &prev_mode);
    _prev_mode = prev_mode;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_stdout, &csbi);
    _default_attr = csbi.wAttributes;
}

//------------------------------------------------------------------------------
void WinTerminalOut::end()
{
    SetConsoleTextAttribute(_stdout, _default_attr);
    SetConsoleMode(_stdout, _prev_mode);
    _stdout = nullptr;
}

//------------------------------------------------------------------------------
void WinTerminalOut::write(const char* chars, int32 length)
{
    StrIter iter(chars, length);
    while (length > 0)
    {
        wchar_t wbuf[384];
        int32 n = min<int32>(sizeof_array(wbuf), length + 1);
        n = to_utf16(wbuf, n, iter);

        DWORD written;
        WriteConsoleW(_stdout, wbuf, n, &written, nullptr);

        n = int32(iter.get_pointer() - chars);
        length -= n;
        chars += n;
    }
}

//------------------------------------------------------------------------------
void WinTerminalOut::flush()
{
    // When writing to the console conhost.exe will restart the cursor blink
    // timer and hide it which can be disorientating, especially when moving
    // around a line. The below will make sure it stays visible.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_stdout, &csbi);
    SetConsoleCursorPosition(_stdout, csbi.dwCursorPosition);
}

//------------------------------------------------------------------------------
int32 WinTerminalOut::get_columns() const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_stdout, &csbi);
    return csbi.dwSize.X;
}

//------------------------------------------------------------------------------
int32 WinTerminalOut::get_rows() const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_stdout, &csbi);
    return (csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
}
