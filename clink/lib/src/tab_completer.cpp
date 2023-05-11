// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "tab_completer.h"
#include "binder.h"
#include "editor_module.h"
#include "line_buffer.h"
#include "line_state.h"
#include "matches.h"

#include <core/base.h>
#include <core/settings.h>
#include <core/str_iter.h>
#include <terminal/printer.h>
#include <terminal/setting_colour.h>

//------------------------------------------------------------------------------
EditorModule* tab_completer_create()
{
    return new TabCompleter();
}

//------------------------------------------------------------------------------
void tab_completer_destroy(EditorModule* completer)
{
    delete completer;
}



//------------------------------------------------------------------------------
static SettingInt g_query_threshold(
    "match.query_threshold",
    "Ask if no. matches > threshold",
    "If there are more than 'threshold' matches then ask the user before\n"
    "displaying them all.",
    100);

static SettingBool g_vertical(
    "match.vertical",
    "Display matches vertically",
    "Toggles the display of ordered matches between columns or rows.",
    true);

static SettingInt g_column_pad(
    "match.column_pad",
    "Space between columns",
    "Adjusts the amount of whitespace padding between columns of matches.",
    2);

SettingInt g_max_width(
    "match.max_width",
    "Maximum display width",
    "The maximum number of terminal columns to use when displaying matches.",
    106);

SettingColour g_colour_interact(
    "Colour.interact",
    "For user-interaction prompts",
    "Used when Clink displays text or prompts such as a pager's 'More?'. Naming\n"
    "these settings is hard. Describing them even more so.",
    SettingColour::value_green, SettingColour::value_bg_default);

SettingColour g_colour_minor(
    "Colour.minor",
    "Minor Colour value",
    "The Colour used to display minor elements such as the lower common\n"
    "denominator of active matches in tab completion's display.",
    SettingColour::value_dark_grey, SettingColour::value_bg_default);

SettingColour g_colour_major(
    "Colour.major",
    "Major Colour value",
    "The Colour used to display major elements like remainder of active matches\n"
    "still to be completed.",
    SettingColour::value_grey, SettingColour::value_bg_default);

SettingColour g_colour_highlight(
    "Colour.highlight",
    "Colour for highlights",
    "The Colour used for displaying highlighted elements such as the next\n"
    "character when invoking tab completion.",
    SettingColour::value_light_green, SettingColour::value_bg_default);



//------------------------------------------------------------------------------
enum {
    bind_id_prompt      = 20,
    bind_id_prompt_yes,
    bind_id_prompt_no,
    bind_id_pager_page,
    bind_id_pager_line,
    bind_id_pager_stop,
};



//------------------------------------------------------------------------------
void TabCompleter::bind_input(Binder& binder)
{
    int32 default_group = binder.get_group();
    binder.bind(default_group, "\t", state_none);

    _prompt_bind_group = binder.create_group("tab_complete_prompt");
    binder.bind(_prompt_bind_group, "y", bind_id_prompt_yes);
    binder.bind(_prompt_bind_group, "Y", bind_id_prompt_yes);
    binder.bind(_prompt_bind_group, " ", bind_id_prompt_yes);
    binder.bind(_prompt_bind_group, "\t", bind_id_prompt_yes);
    binder.bind(_prompt_bind_group, "\r", bind_id_prompt_yes);
    binder.bind(_prompt_bind_group, "n", bind_id_prompt_no);
    binder.bind(_prompt_bind_group, "N", bind_id_prompt_no);
    binder.bind(_prompt_bind_group, "^C", bind_id_prompt_no); // ctrl-c
    binder.bind(_prompt_bind_group, "^D", bind_id_prompt_no); // ctrl-d
    binder.bind(_prompt_bind_group, "^[", bind_id_prompt_no); // esc

    _pager_bind_group = binder.create_group("tab_complete_pager");
    binder.bind(_pager_bind_group, " ", bind_id_pager_page);
    binder.bind(_pager_bind_group, "\t", bind_id_pager_page);
    binder.bind(_pager_bind_group, "\r", bind_id_pager_line);
    binder.bind(_pager_bind_group, "q", bind_id_pager_stop);
    binder.bind(_pager_bind_group, "Q", bind_id_pager_stop);
    binder.bind(_pager_bind_group, "^C", bind_id_pager_stop); // ctrl-c
    binder.bind(_pager_bind_group, "^D", bind_id_pager_stop); // ctrl-d
    binder.bind(_pager_bind_group, "^[", bind_id_pager_stop); // esc
}

//------------------------------------------------------------------------------
void TabCompleter::on_begin_line(const Context& context)
{
}

//------------------------------------------------------------------------------
void TabCompleter::on_end_line()
{
}

//------------------------------------------------------------------------------
void TabCompleter::on_matches_changed(const Context& context)
{
    _waiting = false;
}

//------------------------------------------------------------------------------
void TabCompleter::on_input(const Input& input, Result& result, const Context& context)
{
    auto& matches = context.matches;
    if (matches.get_match_count() == 0)
        return;

    if (!_waiting)
    {
        // One match? Accept it.
        if (matches.get_match_count() == 1)
        {
            result.accept_match(0);
            return;
        }

        // Append as much of the lowest common denominator of matches as we can. If
        // there is an LCD then on_matches_changed() gets called.
        _waiting = true;
        result.append_match_lcd();
        return;
    }

    const char* keys = input.keys;
    int32 next_state = state_none;

    switch (input.id)
    {
    case state_none:            next_state = begin_print(context);  break;
    case bind_id_prompt_no:     next_state = state_none;            break;
    case bind_id_prompt_yes:    next_state = state_print_page;      break;
    case bind_id_pager_page:    next_state = state_print_page;      break;
    case bind_id_pager_line:    next_state = state_print_one;       break;
    case bind_id_pager_stop:    next_state = state_none;            break;
    }

    if (next_state > state_print)
        next_state = print(context, next_state == state_print_one);

    // '_prev_group' is >= 0 if tab completer has set a bind group. As the bind
    // groups are one-shot we restore the original back each time.
    if (_prev_group != -1)
    {
        result.set_bind_group(_prev_group);
        _prev_group = -1;
    }

    switch (next_state)
    {
    case state_query:
        _prev_group = result.set_bind_group(_prompt_bind_group);
        return;

    case state_pager:
        _prev_group = result.set_bind_group(_pager_bind_group);
        return;
    }

    context.printer.print("\n");
    result.redraw();
}

//------------------------------------------------------------------------------
TabCompleter::State TabCompleter::begin_print(const Context& context)
{
    const Matches& matches = context.matches;
    int32 match_count = matches.get_match_count();

    _longest = 0;
    _row = 0;

    // Get the longest match length.
    for (int32 i = 0, n = matches.get_match_count(); i < n; ++i)
        _longest = max<int32>(matches.get_cell_count(i), _longest);

    if (!_longest)
        return state_none;

    context.printer.print("\n");

    int32 query_threshold = g_query_threshold.get();
    if (query_threshold > 0 && query_threshold <= match_count)
    {
        Str<40> prompt;
        prompt.format("Show %d matches? [Yn]", match_count);
        context.printer.print(g_colour_interact.get(), prompt.c_str(), prompt.length());

        return state_query;
    }

    return state_print_page;
}

//------------------------------------------------------------------------------
TabCompleter::State TabCompleter::print(const Context& context, bool single_row)
{
    auto& printer = context.printer;

    const Matches& matches = context.matches;

    Attributes minor_attr = g_colour_minor.get();
    Attributes major_attr = g_colour_major.get();
    Attributes highlight_attr = g_colour_highlight.get();

    printer.print("\r");

    int32 match_count = matches.get_match_count();

    Str<288> lcd;
    matches.get_match_lcd(lcd);
    int32 lcd_length = lcd.length();

    // Calculate the number of columns of matches per row.
    int32 column_pad = g_column_pad.get();
    int32 cell_columns = min<int32>(g_max_width.get(), printer.get_columns());
    int32 columns = max(1, (cell_columns + column_pad) / (_longest + column_pad));
    int32 total_rows = (match_count + columns - 1) / columns;

    bool vertical = g_vertical.get();
    int32 index_step = vertical ? total_rows : 1;

    int32 max_rows = single_row ? 1 : (total_rows - _row - 1);
    max_rows = min<int32>(printer.get_rows() - 2 - (_row != 0), max_rows);
    for (; max_rows >= 0; --max_rows, ++_row)
    {
        int32 index = vertical ? _row : (_row * columns);
        for (int32 x = columns - 1; x >= 0; --x)
        {
            if (index >= match_count)
                continue;

            // Print the match.
            const char* match = matches.get_displayable(index);
            const char* post_lcd = match + lcd_length;

            StrIter iter(post_lcd);
            iter.next();
            const char* match_tail = iter.get_pointer();

            printer.print(minor_attr, match, lcd_length);
            printer.print(highlight_attr, post_lcd, int32(match_tail - post_lcd));
            printer.print(major_attr, match_tail, int32(strlen(match_tail)));

            // Move the cursor to the next column
            if (x)
            {
                int32 spaces_needed = column_pad;
                if (total_rows > 1)
                {
                    int32 visible_chars = matches.get_cell_count(index);
                    spaces_needed = _longest - visible_chars + column_pad;
                }

                for (int32 i = spaces_needed; i >= 0;)
                {
                    const char spaces[] = "                            ";
                    printer.print(spaces, min<int32>(sizeof_array(spaces) - 1, i));
                    i -= sizeof_array(spaces) - 1;
                }
            }

            index += index_step;
        }

        printer.print("\n");
    }

    if (_row == total_rows)
        return state_none;

    printer.print(g_colour_interact.get(), "-- More --");
    return state_pager;
}

//------------------------------------------------------------------------------
void TabCompleter::on_terminal_resize(int32 columns, int32 rows, const Context& context)
{
}
