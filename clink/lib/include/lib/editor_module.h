// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class LineBuffer;
class LineState;
class Matches;
class Printer;

//------------------------------------------------------------------------------
class EditorModule
{
public:
    struct Result
    {
        virtual void        pass() = 0;
        virtual void        redraw() = 0;
        virtual void        done(bool eof=false) = 0;
        virtual void        append_match_lcd() = 0;
        virtual void        accept_match(uint32 index) = 0;
        virtual int32       set_bind_group(int32 bind_group) = 0;
    };

    struct Input
    {
        const char*         keys;
        uint8               id;
    };

    struct Context
    {
        const char*         prompt;
        Printer&            printer;
        LineBuffer&         buffer;
        const LineState&    line;
        const Matches&      matches;
    };

    struct Binder
    {
        virtual int32       get_group(const char* name=nullptr) const = 0;
        virtual int32       create_group(const char* name) = 0;
        virtual bool        bind(uint32 group, const char* chord, uint8 id) = 0;
    };

    virtual                 ~EditorModule() = default;
    virtual void            bind_input(Binder& binder) = 0;
    virtual void            on_begin_line(const Context& context) = 0;
    virtual void            on_end_line() = 0;
    virtual void            on_matches_changed(const Context& context) = 0;
    virtual void            on_input(const Input& Input, Result& result, const Context& context) = 0;
    virtual void            on_terminal_resize(int32 columns, int32 rows, const Context& context) = 0;
};
