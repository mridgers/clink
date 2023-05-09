// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "str.h"

//------------------------------------------------------------------------------
template <typename T>
class StrIterImpl
{
public:
    explicit        StrIterImpl(const T* s=(const T*)L"", int len=-1);
    explicit        StrIterImpl(const StrImpl<T>& s, int len=-1);
    const T*        get_pointer() const;
    int             peek();
    int             next();
    bool            more() const;
    unsigned int    length() const;

private:
    const T*        m_ptr;
    const T*        m_end;
};

//------------------------------------------------------------------------------
template <typename T> StrIterImpl<T>::StrIterImpl(const T* s, int len)
: m_ptr(s)
, m_end(m_ptr + len)
{
}

//------------------------------------------------------------------------------
template <typename T> StrIterImpl<T>::StrIterImpl(const StrImpl<T>& s, int len)
: m_ptr(s.c_str())
, m_end(m_ptr + len)
{
}

//------------------------------------------------------------------------------
template <typename T> const T* StrIterImpl<T>::get_pointer() const
{
    return m_ptr;
};

//------------------------------------------------------------------------------
template <typename T> int StrIterImpl<T>::peek()
{
    const T* ptr = m_ptr;
    int ret = next();
    m_ptr = ptr;
    return ret;
}

//------------------------------------------------------------------------------
template <typename T> bool StrIterImpl<T>::more() const
{
    return (m_ptr != m_end && *m_ptr != '\0');
}



//------------------------------------------------------------------------------
typedef StrIterImpl<char>       StrIter;
typedef StrIterImpl<wchar_t>    WstrIter;
