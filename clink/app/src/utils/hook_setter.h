// Copyright (c) 2012 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class HookSetter
{
public:
    typedef void (__stdcall *funcptr_t)();

                                HookSetter();
    template <typename RET,
             typename... ARGS>
    bool                        add_iat(void* module, const char* name, RET (__stdcall *hook)(ARGS...));
    template <typename RET,
             typename... ARGS>
    bool                        add_jmp(void* module, const char* name, RET (__stdcall *hook)(ARGS...));
    int                         commit();

private:
    enum HookType
    {
        hook_type_iat_by_name,
        //hook_type_iat_by_addr,
        hook_type_jmp,
    };

    struct HookDesc
    {
        void*                   module;
        const char*             name;
        funcptr_t               hook;
        HookType                type;
    };

    HookDesc*                   add_desc(HookType Type, void* module, const char* name, funcptr_t hook);
    bool                        commit_iat(void* self, const HookDesc& desc);
    bool                        commit_jmp(void* self, const HookDesc& desc);
    HookDesc                    m_descs[4];
    int                         m_desc_count;
};

//------------------------------------------------------------------------------
template <typename RET, typename... ARGS>
bool HookSetter::add_iat(void* module, const char* name, RET (__stdcall *hook)(ARGS...))
{
    return (add_desc(hook_type_iat_by_name, module, name, funcptr_t(hook)) != nullptr);
}

//------------------------------------------------------------------------------
template <typename RET, typename... ARGS>
bool HookSetter::add_jmp(void* module, const char* name, RET (__stdcall *hook)(ARGS...))
{
    return (add_desc(hook_type_jmp, module, name, funcptr_t(hook)) != nullptr);
}
