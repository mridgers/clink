// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class StrBase;

//------------------------------------------------------------------------------
class Matches
{
public:
    virtual unsigned int    get_match_count() const = 0;
    virtual const char*     get_match(unsigned int index) const = 0;
    virtual const char*     get_displayable(unsigned int index) const = 0;
    virtual const char*     get_aux(unsigned int index) const = 0;
    virtual char            get_suffix(unsigned int index) const = 0;
    virtual unsigned int    get_cell_count(unsigned int index) const = 0;
    virtual bool            has_aux() const = 0;
    virtual void            get_match_lcd(StrBase& out) const = 0;
};



//------------------------------------------------------------------------------
struct MatchDesc
{
    const char*             match;
    const char*             displayable;
    const char*             aux;
    char                    suffix;
};

//------------------------------------------------------------------------------
class MatchBuilder
{
public:
                            MatchBuilder(Matches& matches);
    bool                    add_match(const char* match);
    bool                    add_match(const MatchDesc& desc);
    void                    set_prefix_included(bool included=true);

private:
    Matches&                m_matches;
};
