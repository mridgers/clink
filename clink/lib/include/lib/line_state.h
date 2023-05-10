// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/str_iter.h>

template <typename T> class Array;

//------------------------------------------------------------------------------
struct Word
{
    uint32              offset : 14;
    uint32              length : 10;
    uint32              quoted : 1;
    uint32              delim  : 7;
};

//------------------------------------------------------------------------------
class LineState
{
public:
                        LineState(const char* line, uint32 cursor, uint32 command_offset, const Array<Word>& words);
    const char*         get_line() const;
    uint32              get_cursor() const;
    uint32              get_command_offset() const;
    const Array<Word>&  get_words() const;
    uint32              get_word_count() const;
    bool                get_word(uint32 index, StrBase& out) const;
    StrIter             get_word(uint32 index) const;
    bool                get_end_word(StrBase& out) const;
    StrIter             get_end_word() const;

private:
    const Array<Word>&  _words;
    const char*         _line;
    uint32              _cursor;
    uint32              _command_offset;
};
