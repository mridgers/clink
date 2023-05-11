// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "attributes.h"

#include <core/base.h>
#include <core/settings.h>

#include <new>

//------------------------------------------------------------------------------
class SettingColour
{
public:
    #define COLOUR_X(x) value_##x,
    enum : uint8
    {
        COLOUR_XS

        value_fg_normal,
        value_fg_bright,
        value_fg_default,
        value_fg_count,

        value_bg_default = value_fg_normal,
        value_bg_count,
    };
    #undef COLOUR_X
                        SettingColour(const char* name, const char* short_desc, int32 default_fg, int32 default_bg);
                        SettingColour(const char* name, const char* short_desc, const char* long_desc, int32 default_fg, int32 default_bg);
    Attributes          get() const;

private:
    template <class T, bool AUTO_DTOR=true>
    struct Late
    {
        template <typename... ARGS>
        void            construct(ARGS... args) { new (buffer) T(args...); }
        void            destruct()              { (*this)->~T(); }
                        ~Late()                 { if (AUTO_DTOR) destruct(); }
        T*              operator -> ()          { return (T*)buffer; }
        const T*        operator -> () const    { return (T*)buffer; }
        align_to(8)     char buffer[sizeof(T)];
    };

    Late<SettingEnum>   _fg;
    Late<SettingEnum>   _bg;
};
