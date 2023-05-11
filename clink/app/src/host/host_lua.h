// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <lua/lua_match_generator.h>
#include <lua/lua_state.h>

//------------------------------------------------------------------------------
class HostLua
{
public:
                        HostLua();
                        operator LuaState& ();
                        operator MatchGenerator& ();
    void                load_scripts();

private:
    void                load_scripts(const char* paths);
    void                load_script(const char* path);
    LuaState            _state;
    LuaMatchGenerator _generator;
};
