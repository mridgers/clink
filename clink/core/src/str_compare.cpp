// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "str_compare.h"

threadlocal int32 StrCompareScope::ts_mode = StrCompareScope::exact;

//------------------------------------------------------------------------------
StrCompareScope::StrCompareScope(int32 mode)
{
    _prev_mode = ts_mode;
    ts_mode = mode;
}

//------------------------------------------------------------------------------
StrCompareScope::~StrCompareScope()
{
    ts_mode = _prev_mode;
}

//------------------------------------------------------------------------------
int32 StrCompareScope::current()
{
    return ts_mode;
}
