// Copyright (c) 2018 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "win_screen_buffer.h"

#include <core/base.h>
#include <core/str_iter.h>

#include <Windows.h>

//------------------------------------------------------------------------------
void WinScreenBuffer::begin()
{
    _handle = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(_handle, &_prev_mode);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);
    _default_attr = csbi.wAttributes & attr_mask_all;
    _bold = !!(_default_attr & attr_mask_bold);
}

//------------------------------------------------------------------------------
void WinScreenBuffer::end()
{
    SetConsoleTextAttribute(_handle, _default_attr);
    SetConsoleMode(_handle, _prev_mode);
    _handle = nullptr;
}

//------------------------------------------------------------------------------
void WinScreenBuffer::write(const char* data, int length)
{
    StrIter iter(data, length);
    while (length > 0)
    {
        wchar_t wbuf[384];
        int n = min<int>(sizeof_array(wbuf), length + 1);
        n = to_utf16(wbuf, n, iter);

        DWORD written;
        WriteConsoleW(_handle, wbuf, n, &written, nullptr);

        n = int(iter.get_pointer() - data);
        length -= n;
        data += n;
    }
}

//------------------------------------------------------------------------------
void WinScreenBuffer::flush()
{
    // When writing to the console conhost.exe will restart the cursor blink
    // timer and hide it which can be disorientating, especially when moving
    // around a line. The below will make sure it stays visible.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);
    SetConsoleCursorPosition(_handle, csbi.dwCursorPosition);
}

//------------------------------------------------------------------------------
int WinScreenBuffer::get_columns() const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);
    return csbi.dwSize.X;
}

//------------------------------------------------------------------------------
int WinScreenBuffer::get_rows() const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);
    return (csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
}

//------------------------------------------------------------------------------
void WinScreenBuffer::clear(ClearType Type)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);

    int width, height, count = 0;
    COORD xy;

    switch (Type)
    {
    case clear_type_all:
        width = csbi.dwSize.X;
        height = (csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
        xy = { 0, csbi.srWindow.Top };
        break;

    case clear_type_before:
        width = csbi.dwSize.X;
        height = csbi.dwCursorPosition.Y - csbi.srWindow.Top;
        xy = { 0, csbi.srWindow.Top };
        count = csbi.dwCursorPosition.X + 1;
        break;

    case clear_type_after:
        width = csbi.dwSize.X;
        height = csbi.srWindow.Bottom - csbi.dwCursorPosition.Y;
        xy = { csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y };
        count = width - csbi.dwCursorPosition.X;
        break;
    }

    count += width * height;

    DWORD written;
    FillConsoleOutputCharacterW(_handle, ' ', count, xy, &written);
    FillConsoleOutputAttribute(_handle, csbi.wAttributes, count, xy, &written);
}

//------------------------------------------------------------------------------
void WinScreenBuffer::clear_line(ClearType Type)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);

    int width;
    COORD xy;
    switch (Type)
    {
    case clear_type_all:
        width = csbi.dwSize.X;
        xy = { 0, csbi.dwCursorPosition.Y };
        break;

    case clear_type_before:
        width = csbi.dwCursorPosition.X + 1;
        xy = { 0, csbi.dwCursorPosition.Y };
        break;

    case clear_type_after:
        width = csbi.dwSize.X - csbi.dwCursorPosition.X;
        xy = { csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y };
        break;
    }

    DWORD written;
    FillConsoleOutputCharacterW(_handle, ' ', width, xy, &written);
    FillConsoleOutputAttribute(_handle, csbi.wAttributes, width, xy, &written);
}

//------------------------------------------------------------------------------
void WinScreenBuffer::set_cursor(int column, int row)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);

    const SMALL_RECT& window = csbi.srWindow;
    int width = (window.Right - window.Left) + 1;
    int height = (window.Bottom - window.Top) + 1;

    column = clamp(column, 0, width);
    row = clamp(row, 0, height);

    COORD xy = { short(window.Left + column), short(window.Top + row) };
    SetConsoleCursorPosition(_handle, xy);
}

//------------------------------------------------------------------------------
void WinScreenBuffer::move_cursor(int dx, int dy)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);

    COORD xy = {
        short(clamp(csbi.dwCursorPosition.X + dx, 0, csbi.dwSize.X - 1)),
        short(clamp(csbi.dwCursorPosition.Y + dy, 0, csbi.dwSize.Y - 1)),
    };
    SetConsoleCursorPosition(_handle, xy);
}

//------------------------------------------------------------------------------
void WinScreenBuffer::insert_chars(int count)
{
    if (count <= 0)
        return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);

    SMALL_RECT rect;
    rect.Left = csbi.dwCursorPosition.X;
    rect.Right = csbi.dwSize.X;
    rect.Top = rect.Bottom = csbi.dwCursorPosition.Y;

    CHAR_INFO fill;
    fill.Char.AsciiChar = ' ';
    fill.Attributes = csbi.wAttributes;

    csbi.dwCursorPosition.X += count;

    ScrollConsoleScreenBuffer(_handle, &rect, NULL, csbi.dwCursorPosition, &fill);
}

//------------------------------------------------------------------------------
void WinScreenBuffer::delete_chars(int count)
{
    if (count <= 0)
        return;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);

    SMALL_RECT rect;
    rect.Left = csbi.dwCursorPosition.X + count;
    rect.Right = csbi.dwSize.X - 1;
    rect.Top = rect.Bottom = csbi.dwCursorPosition.Y;

    CHAR_INFO fill;
    fill.Char.AsciiChar = ' ';
    fill.Attributes = csbi.wAttributes;

    ScrollConsoleScreenBuffer(_handle, &rect, NULL, csbi.dwCursorPosition, &fill);

    int chars_moved = rect.Right - rect.Left + 1;
    if (chars_moved < count)
    {
        COORD xy = csbi.dwCursorPosition;
        xy.X += chars_moved;

        count -= chars_moved;

        DWORD written;
        FillConsoleOutputCharacterW(_handle, ' ', count, xy, &written);
        FillConsoleOutputAttribute(_handle, csbi.wAttributes, count, xy, &written);
    }
}

//------------------------------------------------------------------------------
void WinScreenBuffer::set_attributes(const Attributes attr)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);

    int out_attr = csbi.wAttributes & attr_mask_all;

    // Note to self; lookup table is probably much faster.
    auto swizzle = [] (int rgbi) {
        int b_r_ = ((rgbi & 0x01) << 2) | !!(rgbi & 0x04);
        return (rgbi & 0x0a) | b_r_;
    };

    // Bold
    if (auto bold_attr = attr.get_bold())
        _bold = !!(bold_attr.value);

    // Underline
    if (auto underline = attr.get_underline())
    {
        if (underline.value)
            out_attr |= attr_mask_underline;
        else
            out_attr &= ~attr_mask_underline;
    }

    // Foreground Colour
    bool bold = _bold;
    if (auto fg = attr.get_fg())
    {
        int value = fg.is_default ? _default_attr : swizzle(fg->value);
        value &= attr_mask_fg;
        out_attr = (out_attr & attr_mask_bg) | value;
        bold |= (value > 7);
    }
    else
        bold |= (out_attr & attr_mask_bold) != 0;

    if (bold)
        out_attr |= attr_mask_bold;
    else
        out_attr &= ~attr_mask_bold;

    // Background Colour
    if (auto bg = attr.get_bg())
    {
        int value = bg.is_default ? _default_attr : (swizzle(bg->value) << 4);
        out_attr = (out_attr & attr_mask_fg) | (value & attr_mask_bg);
    }

    // TODO: add rgb/xterm256 support back.

    out_attr |= csbi.wAttributes & ~attr_mask_all;
    SetConsoleTextAttribute(_handle, short(out_attr));
}
