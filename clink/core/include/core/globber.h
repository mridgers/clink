// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "str.h"

#include <Windows.h>

//------------------------------------------------------------------------------
class Globber
{
public:
                        Globber(const char* pattern);
                        ~Globber();
    void                files(bool state)       { _files = state; }
    void                directories(bool state) { _directories = state; }
    void                suffix_dirs(bool state) { _dir_suffix = state; }
    void                hidden(bool state)      { _hidden = state; }
    void                system(bool state)      { _system = state; }
    void                dots(bool state)        { _dots = state; }
    bool                next(StrBase& out, bool rooted=true);

private:
                        Globber(const Globber&) = delete;
    void                operator = (const Globber&) = delete;
    void                next_file();
    WIN32_FIND_DATAW    _data;
    HANDLE              _handle;
    Str<280>            _root;
    bool                _files;
    bool                _directories;
    bool                _dir_suffix;
    bool                _hidden;
    bool                _system;
    bool                _dots;
};
