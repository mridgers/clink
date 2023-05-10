// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"

#if defined(CLINK_FINAL)

//------------------------------------------------------------------------------
void lua_load_script_impl(LuaState& state, const char* script, int32 length)
{
    state.do_string(script, length);
}

#else // CLINK_FINAL

//------------------------------------------------------------------------------
void lua_load_script_impl(LuaState& state, const char* path, int32 length)
{
    state.do_file(path);
}

#endif // CLINK_FINAL
