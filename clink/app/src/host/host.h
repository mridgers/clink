// Copyright (c) 2013 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "history/history_db.h"

#include <lib/line_editor.h>

class LuaState;
class StrBase;

//------------------------------------------------------------------------------
class Host
{
public:
                    Host(const char* name);
    virtual         ~Host();
    virtual bool    validate() = 0;
    virtual bool    initialise() = 0;
    virtual void    shutdown() = 0;

protected:
    bool            edit_line(const char* prompt, StrBase& out);
    virtual void    initialise_lua(LuaState& lua) = 0;
    virtual void    initialise_editor_desc(LineEditor::Desc& desc) = 0;

private:
    void            filter_prompt(const char* in, StrBase& out);
    const char*     _name;
    HistoryDb       _history;
};
