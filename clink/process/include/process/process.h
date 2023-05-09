// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <Windows.h>

class StrBase;

//------------------------------------------------------------------------------
class Process
{
public:
    enum Arch { arch_unknown, arch_x86, arch_x64 };

                                Process(int pid=-1);
    int                         get_pid() const;
    bool                        get_file_name(StrBase& out) const;
    Arch                        get_arch() const;
    int                         get_parent_pid() const;
    void*                       inject_module(const char* dll);
    template <typename T> void* remote_call(void* function, T const& param);
    void                        pause();
    void                        unpause();

private:
    void*                       remote_call(void* function, const void* param, int param_size);
    void                        pause(bool suspend);
    int                         m_pid;

    struct Handle
    {
        Handle(HANDLE h) : m_handle(h)  {}
        ~Handle()                       { CloseHandle(m_handle); }
        explicit operator bool () const { return (m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE); }
        operator HANDLE () const        { return m_handle; }
        HANDLE m_handle;
    };
};

//------------------------------------------------------------------------------
inline int Process::get_pid() const
{
    return m_pid;
}

//------------------------------------------------------------------------------
template <typename T>
void* Process::remote_call(void* function, T const& param)
{
    return remote_call(function, &param, sizeof(param));
}

//------------------------------------------------------------------------------
inline void Process::pause()
{
    pause(true);
}

//------------------------------------------------------------------------------
inline void Process::unpause()
{
    pause(false);
}
