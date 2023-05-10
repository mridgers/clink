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
    virtual int     read() override;

private:
    void            read_console();
    void            process_input(const KEY_EVENT_RECORD& key_event);
    void            push(unsigned int value);
    void            push(const char* seq);
    unsigned char   pop();
    void*           _stdin = nullptr;
    unsigned int    _dimensions = 0;
    unsigned long   _prev_mode = 0;
    unsigned char   _buffer_head = 0;
    unsigned char   _buffer_count = 0;
    unsigned char   _buffer[16]; // must be power of two.
};
