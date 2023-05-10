// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class StrBase;

//------------------------------------------------------------------------------
class Matches
{
public:
    virtual uint32          get_match_count() const = 0;
    virtual const char*     get_match(uint32 index) const = 0;
    virtual const char*     get_displayable(uint32 index) const = 0;
    virtual const char*     get_aux(uint32 index) const = 0;
    virtual char            get_suffix(uint32 index) const = 0;
    virtual uint32          get_cell_count(uint32 index) const = 0;
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
    Matches&                _matches;
};
