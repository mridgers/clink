// Copyright (c) 2020 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "line_editor_tester.h"
#include "editor_module.h"
#include "line_state.h"

#include <core/array.h>

//------------------------------------------------------------------------------
struct DelimModule
    : public EditorModule
{
    virtual void    bind_input(Binder& binder) override {}
    virtual void    on_begin_line(const Context& context) override {}
    virtual void    on_end_line() override {}
    virtual void    on_matches_changed(const Context& context) override;
    virtual void    on_input(const Input& Input, Result& result, const Context& context) override {}
    virtual void    on_terminal_resize(int32 columns, int32 rows, const Context& context) override {}
    uint8           delim = 'a';
};

//------------------------------------------------------------------------------
void DelimModule::on_matches_changed(const Context& context)
{
    const LineState& line = context.line;

    uint32 word_count = line.get_word_count();
    REQUIRE(word_count > 0);

    const Word* word = line.get_words().back();
    delim = word->delim;
}



//------------------------------------------------------------------------------
TEST_CASE("editor")
{
    LineEditor::Desc desc;
    desc.word_delims = " =";
    LineEditorTester tester(desc);

    DelimModule module;
    LineEditor* editor = tester.get_editor();
    editor->add_module(module);

    SECTION("first")
    {
        SECTION("0") { tester.set_input(""); }
        SECTION("1") { tester.set_input("one"); }
        SECTION("2") { tester.set_input("one t\b\b"); }

        tester.run(true);
        REQUIRE(module.delim == '\0');
    }

    SECTION("space")
    {
        SECTION("0") { tester.set_input("one "); }
        SECTION("1") { tester.set_input("one t\b"); }
        SECTION("2") { tester.set_input("one two"); }
        SECTION("3") { tester.set_input("one two t\b\b"); }
        SECTION("4") { tester.set_input("one= "); }
        SECTION("5") { tester.set_input("one= two"); }
        SECTION("6") { tester.set_input("one= two t\b\b"); }
        SECTION("7") { tester.set_input("one= = two"); }

        tester.run(true);
        REQUIRE(module.delim == ' ');
    }

    SECTION("equals")
    {
        SECTION("0") { tester.set_input("one two="); }
        SECTION("1") { tester.set_input("one two ="); }
        SECTION("2") { tester.set_input("one two=t\b"); }
        SECTION("3") { tester.set_input("one two =t\b"); }
        SECTION("4") { tester.set_input("one two=three"); }
        SECTION("5") { tester.set_input("one two=three f\b\b"); }

        tester.run(true);
        REQUIRE(module.delim == '=');
    }
}
