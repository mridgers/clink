// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"

#include <core/base.h>
#include <core/globber.h>
#include <core/os.h>
#include <core/path.h>
#include <core/settings.h>
#include <core/str.h>
#include <process/process.h>

//------------------------------------------------------------------------------
extern SettingBool g_glob_hidden;
extern SettingBool g_glob_system;
extern SettingBool g_glob_unc;



//------------------------------------------------------------------------------
static const char* get_string(lua_State* state, int32 index)
{
    if (lua_gettop(state) < index || !lua_isstring(state, index))
        return nullptr;

    return lua_tostring(state, index);
}

//------------------------------------------------------------------------------
/// -name:  os.chdir
/// -arg:   path:string
/// -ret:   boolean
static int32 set_current_dir(lua_State* state)
{
    bool ok = false;
    if (const char* dir = get_string(state, 1))
        ok = os::set_current_dir(dir);

    lua_pushboolean(state, (ok == true));
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.getcwd
/// -ret:   string
static int32 get_current_dir(lua_State* state)
{
    Str<288> dir;
    os::get_current_dir(dir);

    lua_pushstring(state, dir.c_str());
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.mkdir
/// -arg:   path:string
/// -ret:   boolean
static int32 make_dir(lua_State* state)
{
    bool ok = false;
    if (const char* dir = get_string(state, 1))
        ok = os::make_dir(dir);

    lua_pushboolean(state, (ok == true));
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.rmdir
/// -arg:   path:string
/// -ret:   boolean
static int32 remove_dir(lua_State* state)
{
    bool ok = false;
    if (const char* dir = get_string(state, 1))
        ok = os::remove_dir(dir);

    lua_pushboolean(state, (ok == true));
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.isdir
/// -arg:   path:string
/// -ret:   boolean
static int32 is_dir(lua_State* state)
{
    const char* path = get_string(state, 1);
    if (path == nullptr)
        return 0;

    lua_pushboolean(state, (os::get_path_type(path) == os::path_type_dir));
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.isfile
/// -arg:   path:string
/// -ret:   boolean
static int32 is_file(lua_State* state)
{
    const char* path = get_string(state, 1);
    if (path == nullptr)
        return 0;

    lua_pushboolean(state, (os::get_path_type(path) == os::path_type_file));
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.unlink
/// -arg:   path:string
/// -ret:   boolean
static int32 unlink(lua_State* state)
{
    const char* path = get_string(state, 1);
    if (path == nullptr)
        return 0;

    if (os::unlink(path))
    {
        lua_pushboolean(state, 1);
        return 1;
    }

    lua_pushnil(state);
    lua_pushstring(state, "error");
    lua_pushinteger(state, 1);
    return 3;
}

//------------------------------------------------------------------------------
/// -name:  os.move
/// -arg:   src:string
/// -arg:   dest:string
/// -ret:   boolean
static int32 move(lua_State* state)
{
    const char* src = get_string(state, 1);
    const char* dest = get_string(state, 2);
    if (src != nullptr && dest != nullptr && os::move(src, dest))
    {
        lua_pushboolean(state, 1);
        return 1;
    }

    lua_pushnil(state);
    lua_pushstring(state, "error");
    lua_pushinteger(state, 1);
    return 3;
}

//------------------------------------------------------------------------------
/// -name:  os.copy
/// -arg:   src:string
/// -arg:   dest:string
/// -ret:   boolean
static int32 copy(lua_State* state)
{
    const char* src = get_string(state, 1);
    const char* dest = get_string(state, 2);
    if (src == nullptr || dest == nullptr)
        return 0;

    lua_pushboolean(state, (os::copy(src, dest) == true));
    return 1;
}

//------------------------------------------------------------------------------
static int32 glob_impl(lua_State* state, bool dirs_only)
{
    auto not_ok = [=] () {
        lua_pushcclosure(state, [] (lua_State*) -> int32 { return 0; }, 0);
        return 1;
    };

    const char* mask = get_string(state, 1);
    if (mask == nullptr)
        return not_ok();

    if (path::is_separator(mask[0]) && path::is_separator(mask[1]))
        if (!g_glob_unc.get())
            return not_ok();

    auto impl = [] (lua_State* state) -> int32 {
        int32 self_index = lua_upvalueindex(1);
        auto* glbbr = (Globber*)lua_touserdata(state, self_index);

        Str<288> file;
        if (!glbbr->next(file, false))
        {
            delete glbbr;
            return 0;
        }

        lua_pushstring(state, file.c_str());
        return 1;
    };

    Globber* glbbr = new Globber(mask);
    glbbr->files(!dirs_only);
    glbbr->hidden(g_glob_hidden.get());
    glbbr->system(g_glob_system.get());

    lua_pushlightuserdata(state, glbbr);
    lua_pushcclosure(state, impl, 1);
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.globdirs
/// -arg:   globpattern:string
/// -ret:   table
static int32 glob_dirs(lua_State* state)
{
    return glob_impl(state, true);
}

//------------------------------------------------------------------------------
/// -name:  os.globfiles
/// -arg:   globpattern:string
/// -ret:   table
static int32 glob_files(lua_State* state)
{
    return glob_impl(state, false);
}

//------------------------------------------------------------------------------
/// -name:  os.getenv
/// -arg:   path:string
/// -ret:   string or nil
static int32 get_env(lua_State* state)
{
    const char* name = get_string(state, 1);
    if (name == nullptr)
        return 0;

    Str<128> value;
    if (!os::get_env(name, value))
        return 0;

    lua_pushstring(state, value.c_str());
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.setenv
/// -arg:   name:string
/// -arg:   value:string
/// -ret:   boolean
static int32 set_env(lua_State* state)
{
    const char* name = get_string(state, 1);
    const char* value = get_string(state, 2);
    if (name == nullptr)
        return 0;

    bool ok = os::set_env(name, value);
    lua_pushboolean(state, (ok == true));
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.getenvnames
/// -ret:   table
static int32 get_env_names(lua_State* state)
{
    lua_createtable(state, 0, 0);

    char* root = GetEnvironmentStrings();
    if (root == nullptr)
        return 1;

    char* strings = root;
    int32 i = 1;
    while (*strings)
    {
        // Skip env vars that start with a '='. They're hidden ones.
        if (*strings == '=')
        {
            strings += strlen(strings) + 1;
            continue;
        }

        char* eq = strchr(strings, '=');
        if (eq == nullptr)
            break;

        *eq = '\0';

        lua_pushstring(state, strings);
        lua_rawseti(state, -2, i++);

        ++eq;
        strings = eq + strlen(eq) + 1;
    }

    FreeEnvironmentStrings(root);
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.gethost
/// -ret:   string
static int32 get_host(lua_State* state)
{
    Str<280> host;
    if (Process().get_file_name(host))
        return 0;

    lua_pushstring(state, host.c_str());
    return 1;
}

//------------------------------------------------------------------------------
/// -name:  os.getaliases
/// -ret:   string
static int32 get_aliases(lua_State* state)
{
    lua_createtable(state, 0, 0);

#if !defined(__MINGW32__) && !defined(__MINGW64__)
    Str<280> path;
    if (!Process().get_file_name(path))
        return 1;

    // Not const because Windows' alias API won't accept it.
    char* name = (char*)path::get_name(path.c_str());

    // Get the aliases (aka. Doskey macros).
    int32 buffer_size = GetConsoleAliasesLength(name);
    if (buffer_size == 0)
        return 1;

    Str<> buffer;
    buffer.reserve(buffer_size);
    if (GetConsoleAliases(buffer.data(), buffer.size(), name) == 0)
        return 1;

    // Parse the result into a lua table.
    const char* alias = buffer.c_str();
    for (int32 i = 1; int32(alias - buffer.c_str()) < buffer_size; ++i)
    {
        const char* c = strchr(alias, '=');
        if (c == nullptr)
            break;

        lua_pushlstring(state, alias, size_t(c - alias));
        lua_rawseti(state, -2, i);

        ++c;
        alias = c + strlen(c) + 1;
    }

#endif // __MINGW32__
    return 1;
}

//------------------------------------------------------------------------------
void os_lua_initialise(LuaState& lua)
{
    struct {
        const char* name;
        int32       (*method)(lua_State*);
    } methods[] = {
        { "chdir",       &set_current_dir },
        { "getcwd",      &get_current_dir },
        { "mkdir",       &make_dir },
        { "rmdir",       &remove_dir },
        { "isdir",       &is_dir },
        { "isfile",      &is_file },
        { "unlink",      &unlink },
        { "move",        &move },
        { "copy",        &copy },
        { "globdirs",    &glob_dirs },
        { "globfiles",   &glob_files },
        { "getenv",      &get_env },
        { "setenv",      &set_env },
        { "getenvnames", &get_env_names },
        { "gethost",     &get_host },
        { "getaliases",  &get_aliases },
    };

    lua_State* state = lua.get_state();

    lua_getglobal(state, "os");

    for (const auto& method : methods)
    {
        lua_pushstring(state, method.name);
        lua_pushcfunction(state, method.method);
        lua_rawset(state, -3);
    }

    lua_pop(state, 1);
}
