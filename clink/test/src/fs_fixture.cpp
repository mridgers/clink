// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "fs_fixture.h"

#include <core/base.h>
#include <core/globber.h>
#include <core/os.h>
#include <core/path.h>

//------------------------------------------------------------------------------
static const char* g_default_fs[] = {
    "file1",
    "file2",
    "case_map-1",
    "case_map_2",
    "dir1/only",
    "dir1/file1",
    "dir1/file2",
    "dir2/.",
    nullptr,
};

//------------------------------------------------------------------------------
FsFixture::FsFixture(const char** fs)
{
    os::get_env("tmp", _root);
    REQUIRE(!_root.empty());

    Str<64> id;
    id.format("clink_test_%d", rand());
    path::append(_root, id.c_str());

    bool existed = !os::make_dir(_root.c_str());
    os::set_current_dir(_root.c_str());

    if (existed)
        clean(_root.c_str());

    if (fs == nullptr)
        fs = g_default_fs;

    const char** item = fs;
    while (const char* file = *item++)
    {
        Str<> dir;
        path::get_directory(file, dir);
        os::make_dir(dir.c_str());

        if (FILE* f = fopen(file, "wt"))
            fclose(f);
    }

    _fs = fs;
}

//------------------------------------------------------------------------------
FsFixture::~FsFixture()
{
    os::set_current_dir(_root.c_str());
    os::set_current_dir("..");

    clean(_root.c_str());
}

//------------------------------------------------------------------------------
void FsFixture::clean(const char* path)
{
    Str<> file;
    path::join(path, "*", file);

    Globber globber(file.c_str());
    globber.hidden(true);
    while (globber.next(file))
    {
        if (os::get_path_type(file.c_str()) == os::path_type_dir)
            clean(file.c_str());
        else
            REQUIRE(os::unlink(file.c_str()));
    }

    REQUIRE(os::remove_dir(path));
}

//------------------------------------------------------------------------------
const char* FsFixture::get_root() const
{
    return _root.c_str();
}
