// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/str.h>

//------------------------------------------------------------------------------
class FsFixture
{
public:
                    FsFixture(const char** fs=nullptr);
                    ~FsFixture();
    const char*     get_root() const;

private:
    void            clean(const char* path);
    Str<>           _root;
    const char**    _fs;
};
