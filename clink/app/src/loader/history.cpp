// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "history/history_db.h"
#include "utils/app_context.h"

#include <core/base.h>
#include <core/settings.h>
#include <core/str.h>
#include <core/str_tokeniser.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
void puts_help(const char**, int32);

//------------------------------------------------------------------------------
class HistoryScope
{
public:
                    HistoryScope();
    HistoryDb*      operator -> ()      { return &_history; }
    HistoryDb&      operator * ()       { return _history; }

private:
    Str<280>        _path;
    HistoryDb       _history;
};

//------------------------------------------------------------------------------
HistoryScope::HistoryScope()
{
    // Load settings.
    AppContext::get()->get_settings_path(_path);
    settings::load(_path.c_str());

    _history.initialise();
}



//------------------------------------------------------------------------------
static void print_history(uint32 tail_count)
{
    HistoryScope history;

    StrIter line;
    char buffer[HistoryDb::max_line_length];

    int32 count = 0;
    {
        HistoryDb::Iter iter = history->read_lines(buffer);
        while (iter.next(line))
            ++count;
    }

    int32 index = 1;
    HistoryDb::Iter iter = history->read_lines(buffer);

    int32 skip = count - tail_count;
    for (int32 i = 0; i < skip; ++i, ++index, iter.next(line));

    for (; iter.next(line); ++index)
        printf("%5d  %.*s\n", index, line.length(), line.get_pointer());
}

//------------------------------------------------------------------------------
static bool print_history(const char* arg)
{
    if (arg == nullptr)
    {
        print_history(INT_MIN);
        return true;
    }

    // Check the argument is just digits.
    for (const char* c = arg; *c; ++c)
        if (uint32(*c - '0') > 10)
            return false;

    print_history(atoi(arg));
    return true;
}

//------------------------------------------------------------------------------
static int32 add(const char* line)
{
    HistoryScope history;
    history->add(line);

    printf("Added '%s' to history.\n", line);
    return 0;
}

//------------------------------------------------------------------------------
static int32 remove(int32 index)
{
    HistoryScope history;

    if (index <= 0)
        return 1;

    char buffer[HistoryDb::max_line_length];
    HistoryDb::LineId line_id = 0;
    {
        StrIter line;
        HistoryDb::Iter iter = history->read_lines(buffer);
        for (int32 i = index - 1; i > 0 && iter.next(line); --i);

        line_id = iter.next(line);
    }

    bool ok = history->remove(line_id);

    auto fmt =  ok ?  "Deleted item %d.\n" : "Unable to delete history item %d.\n";
    printf(fmt, index);
    return (ok != true);
}

//------------------------------------------------------------------------------
static int32 clear()
{
    HistoryScope history;
    history->clear();

    puts("History cleared.");
    return 0;
}

//------------------------------------------------------------------------------
static int32 print_expansion(const char* line)
{
    HistoryScope history;
    history->load_rl_history();
    Str<> out;
    history->expand(line, out);
    puts(out.c_str());
    return 0;
}

//------------------------------------------------------------------------------
static int32 print_help()
{
    extern const char* g_clink_header;

    const char* help[] = {
        "[n]",          "Print history items (only the last N items if specified).",
        "clear",        "Completly clears the command history.",
        "delete <n>",   "Delete Nth item (negative N indexes history backwards).",
        "add <...>",    "Join remaining arguments and appends to the history.",
        "expand <...>", "Print substitution result.",
    };

    puts(g_clink_header);
    puts("Usage: history <verb> [option]\n");

    puts("Verbs:");
    puts_help(help, sizeof_array(help));

    puts("The 'history' command can also emulates Bash's builtin history command. The\n"
        "arguments -c, -d <n>, -p <...> and -s <...> are supported.\n");

    return 1;
}

//------------------------------------------------------------------------------
static void get_line(int32 start, int32 end, char** argv, StrBase& out)
{
    for (int32 j = start; j < end; ++j)
    {
        if (!out.empty())
            out << " ";

        out << argv[j];
    }
}

//------------------------------------------------------------------------------
static int32 history_bash(int32 argc, char** argv)
{
    int32 i;
    while ((i = getopt(argc, argv, "+cd:ps")) != -1)
    {
        switch (i)
        {
        case 'c': // clear history
            return clear();

        case 'd': // remove an item of history
            return remove(atoi(optarg));

        case 'p': // print expansion
        case 's': // add to history
            {
                Str<> line;
                get_line(optind, argc, argv, line);
                if (line.empty())
                    return print_help();

                return (i == 's') ? add(line.c_str()) : print_expansion(line.c_str());
            }

        case ':': // option's missing argument.
        case '?': // unknown option.
            return print_help();

        default:
            return -1;
        }
    }

    return -1;
}

//------------------------------------------------------------------------------
int32 history(int32 argc, char** argv)
{
    // Check to see if the user asked from some help!
    for (int32 i = 1; i < argc; ++i)
        if (_stricmp(argv[1], "--help") == 0 || _stricmp(argv[1], "-h") == 0)
            return print_help();

    // Try Bash-style arguments first...
    int32 bash_ret = history_bash(argc, argv);
    if (optind != 1)
        return bash_ret;

    // ...and the try Clink style arguments.
    if (argc > 1)
    {
        const char* verb = argv[1];

        // 'clear' command
        if (_stricmp(verb, "clear") == 0)
            return clear();

        // 'delete' command
        if (_stricmp(verb, "delete") == 0)
        {
            if (argc < 3)
            {
                puts("history: argument required for verb 'delete'");
                return print_help();
            }
            else
                return remove(atoi(argv[2]));
        }

        Str<> line;

        // 'add' command
        if (_stricmp(verb, "add") == 0)
        {
            get_line(2, argc, argv, line);
            return line.empty() ? print_help() : add(line.c_str());
        }

        // 'expand' command
        if (_stricmp(verb, "expand") == 0)
        {
            get_line(2, argc, argv, line);
            return line.empty() ? print_help() : print_expansion(line.c_str());
        }
    }

    // Failing all else try to display the history.
    if (argc > 2)
        return print_help();

    const char* arg = (argc > 1) ? argv[1] : nullptr;
    if (!print_history(arg))
        return print_help();

    return 0;
}
