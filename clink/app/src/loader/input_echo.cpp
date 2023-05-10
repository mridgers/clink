// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/str.h>
#include <terminal/terminal.h>
#include <terminal/terminal_in.h>

//------------------------------------------------------------------------------
int32 input_echo(int32 argc, char** argv)
{
    for (int32 i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];
        if (_stricmp(arg, "--help") == 0 || _stricmp(arg, "-h") == 0)
        {
            extern const char* g_clink_header;
            puts(g_clink_header);
            printf("Usage: %s\n\n", argv[0]);
            puts("Echos the sequence of characters for each key pressed.\n");
            return 0;
        }
    }

    Terminal terminal = terminal_create();
    TerminalIn& Input = *terminal.in;
    Input.begin();

    bool quit = false;
    while (!quit)
    {
        Input.select();
        while (1)
        {
            int32 c = Input.read();
            if (c < 0)
                break;

            if (c > 0x7f)
                printf("\\x%02x", uint32(c));
            else if (c < 0x20)
                printf("^%c", c|0x40);
            else
                printf("%c", c);

            if (quit = (c == ('C' & 0x1f))) // Ctrl-c
                break;
        }

        puts("");
    }

    Input.end();
    return 0;
}
