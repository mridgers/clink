// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "line_state.h"

#include <core/array.h>
#include <core/str.h>

//------------------------------------------------------------------------------
LineState::LineState(
    const char* line,
    uint32 cursor,
    uint32 command_offset,
    const Array<Word>& words)
: _words(words)
, _line(line)
, _cursor(cursor)
, _command_offset(command_offset)
{
}

//------------------------------------------------------------------------------
const char* LineState::get_line() const
{
    return _line;
}

//------------------------------------------------------------------------------
uint32 LineState::get_cursor() const
{
    return _cursor;
}

//------------------------------------------------------------------------------
uint32 LineState::get_command_offset() const
{
    return _command_offset;
}

//------------------------------------------------------------------------------
const Array<Word>& LineState::get_words() const
{
    return _words;
}

//------------------------------------------------------------------------------
uint32 LineState::get_word_count() const
{
    return _words.size();
}

//------------------------------------------------------------------------------
bool LineState::get_word(uint32 index, StrBase& out) const
{
    const Word* word = _words[index];
    if (word == nullptr)
        return false;

    out.concat(_line + word->offset, word->length);
    return true;
}

//------------------------------------------------------------------------------
StrIter LineState::get_word(uint32 index) const
{
    if (const Word* word = _words[index])
        return StrIter(_line + word->offset, word->length);

    return StrIter();
}

//------------------------------------------------------------------------------
bool LineState::get_end_word(StrBase& out) const
{
    int32 n = get_word_count();
    return (n ? get_word(n - 1, out) : false);
}

//------------------------------------------------------------------------------
StrIter LineState::get_end_word() const
{
    int32 n = get_word_count();
    return (n ? get_word(n - 1) : StrIter());
}
