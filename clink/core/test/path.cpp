// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/path.h>
#include <core/str.h>

//------------------------------------------------------------------------------
TEST_CASE("path::get_base_name()")
{
    SECTION("Basic")
    {
        Str<> s;

        SECTION("0") { path::get_base_name("one/two/three/filename.ext", s); }
        SECTION("1") { path::get_base_name("one/two/three/filename.ext", s); }
        SECTION("2") { path::get_base_name("one/two/three\\filename.ext", s); }
        SECTION("3") { path::get_base_name("filename.ext", s); }
        SECTION("4") { path::get_base_name("filename", s); }
        SECTION("5") { path::get_base_name("c:filename.ext", s); }
        SECTION("6") { path::get_base_name("c:filename", s); }

        REQUIRE(s.equals("filename"));
    }

    SECTION("Other")
    {
        Str<> s;

        path::get_base_name("one/two/three/filename...ext", s);
        REQUIRE(s.equals("filename.."));
        s.clear();

        path::get_base_name("one/two/three/filename..ext.ext", s);
        REQUIRE(s.equals("filename..ext"));
        s.clear();

        path::get_base_name("one/two/three/filename...", s);
        REQUIRE(s.equals("filename.."));
        s.clear();

        path::get_base_name("one/two/three/filename.ext.", s);
        REQUIRE(s.equals("filename.ext"));
        s.clear();
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::get_directory()")
{
    SECTION("Copy")
    {
        Str<> s, t;

        SECTION("0") { t << "one/two/three/filename.ext"; }
        SECTION("1") { t << "one/two/three\\filename.ext"; }

        path::get_directory(t.c_str(), s);
        path::normalise(s, '/');
        REQUIRE(s.equals("one/two/three"));
    }

    SECTION("In-place")
    {
        Str<> s;

        SECTION("0") { s << "one/two/three/filename.ext"; }
        SECTION("1") { s << "one/two\\three/filename.ext"; }

        path::get_directory(s);
        path::normalise(s, '/');
        REQUIRE(s.equals("one/two/three"));
    }

    SECTION("Trailing slash")
    {
        Str<> s;

        SECTION("0") { s << "one/two/three/"; }
        SECTION("1") { s << "one/two/three\\"; }
        SECTION("2") { s << "one/two/three///"; }
        SECTION("3") { s << "one/two/three\\\\\\"; }

        path::get_directory(s);
        path::normalise(s, '/');
        REQUIRE(s.equals("one/two/three"));
    }

    SECTION("No slash")
    {
        Str<> s;

        SECTION("0") { s << "one"; }
        SECTION("1") { s << ""; }

        path::get_directory(s);
        REQUIRE(s.equals(""));
    }

    SECTION("Root copy")
    {
        Str<> s, t;

        SECTION("0") { t << "/"; }
        SECTION("1") { t << "\\"; }
        SECTION("2") { t << "/one"; }
        SECTION("3") { t << "\\one"; }

        path::get_directory(t.c_str(), s);
        path::normalise(s, '/');
        REQUIRE(s.equals("/"));
    }

    SECTION("Root in-place")
    {
        Str<> s;

        SECTION("0") { s << "/"; }
        SECTION("1") { s << "\\"; }
        SECTION("2") { s << "/one"; }
        SECTION("3") { s << "\\one"; }

        path::get_directory(s);
        path::normalise(s, '/');
        REQUIRE(s.equals("/"));
    }

    SECTION("Drive letter")
    {
        Str<> s;

        path::get_directory("e:\\one", s);
        REQUIRE(s.equals("e:\\"));

        s.clear();
        path::get_directory("e:/one", s);
        REQUIRE(s.equals("e:/"));

        s.clear();
        path::get_directory("e:one", s);
        REQUIRE(s.equals("e:"));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::get_drive()")
{
    SECTION("Has drive")
    {
        Str<> s, t;

        SECTION("0") { s << "e:"; }
        SECTION("1") { s << "e:/"; }
        SECTION("2") { s << "e:/one/filename.ext"; }
        SECTION("3") { s << "e:one/filename.ext"; }
        SECTION("4") { s << "E:\\one/filename.ext"; }
        SECTION("5") { s << "E:one/filename.ext"; }

        REQUIRE(path::get_drive(s.c_str(), t));
        REQUIRE((t.equals("e:") || t.equals("E:")));
        REQUIRE(path::get_drive(t));
    }

    SECTION("No drive")
    {
        Str<> s, t;

        SECTION("0") { s << ""; }
        SECTION("1") { s << ":"; }
        SECTION("2") { s << ":/"; }
        SECTION("3") { s << "[:/"; }
        SECTION("4") { s << "{:"; }
        SECTION("5") { s << "@:"; }
        SECTION("6") { s << "`:\\"; }
        SECTION("7") { s << "/one/filename.ext"; }
        SECTION("8") { s << "one/filename.ext"; }
        SECTION("9") { s << "filename.ext"; }

        REQUIRE(!path::get_drive(s.c_str(), t));
        REQUIRE(!path::get_drive(s));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::get_extension()")
{
    SECTION("Has extension")
    {
        Str<> s;

        SECTION("0") { path::get_extension("one/two/three/filename.ext", s); }
        SECTION("1") { path::get_extension("one/two/three/filename.ext", s); }
        SECTION("2") { path::get_extension("one/two/three\\filename.ext", s); }
        SECTION("3") { path::get_extension("./two/three\\filename.ext", s); }
        SECTION("4") { path::get_extension("filename.ext", s); }
        SECTION("5") { path::get_extension("filename..ext", s); }
        SECTION("6") { path::get_extension(".ext", s); }

        REQUIRE(s.equals(".ext"));
    }

    SECTION("Misc")
    {
        Str<> s;

        SECTION("0") { REQUIRE(!path::get_extension("..", s)); }
        SECTION("1") { REQUIRE(!path::get_extension("", s)); }
        SECTION("2") { REQUIRE(!path::get_extension("abc", s)); }
        SECTION("3") { REQUIRE(!path::get_extension("./one/two", s)); }
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::get_name()")
{
    SECTION("Basic")
    {
        Str<> s;

        SECTION("0") { path::get_name("one/two/three/filename.ext", s); }
        SECTION("1") { path::get_name("one/two/three\\filename.ext", s); }
        SECTION("2") { path::get_name("filename.ext", s); }

        REQUIRE(s.equals("filename.ext"));
    }

    SECTION("Trailing slash")
    {
        const char* name;

        SECTION("0") { name = path::get_name("one/two/"); }
        SECTION("1") { name = path::get_name("one/two//"); }
        SECTION("2") { name = path::get_name("one/two///"); }
        SECTION("3") { name = path::get_name("one\\two\\\\"); }
        SECTION("4") { name = path::get_name("/two/"); }
        SECTION("5") { name = path::get_name("two/"); }

        REQUIRE(name[0] == '\0');
    }

    SECTION("Other")
    {
        const char* in;

        SECTION("0") { in = ""; }
        SECTION("1") { in = "//"; }
        SECTION("2") { in = "/\\/"; }
        SECTION("3") { in = "\\"; }

        REQUIRE(path::get_name(in)[0] == '\0');
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::is_rooted()")
{
    SECTION("True")
    {
        REQUIRE(path::is_rooted("e:/"));
        REQUIRE(path::is_rooted("e:\\"));
        REQUIRE(path::is_rooted("/"));
        REQUIRE(path::is_rooted("\\"));
        REQUIRE(path::is_rooted("e:/one"));
        REQUIRE(path::is_rooted("e:\\one"));
        REQUIRE(path::is_rooted("/one"));
        REQUIRE(path::is_rooted("\\one"));
    }

    SECTION("False")
    {
        REQUIRE(!path::is_rooted("e:"));
        REQUIRE(!path::is_rooted("e:one"));
        REQUIRE(!path::is_rooted("one"));
        REQUIRE(!path::is_rooted(""));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::is_root()")
{
    SECTION("True")
    {
        REQUIRE(path::is_root("e:"));
        REQUIRE(path::is_root("e:/"));
        REQUIRE(path::is_root("e:\\"));
        REQUIRE(path::is_root("/"));
        REQUIRE(path::is_root("\\"));
        REQUIRE(path::is_root(""));
    }

    SECTION("False")
    {
        REQUIRE(!path::is_root("e:one"));
        REQUIRE(!path::is_root("e:/one"));
        REQUIRE(!path::is_root("e:\\one"));
        REQUIRE(!path::is_root("/one"));
        REQUIRE(!path::is_root("\\one"));
        REQUIRE(!path::is_root("one"));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::join()")
{
    SECTION("Basic")
    {
        Str<> s;

        SECTION("0") { path::join("one/two", "three/four", s); }
        SECTION("1") { path::join("one/two/", "three/four", s); }
        SECTION("2") { path::join("one/two\\", "three/four", s); }

        path::normalise(s);
        REQUIRE(s.equals("one\\two\\three\\four"));
    }

    SECTION("Rooted rhs")
    {
        Str<> s, t;

        SECTION("0") { t = "/three/four"; }
        SECTION("1") { t = "\\three/four"; }
        SECTION("2") { t = "a:\\three/four"; }
        SECTION("2") { t = "b:/three/four"; }

        path::append(s, t.c_str());
        path::normalise(s);
        path::normalise(t);
        REQUIRE(s.equals(t.c_str()));
    }

    SECTION("One side empty")
    {
        SECTION("Left")
        {
            Str<> s;
            path::join("", "one\\two", s);
            REQUIRE(s.equals("one\\two"));
        }

        SECTION("Right")
        {
            Str<> s;
            path::join("one\\two", "", s);
            REQUIRE(s.equals("one\\two\\"));
        }
    }

    SECTION("Drive letter")
    {
        Str<> s;

        // Absolute
        s.copy("x:/");
        path::append(s, "one");
        REQUIRE(s.equals("x:/one"));

        s.copy("x:\\");
        path::append(s, "one");
        REQUIRE(s.equals("x:\\one"));

        // Relative
        s.copy("x:");
        path::append(s, "one");
        REQUIRE(s.equals("x:one"));
    }
}

//------------------------------------------------------------------------------
TEST_CASE("path::join(get_dir(), get_name())")
{
    const char* in;
    const char* out;

    SECTION("Plain")
    {
        SECTION("0") { in = "one/two"; }
        SECTION("1") { in = "one\\two"; }
        out = "one/two";
    }

    SECTION("Trailing slash")
    {
        SECTION("0") { in = "one/two/"; }
        SECTION("1") { in = "one/two\\"; }
        SECTION("2") { in = "one\\two\\"; }
        out = "one/two/";
    }

    Str<> dir, name;
    path::get_directory(in, dir);
    path::get_name(in, name);

    Str<> join;
    path::join(dir.c_str(), name.c_str(), join);
    path::normalise(join, '/');

    REQUIRE(join.equals(out));
}

//------------------------------------------------------------------------------
TEST_CASE("path::normalise() 1")
{
    Str<> s;

    s.copy("X://0/\\/1/2//\\3/\\\\//4//");
    path::normalise(s);
    REQUIRE(s.equals("X:\\0\\1\\2\\3\\4\\"));

    s << "//\\//";
    path::normalise(s, '/');
    REQUIRE(s.equals("X:/0/1/2/3/4/"));

    s.copy("abcdef");
    path::normalise(s);
    REQUIRE(s.equals("abcdef"));
}

//------------------------------------------------------------------------------
TEST_CASE("path::normalise() 2")
{
    auto Test = [] (const char* test_path, const char* expected)
    {
        Str<> out(test_path);
        path::normalise(out, '/');
        REQUIRE(out.equals(expected), [&] () {
            puts(test_path);
            puts(expected);
            puts(out.c_str());
        });
    };

    Test("123/xxx/xxx/../../456", "123/456");
    Test("/123/xxx/xxx/../../456", "/123/456");
    Test("a:/123/xxx/xxx/../../456", "a:/123/456");
    Test("a:/123/xxx/xxx/../../456/", "a:/123/456/");
    Test("a:/123/xxx/xxx/../../../../../../456/", "a:/456/");
    Test("a:/xxx/yyy/../../123/456", "a:/123/456");
    Test("a:/xxx/../../123/456", "a:/123/456");
    Test("a:/xxx/../../123/456", "a:/123/456");
    Test("a:/123/xxx/..", "a:/123/");
    Test("a:/123/./xxx/..", "a:/123/");
    Test("a:/123/.", "a:/123/");

    Test("a:/1/2/x/../../2/3/4", "a:/1/2/3/4");
    Test("/1/x/../../2/3/4", "/2/3/4");
    Test("a://1/2//../2", "a:/1/2");
    Test("a:/1//2////../2", "a:/1/2");
    Test("a://1/../1/2", "a:/1/2");

    Test("a:/1/./2/", "a:/1/2/");

    Test("", "");
    Test("/..", "/");

    Test("..", "..");
    Test("../..", "../..");
    Test("../../..", "../../..");
    Test("../xxx/../..", "../..");
    Test("../../xxx/../..", "../../..");
}
