// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class ScreenBuffer;
class TerminalIn;
class TerminalOut;

//------------------------------------------------------------------------------
struct Terminal
{
    TerminalIn*     in;
    TerminalOut*    out;
};

//------------------------------------------------------------------------------
Terminal            terminal_create(ScreenBuffer* screen=nullptr);
void                terminal_destroy(const Terminal& terminal);
