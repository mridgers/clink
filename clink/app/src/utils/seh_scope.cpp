// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "seh_scope.h"
#include "utils/app_context.h"

#include <core/str.h>

//------------------------------------------------------------------------------
static LONG WINAPI exception_filter(EXCEPTION_POINTERS* info)
{
#if defined(_MSC_VER)
    Str<MAX_PATH, false> buffer;
    if (const AppContext* context = AppContext::get())
        context->get_state_dir(buffer);
    else
    {
        AppContext::Desc desc;
        AppContext app_context(desc);
        app_context.get_state_dir(buffer);
    }
    buffer << "\\clink.dmp";

    fputs("\n!!! CLINK'S CRASHED!", stderr);
    fputs("\n!!!", stderr);
    fputs("\n!!! Writing core dump", stderr);
    fputs("\n!!! ", stderr);
    fputs(buffer.c_str(), stderr);

    // Write a core dump file.
    BOOL ok = FALSE;
    Wstr<256> wbuffer(buffer.c_str());
    HANDLE file = CreateFileW(wbuffer.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (file != INVALID_HANDLE_VALUE)
    {
        DWORD pid = GetCurrentProcessId();
        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (process != nullptr)
        {
            MINIDUMP_EXCEPTION_INFORMATION mdei = { GetCurrentThreadId(), info, FALSE };
            ok = MiniDumpWriteDump(process, pid, file, MiniDumpNormal, &mdei, nullptr, nullptr);
        }
        CloseHandle(process);
        CloseHandle(file);
    }
    fputs(ok ? "\n!!! ...ok" : "\n!!! ...failed", stderr);
    fputs("\n!!!", stderr);

    // Output some useful modules' base addresses
    buffer.format("\n!!! Clink: 0x%p", GetModuleHandle(CLINK_DLL));
    fputs(buffer.c_str(), stderr);

    buffer.format("\n!!! Host: 0x%p", GetModuleHandle(nullptr));
    fputs(buffer.c_str(), stderr);

    // Output information about the exception that occurred.
    EXCEPTION_RECORD& record = *(info->ExceptionRecord);
    if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        uintptr_t access = record.ExceptionInformation[0];
        uintptr_t addr = record.ExceptionInformation[1];
        buffer.format("\n!!! Guru: 0x%08x addr=0x%p (%d)", record.ExceptionCode, addr, access);
    }
    else
        buffer.format("\n!!! Guru: 0x%08x", record.ExceptionCode);
    fputs(buffer.c_str(), stderr);

    // Output a backtrace.
    fputs("\n!!!", stderr);
    fputs("\n!!! Backtrace:", stderr);
    bool skip = true;
    void* backtrace[48];
    int32 bt_length = CaptureStackBackTrace(0, sizeof_array(backtrace), backtrace, nullptr);
    for (int32 i = 0; i < bt_length; ++i)
    {
        if (skip &= (backtrace[i] != record.ExceptionAddress))
            continue;

        buffer.format("\n!!! 0x%p", backtrace[i]);
        fputs(buffer.c_str(), stderr);
    }

    fputs("\n\nPress Enter to exit...", stderr);
    fgetc(stdin);
#endif // _MSC_VER

    // Would be awesome if we could unhook ourself, unload, and allow cmd.exe
    // to continue!

    return EXCEPTION_CONTINUE_SEARCH;
}



//------------------------------------------------------------------------------
SehScope::SehScope()
{
    _prev_filter = (void*)SetUnhandledExceptionFilter(exception_filter);
}

//------------------------------------------------------------------------------
SehScope::~SehScope()
{
    SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER(_prev_filter));
}
