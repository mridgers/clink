// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "process.h"
#include "vm.h"
#include "pe.h"

#include <core/base.h>
#include <core/path.h>
#include <core/str.h>

#include <PsApi.h>
#include <TlHelp32.h>
#include <stddef.h>

//------------------------------------------------------------------------------
Process::Process(int32 pid)
: _pid(pid)
{
    if (_pid == -1)
        _pid = GetCurrentProcessId();
}

//------------------------------------------------------------------------------
int32 Process::get_parent_pid() const
{
    LONG (WINAPI *NtQueryInformationProcess)(HANDLE, ULONG, PVOID, ULONG, PULONG);

    *(FARPROC*)&NtQueryInformationProcess = GetProcAddress(
        LoadLibraryA("ntdll.dll"),
        "NtQueryInformationProcess"
    );

    if (NtQueryInformationProcess != nullptr)
    {
        Handle handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _pid);
        ULONG size = 0;
        ULONG_PTR pbi[6];
        LONG ret = NtQueryInformationProcess(handle, 0, &pbi, sizeof(pbi), &size);
        if ((ret >= 0) && (size == sizeof(pbi)))
            return (DWORD)(pbi[5]);
    }

    return 0;
}

//------------------------------------------------------------------------------
bool Process::get_file_name(StrBase& out) const
{
    Handle handle = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, _pid);
    if (!handle)
        return false;

    static DWORD (WINAPI *func)(HANDLE, HMODULE, LPWSTR, DWORD) = nullptr;
    if (func == nullptr)
        if (HMODULE psapi = LoadLibrary("psapi.dll"))
            *(FARPROC*)&func = GetProcAddress(psapi, "GetModuleFileNameExW");

    if (func != nullptr)
    {
        Wstr<270> w_out;
        if (func(handle, nullptr, w_out.data(), w_out.size()) != 0)
        {
            out = w_out.c_str();
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
Process::Arch Process::get_arch() const
{
    int32 is_x64_os;
    SYSTEM_INFO system_info;
    GetNativeSystemInfo(&system_info);
    is_x64_os = system_info.wProcessorArchitecture;
    is_x64_os = (is_x64_os == PROCESSOR_ARCHITECTURE_AMD64);

    if (is_x64_os)
    {
        Handle handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _pid);
        if (!handle)
            return arch_unknown;

        BOOL is_wow64;
        if (IsWow64Process(handle, &is_wow64) == FALSE)
            return arch_unknown;

        return is_wow64 ? arch_x86 : arch_x64;
    }

    return arch_x86;
}

//------------------------------------------------------------------------------
void Process::pause(bool suspend)
{
    Handle th32 = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, _pid);
    if (!th32)
        return;

    THREADENTRY32 te = { sizeof(te) };
    BOOL ok = Thread32First(th32, &te);
    while (ok != FALSE)
    {
        if (te.th32OwnerProcessID == _pid)
        {
            Handle thread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
            suspend ? SuspendThread(thread) : ResumeThread(thread);
        }

        ok = Thread32Next(th32, &te);
    }
}

//------------------------------------------------------------------------------
void* Process::inject_module(const char* dll_path)
{
    // Check we can inject into the target.
    if (Process().get_arch() != get_arch())
        return nullptr;

    // Get the address to LoadLibrary. Note that we get LoadLibrary address
    // directly from kernel32.dll's export table. If our import table has had
    // LoadLibraryW hooked then we'd get a potentially invalid address if we
    // were to just use &LoadLibraryW.
    PeInfo kernel32(LoadLibrary("kernel32.dll"));
    void* thread_proc = (void*)kernel32.get_export("LoadLibraryW");

    Wstr<280> wpath(dll_path);
    return remote_call(thread_proc, wpath.data(), wpath.length() * sizeof(wchar_t));
}

//------------------------------------------------------------------------------
void* Process::remote_call(void* function, const void* param, int32 param_size)
{
    // Open the Process so we can operate on it.
    uint32 Flags = PROCESS_QUERY_INFORMATION|PROCESS_CREATE_THREAD;
    Handle process_handle = OpenProcess(Flags, FALSE, _pid);
    if (!process_handle)
        return nullptr;

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable : 4200)
#endif
    struct ThunkData
    {
        void*   (*func)(void*);
        void*   out;
        char    in[];
    };
#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#if 1
#   if ARCHITECTURE == 86
    uint8 thunk[] = {
        //0xcc,
        0x8b, 0x44, 0x24, 0x04,
        0x8d, 0x48, 0x08,
        0x51,
        0xff, 0x10,
        0x8b, 0x4c, 0x24, 0x04,
        0x89, 0x41, 0x04,
        0xc3,
    };
#   elif ARCHITECTURE == 64
    uint8 thunk[] = {
        //0xcc,
        0x57,
        0x48, 0x83, 0xec, 0x20,
        0x48, 0x89, 0xcf,
        0x48, 0x83, 0xc1, 0x10,
        0xff, 0x17,
        0x48, 0x89, 0x47, 0x08,
        0x48, 0x83, 0xc4, 0x20,
        0x5f,
        0xc3,
    };
#   endif
#else
    const auto& thunk_impl = [] (ThunkData& data) {
        data.out = data.func(data.in);
    };
    auto* thunk = static_cast<void (__stdcall*)(ThunkData&)>(thunk_impl);
#endif

    static int32 thunk_size;
    if (!thunk_size)
        for (const auto* c = (uint8*)thunk; ++thunk_size, *c++ != 0xc3;);

    Vm vm(_pid);
    Vm::Region Region = vm.alloc(1, Vm::access_write);
    if (Region.base == nullptr)
        return nullptr;

    int32 write_offset = 0;
    const auto& vm_write = [&] (const void* data, int32 size) {
        void* addr = (char*)Region.base + write_offset;
        vm.write(addr, data, size);
        write_offset = (write_offset + size + 7) & ~7;
        return addr;
    };

    vm_write((void*)thunk, thunk_size);
    void* thunk_ptrs[2] = { function };
    char* remote_thunk_data = (char*)vm_write(thunk_ptrs, sizeof(thunk_ptrs));
    vm_write(param, param_size);
    vm.set_access(Region, Vm::access_rwx); // writeable so thunk() can write output.

    static_assert(sizeof(thunk_ptrs) == sizeof(ThunkData), "");
    static_assert((offsetof(ThunkData, in) & 7) == 0, "");

    pause();

    // The 'remote call' is actually a thread that's created in the Process and
    // then waited on for completion.
    DWORD thread_id;
    Handle remote_thread = CreateRemoteThread(process_handle, nullptr, 0,
        (LPTHREAD_START_ROUTINE)Region.base, remote_thunk_data, 0, &thread_id);
    if (!remote_thread)
    {
        unpause();
        return 0;
    }

    WaitForSingleObject(remote_thread, INFINITE);
    unpause();

    void* call_ret = nullptr;
    vm.read(&call_ret, remote_thunk_data + offsetof(ThunkData, out), sizeof(call_ret));
    vm.free(Region);

    return call_ret;
}
