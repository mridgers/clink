// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/str.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

//------------------------------------------------------------------------------
template <class T>
class LuaBindable
{
public:
    typedef int32       (T::*method_t)(lua_State*);

    struct Method
    {
        const char*     name;
        method_t        ptr;
    };

                        LuaBindable(const char* name, const Method* methods);
                        ~LuaBindable();
    void                push(lua_State* state);

private:
    static int32        call(lua_State* state);
    void                bind();
    void                unbind();
    const char*         _name;
    const Method*       _methods;
    lua_State*          _state;
    int32               _registry_ref;
};

//------------------------------------------------------------------------------
template <class T>
LuaBindable<T>::LuaBindable(const char* name, const Method* methods)
: _name(name)
, _methods(methods)
, _state(nullptr)
, _registry_ref(LUA_NOREF)
{
}

//------------------------------------------------------------------------------
template <class T>
LuaBindable<T>::~LuaBindable()
{
    unbind();
}

//------------------------------------------------------------------------------
template <class T>
void LuaBindable<T>::bind()
{
    void* self = lua_newuserdata(_state, sizeof(void*));
    *(void**)self = this;

    Str<48> mt_name;
    mt_name << _name << "_mt";
    if (luaL_newmetatable(_state, mt_name.c_str()))
    {
        lua_createtable(_state, 0, 0);

        auto* methods = _methods;
        while (methods != nullptr && methods->name != nullptr)
        {
            auto* ptr = (method_t*)lua_newuserdata(_state, sizeof(method_t));
            *ptr = methods->ptr;

            if (luaL_newmetatable(_state, "LuaBindable"))
            {
                lua_pushliteral(_state, "__call");
                lua_pushcfunction(_state, &LuaBindable<T>::call);
                lua_rawset(_state, -3);
            }

            lua_setmetatable(_state, -2);
            lua_setfield(_state, -2, methods->name);

            ++methods;
        }

        lua_setfield(_state, -2, "__index");
    }

    lua_setmetatable(_state, -2);
    _registry_ref = luaL_ref(_state, LUA_REGISTRYINDEX);
}

//------------------------------------------------------------------------------
template <class T>
void LuaBindable<T>::unbind()
{
    if (_state == nullptr || _registry_ref == LUA_NOREF)
        return;

    lua_rawgeti(_state, LUA_REGISTRYINDEX, _registry_ref);
    if (void* self = lua_touserdata(_state, -1))
        *(void**)self = nullptr;
    lua_pop(_state, 1);

    luaL_unref(_state, LUA_REGISTRYINDEX, _registry_ref);
    _registry_ref = LUA_NOREF;
    _state = nullptr;
}

//------------------------------------------------------------------------------
template <class T>
void LuaBindable<T>::push(lua_State* state)
{
    _state = state;

    if (_registry_ref == LUA_NOREF)
        bind();

    lua_rawgeti(_state, LUA_REGISTRYINDEX, _registry_ref);
}

//------------------------------------------------------------------------------
template <class T>
int32 LuaBindable<T>::call(lua_State* state)
{
    auto* const* self = (T* const*)lua_touserdata(state, 2);
    if (self == nullptr || *self == nullptr)
        return 0;

    auto* const ptr = (method_t const*)lua_touserdata(state, 1);
    if (ptr == nullptr)
        return 0;

    lua_remove(state, 1);
    lua_remove(state, 1);

    return ((*self)->*(*ptr))(state);
}
