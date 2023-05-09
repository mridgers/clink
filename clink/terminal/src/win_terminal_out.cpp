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
    m_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(m_stdout, &m_prev_mode);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_stdout, &csbi);
    m_default_attr = csbi.wAttributes;
}

//------------------------------------------------------------------------------
void WinTerminalOut::end()
{
    SetConsoleTextAttribute(m_stdout, m_default_attr);
    SetConsoleMode(m_stdout, m_prev_mode);
    m_stdout = nullptr;
}

//------------------------------------------------------------------------------
void WinTerminalOut::write(const char* chars, int length)
{
    StrIter iter(chars, length);
    while (length > 0)
    {
        wchar_t wbuf[384];
        int n = min<int>(sizeof_array(wbuf), length + 1);
        n = to_utf16(wbuf, n, iter);

        DWORD written;
        WriteConsoleW(m_stdout, wbuf, n, &written, nullptr);

        n = int(iter.get_pointer() - chars);
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
    GetConsoleScreenBufferInfo(m_stdout, &csbi);
    SetConsoleCursorPosition(m_stdout, csbi.dwCursorPosition);
}

//------------------------------------------------------------------------------
int WinTerminalOut::get_columns() const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_stdout, &csbi);
    return csbi.dwSize.X;
}

//------------------------------------------------------------------------------
int WinTerminalOut::get_rows() const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_stdout, &csbi);
    return (csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
}
