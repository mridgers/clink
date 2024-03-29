// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "rl_buffer.h"

#include <core/base.h>

extern "C" {
#include <readline/history.h>
#include <readline/readline.h>
}

//------------------------------------------------------------------------------
void RlBuffer::reset()
{
    using_history();
    remove(0, ~0u);
}

//------------------------------------------------------------------------------
void RlBuffer::begin_line()
{
    _need_draw = true;
}

//------------------------------------------------------------------------------
void RlBuffer::end_line()
{
}

//------------------------------------------------------------------------------
const char* RlBuffer::get_buffer() const
{
    return rl_line_buffer;
}

//------------------------------------------------------------------------------
uint32 RlBuffer::get_length() const
{
    return rl_end;
}

//------------------------------------------------------------------------------
uint32 RlBuffer::get_cursor() const
{
    return rl_point;
}

//------------------------------------------------------------------------------
uint32 RlBuffer::set_cursor(uint32 pos)
{
    return rl_point = min<uint32>(pos, rl_end);
}

//------------------------------------------------------------------------------
bool RlBuffer::insert(const char* text)
{
    return (_need_draw = (text[rl_insert_text(text)] == '\0'));
}

//------------------------------------------------------------------------------
bool RlBuffer::remove(uint32 from, uint32 to)
{
    to = min(to, get_length());
    _need_draw = !!rl_delete_text(from, to);
    set_cursor(get_cursor());
    return _need_draw;
}

//------------------------------------------------------------------------------
void RlBuffer::draw()
{
    if (_need_draw)
    {
        rl_redisplay();
        _need_draw = false;
    }
}

//------------------------------------------------------------------------------
void RlBuffer::redraw()
{
    rl_forced_update_display();
}

//------------------------------------------------------------------------------
void RlBuffer::begin_undo_group()
{
    rl_begin_undo_group();
}

//------------------------------------------------------------------------------
void RlBuffer::end_undo_group()
{
    rl_end_undo_group();
}
