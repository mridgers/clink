// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "terminal.h"
#include "ecma48_terminal_out.h"
#include "win_screen_buffer.h"
#include "win_terminal_in.h"

#include <core/base.h>

//------------------------------------------------------------------------------
Terminal terminal_create(ScreenBuffer* screen)
{
#if defined(PLATFORM_WINDOWS)
    if (screen == nullptr)
        screen = new WinScreenBuffer(); // TODO: this leaks.

    return {
        new WinTerminalIn(),
        new Ecma48TerminalOut(*screen),
    };
#else
    return {};
#endif
}

//------------------------------------------------------------------------------
void terminal_destroy(const Terminal& terminal)
{
    delete terminal.out;
    delete terminal.in;
}
