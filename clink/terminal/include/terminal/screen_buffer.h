// Copyright (c) 2018 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "attributes.h"

//------------------------------------------------------------------------------
class ScreenBuffer
{
public:
    enum ClearType
    {
        clear_type_before,
        clear_type_after,
        clear_type_all,
    };

    virtual         ~ScreenBuffer() = default;
    virtual void    begin() = 0;
    virtual void    end() = 0;
    virtual void    write(const char* data, int length) = 0;
    virtual void    flush() = 0;
    virtual int     get_columns() const = 0;
    virtual int     get_rows() const = 0;
    virtual void    clear(ClearType Type) = 0;
    virtual void    clear_line(ClearType Type) = 0;
    virtual void    set_cursor(int column, int row) = 0;
    virtual void    move_cursor(int dx, int dy) = 0;
    virtual void    insert_chars(int count) = 0;
    virtual void    delete_chars(int count) = 0;
    virtual void    set_attributes(const Attributes attr) = 0;
};
