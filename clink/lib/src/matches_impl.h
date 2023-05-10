// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "matches.h"

#include <vector>

//------------------------------------------------------------------------------
struct MatchInfo
{
    unsigned short  store_id;
    unsigned short  displayable_store_id;
    unsigned short  aux_store_id;
    unsigned char   cell_count;
    unsigned char   suffix : 7; // TODO: suffix can be in Store instead of info.
    unsigned char   select : 1;
};



//------------------------------------------------------------------------------
class MatchStore
{
public:
    const char*             get(unsigned int id) const;

protected:
    static const int        alignment_bits = 1;
    static const int        alignment = 1 << alignment_bits;
    char*                   _ptr;
    unsigned int            _size;
};



//------------------------------------------------------------------------------
class MatchesImpl
    : public Matches
{
public:
                            MatchesImpl(unsigned int store_size=0x10000);
    virtual unsigned int    get_match_count() const override;
    virtual const char*     get_match(unsigned int index) const override;
    virtual const char*     get_displayable(unsigned int index) const override;
    virtual const char*     get_aux(unsigned int index) const override;
    virtual char            get_suffix(unsigned int index) const override;
    virtual unsigned int    get_cell_count(unsigned int index) const override;
    virtual bool            has_aux() const override;
    bool                    is_prefix_included() const;
    virtual void            get_match_lcd(StrBase& out) const override;

private:
    friend class            MatchPipeline;
    friend class            MatchBuilder;
    void                    set_prefix_included(bool included);
    bool                    add_match(const MatchDesc& desc);
    unsigned int            get_info_count() const;
    MatchInfo*              get_infos();
    const MatchStore&       get_store() const;
    void                    reset();
    void                    coalesce(unsigned int count_hint);

private:
    class StoreImpl
        : public MatchStore
    {
    public:
                            StoreImpl(unsigned int size);
                            ~StoreImpl();
        void                reset();
        int                 store_front(const char* str);
        int                 store_back(const char* str);

    private:
        unsigned int        get_size(const char* str) const;
        unsigned int        _front;
        unsigned int        _back;
    };

    typedef std::vector<MatchInfo> Infos;

    StoreImpl               _store;
    Infos                   _infos;
    unsigned short          _count = 0;
    bool                    _coalesced = false;
    bool                    _has_aux = false;
    bool                    _prefix_included = false;
};
