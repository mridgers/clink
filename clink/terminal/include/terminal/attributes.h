// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
#define COLOUR_XS\
    COLOUR_X(black)\
    COLOUR_X(red)\
    COLOUR_X(green)\
    COLOUR_X(yellow)\
    COLOUR_X(blue)\
    COLOUR_X(magenta)\
    COLOUR_X(cyan)\
    COLOUR_X(grey)\
    COLOUR_X(dark_grey)\
    COLOUR_X(light_red)\
    COLOUR_X(light_green)\
    COLOUR_X(light_yellow)\
    COLOUR_X(light_blue)\
    COLOUR_X(light_magenta)\
    COLOUR_X(light_cyan)\
    COLOUR_X(white)

#define COLOUR_X(x) colour_##x,
enum : uint8
{
    COLOUR_XS
    colour_count,
};
#undef COLOUR_X

//------------------------------------------------------------------------------
class Attributes
{
public:
    struct Colour
    {
        union
        {
            struct
            {
                uint16          r : 5;
                uint16          g : 5;
                uint16          b : 5;
                uint16          is_rgb : 1;
            };
            uint16              value;
        };

        bool                    operator == (const Colour& rhs) const { return value == rhs.value; }
        void                    as_888(uint8 (&out)[3]) const;
    };

    template <typename T>
    struct Attribute
    {
        explicit                operator bool () const  { return bool(set); }
        const T*                operator -> () const { return &value; }
        const T                 value;
        const uint8             set : 1;
        const uint8             is_default : 1;
    };

    enum Default { defaults };

                                Attributes();
                                Attributes(Default);
    bool                        operator == (const Attributes rhs);
    bool                        operator != (const Attributes rhs) { return !(*this == rhs); }
    static Attributes           merge(const Attributes first, const Attributes second);
    static Attributes           diff(const Attributes from, const Attributes to);
    void                        reset_fg();
    void                        reset_bg();
    void                        set_fg(uint8 value);
    void                        set_bg(uint8 value);
    void                        set_fg(uint8 r, uint8 g, uint8 b);
    void                        set_bg(uint8 r, uint8 g, uint8 b);
    void                        set_bold(bool state=true);
    void                        set_underline(bool state=true);
    Attribute<Colour>           get_fg() const;
    Attribute<Colour>           get_bg() const;
    Attribute<bool>             get_bold() const;
    Attribute<bool>             get_underline() const;

private:
    union Flags
    {
        struct
        {
            uint8               fg : 1;
            uint8               bg : 1;
            uint8               bold : 1;
            uint8               underline : 1;
        };
        uint8                   all;
    };

    union
    {
        struct
        {
            Colour              _fg;
            Colour              _bg;
            uint16              _bold : 1;
            uint16              _underline : 1;
            Flags               _flags;
            uint8               _unused;
        };
        uint64                  _state;
    };
};
