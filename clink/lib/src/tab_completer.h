// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "editor_module.h"

//------------------------------------------------------------------------------
class TabCompleter
    : public EditorModule
{
private:
    enum State : unsigned char
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
    virtual void    on_terminal_resize(int columns, int rows, const Context& context) override;
    State           begin_print(const Context& context);
    State           print(const Context& context, bool single_row);
    int             m_longest = 0;
    int             m_row = 0;
    int             m_prompt_bind_group = -1;
    int             m_pager_bind_group = -1;
    int             m_prev_group = -1;
    bool            m_waiting = false;
};
