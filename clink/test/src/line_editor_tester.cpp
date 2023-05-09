// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "line_editor_tester.h"

#include <core/base.h>
#include <lib/editor_module.h>
#include <lib/matches.h>

#include <stdio.h>

//------------------------------------------------------------------------------
class EmptyModule
    : public EditorModule
{
public:
    virtual void    bind_input(Binder& binder) override {}
    virtual void    on_begin_line(const Context& context) override {}
    virtual void    on_end_line() override {}
    virtual void    on_matches_changed(const Context& context) override {}
    virtual void    on_input(const Input& input, Result& result, const Context& context) override {}
    virtual void    on_terminal_resize(int columns, int rows, const Context& context) override {}
};



//------------------------------------------------------------------------------
class TestModule
    : public EmptyModule
{
public:
    const Matches*  get_matches() const;

private:
    virtual void    bind_input(Binder& binder) override;
    virtual void    on_matches_changed(const Context& context) override;
    virtual void    on_input(const Input& input, Result& result, const Context& context) override;
    const Matches*  m_matches = nullptr;
};

//------------------------------------------------------------------------------
const Matches* TestModule::get_matches() const
{
    return m_matches;
}

//------------------------------------------------------------------------------
void TestModule::bind_input(Binder& binder)
{
    int default_group = binder.get_group();
    binder.bind(default_group, DO_COMPLETE, 0);
}

//------------------------------------------------------------------------------
void TestModule::on_matches_changed(const Context& context)
{
    m_matches = &(context.matches);
}

//------------------------------------------------------------------------------
void TestModule::on_input(const Input&, Result& result, const Context& context)
{
    if (context.matches.get_match_count() == 1)
        result.accept_match(0);
    else
        result.append_match_lcd();
}



//------------------------------------------------------------------------------
LineEditorTester::LineEditorTester()
{
    create_line_editor();
}

//------------------------------------------------------------------------------
LineEditorTester::LineEditorTester(const LineEditor::Desc& desc)
{
    create_line_editor(&desc);
}

//------------------------------------------------------------------------------
void LineEditorTester::create_line_editor(const LineEditor::Desc* desc)
{
    // Create a line editor.
    LineEditor::Desc inner_desc;
    if (desc != nullptr)
        inner_desc = *desc;
    inner_desc.input = &m_terminal_in;
    inner_desc.output = &m_terminal_out;

    m_editor = line_editor_create(inner_desc);
    REQUIRE(m_editor != nullptr);
}

//------------------------------------------------------------------------------
LineEditorTester::~LineEditorTester()
{
    line_editor_destroy(m_editor);
}

//------------------------------------------------------------------------------
LineEditor* LineEditorTester::get_editor() const
{
    return m_editor;
}

//------------------------------------------------------------------------------
void LineEditorTester::set_input(const char* input)
{
    m_input = input;
}

//------------------------------------------------------------------------------
void LineEditorTester::set_expected_output(const char* expected)
{
    m_expected_output = expected;
}

//------------------------------------------------------------------------------
void LineEditorTester::run(bool expectationless)
{
    bool has_expectations = expectationless;
    has_expectations |= m_has_matches || (m_expected_output != nullptr);
    REQUIRE(has_expectations);

    REQUIRE(m_input != nullptr);
    m_terminal_in.set_input(m_input);

    // If we're expecting some matches then add a module to catch the
    // matches object.
    TestModule match_catch;
    m_editor->add_module(match_catch);

    // First update doesn't read input. We do however want to read at least one
    // character before bailing on the loop.
    REQUIRE(m_editor->update());
    do
    {
        REQUIRE(m_editor->update());
    }
    while (m_terminal_in.has_input());

    if (m_has_matches)
    {
        const Matches* matches = match_catch.get_matches();
        REQUIRE(matches != nullptr);

        unsigned int match_count = matches->get_match_count();
        REQUIRE(m_expected_matches.size() == match_count, [&] () {
            printf(" input; %s#\n", m_input);

            char line[256];
            m_editor->get_line(line, sizeof_array(line));
            printf("output; %s#\n", line);

            puts("\nexpected;");
            for (const char* match : m_expected_matches)
                printf("  %s\n", match);

            puts("\ngot;");
            for (int i = 0, n = matches->get_match_count(); i < n; ++i)
                printf("  %s\n", matches->get_match(i));
        });

        for (const char* expected : m_expected_matches)
        {
            bool match_found = false;

            for (unsigned int i = 0; i < match_count; ++i)
                if (match_found = (strcmp(expected, matches->get_match(i)) == 0))
                    break;

            REQUIRE(match_found, [&] () {
                printf("match '%s' not found\n", expected);
            });
        }
    }

    // Check the output is as expected.
    if (m_expected_output != nullptr)
    {
        char line[256];
        REQUIRE(m_editor->get_line(line, sizeof_array(line)));
        REQUIRE(strcmp(m_expected_output, line) == 0, [&] () {
            printf("       input; %s#\n", m_input);
            printf("out expected; %s#\n", m_expected_output);
            printf("     out got; %s#\n", line);
        });
    }

    m_input = nullptr;
    m_expected_output = nullptr;
    m_expected_matches.clear();

    char t;
    m_editor->get_line(&t, 1);
}

//------------------------------------------------------------------------------
void LineEditorTester::expected_matches_impl(int dummy, ...)
{
    m_expected_matches.clear();

    va_list arg;
    va_start(arg, dummy);

    while (const char* match = va_arg(arg, const char*))
        m_expected_matches.push_back(match);

    va_end(arg);
    m_has_matches = true;
}
