// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "bind_resolver.h"
#include "binder.h"
#include "editor_module.h"
#include "line_editor.h"
#include "line_state.h"
#include "matches_impl.h"
#include "rl/rl_module.h"
#include "rl/rl_buffer.h"

#include <core/array.h>
#include <terminal/printer.h>

//------------------------------------------------------------------------------
class LineEditorImpl
    : public LineEditor
{
public:
                        LineEditorImpl(const Desc& desc);
    virtual bool        add_module(EditorModule& module) override;
    virtual bool        add_generator(MatchGenerator& generator) override;
    virtual bool        get_line(char* out, int out_size) override;
    virtual bool        edit(char* out, int out_size) override;
    virtual bool        update() override;

private:
    typedef EditorModule                    Module;
    typedef FixedArray<EditorModule*, 16>   Modules;
    typedef FixedArray<MatchGenerator*, 32> Generators;
    typedef FixedArray<Word, 72>            Words;

    enum Flags : unsigned char
    {
        flag_init       = 1 << 0,
        flag_editing    = 1 << 1,
        flag_done       = 1 << 2,
        flag_eof        = 1 << 3,
    };

    void                initialise();
    void                begin_line();
    void                end_line();
    void                find_command_bounds(const char*& start, int& length);
    void                collect_words();
    void                update_internal();
    void                update_input();
    void                accept_match(unsigned int index);
    void                append_match_lcd();
    Module::Context     get_context(const LineState& line) const;
    LineState           get_linestate() const;
    void                set_flag(unsigned char flag);
    void                clear_flag(unsigned char flag);
    bool                check_flag(unsigned char flag) const;
    RlModule            _module;
    RlBuffer            _buffer;
    Desc                _desc;
    Modules             _modules;
    Generators          _generators;
    Binder              _binder;
    BindResolver        _bind_resolver = { _binder };
    Words               _words;
    MatchesImpl         _matches;
    Printer             _printer;
    unsigned int        _prev_key;
    unsigned short      _command_offset;
    unsigned char       _keys_size;
    unsigned char       _flags = 0;
};
