// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "printer.h"
#include "terminal_out.h"

#include <core/str.h>

//------------------------------------------------------------------------------
Printer::Printer(TerminalOut& Terminal)
: m_terminal(Terminal)
, m_set_attr(Attributes::defaults)
, m_next_attr(Attributes::defaults)
{
}

//------------------------------------------------------------------------------
void Printer::print(const char* data, int bytes)
{
    if (bytes <= 0)
        return;

    if (m_next_attr != m_set_attr)
        flush_attributes();

    m_terminal.write(data, bytes);
}

//------------------------------------------------------------------------------
void Printer::print(const Attributes attr, const char* data, int bytes)
{
    Attributes prev_attr = set_attributes(attr);
    print(data, bytes);
    set_attributes(prev_attr);
}

//------------------------------------------------------------------------------
unsigned int Printer::get_columns() const
{
    return m_terminal.get_columns();
}

//------------------------------------------------------------------------------
unsigned int Printer::get_rows() const
{
    return m_terminal.get_rows();
}

//------------------------------------------------------------------------------
Attributes Printer::set_attributes(const Attributes attr)
{
    Attributes prev_attr = m_next_attr;
    m_next_attr = Attributes::merge(m_next_attr, attr);
    return prev_attr;
}

//------------------------------------------------------------------------------
void Printer::flush_attributes()
{
    Attributes diff = Attributes::diff(m_set_attr, m_next_attr);

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
        m_terminal.write("\x1b[");
        m_terminal.write(params.c_str(), params.length());
        m_terminal.write("m");
    }

    m_set_attr = m_next_attr;
}

//------------------------------------------------------------------------------
Attributes Printer::get_attributes() const
{
    return m_next_attr;
}

//------------------------------------------------------------------------------
void Printer::insert(int count)
{
}

//------------------------------------------------------------------------------
void Printer::move_cursor(int dc, int dr)
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
