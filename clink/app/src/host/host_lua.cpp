// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "host_lua.h"
#include "utils/app_context.h"

#include <core/globber.h>
#include <core/os.h>
#include <core/path.h>
#include <core/settings.h>
#include <core/str.h>
#include <core/str_tokeniser.h>

extern "C" {
#include <lua.h>
}

//------------------------------------------------------------------------------
static SettingStr g_clink_path(
    "clink.path",
    "Paths to load Lua completion scripts from",
    "These paths will be searched for Lua scripts that provide custom\n"
    "match generation. Multiple paths should be delimited by semicolons.",
    "");

//------------------------------------------------------------------------------
HostLua::HostLua()
: _generator(_state)
{
    Str<280> bin_path;
    AppContext::get()->get_binaries_dir(bin_path);

    Str<280> exe_path;
    exe_path << bin_path << "\\" CLINK_EXE;

    lua_State* state = _state.get_state();
    lua_pushstring(state, exe_path.c_str());
    lua_setglobal(state, "CLINK_EXE");
}

//------------------------------------------------------------------------------
HostLua::operator LuaState& ()
{
    return _state;
}

//------------------------------------------------------------------------------
HostLua::operator MatchGenerator& ()
{
    return _generator;
}

//------------------------------------------------------------------------------
void HostLua::load_scripts()
{
    const char* setting_clink_path = g_clink_path.get();
    load_scripts(setting_clink_path);

    Str<256> env_clink_path;
    os::get_env("clink_path", env_clink_path);
    load_scripts(env_clink_path.c_str());
}

//------------------------------------------------------------------------------
void HostLua::load_scripts(const char* paths)
{
    if (paths == nullptr || paths[0] == '\0')
        return;

    Str<280> token;
    StrTokeniser tokens(paths, ";");
    while (tokens.next(token))
        load_script(token.c_str());
}

//------------------------------------------------------------------------------
void HostLua::load_script(const char* path)
{
    Str<280> buffer;
    path::join(path, "*.lua", buffer);

    Globber lua_globs(buffer.c_str());
    lua_globs.directories(false);

    while (lua_globs.next(buffer))
        _state.do_file(buffer.c_str());
}
