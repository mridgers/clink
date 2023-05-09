// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "attributes.h"

class TerminalOut;

//------------------------------------------------------------------------------
class Printer
{
public:
                            Printer(TerminalOut& Terminal);
    void                    print(const char* data, int bytes);
    void                    print(const Attributes attr, const char* data, int bytes);
    template <int S> void   print(const char (&data)[S]);
    template <int S> void   print(const Attributes attr, const char (&data)[S]);
    unsigned int            get_columns() const;
    unsigned int            get_rows() const;
    Attributes              set_attributes(const Attributes attr);
    Attributes              get_attributes() const;

private: /* TODO: unimplemented API */
    typedef unsigned int    CursorState;
    void                    insert(int count); // -count == delete characters.
    void                    move_cursor(int dc, int dr);
    void                    set_cursor(CursorState state);
    CursorState             get_cursor() const;

private:
    void                    flush_attributes();
    TerminalOut&            m_terminal;
    Attributes              m_set_attr;
    Attributes              m_next_attr;
};

//------------------------------------------------------------------------------
template <int S> void Printer::print(const char (&data)[S])
{
    print(data, S);
}

//------------------------------------------------------------------------------
template <int S> void Printer::print(const Attributes attr, const char (&data)[S])
{
    print(attr, data, S);
}
