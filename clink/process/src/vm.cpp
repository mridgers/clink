// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "vm.h"

#include <Windows.h>

//------------------------------------------------------------------------------
static uint32 g_alloc_granularity = 0;
static uint32 g_page_size         = 0;

//------------------------------------------------------------------------------
static void initialise_page_constants()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    g_alloc_granularity = system_info.dwAllocationGranularity;
    g_page_size = system_info.dwPageSize;
}

//------------------------------------------------------------------------------
static uint32 to_access_flags(uint32 ms_flags)
{
    uint32 ret = 0;
    if (ms_flags & 0x22) ret |= Vm::access_read;
    if (ms_flags & 0x44) ret |= Vm::access_write|Vm::access_read;
    if (ms_flags & 0x88) ret |= Vm::access_cow|Vm::access_write|Vm::access_read;
    if (ms_flags & 0xf0) ret |= Vm::access_execute;
    return ret;
}

//------------------------------------------------------------------------------
static uint32 to_ms_flags(uint32 access_flags)
{
    uint32 ret = PAGE_NOACCESS;
    if (access_flags & Vm::access_cow)          ret = PAGE_WRITECOPY;
    else if (access_flags & Vm::access_write)   ret = PAGE_READWRITE;
    else if (access_flags & Vm::access_read)    ret = PAGE_READONLY;
    if (access_flags & Vm::access_execute)      ret <<= 4;
    return ret;
}



//------------------------------------------------------------------------------
Vm::Vm(int32 pid)
{
    if (pid > 0)
        _handle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|
            PROCESS_VM_WRITE|PROCESS_VM_READ, FALSE, pid);
    else
        _handle = GetCurrentProcess();
}

//------------------------------------------------------------------------------
Vm::~Vm()
{
    if (_handle != nullptr)
        CloseHandle(_handle);
}

//------------------------------------------------------------------------------
size_t Vm::get_block_granularity()
{
    if (!g_alloc_granularity)
        initialise_page_constants();

    return g_alloc_granularity;
}

//------------------------------------------------------------------------------
size_t Vm::get_page_size()
{
    if (!g_page_size)
        initialise_page_constants();

    return g_page_size;
}

//------------------------------------------------------------------------------
void* Vm::get_alloc_base(void* address)
{
    if (_handle == nullptr)
        return nullptr;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(_handle, address, &mbi, sizeof(mbi)))
        return mbi.AllocationBase;

    return nullptr;
}

//------------------------------------------------------------------------------
Vm::Region Vm::get_region(void* address)
{
    if (_handle == nullptr)
        return {};

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(_handle, address, &mbi, sizeof(mbi)))
        return {mbi.BaseAddress, uint32(mbi.RegionSize / get_page_size())};

    return {};
}

//------------------------------------------------------------------------------
void* Vm::get_page(void* address)
{
    return (void*)(uintptr_t(address) & ~(get_page_size() - 1));
}

//------------------------------------------------------------------------------
Vm::Region Vm::alloc(uint32 page_count, uint32 access)
{
    if (_handle == nullptr)
        return {};

    int32 ms_access = to_ms_flags(access);
    size_t size = page_count * get_page_size();
    if (void* base = VirtualAllocEx(_handle, nullptr, size, MEM_COMMIT, ms_access))
        return {base, page_count};

    return {};
}

//------------------------------------------------------------------------------
void Vm::free(const Region& Region)
{
    if (_handle == nullptr)
        return;

    size_t size = Region.page_count * get_page_size();
    VirtualFreeEx(_handle, Region.base, size, MEM_RELEASE);
}

//------------------------------------------------------------------------------
int32 Vm::get_access(const Region& Region)
{
    if (_handle == nullptr)
        return -1;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(_handle, Region.base, &mbi, sizeof(mbi)))
        return to_access_flags(mbi.Protect);

    return -1;
}

//------------------------------------------------------------------------------
void Vm::set_access(const Region& Region, uint32 access)
{
    if (_handle == nullptr)
        return;

    DWORD ms_flags = to_ms_flags(access);
    size_t size = Region.page_count * get_page_size();
    VirtualProtectEx(_handle, Region.base, size, ms_flags, &ms_flags);
}

//------------------------------------------------------------------------------
bool Vm::read(void* dest, const void* src, size_t size)
{
    if (_handle == nullptr)
        return false;

    return (ReadProcessMemory(_handle, src, dest, size, nullptr) != FALSE);
}

//------------------------------------------------------------------------------
bool Vm::write(void* dest, const void* src, size_t size)
{
    if (_handle == nullptr)
        return false;

    return (WriteProcessMemory(_handle, dest, src, size, nullptr) != FALSE);
}

//------------------------------------------------------------------------------
void Vm::flush_icache(const Region& Region)
{
    if (_handle == nullptr)
        return;

    size_t size = Region.page_count * get_page_size();
    FlushInstructionCache(_handle, Region.base, size);
}
