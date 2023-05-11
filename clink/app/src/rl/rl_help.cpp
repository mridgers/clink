// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/base.h>
#include <core/settings.h>
#include <terminal/printer.h>

extern "C" {
#include <readline/readline.h>
}

//------------------------------------------------------------------------------
extern SettingInt g_max_width;

//------------------------------------------------------------------------------
static const char* get_function_name(int32 (*func_addr)(int32, int32))
{
    FUNMAP** funcs = funmap;
    while (*funcs != nullptr)
    {
        FUNMAP* func = *funcs;
        if (func->function == func_addr)
        {
            return func->name;
        }

        ++funcs;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
static void get_key_string(int32 i, int32 map_id, char* buffer)
{
    char* write = buffer;

    switch (map_id)
    {
    case 1: *write++ = 'A'; *write++ = '-'; break;
    case 2: strcpy(buffer, "C-x,"); write += 4; break;
    }

    if (i >= 0 && i < ' ')
    {
        static const char* ctrl_map = "@abcdefghijklmnopqrstuvwxyz[\\]^_";

        *write++ = 'C';
        *write++ = '-';
        *write++ = ctrl_map[i];
        *write++ = '\0';
        return;
    }

    *write++ = i;
    *write++ = '\0';
}

//------------------------------------------------------------------------------
static char** collect_keymap(
    Keymap map,
    char** collector,
    int32* offset,
    int32* max,
    int32 map_id)
{
    int32 i;

    for (i = 0; i < 127; ++i)
    {
        KEYMAP_ENTRY entry = map[i];
        if (entry.type == ISFUNC && entry.function != nullptr)
        {
            int32 blacklisted;
            int32 j;

            // Blacklist some functions
            int32 (*blacklist[])(int32, int32) = {
                rl_insert,
                rl_do_lowercase_version,
            };

            blacklisted = 0;
            for (j = 0; j < sizeof_array(blacklist); ++j)
            {
                if (blacklist[j] == entry.function)
                {
                    blacklisted = 1;
                    break;
                }
            }

            if (!blacklisted)
            {
                char* string;
                const char* name;
                char key[16];

                get_key_string(i, map_id, key);

                name = get_function_name(entry.function);
                if (name == nullptr)
                {
                    continue;
                }

                if (*offset >= *max)
                {
                    *max *= 2;
                    collector = (char**)realloc(collector, sizeof(char*) * *max);
                }

                string = (char*)malloc(strlen(key) + strlen(name) + 32);
                sprintf(string, "%-7s : %s", key, name);

                collector[*offset] = string;
                ++(*offset);
            }
        }
    }

    return collector;
}

//------------------------------------------------------------------------------
void show_rl_help(Printer& Printer)
{
    Keymap map = rl_get_keymap();
    int32 offset = 1;
    int32 max_collect = 64;
    char** collector = (char**)malloc(sizeof(char*) * max_collect);
    collector[0] = nullptr;

    // Build string up the functions in the active keymap.
    collector = collect_keymap(map, collector, &offset, &max_collect, 0);
    if (map[ESC].type == ISKMAP && map[ESC].function != nullptr)
    {
        Keymap esc_map = (KEYMAP_ENTRY*)(map[ESC].function);
        collector = collect_keymap(esc_map, collector, &offset, &max_collect, 1);
    }

    if (map == emacs_standard_keymap)
    {
        Keymap ctrlx_map = (KEYMAP_ENTRY*)(map[24].function);
        int32 type = map[24].type;
        if (type == ISKMAP && ctrlx_map != nullptr)
        {
            collector = collect_keymap(ctrlx_map, collector, &offset, &max_collect, 2);
        }
    }

    // Find the longest match.
    int32 longest = 0;
    for (int32 i = 1; i < offset; ++i)
    {
        int32 l = (int32)strlen(collector[i]);
        if (l > longest)
            longest = l;
    }

    // Display the Matches.
    Printer.print("\n");

    int32 max_width = min<int32>(Printer.get_columns() - 3, g_max_width.get());
    int32 columns = max(1, max_width / (longest + 1));
    for (int32 i = 1, j = columns - 1; i < offset; ++i, --j)
    {
        const char* match = collector[i];

        int32 length = int32(strlen(match));
        Printer.print(match, length);

        const char spaces[] = "                                         ";
        int32 space_count = max(longest - length, 0) + 1;
        Printer.print(spaces, min<int32>(sizeof(spaces) - 1, space_count));

        if (j)
            continue;

        j = columns;
        Printer.print("\n");
    }

    // Tidy up (N.B. the first match is a placeholder and shouldn't be freed).
    while (--offset)
        free(collector[offset]);
    free(collector);
}
