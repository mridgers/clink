// Copyright (c) 2018 Martin Ridgers
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
    virtual void    write(const char* chars, int length) override;
    virtual void    flush() override;
    virtual int     get_columns() const override;
    virtual int     get_rows() const override;

private:
    void*           _stdout = nullptr;
    unsigned long   _prev_mode = 0;
    unsigned short  _default_attr = 0x07;
};
