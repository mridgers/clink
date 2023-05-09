// Copyright (c) 2016 Martin Ridgers
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
        virtual void        accept_match(unsigned int index) = 0;
        virtual int         set_bind_group(int bind_group) = 0;
    };

    struct Input
    {
        const char*         keys;
        unsigned char       id;
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
        virtual int         get_group(const char* name=nullptr) const = 0;
        virtual int         create_group(const char* name) = 0;
        virtual bool        bind(unsigned int group, const char* chord, unsigned char id) = 0;
    };

    virtual                 ~EditorModule() = default;
    virtual void            bind_input(Binder& binder) = 0;
    virtual void            on_begin_line(const Context& context) = 0;
    virtual void            on_end_line() = 0;
    virtual void            on_matches_changed(const Context& context) = 0;
    virtual void            on_input(const Input& Input, Result& result, const Context& context) = 0;
    virtual void            on_terminal_resize(int columns, int rows, const Context& context) = 0;
};
