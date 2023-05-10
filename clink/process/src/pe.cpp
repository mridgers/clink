// Copyright (c) 2012 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "pe.h"

#include <core/log.h>
#include <Windows.h>

//------------------------------------------------------------------------------
PeInfo::PeInfo(void* base)
: _base(base)
{
}

//------------------------------------------------------------------------------
void* PeInfo::rva_to_addr(uint32 rva) const
{
    return (char*)(uintptr_t)rva + (uintptr_t)_base;
}

//------------------------------------------------------------------------------
IMAGE_NT_HEADERS* PeInfo::get_nt_headers() const
{
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)_base;
    return (IMAGE_NT_HEADERS*)((char*)_base + dos_header->e_lfanew);
}

//------------------------------------------------------------------------------
void* PeInfo::get_data_directory(int32 index, int32* size) const
{
    IMAGE_NT_HEADERS* nt = get_nt_headers();
    IMAGE_DATA_DIRECTORY* data_dir = nt->OptionalHeader.DataDirectory + index;
    if (data_dir == nullptr)
        return nullptr;

    if (data_dir->VirtualAddress == 0)
        return nullptr;

    if (size != nullptr)
        *size = data_dir->Size;

    return rva_to_addr(data_dir->VirtualAddress);
}

//------------------------------------------------------------------------------
PeInfo::funcptr_t* PeInfo::import_by_addr(IMAGE_IMPORT_DESCRIPTOR* iid, const void* func_addr) const
{
    void** at = (void**)rva_to_addr(iid->FirstThunk);
    while (*at != 0)
    {
        uintptr_t addr = (uintptr_t)(*at);
        void* addr_loc = at;

        if (addr == (uintptr_t)func_addr)
            return (funcptr_t*)at;

        ++at;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
PeInfo::funcptr_t* PeInfo::import_by_name(IMAGE_IMPORT_DESCRIPTOR* iid, const void* func_name) const
{
    void** at = (void**)rva_to_addr(iid->FirstThunk);
    intptr_t* nt = (intptr_t*)rva_to_addr(iid->OriginalFirstThunk);
    while (*at != 0 && *nt != 0)
    {
        // Check that this import is imported by name (MSB not set)
        if (*nt > 0)
        {
            uint32 rva = (uint32)(*nt & 0x7fffffff);
            IMAGE_IMPORT_BY_NAME* iin;
            iin = (IMAGE_IMPORT_BY_NAME*)rva_to_addr(rva);

            if (_stricmp((const char*)(iin->Name), (const char*)func_name) == 0)
                return (funcptr_t*)at;
        }

        ++at;
        ++nt;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
PeInfo::funcptr_t* PeInfo::get_import_by_name(const char* dll, const char* func_name) const
{
    return iterate_imports(dll, func_name, &PeInfo::import_by_name);
}

//------------------------------------------------------------------------------
PeInfo::funcptr_t* PeInfo::get_import_by_addr(const char* dll, funcptr_t func_addr) const
{
    return iterate_imports(dll, (const void*)func_addr, &PeInfo::import_by_addr);
}

//------------------------------------------------------------------------------
PeInfo::funcptr_t PeInfo::get_export(const char* func_name) const
{
    if (_base == nullptr)
        return nullptr;

    IMAGE_EXPORT_DIRECTORY* ied = (IMAGE_EXPORT_DIRECTORY*)get_data_directory(0);
    if (ied == nullptr)
    {
        LOG("No export directory found at base %p", _base);
        return nullptr;
    }

    DWORD* names = (DWORD*)rva_to_addr(ied->AddressOfNames);
    WORD* ordinals = (WORD*)rva_to_addr(ied->AddressOfNameOrdinals);
    DWORD* addresses = (DWORD*)rva_to_addr(ied->AddressOfFunctions);

    for (int32 i = 0; i < int32(ied->NumberOfNames); ++i)
    {
        const char* export_name = (const char*)rva_to_addr(names[i]);
        if (_stricmp(export_name, func_name))
            continue;

        WORD ordinal = ordinals[i];
        return funcptr_t(rva_to_addr(addresses[ordinal]));
    }

    return nullptr;
}

//------------------------------------------------------------------------------
PeInfo::funcptr_t* PeInfo::iterate_imports(
    const char* dll,
    const void* param,
    import_iter_t iter_func) const
{
    IMAGE_IMPORT_DESCRIPTOR* iid;
    iid = (IMAGE_IMPORT_DESCRIPTOR*)get_data_directory(1, nullptr);
    if (iid == nullptr)
    {
        LOG("Failed to find import desc for base %p", _base);
        return 0;
    }

    while (iid->Name)
    {
        char* name;
        size_t len;

        len = (dll != nullptr) ? strlen(dll) : 0;
        name = (char*)rva_to_addr(iid->Name);
        if (dll == nullptr || _strnicmp(name, dll, len) == 0)
        {
            LOG("Checking imports in '%s'", name);
            if (funcptr_t* ret = (this->*iter_func)(iid, param))
                return ret;
        }

        ++iid;
    }

    return nullptr;
}
