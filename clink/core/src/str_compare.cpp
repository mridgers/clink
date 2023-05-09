// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "str_compare.h"

threadlocal int StrCompareScope::ts_mode = StrCompareScope::exact;

//------------------------------------------------------------------------------
StrCompareScope::StrCompareScope(int mode)
{
    m_prev_mode = ts_mode;
    ts_mode = mode;
}

//------------------------------------------------------------------------------
StrCompareScope::~StrCompareScope()
{
    ts_mode = m_prev_mode;
}

//------------------------------------------------------------------------------
int StrCompareScope::current()
{
    return ts_mode;
}
