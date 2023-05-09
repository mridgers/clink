// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "str_tokeniser.h"

#include <new>

//------------------------------------------------------------------------------
template <>
StrToken StrTokeniserImpl<char>::next(StrImpl<char>& out)
{
    const char* start;
    int length;
    auto ret = next_impl(start, length);

    out.clear();
    if (ret)
        out.concat(start, length);

    return ret;
}

//------------------------------------------------------------------------------
template <>
StrToken StrTokeniserImpl<wchar_t>::next(StrImpl<wchar_t>& out)
{
    const wchar_t* start;
    int length;
    auto ret = next_impl(start, length);

    out.clear();
    if (ret)
        out.concat(start, length);

    return ret;
}

//------------------------------------------------------------------------------
template <>
StrToken StrTokeniserImpl<char>::next(const char*& start, int& length)
{
    return next_impl(start, length);
}

//------------------------------------------------------------------------------
template <>
StrToken StrTokeniserImpl<wchar_t>::next(const wchar_t*& start, int& length)
{
    return next_impl(start, length);
}

//------------------------------------------------------------------------------
template <>
StrToken StrTokeniserImpl<char>::next(StrIterImpl<char>& out)
{
    const char* start;
    int length;
    if (auto ret = next_impl(start, length))
    {
        new (&out) StrIterImpl<char>(start, length);
        return ret;
    }

    return StrToken::invalid_delim;
}

//------------------------------------------------------------------------------
template <>
StrToken StrTokeniserImpl<wchar_t>::next(StrIterImpl<wchar_t>& out)
{
    const wchar_t* start;
    int length;
    if (auto ret = next_impl(start, length))
    {
        new (&out) StrIterImpl<wchar_t>(start, length);
        return ret;
    }

    return StrToken::invalid_delim;
}

//------------------------------------------------------------------------------
template <typename T>
int StrTokeniserImpl<T>::get_right_quote(int left) const
{
    for (const Quote& iter : m_quotes)
        if (iter.left == left)
            return iter.right;

    return 0;
}

//------------------------------------------------------------------------------
template <typename T>
StrToken StrTokeniserImpl<T>::next_impl(const T*& out_start, int& out_length)
{
    // Skip initial delimiters.
    char delim = 0;
    while (int c = m_iter.peek())
    {
        const char* candidate = strchr(m_delims, c);
        if (candidate == nullptr)
            break;

        delim = *candidate;
        m_iter.next();
    }

    // Extract the delimited string.
    const T* start = m_iter.get_pointer();

    int quote_close = 0;
    while (int c = m_iter.peek())
    {
        if (quote_close)
        {
            quote_close = (quote_close == c) ? 0 : quote_close;
            m_iter.next();
            continue;
        }

        if (strchr(m_delims, c))
            break;

        quote_close = get_right_quote(c);
        m_iter.next();
    }

    const T* end = m_iter.get_pointer();

    // Empty string? Must be the end of the Input. We're done here.
    if (start == end)
        return StrToken::invalid_delim;

    // Set the output and return.
    out_start = start;
    out_length = int(end - start);
    return delim;
}
