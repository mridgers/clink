// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/str.h>
#include <core/str_compare.h>

//------------------------------------------------------------------------------
TEST_CASE("String compare")
{
    SECTION("Exact")
    {
        StrCompareScope _(StrCompareScope::exact);

        REQUIRE(str_compare("abc123!@#", "abc123!@#") == -1);
        REQUIRE(str_compare("ABC123!@#", "abc123!@#") == 0);

        REQUIRE(str_compare("abc", "ab") == 2);
        REQUIRE(str_compare("ab", "abc") == 2);

        REQUIRE(str_compare("_", "_") == -1);
        REQUIRE(str_compare("-", "_") == 0);
    }

    SECTION("Case insensitive")
    {
        StrCompareScope _(StrCompareScope::caseless);

        REQUIRE(str_compare("abc123!@#", "abc123!@#") == -1);
        REQUIRE(str_compare("ABC123!@#", "abc123!@#") == -1);

        REQUIRE(str_compare("aBc", "ab") == 2);
        REQUIRE(str_compare("ab", "aBc") == 2);

        REQUIRE(str_compare("_", "_") == -1);
        REQUIRE(str_compare("-", "_") == 0);
    }

    SECTION("Relaxed")
    {
        StrCompareScope _(StrCompareScope::relaxed);

        REQUIRE(str_compare("abc123!@#", "abc123!@#") == -1);
        REQUIRE(str_compare("ABC123!@#", "abc123!@#") == -1);

        REQUIRE(str_compare("abc", "ab") == 2);
        REQUIRE(str_compare("ab", "abc") == 2);

        REQUIRE(str_compare("_", "_") == -1);
        REQUIRE(str_compare("-", "_") == -1);
    }

    SECTION("Scopes")
    {
        StrCompareScope outer(StrCompareScope::exact);
        {
            StrCompareScope inner(StrCompareScope::caseless);
            {
                StrCompareScope inner(StrCompareScope::relaxed);
                REQUIRE(str_compare("-", "_") == -1);
            }
            REQUIRE(str_compare("ABC123!@#", "abc123!@#") == -1);
        }
        REQUIRE(str_compare("abc123!@#", "abc123!@#") == -1);
        REQUIRE(str_compare("ABC123!@#", "abc123!@#") == 0);
        REQUIRE(str_compare("-", "_") == 0);
    }

    SECTION("Iterator state (same)")
    {
        StrIter lhs_iter("abc123");
        StrIter rhs_iter("abc123");

        REQUIRE(str_compare(lhs_iter, rhs_iter) == -1);
        REQUIRE(lhs_iter.more() == false);
        REQUIRE(rhs_iter.more() == false);
    }

    SECTION("Iterator state (different)")
    {
        StrIter lhs_iter("abc123");
        StrIter rhs_iter("abc321");

        REQUIRE(str_compare(lhs_iter, rhs_iter) == 3);
        REQUIRE(lhs_iter.peek() == '1');
        REQUIRE(rhs_iter.peek() == '3');
    }

    SECTION("Iterator state (lhs shorter)")
    {
        StrIter lhs_iter("abc");
        StrIter rhs_iter("abc321");

        REQUIRE(str_compare(lhs_iter, rhs_iter) == 3);
        REQUIRE(lhs_iter.more() == false);
        REQUIRE(rhs_iter.peek() == '3');
    }

    SECTION("Iterator state (lhs shorter)")
    {
        StrIter lhs_iter("abc123");
        StrIter rhs_iter("abc");

        REQUIRE(str_compare(lhs_iter, rhs_iter) == 3);
        REQUIRE(lhs_iter.peek() == '1');
        REQUIRE(rhs_iter.more() == false);
    }

    SECTION("UTF-8")
    {
        REQUIRE(str_compare("\xc2\x80", "\xc2\x80") == -1);
        REQUIRE(str_compare("\xc2\x80""abc", "\xc2\x80") == 2);
    }

    SECTION("UTF-16")
    {
        REQUIRE(str_compare(L"abc123", L"abc123") == -1);
        REQUIRE(str_compare(L"\xd800\xdc00" L"abc", L"\xd800\xdc00") == 2);
    }
}
