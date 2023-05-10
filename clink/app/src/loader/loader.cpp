// Copyright (c) 2012 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "utils/app_context.h"
#include "utils/seh_scope.h"
#include "version.h"

#include <core/base.h>
#include <core/str.h>

extern "C" {
#include <getopt.h>
}

//------------------------------------------------------------------------------
int32 autorun(int32, char**);
int32 clink_info(int32, char**);
int32 draw_test(int32, char**);
int32 history(int32, char**);
int32 inject(int32, char**);
int32 input_echo(int32, char**);
int32 set(int32, char**);
int32 testbed(int32, char**);

//------------------------------------------------------------------------------
void puts_help(const char** help_pairs, int32 count)
{
    count &= ~1;

    int32 max_len = -1;
    for (int32 i = 0, n = count; i < n; i += 2)
        max_len = max((int32)strlen(help_pairs[i]), max_len);

    for (int32 i = 0, n = count; i < n; i += 2)
    {
        const char* arg = help_pairs[i];
        const char* desc = help_pairs[i + 1];
        printf("  %-*s  %s\n", max_len, arg, desc);
    }

    puts("");
}

//------------------------------------------------------------------------------
static void show_usage()
{
    const char* help_usage = "Usage: [options] <verb> [verb_options]\n";
    const char* help_verbs[] = {
        "Verbs:",          "",
        "inject",          "Injects Clink into a Process",
        "autorun",         "Manage Clink's entry in cmd.exe's autorun",
        "set",             "Adjust Clink's settings",
        "history",         "List and operate on the command history",
        "info",            "Prints information about Clink",
        "echo",            "Echo key sequences",
        "",                "('<verb> --help' for more details)",
        "Options:",        "",
        "--profile <dir>", "Use <dir> as Clink's profile directory",
        "--version",       "Print Clink's version and exit",
    };

    extern const char* g_clink_header;

    puts(g_clink_header);
    puts(help_usage);
    puts_help(help_verbs, sizeof_array(help_verbs));
}

//------------------------------------------------------------------------------
static int32 dispatch_verb(const char* verb, int32 argc, char** argv)
{
    struct {
        const char* verb;
        int32 (*handler)(int32, char**);
    } handlers[] = {
        "autorun",   autorun,
        "drawtest",  draw_test,
        "echo",      input_echo,
        "history",   history,
        "info",      clink_info,
        "inject",    inject,
        "set",       set,
        "testbed",   testbed,
    };

    for (int32 i = 0; i < sizeof_array(handlers); ++i)
    {
        if (strcmp(verb, handlers[i].verb) == 0)
        {
            int32 ret;
            int32 t;

            t = optind;
            optind = 1;

            ret = handlers[i].handler(argc, argv);

            optind = t;
            return ret;
        }
    }

    printf("*** ERROR: Unknown verb -- '%s'\n", verb);
    show_usage();
    return 0;
}

//------------------------------------------------------------------------------
int32 loader(int32 argc, char** argv)
{
    SehScope seh;

    struct option options[] = {
        { "help",    no_argument,       nullptr, 'h' },
        { "profile", required_argument, nullptr, 'p' },
        { "version", no_argument,       nullptr, 'v' },
        { nullptr,   0,                 nullptr, 0 }
    };

    // Without arguments, show help.
    if (argc <= 1)
    {
        show_usage();
        return 0;
    }

    AppContext::Desc app_desc;
    app_desc.inherit_id = true;

    // Parse arguments
    int32 arg;
    while ((arg = getopt_long(argc, argv, "+hp:", options, nullptr)) != -1)
    {
        switch (arg)
        {
        case 'p':
            StrBase(app_desc.state_dir).copy(optarg);
            break;

        case 'v':
            puts(CLINK_VERSION_STR);
            return 0;

        case '?':
            return 0;

        default:
            show_usage();
            return 0;
        }
    }

    // Dispatch the verb if one was found.
    int32 ret = 0;
    if (optind < argc)
    {
        AppContext app_context(app_desc);
        ret = dispatch_verb(argv[optind], argc - optind, argv + optind);
    }
    else
        show_usage();

    return ret;
}

//------------------------------------------------------------------------------
int32 loader_main_impl()
{
    int32 argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int32 i = 0; i < argc; ++i)
        to_utf8((char*)argv[i], 0xffff, argv[i]);

    int32 ret = loader(argc, (char**)argv);

    LocalFree(argv);
    return ret;
}
