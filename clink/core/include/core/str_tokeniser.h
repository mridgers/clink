// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "array.h"
#include "str.h"
#include "str_iter.h"

//------------------------------------------------------------------------------
class StrToken
{
public:
    enum : uint8 {
        invalid_delim   = 0xff,
    };
                        StrToken(char c) : delim(c) {}
    explicit            operator bool () const       { return (delim != invalid_delim); }
    uint8               delim;
};



//------------------------------------------------------------------------------
template <typename T>
class StrTokeniserImpl
{
public:
                        StrTokeniserImpl(const T* in=(const T*)L"", const char* delims=" ");
                        StrTokeniserImpl(const StrIterImpl<T>& in, const char* delims=" ");
    bool                add_quote_pair(const char* pair);
    StrToken            next(StrImpl<T>& out);
    StrToken            next(const T*& start, int32& length);
    StrToken            next(StrIterImpl<T>& out);

private:
    struct Quote
    {
        char            left;
        char            right;
    };

    typedef FixedArray<Quote, 4> quotes;

    int32               get_right_quote(int32 left) const;
    StrToken            next_impl(const T*& out_start, int32& out_length);
    quotes              _quotes;
    StrIterImpl<T>      _iter;
    const char*         _delims;
};

//------------------------------------------------------------------------------
template <typename T>
StrTokeniserImpl<T>::StrTokeniserImpl(const T* in, const char* delims)
: _iter(in)
, _delims(delims)
{
}

//------------------------------------------------------------------------------
template <typename T>
StrTokeniserImpl<T>::StrTokeniserImpl(const StrIterImpl<T>& in, const char* delims)
: _iter(in)
, _delims(delims)
{
}

//------------------------------------------------------------------------------
template <typename T>
bool StrTokeniserImpl<T>::add_quote_pair(const char* pair)
{
    if (pair == nullptr || !pair[0])
        return false;

    Quote* q = _quotes.push_back();
    if (q == nullptr)
        return false;

    *q = { pair[0], (pair[1] ? pair[1] : pair[0]) };
    return true;
}

//------------------------------------------------------------------------------
typedef StrTokeniserImpl<char>      StrTokeniser;
typedef StrTokeniserImpl<wchar_t> WstrTokeniser;
