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
    const T*        _ptr;
    const T*        _end;
};

//------------------------------------------------------------------------------
template <typename T> StrIterImpl<T>::StrIterImpl(const T* s, int len)
: _ptr(s)
, _end(_ptr + len)
{
}

//------------------------------------------------------------------------------
template <typename T> StrIterImpl<T>::StrIterImpl(const StrImpl<T>& s, int len)
: _ptr(s.c_str())
, _end(_ptr + len)
{
}

//------------------------------------------------------------------------------
template <typename T> const T* StrIterImpl<T>::get_pointer() const
{
    return _ptr;
};

//------------------------------------------------------------------------------
template <typename T> int StrIterImpl<T>::peek()
{
    const T* ptr = _ptr;
    int ret = next();
    _ptr = ptr;
    return ret;
}

//------------------------------------------------------------------------------
template <typename T> bool StrIterImpl<T>::more() const
{
    return (_ptr != _end && *_ptr != '\0');
}



//------------------------------------------------------------------------------
typedef StrIterImpl<char>       StrIter;
typedef StrIterImpl<wchar_t>    WstrIter;
