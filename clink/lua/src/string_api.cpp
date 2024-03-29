// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"

#include <core/str_hash.h>
#include <core/str_tokeniser.h>

//------------------------------------------------------------------------------
static const char* get_string(lua_State* state, int32 index)
{
    if (lua_gettop(state) < index || !lua_isstring(state, index))
        return nullptr;

    return lua_tostring(state, index);
}

//------------------------------------------------------------------------------
/// -name:  string.hash
/// -arg:   x
/// -ret:   x
static int32 hash(lua_State* state)
{
    const char* in = get_string(state, 1);
    if (in == nullptr)
        return 0;

    lua_pushinteger(state, str_hash(in));
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  string.explode
/// -arg:   x
/// -ret:   x
static int32 explode(lua_State* state)
{
    const char* in = get_string(state, 1);
    if (in == nullptr)
        return 0;

    const char* delims = get_string(state, 2);
    if (delims == nullptr)
        delims = " ";

    StrTokeniser tokens(in, delims);

    if (const char* quote_pair = get_string(state, 3))
        tokens.add_quote_pair(quote_pair);

    lua_createtable(state, 16, 0);

    int32 count = 0;
    const char* start;
    int32 length;
    while (StrToken token = tokens.next(start, length))
    {
        lua_pushlstring(state, start, length);
        lua_rawseti(state, -2, ++count);
    }

    return 1;
}

//------------------------------------------------------------------------------
void string_lua_initialise(LuaState& lua)
{
    struct {
        const char* name;
        int32       (*method)(lua_State*);
    } methods[] = {
        { "hash",       &hash },
        { "explode",    &explode },
    };

    lua_State* state = lua.get_state();

    lua_getglobal(state, "string");

    for (const auto& method : methods)
    {
        lua_pushstring(state, method.name);
        lua_pushcfunction(state, method.method);
        lua_rawset(state, -3);
    }

    lua_pop(state, 1);
}
