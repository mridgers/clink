// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "str.h"
#include "str_iter.h"

//------------------------------------------------------------------------------
template <typename TYPE>
struct Builder
{
                Builder(TYPE* data, int32 max_length);
                ~Builder()                            { *write = '\0'; }
    bool        truncated() const                     { return (write >= end); }
    int32       get_written() const                   { return int32(write - start); }
    Builder&    operator << (int32 value);
    TYPE*       write;
    const TYPE* start;
    const TYPE* end;
};

//------------------------------------------------------------------------------
template <typename TYPE>
Builder<TYPE>::Builder(TYPE* data, int32 max_length)
: write(data)
, start(data)
, end(data + max_length - 1)
{
}

//------------------------------------------------------------------------------
template <>
Builder<wchar_t>& Builder<wchar_t>::operator << (int32 value)
{
    // For code points that don't fit in wchar_t there is 'surrogate pairs'.
    if (value > 0xffff)
        return *this << ((value >> 10) + 0xd7c0) << ((value & 0x3ff) + 0xdc00);

    if (write < end)
        *write++ = wchar_t(value);

    return *this;
}

//------------------------------------------------------------------------------
template <>
Builder<char>& Builder<char>::operator << (int32 value)
{
    if (write < end)
        *write++ = char(value);

    return *this;
}



//------------------------------------------------------------------------------
int32 to_utf8(char* out, int32 max_count, WstrIter& iter)
{
    Builder<char> Builder(out, max_count);

    int32 c;
    while (!Builder.truncated() && (c = iter.next()))
    {
        if (c < 0x80)
        {
            Builder << c;
            continue;
        }

        // How long is this encoding?
        int32 n = 2;
        n += c >= (1 << 11);
        n += c >= (1 << 16);

        // Collect the bytes of the encoding.
        char out_chars[4];
        switch (n)
        {
        case 4: out_chars[3] = (c & 0x3f); c >>= 6;
        case 3: out_chars[2] = (c & 0x3f); c >>= 6;
        case 2: out_chars[1] = (c & 0x3f); c >>= 6;
                out_chars[0] = (c & 0x1f) | char(0xfc0 >> (n - 2));
        }

        for (int32 i = 0; i < n; ++i)
            Builder << (out_chars[i] | 0x80);
    }

    return Builder.get_written();
}

//------------------------------------------------------------------------------
int32 to_utf8(char* out, int32 max_count, const wchar_t* utf16)
{
    WstrIter iter(utf16);
    return to_utf8(out, max_count, iter);
}

//------------------------------------------------------------------------------
int32 to_utf8(StrBase& out, StrIterImpl<wchar_t>& utf16)
{
    int32 length = out.length();

    if (out.is_growable())
        out.reserve(length + utf16.length());

    return to_utf8(out.data() + length, out.size() - length, utf16);
}

//------------------------------------------------------------------------------
int32 to_utf8(StrBase& out, const wchar_t* utf16)
{
    WstrIter iter(utf16);
    return to_utf8(out, iter);
}



//------------------------------------------------------------------------------
int32 to_utf16(wchar_t* out, int32 max_count, StrIter& iter)
{
    Builder<wchar_t> Builder(out, max_count);

    int32 c;
    while (!Builder.truncated() && (c = iter.next()))
        Builder << c;

    return Builder.get_written();
}

//------------------------------------------------------------------------------
int32 to_utf16(wchar_t* out, int32 max_count, const char* utf8)
{
    StrIter iter(utf8);
    return to_utf16(out, max_count, iter);
}

//------------------------------------------------------------------------------
int32 to_utf16(WstrBase& out, StrIterImpl<char>& utf8)
{
    int32 length = out.length();

    if (out.is_growable())
        out.reserve(length + utf8.length());

    return to_utf16(out.data() + length, out.size() - length, utf8);
}

//------------------------------------------------------------------------------
int32 to_utf16(WstrBase& out, const char* utf8)
{
    StrIter iter(utf8);
    return to_utf16(out, iter);
}
