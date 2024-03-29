// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class PeInfo
{
public:
    typedef void        (__stdcall *funcptr_t)();
                        PeInfo(void* base);
    funcptr_t*          get_import_by_name(const char* dll, const char* func_name) const;
    funcptr_t*          get_import_by_addr(const char* dll, funcptr_t func_addr) const;
    funcptr_t           get_export(const char* func_name) const;

private:
    typedef funcptr_t*  (PeInfo::*import_iter_t)(IMAGE_IMPORT_DESCRIPTOR*, const void*) const;

    IMAGE_NT_HEADERS*   get_nt_headers() const;
    void*               get_data_directory(int32 index, int32* size=nullptr) const;
    void*               rva_to_addr(uint32 rva) const;
    funcptr_t*          import_by_addr(IMAGE_IMPORT_DESCRIPTOR* iid, const void* func_addr) const;
    funcptr_t*          import_by_name(IMAGE_IMPORT_DESCRIPTOR* iid, const void* func_name) const;
    funcptr_t*          iterate_imports(const char* dll, const void* param, import_iter_t iter_func) const;
    void*               _base;
};
