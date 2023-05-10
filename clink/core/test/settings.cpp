// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/base.h>
#include <core/settings.h>

//------------------------------------------------------------------------------
TEST_CASE("settings : basic")
{
    auto* first = settings::first();
    {
        SettingBool test("!one", "", "", false);
        REQUIRE(settings::first() == &test);
    }
    REQUIRE(settings::first() == first);
}

//------------------------------------------------------------------------------
TEST_CASE("settings : list")
{
    auto* first = settings::first();

    auto count_settings = [] () {
        int32 count = 0;
        for (auto* i = settings::first(); i != nullptr; i = i->next())
            ++count;

        return count;
    };

    int32 initial_count = count_settings();

    // Create a few settings.
    const char* names[] = {
        "sed", "do", "eiusmod", "tempor", "incididunt", "ut", "labore", "Et",
        "Dolore", "Magna", "Aliqua", "Ut", "Enim", "Ad", "minim", "veniam",
        "quis", "nostrud", "exercitation", "ullamco", "laboris"
    };

    for (int32 i = 0, n = sizeof_array(names); i < n; ++i)
        new SettingInt(names[i], "", nullptr, i);

    // Check they're in the order we expect.
    Setting* iter = settings::first();
    const char* last_name = iter->get_name();
    iter = iter->next();
    for (; iter != nullptr; last_name = iter->get_name(), iter = iter->next())
    {
        REQUIRE(stricmp(last_name, iter->get_name()) <= 0, [&] () {
            printf("%s < %s\n", last_name, iter->get_name());
        });
    }

    REQUIRE(count_settings() - initial_count == sizeof_array(names));

    // Delete this tests' settings
    for (int32 i = 0, n = sizeof_array(names); i < n; ++i)
    {
        auto* s = settings::find(names[i]);
        REQUIRE(s != nullptr);
        delete s;
    }

    REQUIRE(count_settings() == initial_count);
    REQUIRE(settings::first() == first);
}

//------------------------------------------------------------------------------
TEST_CASE("settings : bool")
{
    SettingBool test("one", "", "", true);

    REQUIRE(test.set("0"));     REQUIRE(!test.get());
    REQUIRE(test.set("000"));   REQUIRE(!test.get());
    REQUIRE(test.set("false")); REQUIRE(!test.get());
    REQUIRE(test.set("False")); REQUIRE(!test.get());
    REQUIRE(test.set("FaLsE")); REQUIRE(!test.get());

    REQUIRE(test.set("1"));    REQUIRE(test.get());
    REQUIRE(test.set("101"));  REQUIRE(test.get());
    REQUIRE(test.set("true")); REQUIRE(test.get());
    REQUIRE(test.set("True")); REQUIRE(test.get());
    REQUIRE(test.set("TrUe")); REQUIRE(test.get());

    Str<> out;
    test.get(out);
    REQUIRE(out.equals("True"));

    REQUIRE(!test.set("abc")); REQUIRE(test.get());
    REQUIRE(test.set("0abc")); REQUIRE(!test.get());

    test.get(out);
    REQUIRE(out.equals("False"));
}

//------------------------------------------------------------------------------
TEST_CASE("settings : int32")
{
    SettingInt test("one", "", "", 1);

    REQUIRE(test.set("100")); REQUIRE(test.get() == 100);
    REQUIRE(test.set("101")); REQUIRE(test.get() == 101);
    REQUIRE(test.set("102")); REQUIRE(test.get() == 102);

    Str<> out;
    test.get(out);
    REQUIRE(out.equals("102"));

    REQUIRE(test.set("-2"));  REQUIRE(test.get() == -2);
    REQUIRE(test.set("-03")); REQUIRE(test.get() == -3);
    REQUIRE(test.set("-14")); REQUIRE(test.get() == -14);

    REQUIRE(test.set("999"));
    REQUIRE(!test.set("abc")); REQUIRE(test.get() == 999);
    REQUIRE(test.set("0abc")); REQUIRE(test.get() == 0);
}

//------------------------------------------------------------------------------
TEST_CASE("settings : str")
{
    SettingStr test("one", "", "", "abc");

    REQUIRE(strcmp(test.get(), "abc") == 0);
    REQUIRE(test.set("Abc")); REQUIRE(strcmp(test.get(), "Abc") == 0);
    REQUIRE(test.set("ABc")); REQUIRE(strcmp(test.get(), "ABc") == 0);
    REQUIRE(test.set("ABC")); REQUIRE(strcmp(test.get(), "ABC") == 0);
}

//------------------------------------------------------------------------------
TEST_CASE("settings : enum")
{
    SettingEnum test("one", "", "", "zero,one,two", 1);

    REQUIRE(test.get() == 1);

    Str<> out;
    const char* options[] = { "zero", "one", "two" };
    for (int32 i = 0; i < sizeof_array(options); ++i)
    {
        REQUIRE(test.set(options[i]));
        REQUIRE(test.get() == i);

        test.get(out);
        REQUIRE(out.equals(options[i]));
    }

    REQUIRE(!test.set("abc"));  REQUIRE(test.get() == 2);
    REQUIRE(!test.set("0abc")); REQUIRE(test.get() == 2);
}
