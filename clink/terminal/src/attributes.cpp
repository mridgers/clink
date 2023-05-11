// Copyright (c) Martin Ridgers
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
void Attributes::Colour::as_888(uint8 (&out)[3]) const
{
    out[0] = (r << 3) | (r & 7);
    out[1] = (g << 3) | (g & 7);
    out[2] = (b << 3) | (b & 7);
}



//------------------------------------------------------------------------------
Attributes::Attributes()
: _state(0)
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
    int32 cmp = 1;
    #define CMP_IMPL(x) (_flags.x & rhs._flags.x) ? (_##x == rhs._##x) : 1;
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
    mask._flags.all = ~0;
    mask._fg.value = second._flags.fg ? ~0 : 0;
    mask._bg.value = second._flags.bg ? ~0 : 0;
    mask._bold = second._flags.bold;
    mask._underline = second._flags.underline;

    Attributes out;
    out._state = first._state & ~mask._state;
    out._state |= second._state & mask._state;
    out._flags.all |= first._flags.all;

    return out;
}

//------------------------------------------------------------------------------
Attributes Attributes::diff(const Attributes from, const Attributes to)
{
    Flags changed;
    changed.fg = !(to._fg == from._fg);
    changed.bg = !(to._bg == from._bg);
    changed.bold = (to._bold != from._bold);
    changed.underline = (to._underline != from._underline);

    Attributes out = to;
    out._flags.all &= changed.all;
    return out;
}

//------------------------------------------------------------------------------
void Attributes::reset_fg()
{
    _flags.fg = 1;
    _fg.value = default_code;
}

//------------------------------------------------------------------------------
void Attributes::reset_bg()
{
    _flags.bg = 1;
    _bg.value = default_code;
}

//------------------------------------------------------------------------------
void Attributes::set_fg(uint8 value)
{
    if (value == default_code)
        value = 15;

    _flags.fg = 1;
    _fg.value = value;
}

//------------------------------------------------------------------------------
void Attributes::set_bg(uint8 value)
{
    if (value == default_code)
        value = 15;

    _flags.bg = 1;
    _bg.value = value;
}

//------------------------------------------------------------------------------
void Attributes::set_fg(uint8 r, uint8 g, uint8 b)
{
    _flags.fg = 1;
    _fg.r = r >> 3;
    _fg.g = g >> 3;
    _fg.b = b >> 3;
    _fg.is_rgb = 1;
}

//------------------------------------------------------------------------------
void Attributes::set_bg(uint8 r, uint8 g, uint8 b)
{
    _flags.bg = 1;
    _bg.r = r >> 3;
    _bg.g = g >> 3;
    _bg.b = b >> 3;
    _bg.is_rgb = 1;
}

//------------------------------------------------------------------------------
void Attributes::set_bold(bool state)
{
    _flags.bold = 1;
    _bold = !!state;
}

//------------------------------------------------------------------------------
void Attributes::set_underline(bool state)
{
    _flags.underline = 1;
    _underline = !!state;
}

//------------------------------------------------------------------------------
Attributes::Attribute<Attributes::Colour> Attributes::get_fg() const
{
    return { _fg, _flags.fg, (_fg.value == default_code) };
}

//------------------------------------------------------------------------------
Attributes::Attribute<Attributes::Colour> Attributes::get_bg() const
{
    return { _bg, _flags.bg, (_bg.value == default_code) };
}

//------------------------------------------------------------------------------
Attributes::Attribute<bool> Attributes::get_bold() const
{
    return { bool(_bold), bool(_flags.bold) };
}

//------------------------------------------------------------------------------
Attributes::Attribute<bool> Attributes::get_underline() const
{
    return { bool(_underline), bool(_flags.underline) };
}
