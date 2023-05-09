// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/str_compare.h>
#include <lib/line_editor.h>
#include <lib/match_generator.h>

//------------------------------------------------------------------------------
int testbed(int, char**)
{
    StrCompareScope _(StrCompareScope::relaxed);

    EditorModule* completer = tab_completer_create();

    LineEditor::Desc desc;
    LineEditor* editor = line_editor_create(desc);
    editor->add_module(*completer);
    editor->add_generator(file_match_generator());

    char out[64];
    while (editor->edit(out, sizeof_array(out)));

    line_editor_destroy(editor);
    tab_completer_destroy(completer);
    return 0;
}
