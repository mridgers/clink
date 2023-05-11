// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "str.h"

//------------------------------------------------------------------------------
template <typename T>
class StrIterImpl
{
public:
    explicit        StrIterImpl(const T* s=(const T*)L"", int32 len=-1);
    explicit        StrIterImpl(const StrImpl<T>& s, int32 len=-1);
    const T*        get_pointer() const;
    int32           peek();
    int32           next();
    bool            more() const;
    uint32          length() const;

private:
    const T*        _ptr;
    const T*        _end;
};

//------------------------------------------------------------------------------
template <typename T> StrIterImpl<T>::StrIterImpl(const T* s, int32 len)
: _ptr(s)
, _end(_ptr + len)
{
}

//------------------------------------------------------------------------------
template <typename T> StrIterImpl<T>::StrIterImpl(const StrImpl<T>& s, int32 len)
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
template <typename T> int32 StrIterImpl<T>::peek()
{
    const T* ptr = _ptr;
    int32 ret = next();
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
