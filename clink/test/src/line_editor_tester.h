// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <lib/line_editor.h>
#include <lib/line_buffer.h>
#include <terminal/terminal_in.h>
#include <terminal/terminal_out.h>

#include <vector>

//------------------------------------------------------------------------------
#define DO_COMPLETE "\x01"

//------------------------------------------------------------------------------
class TestTerminalIn
    : public TerminalIn
{
public:
    bool                    has_input() const { return (_read == nullptr) ? false : (*_read != '\0'); }
    void                    set_input(const char* input) { _input = _read = input; }
    virtual void            begin() override {}
    virtual void            end() override {}
    virtual void            select() override {}
    virtual int             read() override { return *(unsigned char*)_read++; }

private:
    const char*             _input = nullptr;
    const char*             _read = nullptr;
};

//------------------------------------------------------------------------------
class TestTerminalOut
    : public TerminalOut
{
public:
    virtual void            begin() override {}
    virtual void            end() override {}
    virtual void            write(const char* chars, int length) override {}
    virtual void            flush() override {}
    virtual int             get_columns() const override { return 80; }
    virtual int             get_rows() const override { return 25; }
    virtual void            set_attributes(const Attributes attr) {}
};



//------------------------------------------------------------------------------
class LineEditorTester
{
public:
                                LineEditorTester();
                                LineEditorTester(const LineEditor::Desc& desc);
                                ~LineEditorTester();
    LineEditor*                 get_editor() const;
    void                        set_input(const char* input);
    template <class ...T> void  set_expected_matches(T... t); // T must be const char*
    void                        set_expected_output(const char* expected);
    void                        run(bool expectationless=false);

private:
    void                        create_line_editor(const LineEditor::Desc* desc=nullptr);
    void                        expected_matches_impl(int dummy, ...);
    TestTerminalIn              _terminal_in;
    TestTerminalOut             _terminal_out;
    std::vector<const char*>    _expected_matches;
    const char*                 _input = nullptr;
    const char*                 _expected_output = nullptr;
    LineEditor*                 _editor = nullptr;
    bool                        _has_matches = false;
};

//------------------------------------------------------------------------------
template <class ...T>
void LineEditorTester::set_expected_matches(T... t)
{
    expected_matches_impl(0, t..., nullptr);
}
