// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "os.h"
#include "path.h"
#include "str.h"

namespace os
{

//------------------------------------------------------------------------------
int32 get_path_type(const char* path)
{
    Wstr<280> wpath(path);
    DWORD attr = GetFileAttributesW(wpath.c_str());
    if (attr == ~0)
        return path_type_invalid;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return path_type_dir;

    return path_type_file;
}

//------------------------------------------------------------------------------
int32 get_file_size(const char* path)
{
    Wstr<280> wpath(path);
    void* handle = CreateFileW(wpath.c_str(), 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    int32 ret = GetFileSize(handle, nullptr); // 2Gb max I suppose...
    CloseHandle(handle);
    return ret;
}

//------------------------------------------------------------------------------
void get_current_dir(StrBase& out)
{
    Wstr<280> wdir;
    GetCurrentDirectoryW(wdir.size(), wdir.data());
    out = wdir.c_str();
}

//------------------------------------------------------------------------------
bool set_current_dir(const char* dir)
{
    Wstr<280> wdir(dir);
    return (SetCurrentDirectoryW(wdir.c_str()) == TRUE);
}

//------------------------------------------------------------------------------
bool make_dir(const char* dir)
{
    int32 Type = get_path_type(dir);
    if (Type == path_type_dir)
        return true;

    Str<> next;
    path::get_directory(dir, next);

    if (!next.empty() && !path::is_root(next.c_str()))
        if (!make_dir(next.c_str()))
            return false;

    if (*dir)
    {
        Wstr<280> wdir(dir);
        return (CreateDirectoryW(wdir.c_str(), nullptr) == TRUE);
    }

    return true;
}

//------------------------------------------------------------------------------
bool remove_dir(const char* dir)
{
    Wstr<280> wdir(dir);
    return (RemoveDirectoryW(wdir.c_str()) == TRUE);
}

//------------------------------------------------------------------------------
bool unlink(const char* path)
{
    Wstr<280> wpath(path);
    return (DeleteFileW(wpath.c_str()) == TRUE);
}

//------------------------------------------------------------------------------
bool move(const char* src_path, const char* dest_path)
{
    Wstr<280> wsrc_path(src_path);
    Wstr<280> wdest_path(dest_path);
    return (MoveFileW(wsrc_path.c_str(), wdest_path.c_str()) == TRUE);
}

//------------------------------------------------------------------------------
bool copy(const char* src_path, const char* dest_path)
{
    Wstr<280> wsrc_path(src_path);
    Wstr<280> wdest_path(dest_path);
    return (CopyFileW(wsrc_path.c_str(), wdest_path.c_str(), FALSE) == TRUE);
}

//------------------------------------------------------------------------------
bool get_temp_dir(StrBase& out)
{
    Wstr<280> wout;
    uint32 size = GetTempPathW(wout.size(), wout.data());
    if (!size)
        return false;

    if (size >= wout.size())
    {
        wout.reserve(size);
        if (!GetTempPathW(wout.size(), wout.data()))
            return false;
    }

    out = wout.c_str();
    return true;
}

//------------------------------------------------------------------------------
bool get_env(const char* name, StrBase& out)
{
    Wstr<32> wname(name);

    int32 len = GetEnvironmentVariableW(wname.c_str(), nullptr, 0);
    if (!len)
        return false;

    Wstr<> wvalue;
    wvalue.reserve(len);
    len = GetEnvironmentVariableW(wname.c_str(), wvalue.data(), wvalue.size());

    out.reserve(len);
    out = wvalue.c_str();
    return true;
}

//------------------------------------------------------------------------------
bool set_env(const char* name, const char* value)
{
    Wstr<32> wname(name);

    Wstr<64> wvalue;
    if (value != nullptr)
        wvalue = value;

    const wchar_t* value_arg = (value != nullptr) ? wvalue.c_str() : nullptr;
    return (SetEnvironmentVariableW(wname.c_str(), value_arg) != 0);
}

}; // namespace os
