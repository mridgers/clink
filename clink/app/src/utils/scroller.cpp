// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "scroller.h"

//------------------------------------------------------------------------------
Scroller::Scroller()
: m_handle(0)
{
    m_cursor_position.X = 0;
    m_cursor_position.Y = 0;
}

//------------------------------------------------------------------------------
void Scroller::begin()
{
    m_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_handle, &csbi);
    m_cursor_position = csbi.dwCursorPosition;
}

//------------------------------------------------------------------------------
void Scroller::end()
{
    SetConsoleCursorPosition(m_handle, m_cursor_position);
    m_handle = 0;
    m_cursor_position.X = 0;
    m_cursor_position.Y = 0;
}

//------------------------------------------------------------------------------
void Scroller::page_up()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_handle, &csbi);
    SMALL_RECT* wnd = &csbi.srWindow;

    int rows_per_page = wnd->Bottom - wnd->Top - 1;
    if (rows_per_page > wnd->Top)
        rows_per_page = wnd->Top;

    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = wnd->Top - rows_per_page;
    SetConsoleCursorPosition(m_handle, csbi.dwCursorPosition);
}

//------------------------------------------------------------------------------
void Scroller::page_down()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_handle, &csbi);
    SMALL_RECT* wnd = &csbi.srWindow;

    int rows_per_page = wnd->Bottom - wnd->Top - 1;

    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = wnd->Bottom + rows_per_page;
    if (csbi.dwCursorPosition.Y > m_cursor_position.Y)
        csbi.dwCursorPosition.Y = m_cursor_position.Y;

    SetConsoleCursorPosition(m_handle, csbi.dwCursorPosition);
}



//------------------------------------------------------------------------------
void ScrollerModule::bind_input(Binder& binder)
{
    m_bind_group = binder.create_group("Scroller");
    if (m_bind_group >= 0)
    {
        int default_group = binder.get_group();
        binder.bind(default_group, "\\e[5;2~", bind_id_start);

        binder.bind(m_bind_group, "\\e[5;2~", bind_id_pgup);
        binder.bind(m_bind_group, "\\e[6;2~", bind_id_pgdown);
        binder.bind(m_bind_group, "", bind_id_catchall);
    }
}

//------------------------------------------------------------------------------
void ScrollerModule::on_begin_line(const Context& context)
{
}

//------------------------------------------------------------------------------
void ScrollerModule::on_end_line()
{
}

//------------------------------------------------------------------------------
void ScrollerModule::on_matches_changed(const Context& context)
{
}

//------------------------------------------------------------------------------
void ScrollerModule::on_input(
    const Input& input,
    Result& result,
    const Context& context)
{
    switch (input.id)
    {
    case bind_id_start:
        m_scroller.begin();
        m_scroller.page_up();
        m_prev_group = result.set_bind_group(m_bind_group);
        return;

    case bind_id_pgup:
        m_scroller.page_up();
        return;

    case bind_id_pgdown:
        m_scroller.page_down();
        return;

    case bind_id_catchall:
        m_scroller.end();
        result.set_bind_group(m_prev_group);
        result.pass();
        return;
    }

}

//------------------------------------------------------------------------------
void ScrollerModule::on_terminal_resize(int columns, int rows, const Context& context)
{
}
