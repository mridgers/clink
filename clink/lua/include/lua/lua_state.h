// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

struct lua_State;

//------------------------------------------------------------------------------
class LuaState
{
public:
                    LuaState();
                    ~LuaState();
    void            initialise();
    void            shutdown();
    bool            do_string(const char* string, int length=-1);
    bool            do_file(const char* path);
    lua_State*      get_state() const;

private:
    lua_State*      m_state;
};

//------------------------------------------------------------------------------
inline lua_State* LuaState::get_state() const
{
    return m_state;
}
