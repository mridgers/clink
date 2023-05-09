// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "editor_module.h"

#include <core/singleton.h>

//------------------------------------------------------------------------------
class RlModule
    : public EditorModule
    , public Singleton<RlModule>
{
public:
                    RlModule(const char* shell_name);
                    ~RlModule();

private:
    virtual void    bind_input(Binder& binder) override;
    virtual void    on_begin_line(const Context& context) override;
    virtual void    on_end_line() override;
    virtual void    on_matches_changed(const Context& context) override;
    virtual void    on_input(const Input& Input, Result& result, const Context& context) override;
    virtual void    on_terminal_resize(int columns, int rows, const Context& context) override;
    void            done(const char* line);
    char*           m_rl_buffer;
    int             m_prev_group;
    int             m_catch_group;
    bool            m_done;
    bool            m_eof;
};
