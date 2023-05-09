// Copyright (c) 2012 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "hook_setter.h"

#include <core/base.h>
#include <core/log.h>
#include <process/hook.h>
#include <process/pe.h>
#include <process/vm.h>

//------------------------------------------------------------------------------
HookSetter::HookSetter()
: m_desc_count(0)
{
}

//------------------------------------------------------------------------------
int HookSetter::commit()
{
    // Each hook needs fixing up, so we find the base address of our module.
    void* self = Vm().get_alloc_base("clink");
    if (self == nullptr)
        return 0;

    // Apply all the hooks add to the setter.
    int success = 0;
    for (int i = 0; i < m_desc_count; ++i)
    {
        const HookDesc& desc = m_descs[i];
        switch (desc.type)
        {
        case hook_type_iat_by_name: success += !!commit_iat(self, desc);  break;
        case hook_type_jmp:         success += !!commit_jmp(self, desc);  break;
        }
    }

    return success;
}

//------------------------------------------------------------------------------
HookSetter::HookDesc* HookSetter::add_desc(
    HookType type,
    void* module,
    const char* name,
    funcptr_t hook)
{
    if (m_desc_count >= sizeof_array(m_descs))
        return nullptr;

    HookDesc& desc = m_descs[m_desc_count];
    desc.type = type;
    desc.module = module;
    desc.hook = hook;
    desc.name = name;

    ++m_desc_count;
    return &desc;
}

//------------------------------------------------------------------------------
bool HookSetter::commit_iat(void* self, const HookDesc& desc)
{
    funcptr_t addr = hook_iat(desc.module, nullptr, desc.name, desc.hook, 1);
    if (addr == nullptr)
    {
        LOG("Unable to hook %s in IAT at base %p", desc.name, desc.module);
        return false;
    }

    // If the target's IAT was hooked then the hook destination is now
    // stored in 'addr'. We hook ourselves with this address to maintain
    // any IAT hooks that may already exist.
    if (hook_iat(self, nullptr, desc.name, addr, 1) == 0)
    {
        LOG("Failed to hook own IAT for %s", desc.name);
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool HookSetter::commit_jmp(void* self, const HookDesc& desc)
{
    // Hook into a DLL's import by patching the start of the function. 'addr' is
    // the Trampoline that can be used to call the original. This method doesn't
    // use the IAT.

    auto* addr = hook_jmp(desc.module, desc.name, desc.hook);
    if (addr == nullptr)
    {
        LOG("Unable to hook %s in %p", desc.name, desc.module);
        return false;
    }

    // Patch our own IAT with the address of a Trampoline so out use of this
    // function calls the original.
    if (hook_iat(self, nullptr, desc.name, addr, 1) == 0)
    {
        LOG("Failed to hook own IAT for %s", desc.name);
        return false;
    }

    return true;
}
