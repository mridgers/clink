// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"

#include <core/base.h>
#include <core/settings.h>

#include <new.h>

//------------------------------------------------------------------------------
/// -name:  settings.get
/// -arg:   name:string
/// -ret:   boolean or string or integer
static int get(lua_State* state)
{
    if (lua_gettop(state) == 0 || !lua_isstring(state, 1))
        return 0;

    const char* key = lua_tostring(state, 1);
    const Setting* setting = settings::find(key);
    if (setting == nullptr)
        return 0;

    int Type = setting->get_type();
    switch (Type)
    {
    case Setting::type_bool:
        {
            bool value = ((SettingBool*)setting)->get();
            lua_pushboolean(state, value == true);
        }
        break;

    case Setting::type_int:
        {
            int value = ((SettingInt*)setting)->get();
            lua_pushinteger(state, value);
        }
        break;

    default:
        {
            Str<> value;
            setting->get(value);
            lua_pushstring(state, value.c_str());
        }
        break;
    }

    return 1;
}

//------------------------------------------------------------------------------
/// -name:  settings.set
/// -arg:   name:string
/// -arg:   value:string
/// -ret:   boolean
static int set(lua_State* state)
{
    if (lua_gettop(state) < 2 || !lua_isstring(state, 1))
        return 0;

    const char* key = lua_tostring(state, 1);
    Setting* setting = settings::find(key);
    if (setting == nullptr)
        return 0;

    const char* value = lua_tostring(state, 2);
    bool ok = setting->set(value);

    lua_pushboolean(state, ok == true);
    return 1;
}

//------------------------------------------------------------------------------
template <typename S, typename... V> void add_impl(lua_State* state, V... value)
{
    const char* name = lua_tostring(state, 1);
    const char* short_desc = (lua_gettop(state) > 2) ? lua_tostring(state, 3) : "";
    const char* long_desc = (lua_gettop(state) > 3) ? lua_tostring(state, 4) : "";

    void* addr = lua_newuserdata(state, sizeof(S));
    new (addr) S(name, short_desc, long_desc, value...);

    if (luaL_newmetatable(state, "settings_mt"))
    {
        lua_pushliteral(state, "__gc");
        lua_pushcfunction(state, [](lua_State* state) -> int {
            Setting* s = (Setting*)lua_touserdata(state, -1);
            s->~Setting();
            return 0;
        });
        lua_rawset(state, -3);
    }

    lua_setmetatable(state, -2);
    luaL_ref(state, LUA_REGISTRYINDEX);
}

//------------------------------------------------------------------------------
/// -name:  settings.add
/// -arg:   name:string
/// -arg:   default:...
/// -ret:   boolean
static int add(lua_State* state)
{
    if (lua_gettop(state) < 2 || !lua_isstring(state, 1))
    {
        lua_pushboolean(state, 0);
        return 1;
    }

    switch (lua_type(state, 2))
    {
    case LUA_TNUMBER:
        add_impl<SettingInt>(state, int(lua_tointeger(state, 2)));
        break;

    case LUA_TBOOLEAN:
        add_impl<SettingBool>(state, lua_toboolean(state, 2) == 1);
        break;

    case LUA_TSTRING:
        add_impl<SettingStr>(state, (const char*)lua_tostring(state, 2));
        break;

    case LUA_TTABLE:
        {
            Str<256> options;
            for (int i = 0, n = int(lua_rawlen(state, 2)); i < n; ++i)
            {
                lua_rawgeti(state, 2, i + 1);
                if (const char* option = lua_tostring(state, -1))
                {
                    if (i) options << ",";
                    options << option;
                }
                lua_pop(state, 1);
            }

            add_impl<SettingEnum>(state, options.c_str(), 0);
        }
        break;

    default:
        lua_pushboolean(state, 0);
        return 1;
    }

    lua_pushboolean(state, 1);
    return 1;
}

//------------------------------------------------------------------------------
void settings_lua_initialise(LuaState& lua)
{
    struct {
        const char* name;
        int         (*method)(lua_State*);
    } methods[] = {
        { "get",    &get },
        { "set",    &set },
        { "add",    &add },
    };

    lua_State* state = lua.get_state();

    lua_createtable(state, sizeof_array(methods), 0);

    for (const auto& method : methods)
    {
        lua_pushstring(state, method.name);
        lua_pushcfunction(state, method.method);
        lua_rawset(state, -3);
    }

    lua_setglobal(state, "settings");
}
