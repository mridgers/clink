// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "vm.h"

#include <Windows.h>

//------------------------------------------------------------------------------
static unsigned int g_alloc_granularity = 0;
static unsigned int g_page_size         = 0;

//------------------------------------------------------------------------------
static void initialise_page_constants()
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    g_alloc_granularity = system_info.dwAllocationGranularity;
    g_page_size = system_info.dwPageSize;
}

//------------------------------------------------------------------------------
static unsigned int to_access_flags(unsigned int ms_flags)
{
    unsigned int ret = 0;
    if (ms_flags & 0x22) ret |= Vm::access_read;
    if (ms_flags & 0x44) ret |= Vm::access_write|Vm::access_read;
    if (ms_flags & 0x88) ret |= Vm::access_cow|Vm::access_write|Vm::access_read;
    if (ms_flags & 0xf0) ret |= Vm::access_execute;
    return ret;
}

//------------------------------------------------------------------------------
static unsigned int to_ms_flags(unsigned int access_flags)
{
    unsigned int ret = PAGE_NOACCESS;
    if (access_flags & Vm::access_cow)          ret = PAGE_WRITECOPY;
    else if (access_flags & Vm::access_write)   ret = PAGE_READWRITE;
    else if (access_flags & Vm::access_read)    ret = PAGE_READONLY;
    if (access_flags & Vm::access_execute)      ret <<= 4;
    return ret;
}



//------------------------------------------------------------------------------
Vm::Vm(int pid)
{
    if (pid > 0)
        m_handle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|
            PROCESS_VM_WRITE|PROCESS_VM_READ, FALSE, pid);
    else
        m_handle = GetCurrentProcess();
}

//------------------------------------------------------------------------------
Vm::~Vm()
{
    if (m_handle != nullptr)
        CloseHandle(m_handle);
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
    if (m_handle == nullptr)
        return nullptr;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(m_handle, address, &mbi, sizeof(mbi)))
        return mbi.AllocationBase;

    return nullptr;
}

//------------------------------------------------------------------------------
Vm::Region Vm::get_region(void* address)
{
    if (m_handle == nullptr)
        return {};

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(m_handle, address, &mbi, sizeof(mbi)))
        return {mbi.BaseAddress, unsigned(mbi.RegionSize / get_page_size())};

    return {};
}

//------------------------------------------------------------------------------
void* Vm::get_page(void* address)
{
    return (void*)(uintptr_t(address) & ~(get_page_size() - 1));
}

//------------------------------------------------------------------------------
Vm::Region Vm::alloc(unsigned int page_count, unsigned int access)
{
    if (m_handle == nullptr)
        return {};

    int ms_access = to_ms_flags(access);
    size_t size = page_count * get_page_size();
    if (void* base = VirtualAllocEx(m_handle, nullptr, size, MEM_COMMIT, ms_access))
        return {base, page_count};

    return {};
}

//------------------------------------------------------------------------------
void Vm::free(const Region& Region)
{
    if (m_handle == nullptr)
        return;

    size_t size = Region.page_count * get_page_size();
    VirtualFreeEx(m_handle, Region.base, size, MEM_RELEASE);
}

//------------------------------------------------------------------------------
int Vm::get_access(const Region& Region)
{
    if (m_handle == nullptr)
        return -1;

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQueryEx(m_handle, Region.base, &mbi, sizeof(mbi)))
        return to_access_flags(mbi.Protect);

    return -1;
}

//------------------------------------------------------------------------------
void Vm::set_access(const Region& Region, unsigned int access)
{
    if (m_handle == nullptr)
        return;

    DWORD ms_flags = to_ms_flags(access);
    size_t size = Region.page_count * get_page_size();
    VirtualProtectEx(m_handle, Region.base, size, ms_flags, &ms_flags);
}

//------------------------------------------------------------------------------
bool Vm::read(void* dest, const void* src, size_t size)
{
    if (m_handle == nullptr)
        return false;

    return (ReadProcessMemory(m_handle, src, dest, size, nullptr) != FALSE);
}

//------------------------------------------------------------------------------
bool Vm::write(void* dest, const void* src, size_t size)
{
    if (m_handle == nullptr)
        return false;

    return (WriteProcessMemory(m_handle, dest, src, size, nullptr) != FALSE);
}

//------------------------------------------------------------------------------
void Vm::flush_icache(const Region& Region)
{
    if (m_handle == nullptr)
        return;

    size_t size = Region.page_count * get_page_size();
    FlushInstructionCache(m_handle, Region.base, size);
}
