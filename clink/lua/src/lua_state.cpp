// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"
#include "lua_script_loader.h"

#include <core/settings.h>
#include <core/os.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

//------------------------------------------------------------------------------
static SettingBool g_lua_debug(
    "lua.debug",
    "Enables Lua debugging",
    "Loads an simple embedded command line debugger when enabled. Breakpoints\n"
    "can added by calling pause().",
    false);

static SettingStr g_lua_path(
    "lua.path",
    "'require' search path",
    "Value to append to package.path. Used to search for Lua scripts specified\n"
    "in require() statements.",
    "");



//------------------------------------------------------------------------------
void clink_lua_initialise(LuaState&);
void io_lua_initialise(LuaState&);
void os_lua_initialise(LuaState&);
void path_lua_initialise(LuaState&);
void settings_lua_initialise(LuaState&);
void string_lua_initialise(LuaState&);



//------------------------------------------------------------------------------
LuaState::LuaState()
: _state(nullptr)
{
    initialise();
}

//------------------------------------------------------------------------------
LuaState::~LuaState()
{
    shutdown();
}

//------------------------------------------------------------------------------
void LuaState::initialise()
{
    shutdown();

    // Create a new Lua state.
    _state = luaL_newstate();
    luaL_openlibs(_state);

    // Set up the package.path value for require() statements.
    Str<280> path;
    if (!os::get_env("lua_path_" LUA_VERSION_MAJOR "_" LUA_VERSION_MINOR, path))
        os::get_env("lua_path", path);

    const char* p = g_lua_path.get();
    if (*p)
    {
        if (!path.empty())
            path << ";";

        path << p;
    }

    if (!path.empty())
    {
        lua_getglobal(_state, "package");
        lua_pushstring(_state, "path");
        lua_pushstring(_state, path.c_str());
        lua_rawset(_state, -3);
    }

    LuaState& self = *this;

    if (g_lua_debug.get())
        lua_load_script(self, lib, debugger);

    clink_lua_initialise(self);
    io_lua_initialise(self);
    os_lua_initialise(self);
    path_lua_initialise(self);
    settings_lua_initialise(self);
    string_lua_initialise(self);
}

//------------------------------------------------------------------------------
void LuaState::shutdown()
{
    if (_state == nullptr)
        return;

    lua_close(_state);
    _state = nullptr;
}

//------------------------------------------------------------------------------
bool LuaState::do_string(const char* string, int length)
{
    if (length < 0)
        length = int(strlen(string));

    bool ok;
    if (ok = !luaL_loadbuffer(_state, string, length, string))
        ok = !lua_pcall(_state, 0, LUA_MULTRET, 0);

    if (!ok)
        if (const char* error = lua_tostring(_state, -1))
            puts(error);

    lua_settop(_state, 0);
    return ok;
}

//------------------------------------------------------------------------------
bool LuaState::do_file(const char* path)
{
    // Open the file
    HANDLE handle;
    {
        Wstr<256> wpath(path);
        handle = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, 0, nullptr);
    }

    if (handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    struct IoBuffer
    {
        HANDLE  handle;
        char    buffer[1024];
    } io = {
        handle,
    };

    auto read_file_func = [] (lua_State*, void* param, size_t* size) -> const char*
    {
        auto& io = *(IoBuffer*)param;
        DWORD bytes_read = 0;
        BOOL ok = ReadFile(io.handle, io.buffer, sizeof(IoBuffer::buffer), &bytes_read, nullptr);
        if (ok == FALSE)
            return nullptr;
        *size = bytes_read;
        return io.buffer;
    };

    int ok;
    {
        Str<280> at_path;
        at_path << "@";
        at_path << path;
        ok = lua_load(_state, read_file_func, &io, at_path.c_str(), nullptr);
        if (ok != LUA_OK)
            if (const char* error = lua_tostring(_state, -1))
                puts(error);
    }

    if (ok == LUA_OK)
    {
        ok = lua_pcall(_state, 0, LUA_MULTRET, 0);
        if (ok != LUA_OK)
            if (const char* error = lua_tostring(_state, -1))
                puts(error);
    }

    lua_settop(_state, 0);
    CloseHandle(handle);
    return (ok == LUA_OK);
}
