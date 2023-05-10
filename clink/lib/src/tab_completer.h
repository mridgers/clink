// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "editor_module.h"

//------------------------------------------------------------------------------
class TabCompleter
    : public EditorModule
{
private:
    enum State : uint8
    {
        state_none,
        state_query,
        state_pager,
        state_print,
        state_print_one,
        state_print_page,
    };

    virtual void    bind_input(Binder& binder) override;
    virtual void    on_begin_line(const Context& context) override;
    virtual void    on_end_line() override;
    virtual void    on_matches_changed(const Context& context) override;
    virtual void    on_input(const Input& input, Result& result, const Context& context) override;
    virtual void    on_terminal_resize(int32 columns, int32 rows, const Context& context) override;
    State           begin_print(const Context& context);
    State           print(const Context& context, bool single_row);
    int32           _longest = 0;
    int32           _row = 0;
    int32           _prompt_bind_group = -1;
    int32           _pager_bind_group = -1;
    int32           _prev_group = -1;
    bool            _waiting = false;
};
