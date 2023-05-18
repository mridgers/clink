// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "terminal.h"
#include "ecma48_terminal_out.h"
#include "win_screen_buffer.h"
#include "win_terminal_in.h"
#include "win_terminal_out.h"

#include <core/base.h>

//------------------------------------------------------------------------------
#if defined(PLATFORM_WINDOWS)
struct WinVirtualTerminal
{
    DWORD           prev_mode = ~0u;
    ScreenBuffer*   screen = nullptr;
};
#endif

//------------------------------------------------------------------------------
Terminal terminal_create(ScreenBuffer* screen)
{
#if defined(PLATFORM_WINDOWS)
    auto* impl = new WinVirtualTerminal();

    Terminal terminal;
    terminal.in = new WinTerminalIn();
    terminal.out = nullptr;
    terminal.impl = uintptr_t(impl);

    // Try and use Win10's VT100 support
    DWORD prev_mode;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(handle, &prev_mode))
    {
        if (SetConsoleMode(handle, prev_mode|ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        {
            impl->prev_mode = prev_mode;
            terminal.out = new WinTerminalOut();
        }
    }

    // If it wasn't possible to enable VT100 support we'll fallback to Clink's
    if (terminal.out == nullptr)
    {
        if (screen == nullptr)
        {
            screen = new WinScreenBuffer();
            impl->screen = screen;
        }

        terminal.out = new Ecma48TerminalOut(*screen);
    }

    return terminal;
#else
    return {};
#endif
}

//------------------------------------------------------------------------------
void terminal_destroy(const Terminal& terminal)
{
    delete terminal.out;
    delete terminal.in;

    auto* impl = (WinVirtualTerminal*)(terminal.impl);

    if (impl->prev_mode != ~0u)
    {
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleMode(handle, impl->prev_mode);
    }

    if (impl->screen != nullptr)
        delete impl->screen;

    delete impl;
}
