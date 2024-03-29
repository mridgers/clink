// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "bind_resolver.h"
#include "binder.h"
#include "editor_module.h"

//------------------------------------------------------------------------------
TEST_CASE("binder")
{
    Binder binder;

    SECTION("Group")
    {
        REQUIRE(binder.create_group("") == -1);
        REQUIRE(binder.create_group(nullptr) == -1);

        REQUIRE(binder.get_group() == 1);

        int32 groups[] = {
            binder.create_group("group1"),
            binder.create_group("group2"),
        };

        REQUIRE(groups[0] != -1);
        REQUIRE(groups[1] != -1);
        REQUIRE(binder.get_group("group0") == -1);
        REQUIRE(binder.get_group("group1") == groups[0]);
        REQUIRE(binder.get_group("group2") == groups[1]);
    }

    SECTION("Overflow : group")
    {
        for (int32 i = 1; i < 256; ++i)
            REQUIRE(binder.create_group("group") == (i * 2) + 1);

        REQUIRE(binder.create_group("group") == -1);
    }

    SECTION("Overflow : module")
    {
        int32 group = binder.get_group();
        for (int32 i = 0; i < 64; ++i)
            REQUIRE(binder.bind(group, "", ((EditorModule*)0)[i], char(i)));

        auto& module = ((EditorModule*)0)[0xff];
        REQUIRE(!binder.bind(group, "", module, 0xff));
    }

    SECTION("Overflow : bind")
    {
        auto& null_module = *(EditorModule*)0;
        int32 default_group = binder.get_group();

        for (int32 i = 0; i < 508; ++i)
        {
            char chord[] = { char((i > 0xff) + 1), char((i % 0xfe) + 1), 0 };
            REQUIRE(binder.bind(default_group, chord, null_module, 0x12));
        }

        REQUIRE(!binder.bind(default_group, "\x01\x02\x03", null_module, 0x12));
    }

    SECTION("Valid chords")
    {
        struct {
            const char* bind;
            const char* input;
        } chords[] = {
            "a",        "a",
            "^",        "^",
            "\\",       "\\",
            "\\t",      "\t",
            "\\e",      "\x1b",
            "abc",      "abc",
            "ab",       "abd",
            "^a",       "\x01",
            "\\C-b",    "\x02",
            "\\C-C",    "\x03",
            "\\M-c",    "\x1b""c",
            "\\M-C",    "\x1b""C",
            "\\M-C-d",  "\x1b\x04",
            "",         "z",
        };

        int32 group = binder.get_group();
        for (const auto& chord : chords)
        {
            auto& module = *(EditorModule*)(&chord);
            REQUIRE(binder.bind(group, chord.bind, module, 123));

            BindResolver resolver(binder);
            for (const char* c = chord.input; *c; ++c)
                if (resolver.step(*c))
                    break;

            auto Binding = resolver.next();
            REQUIRE(Binding);
            REQUIRE(Binding.get_id() == 123);
            REQUIRE(Binding.get_module() == &module);
        }
    }

    SECTION("Invalid chords")
    {
        const char* chords[] = {
            "\\C",   "\\Cx",   "\\C-",
            "\\M",   "\\Mx",   "\\M-",
                               "\\M-C-",
        };

        int32 group = binder.get_group();
        for (const char* chord : chords)
        {
            REQUIRE(!binder.bind(group, chord, *(EditorModule*)0, 234));
        }
    }
}
