// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "attributes.h"

class TerminalOut;

//------------------------------------------------------------------------------
class Printer
{
public:
                            Printer(TerminalOut& terminal);
    void                    print(const char* data, int32 bytes);
    void                    print(const Attributes attr, const char* data, int32 bytes);
    template <int32 S> void   print(const char (&data)[S]);
    template <int32 S> void   print(const Attributes attr, const char (&data)[S]);
    uint32                  get_columns() const;
    uint32                  get_rows() const;
    Attributes              set_attributes(const Attributes attr);
    Attributes              get_attributes() const;

private: /* TODO: unimplemented API */
    typedef uint32          CursorState;
    void                    insert(int32 count); // -count == delete characters.
    void                    move_cursor(int32 dc, int32 dr);
    void                    set_cursor(CursorState state);
    CursorState             get_cursor() const;

private:
    void                    flush_attributes();
    TerminalOut&            _terminal;
    Attributes              _set_attr;
    Attributes              _next_attr;
};

//------------------------------------------------------------------------------
template <int32 S> void Printer::print(const char (&data)[S])
{
    print(data, S);
}

//------------------------------------------------------------------------------
template <int32 S> void Printer::print(const Attributes attr, const char (&data)[S])
{
    print(attr, data, S);
}
