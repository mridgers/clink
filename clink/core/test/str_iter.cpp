// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/str_iter.h>

#include <new>

//------------------------------------------------------------------------------
TEST_CASE("String iterator (StrIter)")
{
    SECTION("Basic")
    {
        StrIter iter("123");
        REQUIRE(iter.length() == 3);
        REQUIRE(iter.next() == '1');
        REQUIRE(iter.next() == '2');
        REQUIRE(iter.next() == '3');
        REQUIRE(iter.next() == 0);
    }

    SECTION("Subset")
    {
        StrIter iter("123", 0);
        REQUIRE(iter.length() == 0);
        REQUIRE(iter.next() == 0);

        new (&iter) StrIter("123", 1);
        REQUIRE(iter.length() == 1);
        REQUIRE(iter.next() == '1');
        REQUIRE(iter.next() == 0);

        new (&iter) StrIter("123", 2);
        REQUIRE(iter.length() == 2);
        REQUIRE(iter.next() == '1');
        REQUIRE(iter.next() == '2');
        REQUIRE(iter.next() == 0);
    }

    SECTION("UTF-8")
    {
        StrIter iter("\xc2\x9b\xc2\x9b\xc2\x9b");
        REQUIRE(iter.next() == 0x9b);
        REQUIRE(iter.next() == 0x9b);
        REQUIRE(iter.next() == 0x9b);
        REQUIRE(iter.next() == 0);
    }

    SECTION("Partial UTF-8")
    {
        StrIter iter("\xc2\x9b\xe0\xa0");
        REQUIRE(iter.next() == 0x9b);
        REQUIRE(iter.next() == 0);

        new (&iter) StrIter("\xc2\x9b", 1);
        REQUIRE(iter.next() == 0);
    }
}

//------------------------------------------------------------------------------
TEST_CASE("String iterator (WstrIter)")
{
    SECTION("Basic")
    {
        WstrIter iter(L"123");
        REQUIRE(iter.next() == '1');
        REQUIRE(iter.next() == '2');
        REQUIRE(iter.next() == '3');
        REQUIRE(iter.next() == 0);
    }

    SECTION("Subset")
    {
        WstrIter iter(L"123", 2);
        REQUIRE(iter.next() == '1');
        REQUIRE(iter.next() == '2');
        REQUIRE(iter.next() == 0);
    }

    SECTION("UTF-16")
    {
        WstrIter iter(L"\x0001\xd800\xdc00");
        REQUIRE(iter.next() == 1);
        REQUIRE(iter.next() == 0x10000);
        REQUIRE(iter.next() == 0);

        new (&iter) WstrIter(L"\xdbff\xdfff");
        REQUIRE(iter.next() == 0x10ffff);
        REQUIRE(iter.next() == 0);
    }

    SECTION("Partial UTF-16")
    {
        WstrIter iter(L"\x0001\xd800");
        REQUIRE(iter.next() == 1);
        REQUIRE(iter.next() == 0);

        new (&iter) WstrIter(L"\xd9ff");
        REQUIRE(iter.next() == 0);

        new (&iter) WstrIter(L"\xdfff");
        REQUIRE(iter.next() == 0xdfff);
        REQUIRE(iter.next() == 0);
    }

    SECTION("OOB")
    {
        StrIter iter("", 10);
        REQUIRE(iter.next() == 0);

        WstrIter witer(L"", 10);
        REQUIRE(witer.next() == 0);
    }

    SECTION("Length")
    {
        {
            StrIter iter("");          REQUIRE(iter.length() == 0);
            WstrIter witer(L"");       REQUIRE(iter.length() == 0);
        }
        {
            StrIter iter("", 10);      REQUIRE(iter.length() == 10);
            WstrIter witer(L"", 10);   REQUIRE(iter.length() == 10);
        }
        {
            StrIter iter("abc");       REQUIRE(iter.length() == 3);
            WstrIter witer(L"abc");    REQUIRE(iter.length() == 3);
        }
        {
            StrIter iter("abc", 2);    REQUIRE(iter.length() == 2);
            WstrIter witer(L"abc", 2); REQUIRE(iter.length() == 2);
        }
    }
}

//------------------------------------------------------------------------------
TEST_CASE("String iterator (null StrIter)")
{
    StrIter null;
    REQUIRE(null.more() == false);
    REQUIRE(null.next() == 0);
    REQUIRE(null.length() == 0);
    REQUIRE(null.get_pointer() != nullptr);
}
