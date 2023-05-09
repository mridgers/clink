// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "attributes.h"

static_assert(sizeof(Attributes) == sizeof(long long), "sizeof(attr) != 64bits");

//------------------------------------------------------------------------------
enum
{
    default_code = 231, // because xterm256's 231 == old-school Colour 15 (white)
};



//------------------------------------------------------------------------------
void Attributes::Colour::as_888(unsigned char (&out)[3]) const
{
    out[0] = (r << 3) | (r & 7);
    out[1] = (g << 3) | (g & 7);
    out[2] = (b << 3) | (b & 7);
}



//------------------------------------------------------------------------------
Attributes::Attributes()
: m_state(0)
{
}

//------------------------------------------------------------------------------
Attributes::Attributes(Default)
: Attributes()
{
    reset_fg();
    reset_bg();
    set_bold(false);
    set_underline(false);
}

//------------------------------------------------------------------------------
bool Attributes::operator == (const Attributes rhs)
{
    int cmp = 1;
    #define CMP_IMPL(x) (m_flags.x & rhs.m_flags.x) ? (m_##x == rhs.m_##x) : 1;
    cmp &= CMP_IMPL(fg);
    cmp &= CMP_IMPL(bg);
    cmp &= CMP_IMPL(bold);
    cmp &= CMP_IMPL(underline);
    #undef CMP_IMPL
    return (cmp != 0);
}

//------------------------------------------------------------------------------
Attributes Attributes::merge(const Attributes first, const Attributes second)
{
    Attributes mask;
    mask.m_flags.all = ~0;
    mask.m_fg.value = second.m_flags.fg ? ~0 : 0;
    mask.m_bg.value = second.m_flags.bg ? ~0 : 0;
    mask.m_bold = second.m_flags.bold;
    mask.m_underline = second.m_flags.underline;

    Attributes out;
    out.m_state = first.m_state & ~mask.m_state;
    out.m_state |= second.m_state & mask.m_state;
    out.m_flags.all |= first.m_flags.all;

    return out;
}

//------------------------------------------------------------------------------
Attributes Attributes::diff(const Attributes from, const Attributes to)
{
    Flags changed;
    changed.fg = !(to.m_fg == from.m_fg);
    changed.bg = !(to.m_bg == from.m_bg);
    changed.bold = (to.m_bold != from.m_bold);
    changed.underline = (to.m_underline != from.m_underline);

    Attributes out = to;
    out.m_flags.all &= changed.all;
    return out;
}

//------------------------------------------------------------------------------
void Attributes::reset_fg()
{
    m_flags.fg = 1;
    m_fg.value = default_code;
}

//------------------------------------------------------------------------------
void Attributes::reset_bg()
{
    m_flags.bg = 1;
    m_bg.value = default_code;
}

//------------------------------------------------------------------------------
void Attributes::set_fg(unsigned char value)
{
    if (value == default_code)
        value = 15;

    m_flags.fg = 1;
    m_fg.value = value;
}

//------------------------------------------------------------------------------
void Attributes::set_bg(unsigned char value)
{
    if (value == default_code)
        value = 15;

    m_flags.bg = 1;
    m_bg.value = value;
}

//------------------------------------------------------------------------------
void Attributes::set_fg(unsigned char r, unsigned char g, unsigned char b)
{
    m_flags.fg = 1;
    m_fg.r = r >> 3;
    m_fg.g = g >> 3;
    m_fg.b = b >> 3;
    m_fg.is_rgb = 1;
}

//------------------------------------------------------------------------------
void Attributes::set_bg(unsigned char r, unsigned char g, unsigned char b)
{
    m_flags.bg = 1;
    m_bg.r = r >> 3;
    m_bg.g = g >> 3;
    m_bg.b = b >> 3;
    m_bg.is_rgb = 1;
}

//------------------------------------------------------------------------------
void Attributes::set_bold(bool state)
{
    m_flags.bold = 1;
    m_bold = !!state;
}

//------------------------------------------------------------------------------
void Attributes::set_underline(bool state)
{
    m_flags.underline = 1;
    m_underline = !!state;
}

//------------------------------------------------------------------------------
Attributes::Attribute<Attributes::Colour> Attributes::get_fg() const
{
    return { m_fg, m_flags.fg, (m_fg.value == default_code) };
}

//------------------------------------------------------------------------------
Attributes::Attribute<Attributes::Colour> Attributes::get_bg() const
{
    return { m_bg, m_flags.bg, (m_bg.value == default_code) };
}

//------------------------------------------------------------------------------
Attributes::Attribute<bool> Attributes::get_bold() const
{
    return { bool(m_bold), bool(m_flags.bold) };
}

//------------------------------------------------------------------------------
Attributes::Attribute<bool> Attributes::get_underline() const
{
    return { bool(m_underline), bool(m_flags.underline) };
}
