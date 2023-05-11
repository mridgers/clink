// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/settings.h>
#include <core/str.h>
#include <host/doskey.h>

//------------------------------------------------------------------------------
static void use_enhanced(bool state)
{
    settings::find("doskey.enhanced")->set(state ? "true" : "false");
}



//------------------------------------------------------------------------------
TEST_CASE("doskey add/remove")
{
    for (int32 i = 0; i < 2; ++i)
    {
        use_enhanced(i != 0);

        Doskey doskey("shell");
        REQUIRE(doskey.add_alias("alias", "text") == true);
        REQUIRE(doskey.add_alias("alias", "") == true);
        REQUIRE(doskey.remove_alias("alias") == true);
    }
}

//------------------------------------------------------------------------------
TEST_CASE("doskey expand : simple")
{
    for (int32 i = 0; i < 2; ++i)
    {
        use_enhanced(i != 0);

        Doskey doskey("shell");
        doskey.add_alias("alias", "text");

        Wstr<> line(L"alias");

        DoskeyAlias alias;
        doskey.resolve(line.data(), alias);
        REQUIRE(alias);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L"text") == true);
        REQUIRE(alias.next(line) == false);

        doskey.remove_alias("alias");
    }
}

//------------------------------------------------------------------------------
TEST_CASE("doskey expand : leading")
{
    for (int32 i = 0; i < 2; ++i)
    {
        use_enhanced(i != 0);

        Doskey doskey("shell");
        doskey.add_alias("alias", "text");

        Wstr<> line(L" alias");

        DoskeyAlias alias;
        doskey.resolve(line.c_str(), alias);
        REQUIRE(bool(alias) == (i == 1));

        doskey.add_alias("\"alias", "text\"");

        REQUIRE(doskey.remove_alias("alias") == true);
        REQUIRE(doskey.remove_alias("\"alias") == true);
    }
}

//------------------------------------------------------------------------------
TEST_CASE("doskey args $1-9")
{
    for (int32 i = 0; i < 2; ++i)
    {
        use_enhanced(i != 0);

        Doskey doskey("shell");
        doskey.add_alias("alias", " $1$2 $3$5$6$7$8$9 "); // no $4 deliberately

        Wstr<> line(L"alias a b c d e f g h i j k l");

        DoskeyAlias alias;
        doskey.resolve(line.c_str(), alias);
        REQUIRE(alias);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L" ab cefghi ") == true);
        REQUIRE(alias.next(line) == false);

        line = L"alias a b c d e";

        doskey.resolve(line.c_str(), alias);
        REQUIRE(alias);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L" ab ce ") == true);
        REQUIRE(alias.next(line) == false);

        doskey.remove_alias("alias");
    }
}

//------------------------------------------------------------------------------
TEST_CASE("doskey args $*")
{
    for (int32 i = 0; i < 2; ++i)
    {
        use_enhanced(i != 0);

        Doskey doskey("shell");
        doskey.add_alias("alias", " $* ");

        Wstr<> line(L"alias a b c d e f g h i j k l m n o p");

        DoskeyAlias alias;
        doskey.resolve(line.c_str(), alias);
        REQUIRE(alias);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L" a b c d e f g h i j k l m n o p ") == true);
        REQUIRE(alias.next(line) == false);

        doskey.remove_alias("alias");
    }

    {
        use_enhanced(false);

        Doskey doskey("shell");
        doskey.add_alias("alias", "$*");

        Wstr<> line;
        line << L"alias ";
        for (int32 i = 0; i < 12; ++i)
            line << L"0123456789abcdef";

        DoskeyAlias alias;
        doskey.resolve(line.c_str(), alias);
    }
}

//------------------------------------------------------------------------------
TEST_CASE("doskey $? chars")
{
    for (int32 i = 0; i < 2; ++i)
    {
        use_enhanced(i != 0);

        Doskey doskey("shell");
        doskey.add_alias("alias", "$$ $g$G $l$L $b$B $Z");

        Wstr<> line(L"alias");

        DoskeyAlias alias;
        doskey.resolve(line.c_str(), alias);
        REQUIRE(alias);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L"$ >> << || $Z") == true);
        REQUIRE(alias.next(line) == false);

        doskey.remove_alias("alias");
    }
}

//------------------------------------------------------------------------------
TEST_CASE("doskey multi-command")
{
    for (int32 i = 0; i < 2; ++i)
    {
        use_enhanced(i != 0);

        Doskey doskey("shell");
        doskey.add_alias("alias", "one $3 $t $2 two_$T$*three");

        Wstr<> line(L"alias a b c");

        DoskeyAlias alias;
        doskey.resolve(line.c_str(), alias);
        REQUIRE(alias);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L"one c ") == true);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L" b two_") == true);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(L"a b cthree") == true);

        REQUIRE(alias.next(line) == false);

        doskey.remove_alias("alias");
    }
}

//------------------------------------------------------------------------------
TEST_CASE("doskey pipe/redirect")
{
    use_enhanced(false);

    Doskey doskey("shell");
    doskey.add_alias("alias", "one $*");

    Wstr<> line(L"alias|piped");
    DoskeyAlias alias;
    doskey.resolve(line.c_str(), alias);
    REQUIRE(!alias);

    line = L"alias |piped";
    doskey.resolve(line.c_str(), alias);
    REQUIRE(alias);
    REQUIRE(alias.next(line) == true);
    REQUIRE(line.equals(L"one |piped") == true);

    doskey.remove_alias("alias");
}

//------------------------------------------------------------------------------
TEST_CASE("doskey pipe/redirect : new")
{
    use_enhanced(true);

    Doskey doskey("shell");

    auto Test = [&] (const wchar_t* Input, const wchar_t* output) {
        Wstr<> line(Input);

        DoskeyAlias alias;
        doskey.resolve(line.c_str(), alias);
        REQUIRE(alias);

        REQUIRE(alias.next(line) == true);
        REQUIRE(line.equals(output) == true);
        REQUIRE(alias.next(line) == false);
    };

    doskey.add_alias("alias", "one");
    SECTION("Basic 1")
    { Test(L"alias|piped", L"one|piped"); }
    SECTION("Basic 2")
    { Test(L"alias|alias", L"one|one"); }
    SECTION("Basic 3")
    { Test(L"alias|alias&alias", L"one|one&one"); }
    SECTION("Basic 4")
    { Test(L"&|alias", L"&|one"); }
    SECTION("Basic 5")
    { Test(L"alias||", L"one||"); }
    SECTION("Basic 6")
    { Test(L"&&alias&|", L"&&one&|"); }
    SECTION("Basic 5")
    { Test(L"alias|x|alias", L"one|x|one"); }
    doskey.remove_alias("alias");

    #define ARGS L"two \"three four\" 5"
    doskey.add_alias("alias", "cmd $1 $2 $3");
    SECTION("Args 1")
    { Test(L"alias " ARGS L"|piped", L"cmd " ARGS L"|piped"); }
    SECTION("Args 2")
    { Test(L"alias " ARGS L"|alias " ARGS, L"cmd " ARGS L"|cmd " ARGS); }
    doskey.remove_alias("alias");
    #undef ARGS
}
