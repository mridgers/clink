// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "setting_colour.h"

//------------------------------------------------------------------------------
#define COLOUR_X(x) #x ","
static const char* colour_fg_values = COLOUR_XS "normal,bright,default";
static const char* colour_bg_values = COLOUR_XS "default";
#undef COLOUR_X

//------------------------------------------------------------------------------
SettingColour::SettingColour(
    const char* name,
    const char* short_desc,
    int default_fg,
    int default_bg)
: SettingColour(name, short_desc, nullptr, default_fg, default_bg)
{
}

//------------------------------------------------------------------------------
SettingColour::SettingColour(
    const char* name,
    const char* short_desc,
    const char* long_desc,
    int default_fg,
    int default_bg)
{
    Str<64> inner_name;
    inner_name << name << ".fg";
    _fg.construct(inner_name.c_str(), short_desc, long_desc, colour_fg_values, default_fg);

    inner_name.clear();
    inner_name << name << ".bg";
    _bg.construct(inner_name.c_str(), short_desc, long_desc, colour_bg_values, default_bg);
}

//------------------------------------------------------------------------------
Attributes SettingColour::get() const
{
    Attributes out = Attributes::defaults;

    int fg = _fg->get();
    switch (fg)
    {
    case value_fg_normal:   out.set_bold(false);    break;
    case value_fg_bright:   out.set_bold(true);     break;
    case value_fg_default:  out.reset_fg();         break;
    default:                out.set_fg(fg);         break;
    }

    int bg = _bg->get();
    switch (bg)
    {
    case value_bg_default:  out.reset_bg();         break;
    default:                out.set_bg(bg);         break;
    }

    return out;
}
