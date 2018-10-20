// Copyright (c) 2018 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "fs_fixture.h"

#include <lua/lua_state.h>

extern "C" {
#include <lua.h>
}

//------------------------------------------------------------------------------
TEST_CASE("Lua os.glob*")
{
    // This test fs is sorted.
    static const char* glob_test_fs[] = {
        "zero", "one",
        "dir_zero\\", "dir_one\\",
        nullptr,
    };
    fs_fixture fs(glob_test_fs);

    lua_state lua;
    lua_State* state = lua.get_state();

    lua_getglobal(state, "os");

    int expected;
    SECTION("globfiles")
    {
        lua_pushliteral(state, "globfiles");
        lua_rawget(state, -2);
        expected = 4;
    }

    SECTION("globdirs")
    {
        lua_pushliteral(state, "globdirs");
        lua_rawget(state, -2);
        expected = 2;
    }

    lua_pushliteral(state, "*");
    lua_call(state, 1, 1);

    REQUIRE(lua_iscfunction(state, -1));

    while (true)
    {
        lua_pushvalue(state, -1);
        lua_call(state, 0, 1);
        if (lua_isnil(state, -1))
        {
            lua_pop(state, 1);
            break;
        }

        REQUIRE(lua_isstring(state, -1));
        const char* candidate = lua_tostring(state, -1);
        for (const char* fs_item : glob_test_fs)
        {
            if (strcmp(candidate, fs_item))
                continue;

            lua_pop(state, 1);
            --expected;
            break;
        }
    };

    REQUIRE(expected == 0);

    REQUIRE(lua_gettop(state) == 2);
    lua_pop(state, 2);
}

TEST_CASE("Lua os.globfiles(nil)")
{
    lua_state lua;
    lua_State* state = lua.get_state();

    lua_getglobal(state, "os");

    lua_pushliteral(state, "globfiles");
    lua_rawget(state, -2);
    lua_call(state, 0, 1);

    REQUIRE(lua_gettop(state) == 2);
    lua_pop(state, 2);
}
