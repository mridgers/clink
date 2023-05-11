// Copyright (c) Martin Ridgers
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
    Vm vm;
    Vm::Region Region = { vm.get_page(where), 1 };
    uint32 prev_access = vm.get_access(Region);
    vm.set_access(Region, Vm::access_write);

    if (!vm.write(where, &to_write, sizeof(to_write)))
        LOG("VM write to %p failed (err = %d)", where, GetLastError());

    vm.set_access(Region, prev_access);
}

//------------------------------------------------------------------------------
static funcptr_t get_proc_addr(const char* dll, const char* func_name)
{
    if (void* base = LoadLibraryA(dll))
        return PeInfo(base).get_export(func_name);

    LOG("Failed to load library '%s'", dll);
    return nullptr;
}

//------------------------------------------------------------------------------
funcptr_t hook_iat(
    void* base,
    const char* dll,
    const char* func_name,
    funcptr_t hook,
    int32 find_by_name
)
{
    LOG("Attempting to hook IAT for module %p", base);
    LOG("Target is %s,%s (by_name=%d)", dll, func_name, find_by_name);

    funcptr_t* import;

    // Find entry and replace it.
    PeInfo pe(base);
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

    Vm().flush_icache();
    return prev_addr;
}



//------------------------------------------------------------------------------
class TrampolineAllocator
{
public:
    static TrampolineAllocator*     get(funcptr_t to_hook);

    void*                           alloc(int32 size);
    void                            reset_access();

private:
                                    TrampolineAllocator(int32 prev_access);
    uint32                          _magic = magic;
    uint32                          _prev_access;
    uint32                          _used = 0;
    static const uint32             magic = 0x4b4e4c43;
};

//------------------------------------------------------------------------------
TrampolineAllocator* TrampolineAllocator::get(funcptr_t to_hook)
{
    // Find the end of the text Section
    void* text_addr = (void*)to_hook;

    Vm vm;
    Vm::Region Region = vm.get_region(text_addr);
    auto* text_end = (char*)(Region.base) + (Region.page_count * Vm::get_page_size());

    // Make the end of the text segment writeable
    auto make_writeable = [&] ()
    {
        Vm::Region last_page = { text_end - Vm::get_page_size(), 1 };
        int32 prev_access = vm.get_access(last_page);
        vm.set_access(last_page, prev_access|Vm::access_write);
        return prev_access;
    };

    // Have we already set up an allocator at the end?
    auto* ret = (TrampolineAllocator*)(text_end - sizeof(TrampolineAllocator));
    if (ret->_magic == magic)
    {
        make_writeable();
        return ret;
    }

    // Nothing's been established yet. Make sure its all zeros and create one
    for (auto* c = (char*)ret; c < text_end; ++c)
        if (*c != '\0')
            return nullptr;

    int32 prev_access = make_writeable();
    return new (ret) TrampolineAllocator(prev_access);
}

//------------------------------------------------------------------------------
TrampolineAllocator::TrampolineAllocator(int32 prev_access)
: _prev_access(prev_access)
{
}

//------------------------------------------------------------------------------
void* TrampolineAllocator::alloc(int32 size)
{
    size += 15;
    size &= ~15;

    int32 next_used = _used + size;
    auto* ptr = ((uint8*)this) - next_used;

    for (int32 i = 0; i < size; ++i)
        if (ptr[i] != 0)
            return nullptr;

    _used = next_used;
    return ptr;
}

//------------------------------------------------------------------------------
void TrampolineAllocator::reset_access()
{
    Vm vm;
    Vm::Region last_page = { vm.get_page(this), 1 };
    vm.set_access(last_page, _prev_access);
    vm.flush_icache();
}



//------------------------------------------------------------------------------
static void* follow_jump(void* addr)
{
    uint8* t = (uint8*)addr;

    // Check the opcode.
    if ((t[0] & 0xf0) == 0x40) // REX prefix.
        ++t;

    if (t[0] != 0xff)
        return addr;

    // Check the opcode extension from the modr/m byte. We only support one.
    if (t[1] != 0x25)
        return addr;

    int32* imm = (int32*)(t + 2);

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

    // Disassemble enough bytes to be able to write a jmp Instruction at the
    // start of what we want to hook.
    int32 insts_size = 0;
    FixedArray<Instruction, 8> instructions;
    for (InstructionIter iter(target);;)
    {
        Instruction inst = iter.next();
        if (!inst)
        {
            /* decode fail */
            return nullptr;
        }

        if (uint32(inst.get_rel_size() - 1) < 3)
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
    auto* allocator = TrampolineAllocator::get(to_hook);
    if (allocator == nullptr)
    {
        /* unable to create an TrampolineAllocator */
        return nullptr;
    }

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4200)
#endif
    struct Trampoline
    {
        funcptr_t       hook;
        uint8           trampoline_in[];
    };
#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

    insts_size += sizeof(Trampoline);
    insts_size += 5; // for 'jmp disp32'
    auto* tramp = (Trampoline*)(allocator->alloc(insts_size));
    if (tramp == nullptr)
    {
        /* unable to allocate a Trampoline */
        allocator->reset_access();
        return nullptr;
    }

    // Write Trampoline in
    const uint8* read_cursor = (uint8*)target;
    uint8* write_cursor = tramp->trampoline_in;
    for (const Instruction& inst : instructions)
    {
        inst.copy(read_cursor, write_cursor);
        write_cursor += inst.get_length();
        read_cursor += inst.get_length();
    }
    *(char*)(write_cursor + 0) = (char)0xe9;
    *(int32*) (write_cursor + 1) = int32(ptrdiff_t(read_cursor) - ptrdiff_t(write_cursor + 5));

    // Write Trampoline out
    tramp->hook = hook;

    Vm vm;
    Vm::Region target_region = { vm.get_page(target), 1 };
    uint32 prev_access = vm.get_access(target_region);
    vm.set_access(target_region, Vm::access_write);

    write_cursor = (uint8*)target;
    *(int16*)(write_cursor + 0) = 0x25ff;
#ifdef _M_X64
    *(int32*)  (write_cursor + 2) = int32(ptrdiff_t(&tramp->hook) - ptrdiff_t(write_cursor) - 6);
#else
    *(int32*)  (write_cursor + 2) = int32(intptr_t(&tramp->hook));
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
    funcptr_t func_addr = PeInfo(base).get_export(func_name);
    if (func_addr == nullptr)
    {
        LOG("Failed to find function '%s' in '%s'", func_name, module_name);
        return nullptr;
    }

    LOG("Attemping jump hook.");
    LOG("Target is %s, %s @ %p", module_name, func_name, func_addr);

    // Install the hook.
    funcptr_t Trampoline = hook_jmp_impl(func_addr, hook);
    if (Trampoline == nullptr)
    {
        LOG("Jump hook failed.");
        return nullptr;
    }

    LOG("Success!");
    Vm().flush_icache();
    return Trampoline;
}
