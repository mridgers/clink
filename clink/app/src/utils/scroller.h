// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <lib/editor_module.h>

//------------------------------------------------------------------------------
class Scroller
{
public:
                    Scroller();
    void            begin();
    void            end();
    void            page_up();
    void            page_down();

private:
    HANDLE          m_handle;
    COORD           m_cursor_position;
};

//------------------------------------------------------------------------------
class ScrollerModule
    : public EditorModule
{
private:
    virtual void    bind_input(Binder& binder) override;
    virtual void    on_begin_line(const Context& context) override;
    virtual void    on_end_line() override;
    virtual void    on_matches_changed(const Context& context) override;
    virtual void    on_input(const Input& Input, Result& result, const Context& context) override;
    virtual void    on_terminal_resize(int columns, int rows, const Context& context) override;
    Scroller        m_scroller;
    int             m_bind_group;
    int             m_prev_group;

    enum
    {
        bind_id_start,
        bind_id_pgup,
        bind_id_pgdown,
        bind_id_catchall,
    };
};
