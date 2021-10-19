// Copyright (c) 2012 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "hook.h"
#include "inst_iter.h"
#include "pe.h"
#include "vm.h"

#include <core/array.h>
#include <core/base.h>
#include <core/log.h>

#include <new>

//------------------------------------------------------------------------------
static void write_addr(funcptr_t* where, funcptr_t to_write)
{
    vm vm;
    vm::region region = { vm.get_page(where), 1 };
    unsigned int prev_access = vm.get_access(region);
    vm.set_access(region, vm::access_write);

    if (!vm.write(where, &to_write, sizeof(to_write)))
        LOG("VM write to %p failed (err = %d)", where, GetLastError());

    vm.set_access(region, prev_access);
}

//------------------------------------------------------------------------------
static funcptr_t get_proc_addr(const char* dll, const char* func_name)
{
    if (void* base = LoadLibraryA(dll))
        return pe_info(base).get_export(func_name);

    LOG("Failed to load library '%s'", dll);
    return nullptr;
}

//------------------------------------------------------------------------------
funcptr_t hook_iat(
    void* base,
    const char* dll,
    const char* func_name,
    funcptr_t hook,
    int find_by_name
)
{
    LOG("Attempting to hook IAT for module %p", base);
    LOG("Target is %s,%s (by_name=%d)", dll, func_name, find_by_name);

    funcptr_t* import;

    // Find entry and replace it.
    pe_info pe(base);
    if (find_by_name)
        import = pe.get_import_by_name(nullptr, func_name);
    else
    {
        // Get the address of the function we're going to hook.
        funcptr_t func_addr = get_proc_addr(dll, func_name);
        if (func_addr == nullptr)
        {
            LOG("Failed to find function '%s' in '%s'", func_name, dll);
            return nullptr;
        }

        import = pe.get_import_by_addr(nullptr, func_addr);
    }

    if (import == nullptr)
    {
        LOG("Unable to find import in IAT (by_name=%d)", find_by_name);
        return nullptr;
    }

    LOG("Found import at %p (value = %p)", import, *import);

    funcptr_t prev_addr = *import;
    write_addr(import, hook);

    vm().flush_icache();
    return prev_addr;
}



//------------------------------------------------------------------------------
class trampoline_allocator
{
public:
    static trampoline_allocator*    get(funcptr_t to_hook);

    void*                           alloc(int size);
    void                            reset_access();

private:
                                    trampoline_allocator(int prev_access);
    unsigned int                    m_magic = magic;
    unsigned int                    m_prev_access;
    unsigned int                    m_used = 0;
    static const unsigned int       magic = 0x4b4e4c43;
};

//------------------------------------------------------------------------------
trampoline_allocator* trampoline_allocator::get(funcptr_t to_hook)
{
    // Find the end of the text section
    void* text_addr = (void*)to_hook;

    vm vm;
    vm::region region = vm.get_region(text_addr);
    auto* text_end = (char*)(region.base) + (region.page_count * vm::get_page_size());

    // Make the end of the text segment writeable
    auto make_writeable = [&] ()
    {
        vm::region last_page = { text_end - vm::get_page_size(), 1 };
        int prev_access = vm.get_access(last_page);
        vm.set_access(last_page, prev_access|vm::access_write);
        return prev_access;
    };

    // Have we already set up an allocator at the end?
    auto* ret = (trampoline_allocator*)(text_end - sizeof(trampoline_allocator));
    if (ret->m_magic == magic)
    {
        make_writeable();
        return ret;
    }

    // Nothing's been established yet. Make sure its all zeros and create one
    for (auto* c = (char*)ret; c < text_end; ++c)
        if (*c != '\0')
            return nullptr;

    int prev_access = make_writeable();
    return new (ret) trampoline_allocator(prev_access);
}

//------------------------------------------------------------------------------
trampoline_allocator::trampoline_allocator(int prev_access)
: m_prev_access(prev_access)
{
}

//------------------------------------------------------------------------------
void* trampoline_allocator::alloc(int size)
{
    size += 15;
    size &= ~15;

    int next_used = m_used + size;
    auto* ptr = ((unsigned char*)this) - next_used;

    for (int i = 0; i < size; ++i)
        if (ptr[i] != 0)
            return nullptr;

    m_used = next_used;
    return ptr;
}

//------------------------------------------------------------------------------
void trampoline_allocator::reset_access()
{
    vm vm;
    vm::region last_page = { vm.get_page(this), 1 };
    vm.set_access(last_page, m_prev_access);
    vm.flush_icache();
}



//------------------------------------------------------------------------------
static void* follow_jump(void* addr)
{
    unsigned char* t = (unsigned char*)addr;

    // Check the opcode.
    if ((t[0] & 0xf0) == 0x40) // REX prefix.
        ++t;

    if (t[0] != 0xff)
        return addr;

    // Check the opcode extension from the modr/m byte. We only support one.
    if (t[1] != 0x25)
        return addr;

    int* imm = (int*)(t + 2);

    void* dest = addr;
    switch (t[1] & 007)
    {
    case 5:
#ifdef _M_X64
        // dest = [rip + disp32]
        dest = *(void**)(t + 6 + *imm);
#elif defined _M_IX86
        // dest = disp32
        dest = *(void**)(intptr_t)(*imm);
#endif
    }

    LOG("Following jump to %p", dest);
    return dest;
}

//------------------------------------------------------------------------------
static funcptr_t hook_jmp_impl(funcptr_t to_hook, funcptr_t hook)
{
    LOG("Attempting to hook at %p with %p", to_hook, hook);

    void* target = (void*)to_hook;
    target = follow_jump(target);

    // Disassemble enough bytes to be able to write a jmp instruction at the
    // start of what we want to hook.
    int insts_size = 0;
    fixed_array<instruction, 8> instructions;
    for (instruction_iter iter(target);;)
    {
        instruction inst = iter.next();
        if (!inst)
        {
            /* decode fail */
            return nullptr;
        }

        if (unsigned(inst.get_rel_size() - 1) < 3)
        {
            /* relative immediate too small to be relocated */
            return nullptr;
        }

        *(instructions.push_back()) = inst;

        insts_size += inst.get_length();
        if (insts_size >= 6)
        {
            break;
        }

        // This can't really happen but it might so better safe than sorry
        if (instructions.full())
        {
            return nullptr;
        }
    }

    // Allocate some executable memory for storing trampolines and hook address
    auto* allocator = trampoline_allocator::get(to_hook);
    if (allocator == nullptr)
    {
        /* unable to create an trampoline_allocator */
        return nullptr;
    }

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4200)
#endif
    struct trampoline
    {
        funcptr_t       hook;
        unsigned char   trampoline_in[];
    };
#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

    insts_size += sizeof(trampoline);
    insts_size += 5; // for 'jmp disp32'
    auto* tramp = (trampoline*)(allocator->alloc(insts_size));
    if (tramp == nullptr)
    {
        /* unable to allocate a trampoline */
        allocator->reset_access();
        return nullptr;
    }

    // Write trampoline in
    const unsigned char* read_cursor = (unsigned char*)target;
    unsigned char* write_cursor = tramp->trampoline_in;
    for (const instruction& inst : instructions)
    {
        inst.copy(read_cursor, write_cursor);
        write_cursor += inst.get_length();
        read_cursor += inst.get_length();
    }
    *(char*)(write_cursor + 0) = (char)0xe9;
    *(int*) (write_cursor + 1) = int(ptrdiff_t(read_cursor) - ptrdiff_t(write_cursor + 5));

    // Write trampoline out
    tramp->hook = hook;

    vm vm;
    vm::region target_region = { vm.get_page(target), 1 };
    unsigned int prev_access = vm.get_access(target_region);
    vm.set_access(target_region, vm::access_write);

    write_cursor = (unsigned char*)target;
    *(short*)(write_cursor + 0) = 0x25ff;
#ifdef _M_X64
    *(int*)  (write_cursor + 2) = int(ptrdiff_t(&tramp->hook) - ptrdiff_t(write_cursor) - 6);
#else
    *(int*)  (write_cursor + 2) = int(intptr_t(&tramp->hook));
#endif

    vm.set_access(target_region, prev_access);

    // Done
    allocator->reset_access();
    return funcptr_t((void*)(tramp->trampoline_in));
}

//------------------------------------------------------------------------------
funcptr_t hook_jmp(void* base, const char* func_name, funcptr_t hook)
{
    char module_name[96];
    module_name[0] = '\0';
    GetModuleFileName(HMODULE(base), module_name, sizeof_array(module_name));

    // Get the address of the function we're going to hook.
    funcptr_t func_addr = pe_info(base).get_export(func_name);
    if (func_addr == nullptr)
    {
        LOG("Failed to find function '%s' in '%s'", func_name, module_name);
        return nullptr;
    }

    LOG("Attemping jump hook.");
    LOG("Target is %s, %s @ %p", module_name, func_name, func_addr);

    // Install the hook.
    funcptr_t trampoline = hook_jmp_impl(func_addr, hook);
    if (trampoline == nullptr)
    {
        LOG("Jump hook failed.");
        return nullptr;
    }

    LOG("Success!");
    vm().flush_icache();
    return trampoline;
}
