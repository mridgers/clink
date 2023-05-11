// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "printer.h"
#include "terminal_out.h"

#include <core/str.h>

//------------------------------------------------------------------------------
Printer::Printer(TerminalOut& Terminal)
: _terminal(Terminal)
, _set_attr(Attributes::defaults)
, _next_attr(Attributes::defaults)
{
}

//------------------------------------------------------------------------------
void Printer::print(const char* data, int32 bytes)
{
    if (bytes <= 0)
        return;

    if (_next_attr != _set_attr)
        flush_attributes();

    _terminal.write(data, bytes);
}

//------------------------------------------------------------------------------
void Printer::print(const Attributes attr, const char* data, int32 bytes)
{
    Attributes prev_attr = set_attributes(attr);
    print(data, bytes);
    set_attributes(prev_attr);
}

//------------------------------------------------------------------------------
uint32 Printer::get_columns() const
{
    return _terminal.get_columns();
}

//------------------------------------------------------------------------------
uint32 Printer::get_rows() const
{
    return _terminal.get_rows();
}

//------------------------------------------------------------------------------
Attributes Printer::set_attributes(const Attributes attr)
{
    Attributes prev_attr = _next_attr;
    _next_attr = Attributes::merge(_next_attr, attr);
    return prev_attr;
}

//------------------------------------------------------------------------------
void Printer::flush_attributes()
{
    Attributes diff = Attributes::diff(_set_attr, _next_attr);

    Str<64, false> params;
    auto add_param = [&] (const char* x) {
        if (!params.empty())
            params << ";";
        params << x;
    };

    auto fg = diff.get_fg();
    auto bg = diff.get_bg();
    if (fg.is_default & bg.is_default)
    {
        add_param("0");
    }
    else
    {
        if (fg)
        {
            if (!fg.is_default)
            {
                char x[] = "30";
                x[0] += (fg->value > 7) ? 6 : 0;
                x[1] += fg->value & 0x07;
                add_param(x);
            }
            else
                add_param("39");
        }

        if (bg)
        {
            if (!bg.is_default)
            {
                char x[] = "100";
                x[1] += (bg->value > 7) ? 0 : 4;
                x[2] += bg->value & 0x07;
                add_param((bg->value > 7) ? x : x + 1);
            }
            else
                add_param("49");
        }
    }

    if (auto bold = diff.get_bold())
        add_param(bold.value ? "1" : "22");

    if (auto underline = diff.get_underline())
        add_param(underline.value ? "4" : "24");

    if (!params.empty())
    {
        _terminal.write("\x1b[");
        _terminal.write(params.c_str(), params.length());
        _terminal.write("m");
    }

    _set_attr = _next_attr;
}

//------------------------------------------------------------------------------
Attributes Printer::get_attributes() const
{
    return _next_attr;
}

//------------------------------------------------------------------------------
void Printer::insert(int32 count)
{
}

//------------------------------------------------------------------------------
void Printer::move_cursor(int32 dc, int32 dr)
{
}

//------------------------------------------------------------------------------
void Printer::set_cursor(CursorState state)
{
}

//------------------------------------------------------------------------------
Printer::CursorState Printer::get_cursor() const
{
    return 0;
}
