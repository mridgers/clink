// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/base.h>
#include <terminal/ecma48_iter.h>

#include <new>

static Ecma48State g_state;

//------------------------------------------------------------------------------
TEST_CASE("ecma48 chars")
{
    const char* input = "abc123";

    const Ecma48Code* code;

    Ecma48Iter iter(input, g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_chars);
    REQUIRE(code->get_pointer() == input);
    REQUIRE(code->get_length() == 6);

    REQUIRE(!iter.next());
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c0")
{
    const Ecma48Code* code;

    Ecma48Iter iter("\x01 \x10\x1f", g_state);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c0);
    REQUIRE(code->get_code() == 0x01);
    REQUIRE(code->get_length() == 1);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_chars);
    REQUIRE(code->get_length() == 1);
    REQUIRE(code->get_pointer()[0] == ' ');

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c0);
    REQUIRE(code->get_code() == 0x10);
    REQUIRE(code->get_length() == 1);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c0);
    REQUIRE(code->get_code() == 0x1f);
    REQUIRE(code->get_length() == 1);

    REQUIRE(!iter.next());
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 (simple)")
{
    const Ecma48Code* code;

    Ecma48Iter iter("\x1b\x40\x1b\x51\x1b\x5c", g_state);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);
    REQUIRE(code->get_code() == 0x40);
    REQUIRE(code->get_length() == 2);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);
    REQUIRE(code->get_code() == 0x51);
    REQUIRE(code->get_length() == 2);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);
    REQUIRE(code->get_code() == 0x5c);
    REQUIRE(code->get_length() == 2);

    REQUIRE(!iter.next());
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 icf")
{
    const Ecma48Code* code;

    Ecma48Iter iter("\x1b\x60\x1b\x7f", g_state);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_icf);
    REQUIRE(code->get_code() == 0x60);
    REQUIRE(code->get_length() == 2);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_icf);
    REQUIRE(code->get_code() == 0x7f);
    REQUIRE(code->get_length() == 2);

    REQUIRE(!iter.next());
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 csi")
{
    const Ecma48Code* code;
    Ecma48Code::Csi<8> csi;

    Ecma48Iter iter("\x1b[\x40\xc2\x9b\x20\x7e", g_state);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);
    REQUIRE(code->get_length() == 3);

    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.final == 0x40);
    REQUIRE(csi.param_count == 0);

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);
    REQUIRE(code->get_length() == 4);

    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == 0);
    REQUIRE(csi.intermediate == 0x20);
    REQUIRE(csi.final == 0x7e);

    REQUIRE(!iter.next());
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 csi params")
{
    const Ecma48Code* code;
    Ecma48Code::Csi<8> csi;

    // ---
    Ecma48Iter iter("\x1b[123\x7e", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_length() == 6);

    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == 1);
    REQUIRE(csi.params[0] == 123);

    // ---
    new (&iter) Ecma48Iter("\x1b[1;12;123 \x40", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_length() == 12);

    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == 3);
    REQUIRE(csi.params[0] == 1);
    REQUIRE(csi.params[1] == 12);
    REQUIRE(csi.params[2] == 123);
    REQUIRE(csi.intermediate == 0x20);
    REQUIRE(csi.final == 0x40);

    // ---
    new (&iter) Ecma48Iter("\x1b[;@", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_length() == 4);

    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == 2);
    REQUIRE(csi.params[0] == 0);
    REQUIRE(csi.params[1] == 0);
    REQUIRE(csi.final == '@');

    // Overflow
    new (&iter) Ecma48Iter("\x1b[;;;;;;;;;;;;1;2;3;4;5 m", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_length() == 25);

    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == decltype(csi)::max_param_count);

    new (&iter) Ecma48Iter("\x1b[1;2;3;4;5;6;7;8;1;2;3;4;5;6;7;8m", g_state);
    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == decltype(csi)::max_param_count);
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 csi invalid")
{
    const Ecma48Code* code;

    Ecma48Iter iter("\x1b[1;2\01", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c0);
    REQUIRE(code->get_code() == 1);
    REQUIRE(code->get_pointer()[0] == 1);
    REQUIRE(code->get_length() == 1);
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 csi stream")
{
    const char input[] = "\x1b[1;21m";

    Ecma48Iter iter_1(input, g_state, 0);
    for (int32 i = 0; i < sizeof_array(input) - 1; ++i)
    {
        const Ecma48Code* code;

        memset(&iter_1, 0xab, sizeof(iter_1));
        new (&iter_1) Ecma48Iter(input, g_state, i);
        REQUIRE(!iter_1.next());

        Ecma48Iter iter_2(input + i, g_state, sizeof_array(input) - i);
        code = &iter_2.next();
        REQUIRE(*code);
        REQUIRE(code->get_type() == Ecma48Code::type_c1);
        REQUIRE(code->get_length() == 7);

        Ecma48Code::Csi<8> csi;
        REQUIRE(code->decode_csi(csi));
        REQUIRE(csi.param_count == 2);
        REQUIRE(csi.params[0] == 1);
        REQUIRE(csi.params[1] == 21);
        REQUIRE(csi.final == 'm');
    }
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 csi split")
{
    const Ecma48Code* code;

    Ecma48Iter iter(" \x1b[1;2x@@@@", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_chars);
    REQUIRE(code->get_length() == 1);
    REQUIRE(code->get_pointer()[0] == ' ');

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);

    Ecma48Code::Csi<8> csi;
    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == 2);
    REQUIRE(csi.params[0] == 1);
    REQUIRE(csi.params[1] == 2);
    REQUIRE(csi.final == 'x');

    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_chars);
    REQUIRE(code->get_length() == 4);
    REQUIRE(code->get_pointer()[0] == '@');

    REQUIRE(!iter.next());
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 csi private use")
{
    struct {
        const char* input;
        int32 param_count;
        int32 params[8];
    } tests[] = {
        { "\x1b[?x",               0 },
        { "\x1b[?99x",             1, 99 },
        { "\x1b[?;98x",            2, 0, 98 },
        { "\x1b[?1;2;3x",          3, 1, 2, 3 },
        { "\x1b[?2;?4;6x",         3, 2, 4, 6 },
        { "\x1b[?3;6?;9x",         3, 3, 6, 9 },
        { "\x1b[?4;8?;???3;13??x", 4, 4, 8, 3, 13 },
    };

    for (auto Test : tests)
    {
        const Ecma48Code* code;
        Ecma48Iter iter(Test.input, g_state);
        code = &iter.next();
        REQUIRE(*code);

        Ecma48Code::Csi<8> csi;
        REQUIRE(code->decode_csi(csi));
        REQUIRE(csi.private_use);

        REQUIRE(csi.param_count == Test.param_count);
        for (int32 i = 0; i < csi.param_count; ++i)
            REQUIRE(csi.params[i] == Test.params[i]);
    }
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 c1 !csi")
{
    const Ecma48Code* code;

    const char* terminators[] = { "\x1b\\", "\xc2\x9c" };
    for (int32 i = 0, n = sizeof_array(terminators); i < n; ++i)
    {
        const char* announcers[] = {
            "\x1b\x5f", "\xc2\x9f",
            "\x1b\x50", "\xc2\x90",
            "\x1b\x5d", "\xc2\x9d",
            "\x1b\x5e", "\xc2\x9e",
            "\x1b\x58", "\xc2\x98",
        };
        for (int32 j = 0, m = sizeof_array(announcers); j < m; ++j)
        {
            Str<> input;
            input << announcers[j];
            input << "xyz";
            input << terminators[i];

            Ecma48Iter iter(input.c_str(), g_state);
            code = &iter.next();
            REQUIRE(*code);
            REQUIRE(code->get_length() == input.length());
            Str<> ctrl_str;
            code->get_c1_str(ctrl_str);
            REQUIRE(ctrl_str.equals("xyz"));

            REQUIRE(!iter.next());
        }
    }
}

//------------------------------------------------------------------------------
TEST_CASE("ecma48 utf8")
{
    const Ecma48Code* code;

    Ecma48Iter iter("\xc2\x9c", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);
    REQUIRE(code->get_code() == 0x5c);
    REQUIRE(code->get_length() == 2);

    new (&iter) Ecma48Iter("\xc2\x9bz", g_state);
    code = &iter.next();
    REQUIRE(*code);
    REQUIRE(code->get_type() == Ecma48Code::type_c1);
    REQUIRE(code->get_length() == 3);

    Ecma48Code::Csi<8> csi;
    REQUIRE(code->decode_csi(csi));
    REQUIRE(csi.param_count == 0);
    REQUIRE(csi.final == 'z');
}
