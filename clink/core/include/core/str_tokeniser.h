// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "array.h"
#include "str.h"
#include "str_iter.h"

//------------------------------------------------------------------------------
class StrToken
{
public:
    enum : unsigned char {
        invalid_delim   = 0xff,
    };
                        StrToken(char c) : delim(c) {}
    explicit            operator bool () const       { return (delim != invalid_delim); }
    unsigned char       delim;
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
    StrToken            next(const T*& start, int& length);
    StrToken            next(StrIterImpl<T>& out);

private:
    struct Quote
    {
        char            left;
        char            right;
    };

    typedef FixedArray<Quote, 4> quotes;

    int                 get_right_quote(int left) const;
    StrToken            next_impl(const T*& out_start, int& out_length);
    quotes              m_quotes;
    StrIterImpl<T>      m_iter;
    const char*         m_delims;
};

//------------------------------------------------------------------------------
template <typename T>
StrTokeniserImpl<T>::StrTokeniserImpl(const T* in, const char* delims)
: m_iter(in)
, m_delims(delims)
{
}

//------------------------------------------------------------------------------
template <typename T>
StrTokeniserImpl<T>::StrTokeniserImpl(const StrIterImpl<T>& in, const char* delims)
: m_iter(in)
, m_delims(delims)
{
}

//------------------------------------------------------------------------------
template <typename T>
bool StrTokeniserImpl<T>::add_quote_pair(const char* pair)
{
    if (pair == nullptr || !pair[0])
        return false;

    Quote* q = m_quotes.push_back();
    if (q == nullptr)
        return false;

    *q = { pair[0], (pair[1] ? pair[1] : pair[0]) };
    return true;
}

//------------------------------------------------------------------------------
typedef StrTokeniserImpl<char>      StrTokeniser;
typedef StrTokeniserImpl<wchar_t> WstrTokeniser;
