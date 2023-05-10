// Copyright (c) 2013 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "prompt.h"

#include <core/base.h>
#include <core/str.h>
#include <lua/lua_script_loader.h>
#include <lua/lua_state.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <algorithm>

//------------------------------------------------------------------------------
#define MR(x)                        L##x L"\x08"
const wchar_t* g_prompt_tag          = L"@CLINK_PROMPT";
const wchar_t* g_prompt_tag_hidden   = MR("C") MR("L") MR("I") MR("N") MR("K") MR(" ");
const wchar_t* g_prompt_tags[]       = { g_prompt_tag_hidden, g_prompt_tag };
#undef MR



//------------------------------------------------------------------------------
Prompt::Prompt()
: _data(nullptr)
{
}

//------------------------------------------------------------------------------
Prompt::Prompt(Prompt&& rhs)
: _data(nullptr)
{
    std::swap(_data, rhs._data);
}

//------------------------------------------------------------------------------
Prompt::~Prompt()
{
    clear();
}

//------------------------------------------------------------------------------
Prompt& Prompt::operator = (Prompt&& rhs)
{
    clear();
    std::swap(_data, rhs._data);
    return *this;
}

//------------------------------------------------------------------------------
void Prompt::clear()
{
    if (_data != nullptr)
        free(_data);

    _data = nullptr;
}

//------------------------------------------------------------------------------
const wchar_t* Prompt::get() const
{
    return _data;
}

//------------------------------------------------------------------------------
void Prompt::set(const wchar_t* chars, int char_count)
{
    clear();

    if (chars == nullptr)
        return;

    if (char_count <= 0)
        char_count = int(wcslen(chars));

    _data = (wchar_t*)malloc(sizeof(*_data) * (char_count + 1));
    wcsncpy(_data, chars, char_count);
    _data[char_count] = '\0';
}

//------------------------------------------------------------------------------
bool Prompt::is_set() const
{
    return (_data != nullptr);
}



//------------------------------------------------------------------------------
void TaggedPrompt::set(const wchar_t* chars, int char_count)
{
    clear();

    if (int tag_length = is_tagged(chars, char_count))
        Prompt::set(chars + tag_length, char_count - tag_length);
}

//------------------------------------------------------------------------------
void TaggedPrompt::tag(const wchar_t* value)
{
    clear();

    // Just set 'value' if it is already tagged.
    if (is_tagged(value))
    {
        Prompt::set(value);
        return;
    }

    int length = int(wcslen(value));
    length += int(wcslen(g_prompt_tag_hidden));

    _data = (wchar_t*)malloc(sizeof(*_data) * (length + 1));
    wcscpy(_data, g_prompt_tag_hidden);
    wcscat(_data, value);
}

//------------------------------------------------------------------------------
int TaggedPrompt::is_tagged(const wchar_t* chars, int char_count)
{
    if (char_count <= 0)
        char_count = int(wcslen(chars));

    // For each accepted tag...
    for (int i = 0; i < sizeof_array(g_prompt_tags); ++i)
    {
        const wchar_t* tag = g_prompt_tags[i];
        int tag_length = (int)wcslen(tag);

        if (tag_length > char_count)
            continue;

        // Found a match? Store it the prompt, minus the tag.
        if (wcsncmp(chars, tag, tag_length) == 0)
            return tag_length;
    }

    return 0;
}



//------------------------------------------------------------------------------
PromptFilter::PromptFilter(LuaState& lua)
: _lua(lua)
{
    lua_load_script(lua, app, prompt);
}

//------------------------------------------------------------------------------
void PromptFilter::filter(const char* in, StrBase& out)
{
    lua_State* state = _lua.get_state();

    // Call Lua to filter prompt
    lua_getglobal(state, "clink");
    lua_pushliteral(state, "_filter_prompt");
    lua_rawget(state, -2);

    lua_pushstring(state, in);

    if (lua_pcall(state, 1, 1, 0) != 0)
    {
        puts(lua_tostring(state, -1));
        lua_pop(state, 2);
        return;
    }

    // Collect the filtered prompt.
    const char* prompt = lua_tostring(state, -1);
    out = prompt;

    lua_pop(state, 2);
}



//------------------------------------------------------------------------------
Prompt PromptUtils::extract_from_console()
{
    // Find where the cursor is. This will be the end of the prompt to extract.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(handle, &csbi) == FALSE)
        return Prompt();

    // Work out prompt length.
    COORD cursorXy = csbi.dwCursorPosition;
    unsigned int length = cursorXy.X;
    cursorXy.X = 0;

    wchar_t buffer[256] = {};
    if (length >= sizeof_array(buffer))
        return Prompt();

    // Get the prompt from the Terminal.
    DWORD chars_in;
    if (!ReadConsoleOutputCharacterW(handle, buffer, length, cursorXy, &chars_in))
        return Prompt();

    buffer[chars_in] = '\0';

    // Wrap in a prompt object and return.
    Prompt ret;
    ret.set(buffer);
    return ret;
}
