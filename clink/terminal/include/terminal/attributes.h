// Copyright (c) 2016 Martin Ridgers
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
enum : unsigned char
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
                unsigned short  r : 5;
                unsigned short  g : 5;
                unsigned short  b : 5;
                unsigned short  is_rgb : 1;
            };
            unsigned short      value;
        };

        bool                    operator == (const Colour& rhs) const { return value == rhs.value; }
        void                    as_888(unsigned char (&out)[3]) const;
    };

    template <typename T>
    struct Attribute
    {
        explicit                operator bool () const  { return bool(set); }
        const T*                operator -> () const { return &value; }
        const T                 value;
        const unsigned char     set : 1;
        const unsigned char     is_default : 1;
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
    void                        set_fg(unsigned char value);
    void                        set_bg(unsigned char value);
    void                        set_fg(unsigned char r, unsigned char g, unsigned char b);
    void                        set_bg(unsigned char r, unsigned char g, unsigned char b);
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
            unsigned char       fg : 1;
            unsigned char       bg : 1;
            unsigned char       bold : 1;
            unsigned char       underline : 1;
        };
        unsigned char           all;
    };

    union
    {
        struct
        {
            Colour              _fg;
            Colour              _bg;
            unsigned short      _bold : 1;
            unsigned short      _underline : 1;
            Flags               _flags;
            unsigned char       _unused;
        };
        unsigned long long      _state;
    };
};
