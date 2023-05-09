// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/str.h>
#include <core/str_tokeniser.h>

//------------------------------------------------------------------------------
TEST_CASE("StrTokeniser : basic")
{
    StrTokeniser t("a;b;c", ";");

    Str<> s;
    REQUIRE(t.next(s)); REQUIRE(s.equals("a") == true);
    REQUIRE(t.next(s)); REQUIRE(s.equals("b") == true);
    REQUIRE(t.next(s)); REQUIRE(s.equals("c") == true);
    REQUIRE(!t.next(s));
}

//------------------------------------------------------------------------------
TEST_CASE("StrTokeniser : empty")
{
    {
        Str<> s;
        StrTokeniser t;
        REQUIRE(!t.next(s));
    }{
        Str<> s;
        StrIter i;
        StrTokeniser t(i);
        REQUIRE(!t.next(s));
    }{
        Wstr<> s;
        WstrTokeniser t;
        REQUIRE(!t.next(s));
    }{
        Wstr<> s;
        WstrIter i;
        WstrTokeniser t(i);
        REQUIRE(!t.next(s));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("StrTokeniser : multi delims")
{
    StrTokeniser t("a;b-c.d", ";-.");

    Str<> s;
    REQUIRE(t.next(s)); REQUIRE(s.equals("a") == true);
    REQUIRE(t.next(s)); REQUIRE(s.equals("b") == true);
    REQUIRE(t.next(s)); REQUIRE(s.equals("c") == true);
    REQUIRE(t.next(s)); REQUIRE(s.equals("d") == true);
    REQUIRE(!t.next(s));
}

//------------------------------------------------------------------------------
TEST_CASE("StrTokeniser : ends")
{
    const char* inputs[] = { "a;b;c", ";a;b;c", "a;b;c;" };
    for (auto Input : inputs)
    {
        StrTokeniser t(Input, ";");

        Str<> s;
        REQUIRE(t.next(s)); REQUIRE(s.equals("a") == true);
        REQUIRE(t.next(s)); REQUIRE(s.equals("b") == true);
        REQUIRE(t.next(s)); REQUIRE(s.equals("c") == true);
        REQUIRE(!t.next(s));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("StrTokeniser : delim runs")
{
    const char* inputs[] = { "a;;b--c", "-;a;-b;c", "a;b;-c-;" };
    for (auto Input : inputs)
    {
        StrTokeniser t(Input, ";-");

        Str<> s;
        REQUIRE(t.next(s)); REQUIRE(s.equals("a") == true);
        REQUIRE(t.next(s)); REQUIRE(s.equals("b") == true);
        REQUIRE(t.next(s)); REQUIRE(s.equals("c") == true);
        REQUIRE(!t.next(s));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("StrTokeniser : quote")
{
    StrTokeniser t("'-abc';(-abc);'-a)b;c", ";-");
    t.add_quote_pair("'");
    t.add_quote_pair("()");

    Str<> s;
    REQUIRE(t.next(s)); REQUIRE(s.equals("'-abc'") == true);
    REQUIRE(t.next(s)); REQUIRE(s.equals("(-abc)") == true);
    REQUIRE(t.next(s)); REQUIRE(s.equals("'-a)b;c") == true);
    REQUIRE(!t.next(s));
}

//------------------------------------------------------------------------------
TEST_CASE("StrTokeniser : delim return")
{
    StrTokeniser t("a;b;-c-;d", ";-");

    Str<> s;
    REQUIRE(t.next(s).delim == 0);   REQUIRE(s.equals("a") == true);
    REQUIRE(t.next(s).delim == ';'); REQUIRE(s.equals("b") == true);
    REQUIRE(t.next(s).delim == '-'); REQUIRE(s.equals("c") == true);
    REQUIRE(t.next(s).delim == ';'); REQUIRE(s.equals("d") == true);
    REQUIRE(!t.next(s));
}
