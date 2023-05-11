// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "line_buffer.h"

//------------------------------------------------------------------------------
class RlBuffer
    : public LineBuffer
{
public:
    virtual void            reset() override;
    virtual void            begin_line() override;
    virtual void            end_line() override;
    virtual const char*     get_buffer() const override;
    virtual uint32          get_length() const override;
    virtual uint32          get_cursor() const override;
    virtual uint32          set_cursor(uint32 pos) override;
    virtual bool            insert(const char* text) override;
    virtual bool            remove(uint32 from, uint32 to) override;
    virtual void            draw() override;
    virtual void            redraw() override;
    virtual void            begin_undo_group() override;
    virtual void            end_undo_group() override;

private:
    bool                    _need_draw;
};
