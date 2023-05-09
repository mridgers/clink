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
    void                files(bool state)       { m_files = state; }
    void                directories(bool state) { m_directories = state; }
    void                suffix_dirs(bool state) { m_dir_suffix = state; }
    void                hidden(bool state)      { m_hidden = state; }
    void                system(bool state)      { m_system = state; }
    void                dots(bool state)        { m_dots = state; }
    bool                next(StrBase& out, bool rooted=true);

private:
                        Globber(const Globber&) = delete;
    void                operator = (const Globber&) = delete;
    void                next_file();
    WIN32_FIND_DATAW    m_data;
    HANDLE              m_handle;
    Str<280>            m_root;
    bool                m_files;
    bool                m_directories;
    bool                m_dir_suffix;
    bool                m_hidden;
    bool                m_system;
    bool                m_dots;
};
