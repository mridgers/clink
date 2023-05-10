// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "str_iter.h"

//------------------------------------------------------------------------------
template <>
int StrIterImpl<char>::next()
{
    if (!more())
        return 0;

    int ax = 0;
    int encode_length = 0;
    while (int c = *_ptr++)
    {
        ax = (ax << 6) | (c & 0x7f);
        if (encode_length)
        {
            --encode_length;
            continue;
        }

        if ((c & 0xc0) < 0xc0)
            return ax;

        if (encode_length = !!(c & 0x20))
            encode_length += !!(c & 0x10);

        ax &= (0x1f >> encode_length);

        if (!more())
            break;
    }

    return 0;
}

//------------------------------------------------------------------------------
template <>
int StrIterImpl<wchar_t>::next()
{
    if (!more())
        return 0;

    int ax = 0;
    while (int c = *_ptr++)
    {
        // Decode surrogate pairs.
        if ((c & 0xfc00) == 0xd800)
        {
            ax = c << 10;
            continue;
        }
        else if ((c & 0xfc00) == 0xdc00 && ax >= (1 << 10))
            return ax + c - 0x35fdc00;
        else
            return c;
    }

    return 0;
}

//------------------------------------------------------------------------------
template <>
unsigned int StrIterImpl<char>::length() const
{
    return (unsigned int)((_ptr <= _end) ? _end - _ptr : strlen(_ptr));
}

//------------------------------------------------------------------------------
template <>
unsigned int StrIterImpl<wchar_t>::length() const
{
    return (unsigned int)((_ptr <= _end) ? _end - _ptr : wcslen(_ptr));
}
