// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "terminal_out.h"

//------------------------------------------------------------------------------
class WinTerminalOut
    : public TerminalOut
{
public:
    virtual void    begin() override;
    virtual void    end() override;
    virtual void    write(const char* chars, int32 length) override;
    virtual void    flush() override;
    virtual int32   get_columns() const override;
    virtual int32   get_rows() const override;

private:
    void*           _stdout = nullptr;
    uint32          _prev_mode = 0;
    uint16          _default_attr = 0x07;
};
