// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class TerminalIn
{
public:
    enum {
        input_none              = 0x80000000,
        input_timeout,
        input_abort,
        input_terminal_resize,
    };

    virtual         ~TerminalIn() = default;
    virtual void    begin() = 0;
    virtual void    end() = 0;
    virtual void    select() = 0;
    virtual int32   read() = 0;
};
