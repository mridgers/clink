// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "match_generator.h"
#include "line_state.h"
#include "matches.h"

#include <core/base.h>
#include <core/globber.h>
#include <core/path.h>
#include <core/settings.h>

SettingBool g_glob_hidden(
    "files.hidden",
    "Include hidden files",
    "Includes or excludes files with the 'hidden' Attribute set when generating\n"
    "file lists.",
    true);

SettingBool g_glob_system(
    "files.system",
    "Include system files",
    "Includes or excludes files with the 'system' Attribute set when generating\n"
    "file lists.",
    false);

// TODO: dream up a way around performance problems that UNC paths pose.
SettingBool g_glob_unc(
    "files.unc_paths",
    "Enables UNC/network path matches",
    "UNC (network) paths can cause Clink to stutter slightly when it tries to\n"
    "generate matches. Enable this if matching UNC paths is required.",
    false);



//------------------------------------------------------------------------------
static class : public MatchGenerator
{
    virtual bool generate(const LineState& line, MatchBuilder& Builder) override
    {
        Str<288> buffer;
        line.get_end_word(buffer);

        if (path::is_separator(buffer[0]) && path::is_separator(buffer[1]))
            if (!g_glob_unc.get())
                return true;

        buffer << "*";

        Globber globber(buffer.c_str());
        globber.hidden(g_glob_hidden.get());
        globber.system(g_glob_system.get());
        while (globber.next(buffer, false))
            Builder.add_match(buffer.c_str());

        return true;
    }

    virtual int32 get_prefix_length(const LineState& line) const override
    {
        StrIter end_word = line.get_end_word();
        const char* start = end_word.get_pointer();

        const char* c = start + end_word.length();
        for (; c > start; --c)
            if (path::is_separator(c[-1]))
                break;

        if (start[0] && start[1] == ':')
            c = max(start + 2, c);

        return int32(c - start);
    }
} g_file_generator;


//------------------------------------------------------------------------------
MatchGenerator& file_match_generator()
{
    return g_file_generator;
}
