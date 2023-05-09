// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "base.h"
#include "str_iter.h"

#include <Windows.h>

//------------------------------------------------------------------------------
class StrCompareScope
{
public:
    enum
    {
        exact,
        caseless,
        relaxed,    // case insensitive with -/_ considered equivalent.
    };

                StrCompareScope(int mode);
                ~StrCompareScope();
    static int  current();

private:
    int         m_prev_mode;
    threadlocal static int ts_mode;
};



//------------------------------------------------------------------------------
template <class T, int MODE>
int StrCompareImpl(StrIterImpl<T>& lhs, StrIterImpl<T>& rhs)
{
    const T* start = lhs.get_pointer();

    while (1)
    {
        int c = lhs.peek();
        int d = rhs.peek();
        if (!c || !d)
            break;

        if (MODE > 0)
        {
            c = (c > 0xffff) ? c : int(uintptr_t(CharLowerW(LPWSTR(uintptr_t(c)))));
            d = (d > 0xffff) ? d : int(uintptr_t(CharLowerW(LPWSTR(uintptr_t(d)))));
        }

        if (MODE > 1)
        {
            c = (c == '-') ? '_' : c;
            d = (d == '-') ? '_' : d;
        }

        if (c != d)
            break;

        lhs.next();
        rhs.next();
    }

    if (lhs.more() || rhs.more())
        return int(lhs.get_pointer() - start);

    return -1;
}

//------------------------------------------------------------------------------
template <class T>
int str_compare(StrIterImpl<T>& lhs, StrIterImpl<T>& rhs)
{
    switch (StrCompareScope::current())
    {
    case StrCompareScope::relaxed:    return StrCompareImpl<T, 2>(lhs, rhs);
    case StrCompareScope::caseless: return StrCompareImpl<T, 1>(lhs, rhs);
    default:                          return StrCompareImpl<T, 0>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template <class T>
int str_compare(const T* lhs, const T* rhs)
{
    StrIterImpl<T> lhs_iter(lhs);
    StrIterImpl<T> rhs_iter(rhs);
    return str_compare(lhs_iter, rhs_iter);
}

//------------------------------------------------------------------------------
template <class T>
int str_compare(const StrImpl<T>& lhs, const StrImpl<T>& rhs)
{
    StrIterImpl<T> lhs_iter(lhs);
    StrIterImpl<T> rhs_iter(rhs);
    return str_compare(lhs_iter, rhs_iter);
}
