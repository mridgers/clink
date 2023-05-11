// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "lua_bindable.h"

class LineState;
struct lua_State;

//------------------------------------------------------------------------------
class LineStateLua
    : public LuaBindable<LineStateLua>
{
public:
                        LineStateLua(const LineState& line);
    int32               get_line(lua_State* state);
    int32               get_cursor(lua_State* state);
    int32               get_command_offset(lua_State* state);
    int32               get_word_count(lua_State* state);
    int32               get_word_info(lua_State* state);
    int32               get_word(lua_State* state);
    int32               get_end_word(lua_State* state);

private:
    const LineState&    _line;
};
