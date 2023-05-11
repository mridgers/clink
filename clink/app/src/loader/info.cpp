// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "utils/app_context.h"
#include "version.h"

#include <core/str.h>
#include <core/os.h>
#include <core/path.h>

//------------------------------------------------------------------------------
int32 clink_info(int32 argc, char** argv)
{
    struct {
        const char* name;
        void        (AppContext::*method)(StrBase&) const;
    } infos[] = {
        { "binaries",   &AppContext::get_binaries_dir },
        { "state",      &AppContext::get_state_dir },
        { "log",        &AppContext::get_log_path },
        { "settings",   &AppContext::get_settings_path },
        { "history",    &AppContext::get_history_path },
    };

    const auto* context = AppContext::get();
    const int32 spacing = 8;

    // Version information
    printf("%-*s : %s\n", spacing, "version", CLINK_VERSION_STR);
    printf("%-*s : %d\n", spacing, "session", context->get_id());

    // Paths
    for (const auto& info : infos)
    {
        Str<280> out;
        (context->*info.method)(out);
        printf("%-*s : %s\n", spacing, info.name, out.c_str());
    }

    // Inputrc environment variables.
    const char* env_vars[] = {
        "clink_inputrc",
        "userprofile",
        "localappdata",
        "appdata",
        "home"
    };

    bool labeled = false;
    for (const char* env_var : env_vars)
    {
        const char* label = labeled ? "" : "inputrc";
        labeled = true;
        printf("%-*s : %%%s%%\n", spacing, label, env_var);

        Str<280> out;
        if (!os::get_env(env_var, out))
        {
            printf("%-*s     (unset)\n", spacing, "");
            continue;
        }

        path::append(out, ".inputrc");
        for (int32 i = 0; i < 2; ++i)
        {
            printf("%-*s     %s\n", spacing, "", out.c_str());
            int32 out_length = out.length();
            out.data()[out_length - 8] = '_';
        }
    }

    return 0;
}
