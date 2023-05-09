// Copyright (c) 2016 Martin Ridgers
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
    m_need_draw = true;
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
unsigned int RlBuffer::get_length() const
{
    return rl_end;
}

//------------------------------------------------------------------------------
unsigned int RlBuffer::get_cursor() const
{
    return rl_point;
}

//------------------------------------------------------------------------------
unsigned int RlBuffer::set_cursor(unsigned int pos)
{
    return rl_point = min<unsigned int>(pos, rl_end);
}

//------------------------------------------------------------------------------
bool RlBuffer::insert(const char* text)
{
    return (m_need_draw = (text[rl_insert_text(text)] == '\0'));
}

//------------------------------------------------------------------------------
bool RlBuffer::remove(unsigned int from, unsigned int to)
{
    to = min(to, get_length());
    m_need_draw = !!rl_delete_text(from, to);
    set_cursor(get_cursor());
    return m_need_draw;
}

//------------------------------------------------------------------------------
void RlBuffer::draw()
{
    if (m_need_draw)
    {
        rl_redisplay();
        m_need_draw = false;
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
