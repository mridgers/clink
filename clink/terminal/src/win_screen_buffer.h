// Copyright (c) 2018 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "screen_buffer.h"

//------------------------------------------------------------------------------
class WinScreenBuffer
    : public ScreenBuffer
{
public:
    virtual void    begin() override;
    virtual void    end() override;
    virtual void    write(const char* data, int32 length) override;
    virtual void    flush() override;
    virtual int32   get_columns() const override;
    virtual int32   get_rows() const override;
    virtual void    clear(ClearType Type) override;
    virtual void    clear_line(ClearType Type) override;
    virtual void    set_cursor(int32 column, int32 row) override;
    virtual void    move_cursor(int32 dx, int32 dy) override;
    virtual void    insert_chars(int32 count) override;
    virtual void    delete_chars(int32 count) override;
    virtual void    set_attributes(const Attributes attr) override;

private:
    enum : uint16
    {
        attr_mask_fg        = 0x000f,
        attr_mask_bg        = 0x00f0,
        attr_mask_bold      = 0x0008,
        attr_mask_underline = 0x8000,
        attr_mask_all       = attr_mask_fg|attr_mask_bg|attr_mask_underline,
    };

    void*           _handle = nullptr;
    uint32          _prev_mode = 0;
    uint16          _default_attr = 0x07;
    bool            _bold = false;
};
