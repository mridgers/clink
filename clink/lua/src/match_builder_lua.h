// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "lua_bindable.h"

class MatchBuilder;
struct lua_State;

//------------------------------------------------------------------------------
class MatchBuilderLua
    : public LuaBindable<MatchBuilderLua>
{
public:
                    MatchBuilderLua(MatchBuilder& Builder);
                    ~MatchBuilderLua();
    int             add_match(lua_State* state);
    int             add_matches(lua_State* state);
    int             set_prefix_included(lua_State* state);

private:
    bool            add_match_impl(lua_State* state, int stack_index);
    MatchBuilder&   _builder;
};
