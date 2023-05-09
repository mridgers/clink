// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class LuaState;
class StrBase;

//------------------------------------------------------------------------------
class Prompt
{
public:
                    Prompt();
                    Prompt(Prompt&& rhs);
                    Prompt(const Prompt& rhs) = delete;
                    ~Prompt();
    Prompt&         operator = (Prompt&& rhs);
    Prompt&         operator = (const Prompt& rhs) = delete;
    void            clear();
    const wchar_t*  get() const;
    void            set(const wchar_t* chars, int char_count=0);
    bool            is_set() const;

protected:
    wchar_t*        m_data;
};

//------------------------------------------------------------------------------
class TaggedPrompt
    : public Prompt
{
public:
    void            set(const wchar_t* chars, int char_count=0);
    void            tag(const wchar_t* value);

private:
    int             is_tagged(const wchar_t* chars, int char_count=0);
};

//------------------------------------------------------------------------------
class PromptFilter
{
public:
                    PromptFilter(LuaState& lua);
    void            filter(const char* in, StrBase& out);

private:
    LuaState&       m_lua;
};

//------------------------------------------------------------------------------
class PromptUtils
{
public:
    static Prompt   extract_from_console();
};
