// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

//------------------------------------------------------------------------------
int32 main(int32 argc, char** argv)
{
    const char* prefix = (argc > 1) ? argv[1] : "";
    return (clatch::run(prefix) != true);
}
