// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "scroller.h"

//------------------------------------------------------------------------------
Scroller::Scroller()
: _handle(0)
{
    _cursor_position.X = 0;
    _cursor_position.Y = 0;
}

//------------------------------------------------------------------------------
void Scroller::begin()
{
    _handle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);
    _cursor_position = csbi.dwCursorPosition;
}

//------------------------------------------------------------------------------
void Scroller::end()
{
    SetConsoleCursorPosition(_handle, _cursor_position);
    _handle = 0;
    _cursor_position.X = 0;
    _cursor_position.Y = 0;
}

//------------------------------------------------------------------------------
void Scroller::page_up()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);
    SMALL_RECT* wnd = &csbi.srWindow;

    int32 rows_per_page = wnd->Bottom - wnd->Top - 1;
    if (rows_per_page > wnd->Top)
        rows_per_page = wnd->Top;

    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = wnd->Top - rows_per_page;
    SetConsoleCursorPosition(_handle, csbi.dwCursorPosition);
}

//------------------------------------------------------------------------------
void Scroller::page_down()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle, &csbi);
    SMALL_RECT* wnd = &csbi.srWindow;

    int32 rows_per_page = wnd->Bottom - wnd->Top - 1;

    csbi.dwCursorPosition.X = 0;
    csbi.dwCursorPosition.Y = wnd->Bottom + rows_per_page;
    if (csbi.dwCursorPosition.Y > _cursor_position.Y)
        csbi.dwCursorPosition.Y = _cursor_position.Y;

    SetConsoleCursorPosition(_handle, csbi.dwCursorPosition);
}



//------------------------------------------------------------------------------
void ScrollerModule::bind_input(Binder& binder)
{
    _bind_group = binder.create_group("Scroller");
    if (_bind_group >= 0)
    {
        int32 default_group = binder.get_group();
        binder.bind(default_group, "\\e[5;2~", bind_id_start);

        binder.bind(_bind_group, "\\e[5;2~", bind_id_pgup);
        binder.bind(_bind_group, "\\e[6;2~", bind_id_pgdown);
        binder.bind(_bind_group, "", bind_id_catchall);
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
        _scroller.begin();
        _scroller.page_up();
        _prev_group = result.set_bind_group(_bind_group);
        return;

    case bind_id_pgup:
        _scroller.page_up();
        return;

    case bind_id_pgdown:
        _scroller.page_down();
        return;

    case bind_id_catchall:
        _scroller.end();
        result.set_bind_group(_prev_group);
        result.pass();
        return;
    }

}

//------------------------------------------------------------------------------
void ScrollerModule::on_terminal_resize(int32 columns, int32 rows, const Context& context)
{
}
