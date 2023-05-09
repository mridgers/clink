// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "host.h"
#include "prompt.h"
#include "doskey.h"

#include <core/singleton.h>

class LuaState;

//------------------------------------------------------------------------------
class HostCmd
    : public Host
    , public Singleton<HostCmd>
{
public:
                        HostCmd();
    virtual bool        validate() override;
    virtual bool        initialise() override;
    virtual void        shutdown() override;

private:
    static BOOL WINAPI  read_console(HANDLE input, wchar_t* buffer, DWORD buffer_count, LPDWORD read_in, CONSOLE_READCONSOLE_CONTROL* control);
    static BOOL WINAPI  write_console(HANDLE handle, const wchar_t* chars, DWORD to_write, LPDWORD written, LPVOID);
    static BOOL WINAPI  set_env_var(const wchar_t* name, const wchar_t* value);
    bool                initialise_system();
    virtual void        initialise_lua(LuaState& lua) override;
    virtual void        initialise_editor_desc(LineEditor::Desc& desc) override;
    void                edit_line(const wchar_t* prompt, wchar_t* chars, int max_chars);
    bool                capture_prompt(const wchar_t* chars, int char_count);
    bool                is_interactive() const;
    TaggedPrompt        m_prompt;
    Doskey              m_doskey;
    DoskeyAlias         m_doskey_alias;
};
