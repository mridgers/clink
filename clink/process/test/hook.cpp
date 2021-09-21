// Copyright (c) 2021 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <process/hook.h>

//------------------------------------------------------------------------------
enum
{
    visitor_set_env_var     = 1 << 0,
    visitor_write_console   = 1 << 1,
    visitor_get_std_handle  = 1 << 2,
    visitor_read_console    = 1 << 3,
};

struct originals
{
    BOOL    (WINAPI *set_env_var)(const wchar_t*, const wchar_t*);
    BOOL    (WINAPI *write_console)(HANDLE, const void*, DWORD, LPDWORD, LPVOID);
    HANDLE  (WINAPI *get_std_handle)(DWORD);
};

static originals    g_originals;
static unsigned int g_visited_bits;

//------------------------------------------------------------------------------
BOOL WINAPI set_env_var(const wchar_t* name, const wchar_t* value)
{
    g_visited_bits |= visitor_set_env_var;
    return g_originals.set_env_var(name, value);
}

//------------------------------------------------------------------------------
BOOL WINAPI write_console(
    HANDLE output,
    const wchar_t* chars,
    DWORD to_write,
    LPDWORD written,
    LPVOID unused)
{
    g_visited_bits |= visitor_write_console;
    return g_originals.write_console(output, chars, to_write, written, unused);
}

//------------------------------------------------------------------------------
HANDLE WINAPI get_std_handle(unsigned int handle_id)
{
    g_visited_bits |= visitor_get_std_handle;
    return g_originals.get_std_handle(handle_id);
}

//------------------------------------------------------------------------------
BOOL WINAPI read_console(...)
{
    g_visited_bits |= visitor_read_console;
    return TRUE;
}



//------------------------------------------------------------------------------
TEST_CASE("Hooks")
{
    void* base = GetModuleHandle(nullptr);

    funcptr_t original;

    g_originals.set_env_var = SetEnvironmentVariableW;
    original = hook_iat(base, nullptr, "SetEnvironmentVariableW", funcptr_t(set_env_var), 1);
    REQUIRE(original == funcptr_t(g_originals.set_env_var));

    g_originals.write_console = WriteConsoleW;
    original = hook_iat(base, nullptr, "WriteConsoleW", funcptr_t(write_console), 1);
    REQUIRE(original == funcptr_t(g_originals.write_console));

    g_originals.get_std_handle = GetStdHandle;
    original = hook_iat(base, nullptr, "GetStdHandle", funcptr_t(get_std_handle), 1);
    REQUIRE(original == funcptr_t(g_originals.get_std_handle));

    base = GetModuleHandle("kernel32.dll");
    original = hook_jmp(base, "ReadConsoleW", funcptr_t(read_console));
    REQUIRE(original != nullptr);

    g_visited_bits = 0;

    SetEnvironmentVariableW(L"clink_test", nullptr);
    REQUIRE(g_visited_bits & visitor_set_env_var);

    WriteConsoleW(nullptr, nullptr, 0, nullptr, nullptr);
    REQUIRE(g_visited_bits & visitor_write_console);

    GetStdHandle(0x493);
    REQUIRE(g_visited_bits & visitor_get_std_handle);

    ReadConsoleW(nullptr, nullptr, 0, nullptr, nullptr);
    REQUIRE(g_visited_bits & visitor_read_console);
}
