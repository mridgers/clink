// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/str.h>

#define Str         Wstr
#define STR(x)      L##x
#define NAME_SUFFIX " (wchar_t)"
#include "str.cpp"
#undef Str
