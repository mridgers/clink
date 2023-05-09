// Copyright (c) 2012 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "fs_fixture.h"
#include "line_editor_tester.h"

#include <core/str_compare.h>
#include <lib/match_generator.h>

//------------------------------------------------------------------------------
TEST_CASE("Quoting")
{
    static const char* space_fs[] = {
        "pre_nospace",
        "pre_space 1",
        "pre_space_space 2",
        "single space",
        "dir/space 1",
        "dir/space 2",
        "dir/space_3",
        "dir/single space",
        nullptr,
    };

    FsFixture fs(space_fs);

    SECTION("Double quotes")
    {
        LineEditorTester tester;

        LineEditor* editor = tester.get_editor();
        editor->add_generator(file_match_generator());

        SECTION("None")
        {
            tester.set_input("pr" DO_COMPLETE);
            tester.set_expected_output("pre_");
            tester.run();
        }

        SECTION("Close exisiting")
        {
            tester.set_input("\"singl" DO_COMPLETE);
            tester.set_expected_output("\"single space\" ");
            tester.run();
        }

        SECTION("End-of-word")
        {
            tester.set_input("\"single space\"" DO_COMPLETE);
            tester.set_expected_output("\"single space\" ");
            tester.run();
        }

        SECTION("Surround")
        {
            tester.set_input("sing" DO_COMPLETE);
            tester.set_expected_output("\"single space\" ");
            tester.run();
        }

        SECTION("Prefix")
        {
            tester.set_input("pre_s" DO_COMPLETE);
            tester.set_expected_output("\"pre_space");
            tester.run();
        }

        SECTION("Prefix (case mapped)")
        {
            StrCompareScope _(StrCompareScope::relaxed);
            tester.set_input("pre-s" DO_COMPLETE);
            tester.set_expected_output("\"pre_space");
            tester.run();
        }

        SECTION("Dir (close exisiting)")
        {
            tester.set_input("\"dir/sing" DO_COMPLETE);
            tester.set_expected_output("\"dir\\single space\" ");
            tester.run();
        }

        SECTION("Dir (surround)")
        {
            tester.set_input("dir/sing" DO_COMPLETE);
            tester.set_expected_output("\"dir\\single space\" ");
            tester.run();
        }

        SECTION("Dir (prefix)")
        {
            tester.set_input("dir\\spac" DO_COMPLETE);
            tester.set_expected_output("\"dir\\space");
            tester.run();
        }
    }

    SECTION("Matched pair")
    {
        LineEditor::Desc desc;
        desc.quote_pair = "()";
        LineEditorTester tester(desc);

        LineEditor* editor = tester.get_editor();
        editor->add_generator(file_match_generator());

        SECTION("None")
        {
            tester.set_input("pr" DO_COMPLETE);
            tester.set_expected_output("pre_");
            tester.run();
        }

        SECTION("Close exisiting")
        {
            tester.set_input("(singl" DO_COMPLETE);
            tester.set_expected_output("(single space) ");
            tester.run();
        }

        SECTION("End-of-word")
        {
            tester.set_input("(single space)" DO_COMPLETE);
            tester.set_expected_output("(single space) ");
            tester.run();
        }

        SECTION("Surround")
        {
            tester.set_input("sing" DO_COMPLETE);
            tester.set_expected_output("(single space) ");
            tester.run();
        }

        SECTION("Prefix")
        {
            tester.set_input("pre_s" DO_COMPLETE);
            tester.set_expected_output("(pre_space");
            tester.run();
        }
    }

    SECTION("No quote pair")
    {
        LineEditor::Desc desc;
        desc.quote_pair = nullptr;
        LineEditorTester tester(desc);

        LineEditor* editor = tester.get_editor();
        editor->add_generator(file_match_generator());

        SECTION("None")
        {
            tester.set_input("pr" DO_COMPLETE);
            tester.set_expected_output("pre_");
            tester.run();
        }

        SECTION("Close exisiting")
        {
            tester.set_input("singl" DO_COMPLETE);
            tester.set_expected_output("single space ");
            tester.run();
        }

        SECTION("Surround")
        {
            tester.set_input("sing" DO_COMPLETE);
            tester.set_expected_output("single space ");
            tester.run();
        }

        SECTION("Prefix")
        {
            tester.set_input("pre_s" DO_COMPLETE);
            tester.set_expected_output("pre_space");
            tester.run();
        }
    }
}
