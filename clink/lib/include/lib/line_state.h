// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/str_iter.h>

template <typename T> class Array;

//------------------------------------------------------------------------------
struct Word
{
    unsigned int        offset : 14;
    unsigned int        length : 10;
    unsigned int        quoted : 1;
    unsigned int        delim  : 7;
};

//------------------------------------------------------------------------------
class LineState
{
public:
                        LineState(const char* line, unsigned int cursor, unsigned int command_offset, const Array<Word>& words);
    const char*         get_line() const;
    unsigned int        get_cursor() const;
    unsigned int        get_command_offset() const;
    const Array<Word>&  get_words() const;
    unsigned int        get_word_count() const;
    bool                get_word(unsigned int index, StrBase& out) const;
    StrIter             get_word(unsigned int index) const;
    bool                get_end_word(StrBase& out) const;
    StrIter             get_end_word() const;

private:
    const Array<Word>&  m_words;
    const char*         m_line;
    unsigned int        m_cursor;
    unsigned int        m_command_offset;
};
