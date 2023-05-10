// Copyright (c) 2018 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "terminal_in.h"

//------------------------------------------------------------------------------
class WinTerminalIn
    : public TerminalIn
{
public:
    virtual void    begin() override;
    virtual void    end() override;
    virtual void    select() override;
    virtual int32   read() override;

private:
    void            read_console();
    void            process_input(const KEY_EVENT_RECORD& key_event);
    void            push(uint32 value);
    void            push(const char* seq);
    uint8           pop();
    void*           _stdin = nullptr;
    uint32          _dimensions = 0;
    uint32          _prev_mode = 0;
    uint8           _buffer_head = 0;
    uint8           _buffer_count = 0;
    uint8           _buffer[16]; // must be power of two.
};
