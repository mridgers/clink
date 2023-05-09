// Copyright (c) 2018 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "ecma48_iter.h"
#include "terminal_out.h"

class ScreenBuffer;

//------------------------------------------------------------------------------
class Ecma48TerminalOut
    : public TerminalOut
{
public:
                        Ecma48TerminalOut(ScreenBuffer& screen);
    virtual void        begin() override;
    virtual void        end() override;
    virtual void        write(const char* chars, int length) override;
    virtual void        flush() override;
    virtual int         get_columns() const override;
    virtual int         get_rows() const override;

private:
    void                write_c1(const Ecma48Code& code);
    void                write_c0(int c0);
    void                set_attributes(const Ecma48Code::CsiBase& csi);
    void                erase_in_display(const Ecma48Code::CsiBase& csi);
    void                erase_in_line(const Ecma48Code::CsiBase& csi);
    void                set_cursor(const Ecma48Code::CsiBase& csi);
    void                insert_chars(const Ecma48Code::CsiBase& csi);
    void                delete_chars(const Ecma48Code::CsiBase& csi);
    void                set_private_mode(const Ecma48Code::CsiBase& csi);
    void                reset_private_mode(const Ecma48Code::CsiBase& csi);
    Ecma48State         m_state;
    ScreenBuffer&       m_screen;
};
