// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
struct InjectArgs
{
    // Must be kept simple as it's blitted
    // from one Process to another.

    char    profile_path[512];
    bool    quiet;
    bool    no_log;
};
