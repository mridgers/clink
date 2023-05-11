// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"

#include <core/base.h>
#include <core/path.h>
#include <core/str.h>

//------------------------------------------------------------------------------
/// -name:  clink.getscreeninfo
/// -ret:   table
/// Returns dimensions of the Terminal's buffer (buf*) and visible window (win*).
/// The returned table has the following scheme; { bufwidth:int32, bufheight:int32,
/// winwidth:int32, winheight:int32 }.
static int32 get_screen_info(lua_State* state)
{
    int32 i;
    int32 buffer_width, buffer_height;
    int32 window_width, window_height;
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    buffer_width = csbi.dwSize.X;
    buffer_height = csbi.dwSize.Y;
    window_width = csbi.srWindow.Right - csbi.srWindow.Left;
    window_height = csbi.srWindow.Bottom - csbi.srWindow.Top;

    lua_createtable(state, 0, 4);
    {
        struct {
            const char* name;
            int32       value;
        } table[] = {
            { "bufwidth",   buffer_width },
            { "bufheight",  buffer_height },
            { "winwidth",   window_width },
            { "winheight",  window_height },
        };

        for (i = 0; i < sizeof_array(table); ++i)
        {
            lua_pushstring(state, table[i].name);
            lua_pushinteger(state, table[i].value);
            lua_rawset(state, -3);
        }
    }

    return 1;
}

//------------------------------------------------------------------------------
void clink_lua_initialise(LuaState& lua)
{
    struct {
        const char* name;
        int32       (*method)(lua_State*);
    } methods[] = {
// TODO : move this somewhere else.
        { "getscreeninfo",  &get_screen_info },
//
    };

    lua_State* state = lua.get_state();

    lua_createtable(state, sizeof_array(methods), 0);

    for (const auto& method : methods)
    {
        lua_pushstring(state, method.name);
        lua_pushcfunction(state, method.method);
        lua_rawset(state, -3);
    }

    lua_setglobal(state, "clink");
}
