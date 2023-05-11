// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "match_builder_lua.h"

#include <core/base.h>
#include <core/str.h>
#include <lib/matches.h>

//------------------------------------------------------------------------------
static MatchBuilderLua::Method g_methods[] = {
    { "addmatch",           &MatchBuilderLua::add_match },
    { "addmatches",         &MatchBuilderLua::add_matches },
    { "setprefixincluded",  &MatchBuilderLua::set_prefix_included },
    {}
};



//------------------------------------------------------------------------------
MatchBuilderLua::MatchBuilderLua(MatchBuilder& Builder)
: LuaBindable<MatchBuilderLua>("MatchBuilderLua", g_methods)
, _builder(Builder)
{
}

//------------------------------------------------------------------------------
MatchBuilderLua::~MatchBuilderLua()
{
}

//------------------------------------------------------------------------------
/// -name:  Builder:addmatch
/// -arg:   match:string|table
/// -ret:   boolean
int32 MatchBuilderLua::add_match(lua_State* state)
{
    int32 ret = 0;
    if (lua_gettop(state) > 0)
        ret = !!add_match_impl(state, 1);

    lua_pushboolean(state, ret);
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  Builder:setprefixincluded
/// -arg:   [state:boolean]
int32 MatchBuilderLua::set_prefix_included(lua_State* state)
{
    bool included = true;
    if (lua_gettop(state) > 0)
        included = (lua_toboolean(state, 1) != 0);

    _builder.set_prefix_included(included);

    return 0;
}

//------------------------------------------------------------------------------
/// -name:  Builder:addmatches
/// -arg:   matches:table or function
/// -ret:   integer, boolean
/// This is the equivalent of calling Builder:addmatch() in a for-loop. Returns
/// the number of matches added and a boolean indicating if all matches were
/// added successfully. If matches is a function is called until it returns nil.
int32 MatchBuilderLua::add_matches(lua_State* state)
{
    int32 count = 0;
    int32 total = -1;

    if (lua_gettop(state) > 0)
    {
        if (lua_istable(state, 1))
        {
            total = int32(lua_rawlen(state, 1));
            for (int32 i = 1; i <= total; ++i)
            {
                lua_rawgeti(state, 1, i);
                count += !!add_match_impl(state, -1);
                lua_pop(state, 1);
            }
        }
        else if (lua_isfunction(state, 1))
        {
            for (total = 0;; ++total)
            {
                lua_pushvalue(state, 1);
                lua_call(state, 0, 1);
                if (lua_isnil(state, -1))
                    break;

                count += !!add_match_impl(state, -1);
                lua_pop(state, 1);
            }
        }

        return 2;
    }

    lua_pushinteger(state, count);
    lua_pushboolean(state, count == total);
    return 2;
}

//------------------------------------------------------------------------------
bool MatchBuilderLua::add_match_impl(lua_State* state, int32 stack_index)
{
    if (lua_isstring(state, stack_index))
    {
        const char* match = lua_tostring(state, stack_index);
        return _builder.add_match(match);
    }
    else if (lua_istable(state, stack_index))
    {
        if (stack_index < 0)
            --stack_index;

        MatchDesc desc = {};

        lua_pushliteral(state, "match");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.match = lua_tostring(state, -1);
        lua_pop(state, 1);

        lua_pushliteral(state, "displayable");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.displayable = lua_tostring(state, -1);
        lua_pop(state, 1);

        lua_pushliteral(state, "aux");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.aux = lua_tostring(state, -1);
        lua_pop(state, 1);

        lua_pushliteral(state, "suffix");
        lua_rawget(state, stack_index);
        if (lua_isstring(state, -1))
            desc.suffix = lua_tostring(state, -1)[0];
        lua_pop(state, 1);

        if (desc.match != nullptr)
            return _builder.add_match(desc);
    }

    return false;
}
