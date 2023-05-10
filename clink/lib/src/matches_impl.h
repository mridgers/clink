// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "matches.h"

#include <vector>

//------------------------------------------------------------------------------
struct MatchInfo
{
    uint16          store_id;
    uint16          displayable_store_id;
    uint16          aux_store_id;
    uint8           cell_count;
    uint8           suffix : 7; // TODO: suffix can be in Store instead of info.
    uint8           select : 1;
};



//------------------------------------------------------------------------------
class MatchStore
{
public:
    const char*             get(uint32 id) const;

protected:
    static const int32      alignment_bits = 1;
    static const int32      alignment = 1 << alignment_bits;
    char*                   _ptr;
    uint32                  _size;
};



//------------------------------------------------------------------------------
class MatchesImpl
    : public Matches
{
public:
                            MatchesImpl(uint32 store_size=0x10000);
    virtual uint32          get_match_count() const override;
    virtual const char*     get_match(uint32 index) const override;
    virtual const char*     get_displayable(uint32 index) const override;
    virtual const char*     get_aux(uint32 index) const override;
    virtual char            get_suffix(uint32 index) const override;
    virtual uint32          get_cell_count(uint32 index) const override;
    virtual bool            has_aux() const override;
    bool                    is_prefix_included() const;
    virtual void            get_match_lcd(StrBase& out) const override;

private:
    friend class            MatchPipeline;
    friend class            MatchBuilder;
    void                    set_prefix_included(bool included);
    bool                    add_match(const MatchDesc& desc);
    uint32                  get_info_count() const;
    MatchInfo*              get_infos();
    const MatchStore&       get_store() const;
    void                    reset();
    void                    coalesce(uint32 count_hint);

private:
    class StoreImpl
        : public MatchStore
    {
    public:
                            StoreImpl(uint32 size);
                            ~StoreImpl();
        void                reset();
        int32               store_front(const char* str);
        int32               store_back(const char* str);

    private:
        uint32              get_size(const char* str) const;
        uint32              _front;
        uint32              _back;
    };

    typedef std::vector<MatchInfo> Infos;

    StoreImpl               _store;
    Infos                   _infos;
    uint16                  _count = 0;
    bool                    _coalesced = false;
    bool                    _has_aux = false;
    bool                    _prefix_included = false;
};
