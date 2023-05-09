// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/array.h>
#include <core/str.h>
#include <core/str_iter.h>

//------------------------------------------------------------------------------
class DoskeyAlias
{
public:
                    DoskeyAlias();
    void            reset();
    bool            next(WstrBase& out);
    explicit        operator bool () const;

private:
    friend class    Doskey;
    Wstr<32>        m_buffer;
    const wchar_t*  m_cursor;
};



//------------------------------------------------------------------------------
class Doskey
{
public:
                    Doskey(const char* shell_name);
    bool            add_alias(const char* alias, const char* text);
    bool            remove_alias(const char* alias);
    void            resolve(const wchar_t* chars, DoskeyAlias& out);

private:
    bool            resolve_impl(const WstrIter& in, class WstrStream* out);
    const char*     m_shell_name;
};
