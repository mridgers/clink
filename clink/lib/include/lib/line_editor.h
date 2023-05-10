// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class EditorModule;
class LineBuffer;
class MatchGenerator;
class TerminalIn;
class TerminalOut;

//------------------------------------------------------------------------------
class LineEditor
{
public:
    struct Desc
    {
        // Required.
        TerminalIn*     input = nullptr;
        TerminalOut*    output = nullptr;

        // Optional.
        const char*     shell_name = "clink";
        const char*     prompt = "clink $ ";
        const char*     command_delims = nullptr;
        const char*     quote_pair = "\"";
        const char*     word_delims = " \t";
        const char*     auto_quote_chars = " ";
    };

    virtual             ~LineEditor() = default;
    virtual bool        add_module(EditorModule& module) = 0;
    virtual bool        add_generator(MatchGenerator& generator) = 0;
    virtual bool        get_line(char* out, int32 out_size) = 0;
    virtual bool        edit(char* out, int32 out_size) = 0;
    virtual bool        update() = 0;
};



//------------------------------------------------------------------------------
LineEditor*             line_editor_create(const LineEditor::Desc& desc);
void                    line_editor_destroy(LineEditor* editor);
EditorModule*           tab_completer_create();
void                    tab_completer_destroy(EditorModule* completer);
