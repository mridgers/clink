// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include "fs_fixture.h"

#include <core/base.h>
#include <lua/lua_state.h>

extern "C" {
#include <lua.h>
}

//------------------------------------------------------------------------------
TEST_CASE("Lua io.popen2 - read")
{
    // This Test fs is sorted.
    static const char* io_test_fs[] = {
        "ad", "assumenda", "atque", "commodi", "doloremque", "est", "et", "in",
        "optio", "quia", "rem", "sunt", "tenetur", "ullam", "voluptas", nullptr,
    };
    FsFixture fs(io_test_fs);

    LuaState lua;
    lua_State* state = lua.get_state();

    lua_getglobal(state, "io");

    lua_pushliteral(state, "popen2");
    lua_rawget(state, -2);

    lua_pushliteral(state, "cmd.exe /c dir /b /a-d /on");
    lua_call(state, 1, 1);

    SECTION("read()")
    {
        lua_pushliteral(state, "read");
        lua_gettable(state, -2);

        bool use_l_arg;
        SECTION("read()")    { use_l_arg = false; }
        SECTION("read('l')") { use_l_arg = true; }

        auto Test = [state, use_l_arg] (const char* expected) {
            lua_pushvalue(state, -1); // read() method
            lua_pushvalue(state, -3); // file 'this' object
            if (use_l_arg)
                lua_pushliteral(state, "l");
            lua_call(state, 1 + use_l_arg, 1);
            if (expected != nullptr)
            {
                REQUIRE(lua_isstring(state, -1));
                const char* ret = lua_tostring(state, -1);
                REQUIRE(strcmp(ret, expected) == 0);
            }
            else
                REQUIRE(lua_isnil(state, -1));
            lua_pop(state, 1);
        };

        for (const char* candidate : io_test_fs)
            Test(candidate);

        lua_pop(state, 1);
    }

    SECTION("lines()")
    {
        lua_pushliteral(state, "lines");
        lua_gettable(state, -2);

        lua_pushvalue(state, -2); // file 'this' object
        lua_call(state, 1, 1);

        auto Test = [state] (const char* expected) {
            lua_pushvalue(state, -1);
            lua_call(state, 0, 1);
            if (expected != nullptr)
            {
                const char* ret = lua_tostring(state, -1);
                REQUIRE(strcmp(ret, expected) == 0, [&] () {
                    printf("[%s]\n", expected);
                    printf("[%s]\n", ret);
                });
            }
            else
                REQUIRE(lua_isnil(state, -1));
            lua_pop(state, 1);
        };

        for (const char* candidate : io_test_fs)
            Test(candidate);

        lua_pop(state, 1);
    }

    SECTION("read(X)")
    {
        lua_pushliteral(state, "read");
        lua_gettable(state, -2);

        SECTION("X = 'a'")
        {
            lua_pushvalue(state, -2);    // file 'this' object
            lua_pushliteral(state, "a");
            lua_call(state, 2, 1);
        }

        SECTION("X = ?")
        {
            SECTION("? = 'L'")   { lua_pushliteral(state, "L"); }
            SECTION("? = 7")     { lua_pushinteger(state, 7); }
            SECTION("? = 8")     { lua_pushinteger(state, 8); }
            SECTION("? = 57")    { lua_pushinteger(state, 57); }
            SECTION("? = 1999")  { lua_pushinteger(state, 1999); }

            lua_pushliteral(state, "");
            do
            {
                lua_pushvalue(state, -3);       // popen2.read()
                lua_pushvalue(state, -5);       // file 'this' object
                lua_pushvalue(state, -4);       // Test read() arg
                lua_call(state, 2, 1);
                if (lua_isnil(state, -1))
                {
                    lua_pop(state, 1);
                    lua_remove(state, -2);      // Test read() arg
                    lua_remove(state, -2);      // read()
                    break;
                }

                REQUIRE(lua_isstring(state, -1));
                lua_concat(state, 2);
            } while (true);
        }

        REQUIRE(lua_isstring(state, -1));

        const char* test_data = lua_tostring(state, -1);
        for (const char* candidate : io_test_fs)
        {
            if (candidate == nullptr)
            {
                REQUIRE(*test_data == '\0');
                break;
            }

            for (; *candidate; ++candidate, ++test_data)
                REQUIRE(*candidate == *test_data);

            REQUIRE(test_data[0] == '\r');
            REQUIRE(test_data[1] == '\n');
            test_data += 2;
        }

        lua_pop(state, 1);
    }

    REQUIRE(lua_gettop(state) == 2);
    lua_pop(state, 2);
}

//------------------------------------------------------------------------------
TEST_CASE("Lua io.popen2 - write")
{
    LuaState lua;
    lua_State* state = lua.get_state();

    lua_getglobal(state, "io");

    lua_pushliteral(state, "popen2");
    lua_rawget(state, -2);

    lua_pushliteral(state, "cmd.exe /c set /p 0x493_0x493=&set 0x493");
    lua_call(state, 1, 1);

    lua_pushliteral(state, "write");
    lua_gettable(state, -2);

    // write([data])
    lua_pushvalue(state, -1); // write()
    lua_pushvalue(state, -3); // popen2 object
    lua_pushliteral(state, "CLINK_TEST");
    lua_call(state, 2, 0);

    // write(...)
    lua_pushvalue(state, -2); // popen2 object

    SECTION("write()")
    {
        lua_call(state, 1, 0);
    }

    SECTION("write(nil)")
    {
        lua_pushnil(state);
        lua_call(state, 2, 0);
    }

    SECTION("write('\r\n')")
    {
        lua_pushliteral(state, "\r\n");
        lua_call(state, 2, 0);
    }

    // read()
    lua_pushliteral(state, "read");
    lua_gettable(state, -2);
    lua_pushvalue(state, -2); // popen2 object
    lua_call(state, 1, 1);

    REQUIRE(lua_isstring(state, -1));
    const char* result = lua_tostring(state, -1);
    REQUIRE(strcmp(result, "0x493_0x493=CLINK_TEST") == 0);
    lua_pop(state, 1);

    REQUIRE(lua_gettop(state) == 2);
    lua_pop(state, 2);
}
