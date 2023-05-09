// Copyright (c) 2016 Martin Ridgers
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
    int                 get_line(lua_State* state);
    int                 get_cursor(lua_State* state);
    int                 get_command_offset(lua_State* state);
    int                 get_word_count(lua_State* state);
    int                 get_word_info(lua_State* state);
    int                 get_word(lua_State* state);
    int                 get_end_word(lua_State* state);

private:
    const LineState&    m_line;
};
