// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_match_generator.h"
#include "lua_bindable.h"
#include "lua_script_loader.h"
#include "lua_state.h"
#include "line_state_lua.h"
#include "match_builder_lua.h"

#include <lib/line_state.h>
#include <lib/matches.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

//------------------------------------------------------------------------------
LuaMatchGenerator::LuaMatchGenerator(LuaState& state)
: _state(state)
{
    lua_load_script(_state, lib, generator);
    lua_load_script(_state, lib, arguments);
}

//------------------------------------------------------------------------------
LuaMatchGenerator::~LuaMatchGenerator()
{
}

//------------------------------------------------------------------------------
void LuaMatchGenerator::print_error(const char* error) const
{
    puts("");
    puts(error);
}

//------------------------------------------------------------------------------
bool LuaMatchGenerator::generate(const LineState& line, MatchBuilder& Builder)
{
    lua_State* state = _state.get_state();

    // Call to Lua to generate matches.
    lua_getglobal(state, "clink");
    lua_pushliteral(state, "_generate");
    lua_rawget(state, -2);

    LineStateLua line_lua(line);
    line_lua.push(state);

    MatchBuilderLua builder_lua(Builder);
    builder_lua.push(state);

    if (lua_pcall(state, 2, 1, 0) != 0)
    {
        if (const char* error = lua_tostring(state, -1))
            print_error(error);

        lua_settop(state, 0);
        return false;
    }

    int use_matches = lua_toboolean(state, -1);
    lua_settop(state, 0);

    return !!use_matches;
}

//------------------------------------------------------------------------------
int LuaMatchGenerator::get_prefix_length(const LineState& line) const
{
    lua_State* state = _state.get_state();

    // Call to Lua to calculate prefix length.
    lua_getglobal(state, "clink");
    lua_pushliteral(state, "_get_prefix_length");
    lua_rawget(state, -2);

    LineStateLua line_lua(line);
    line_lua.push(state);

    if (lua_pcall(state, 1, 1, 0) != 0)
    {
        if (const char* error = lua_tostring(state, -1))
            print_error(error);

        lua_settop(state, 0);
        return 0;
    }

    int prefix = int(lua_tointeger(state, -1));
    lua_settop(state, 0);
    return prefix;
}
