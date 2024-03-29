// Copyright (c) Martin Ridgers
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
    virtual void    on_terminal_resize(int32 columns, int32 rows, const Context& context) override;
    void            done(const char* line);
    char*           _rl_buffer;
    int32           _prev_group;
    int32           _catch_group;
    bool            _done;
    bool            _eof;
};
