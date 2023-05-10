// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "globber.h"
#include "os.h"
#include "path.h"

//------------------------------------------------------------------------------
Globber::Globber(const char* pattern)
: _files(true)
, _directories(true)
, _dir_suffix(true)
, _hidden(false)
, _system(false)
, _dots(false)
{
    // Windows: Expand if the path to complete is drive relative (e.g. 'c:foobar')
    // Drive X's current path is stored in the environment variable "=X:"
    Str<288> rooted;
    if (pattern[0] && pattern[1] == ':' && pattern[2] != '\\' && pattern[2] != '/')
    {
        char env_var[4] = { '=', pattern[0], ':', 0 };
        if (os::get_env(env_var, rooted))
        {
            rooted << "/";
            rooted << (pattern + 2);
            pattern = rooted.c_str();
        }
    }

    Wstr<280> wglob(pattern);
    _handle = FindFirstFileW(wglob.c_str(), &_data);
    if (_handle == INVALID_HANDLE_VALUE)
        _handle = nullptr;

    path::get_directory(pattern, _root);
}

//------------------------------------------------------------------------------
Globber::~Globber()
{
    if (_handle != nullptr)
        FindClose(_handle);
}

//------------------------------------------------------------------------------
bool Globber::next(StrBase& out, bool rooted)
{
    if (_handle == nullptr)
        return false;

    Str<280> file_name(_data.cFileName);

    bool skip = false;

    const wchar_t* c = _data.cFileName;
    skip |= (c[0] == '.' && (!c[1] || (c[1] == '.' && !c[2])) && !_dots);

    int32 attr = _data.dwFileAttributes;
    skip |= (attr & FILE_ATTRIBUTE_SYSTEM) && !_system;
    skip |= (attr & FILE_ATTRIBUTE_HIDDEN) && !_hidden;
    skip |= (attr & FILE_ATTRIBUTE_DIRECTORY) && !_directories;
    skip |= !(attr & FILE_ATTRIBUTE_DIRECTORY) && !_files;

    next_file();

    if (skip)
        return next(out, rooted);

    out.clear();
    if (rooted)
        out << _root;

    path::append(out, file_name.c_str());

    if (attr & FILE_ATTRIBUTE_DIRECTORY && _dir_suffix)
        out << "\\";

    return true;
}

//------------------------------------------------------------------------------
void Globber::next_file()
{
    if (FindNextFileW(_handle, &_data))
        return;

    FindClose(_handle);
    _handle = nullptr;
}
