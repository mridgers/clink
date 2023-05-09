// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class StrBase;

//------------------------------------------------------------------------------
namespace path
{

void        normalise(StrBase& in_out, int sep=0);
void        normalise(char* in_out, int sep=0);
bool        is_separator(int c);
const char* next_element(const char* in);
bool        get_base_name(const char* in, StrBase& out);
bool        get_directory(const char* in, StrBase& out);
bool        get_directory(StrBase& in_out);
bool        get_drive(const char* in, StrBase& out);
bool        get_drive(StrBase& in_out);
bool        get_extension(const char* in, StrBase& out);
bool        get_name(const char* in, StrBase& out);
const char* get_name(const char* in);
bool        is_rooted(const char* path);
bool        is_root(const char* path);
bool        join(const char* lhs, const char* rhs, StrBase& out);
bool        append(StrBase& out, const char* rhs);

}; // namespace path
