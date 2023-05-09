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
    typedef int         (T::*method_t)(lua_State*);

    struct Method
    {
        const char*     name;
        method_t        ptr;
    };

                        LuaBindable(const char* name, const Method* methods);
                        ~LuaBindable();
    void                push(lua_State* state);

private:
    static int          call(lua_State* state);
    void                bind();
    void                unbind();
    const char*         m_name;
    const Method*       m_methods;
    lua_State*          m_state;
    int                 m_registry_ref;
};

//------------------------------------------------------------------------------
template <class T>
LuaBindable<T>::LuaBindable(const char* name, const Method* methods)
: m_name(name)
, m_methods(methods)
, m_state(nullptr)
, m_registry_ref(LUA_NOREF)
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
    void* self = lua_newuserdata(m_state, sizeof(void*));
    *(void**)self = this;

    Str<48> mt_name;
    mt_name << m_name << "_mt";
    if (luaL_newmetatable(m_state, mt_name.c_str()))
    {
        lua_createtable(m_state, 0, 0);

        auto* methods = m_methods;
        while (methods != nullptr && methods->name != nullptr)
        {
            auto* ptr = (method_t*)lua_newuserdata(m_state, sizeof(method_t));
            *ptr = methods->ptr;

            if (luaL_newmetatable(m_state, "LuaBindable"))
            {
                lua_pushliteral(m_state, "__call");
                lua_pushcfunction(m_state, &LuaBindable<T>::call);
                lua_rawset(m_state, -3);
            }

            lua_setmetatable(m_state, -2);
            lua_setfield(m_state, -2, methods->name);

            ++methods;
        }

        lua_setfield(m_state, -2, "__index");
    }

    lua_setmetatable(m_state, -2);
    m_registry_ref = luaL_ref(m_state, LUA_REGISTRYINDEX);
}

//------------------------------------------------------------------------------
template <class T>
void LuaBindable<T>::unbind()
{
    if (m_state == nullptr || m_registry_ref == LUA_NOREF)
        return;

    lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_registry_ref);
    if (void* self = lua_touserdata(m_state, -1))
        *(void**)self = nullptr;
    lua_pop(m_state, 1);

    luaL_unref(m_state, LUA_REGISTRYINDEX, m_registry_ref);
    m_registry_ref = LUA_NOREF;
    m_state = nullptr;
}

//------------------------------------------------------------------------------
template <class T>
void LuaBindable<T>::push(lua_State* state)
{
    m_state = state;

    if (m_registry_ref == LUA_NOREF)
        bind();

    lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_registry_ref);
}

//------------------------------------------------------------------------------
template <class T>
int LuaBindable<T>::call(lua_State* state)
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
