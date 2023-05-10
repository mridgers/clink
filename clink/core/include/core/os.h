// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class StrBase;

//------------------------------------------------------------------------------
namespace os
{

enum {
    path_type_invalid,
    path_type_file,
    path_type_dir,
};

int32   get_path_type(const char* path);
int32   get_file_size(const char* path);
void    get_current_dir(StrBase& out);
bool    set_current_dir(const char* dir);
bool    make_dir(const char* dir);
bool    remove_dir(const char* dir);
bool    unlink(const char* path);
bool    move(const char* src_path, const char* dest_path);
bool    copy(const char* src_path, const char* dest_path);
bool    get_temp_dir(StrBase& out);
bool    get_env(const char* name, StrBase& out);
bool    set_env(const char* name, const char* value);

}; // namespace os
