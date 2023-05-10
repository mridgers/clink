// Copyright (c) 2017 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/str_iter.h>

//------------------------------------------------------------------------------
class HistoryDb
{
public:
    enum ExpandResult
    {
        // values match Readline's history_expand() return value.
        expand_error            = -1,
        expand_none             = 0,
        expand_ok               = 1,
        expand_print            = 2,
    };

    static const unsigned int   max_line_length = 8192;
    typedef unsigned int        LineId;

    class Iter
    {
    public:
                                ~Iter();
        LineId                  next(StrIter& out);

    private:
                                Iter() = default;
        friend                  HistoryDb;
        uintptr_t               impl = 0;
    };

                                HistoryDb();
                                ~HistoryDb();
    void                        initialise();
    void                        load_rl_history();
    void                        clear();
    bool                        add(const char* line);
    int                         remove(const char* line);
    bool                        remove(LineId id);
    LineId                      find(const char* line) const;
    ExpandResult                expand(const char* line, StrBase& out) const;
    template <int S> Iter       read_lines(char (&buffer)[S]);
    Iter                        read_lines(char* buffer, unsigned int buffer_size);

private:
    enum : char
    {
        bank_none               = -1,
        bank_master,
        bank_session,
        bank_count,
    };

    friend                      class ReadLineIter;
    void                        reap();
    template <typename T> void  for_each_bank(T&& callback);
    template <typename T> void  for_each_bank(T&& callback) const;
    unsigned int                get_bank_count() const;
    void*                       get_bank(unsigned int index) const;
    void*                       _alive_file;
    void*                       _bank_handles[bank_count];
};

//------------------------------------------------------------------------------
template <int S> HistoryDb::Iter HistoryDb::read_lines(char (&buffer)[S])
{
    return read_lines(buffer, S);
}
