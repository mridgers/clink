// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "matches_impl.h"

#include <core/base.h>
#include <core/str.h>
#include <core/str_compare.h>

//------------------------------------------------------------------------------
MatchBuilder::MatchBuilder(Matches& matches)
: _matches(matches)
{
}

//------------------------------------------------------------------------------
bool MatchBuilder::add_match(const char* match)
{
    MatchDesc desc = { match };
    return add_match(desc);
}

//------------------------------------------------------------------------------
bool MatchBuilder::add_match(const MatchDesc& desc)
{
    return ((MatchesImpl&)_matches).add_match(desc);
}

//------------------------------------------------------------------------------
void MatchBuilder::set_prefix_included(bool included)
{
    return ((MatchesImpl&)_matches).set_prefix_included(included);
}



//------------------------------------------------------------------------------
const char* MatchStore::get(unsigned int id) const
{
    id <<= alignment_bits;
    return (id < _size) ? (_ptr + id) : nullptr;
}



//------------------------------------------------------------------------------
MatchesImpl::StoreImpl::StoreImpl(unsigned int size)
: _front(0)
, _back(size)
{
    _size = size;
    _ptr = (char*)malloc(size);
}

//------------------------------------------------------------------------------
MatchesImpl::StoreImpl::~StoreImpl()
{
    free(_ptr);
}

//------------------------------------------------------------------------------
void MatchesImpl::StoreImpl::reset()
{
    _back = _size;
    _front = 0;
}

//------------------------------------------------------------------------------
int MatchesImpl::StoreImpl::store_front(const char* str)
{
    unsigned int size = get_size(str);
    unsigned int next = _front + size;
    if (next > _back)
        return -1;

    StrBase(_ptr + _front, size).copy(str);

    unsigned int ret = _front;
    _front = next;
    return ret >> alignment_bits;
}

//------------------------------------------------------------------------------
int MatchesImpl::StoreImpl::store_back(const char* str)
{
    unsigned int size = get_size(str);
    unsigned int next = _back - size;
    if (next < _front)
        return -1;

    _back = next;
    StrBase(_ptr + _back, size).copy(str);

    return _back >> alignment_bits;
}

//------------------------------------------------------------------------------
unsigned int MatchesImpl::StoreImpl::get_size(const char* str) const
{
    if (str == nullptr || str[0] == '\0')
        return ~0u;

    int length = int(strlen(str) + 1);
    length = (length + alignment - 1) & ~(alignment - 1);
    return length;
}



//------------------------------------------------------------------------------
MatchesImpl::MatchesImpl(unsigned int store_size)
: _store(min(store_size, 0x10000u))
{
    _infos.reserve(1023);
}

//------------------------------------------------------------------------------
unsigned int MatchesImpl::get_info_count() const
{
    return int(_infos.size());
}

//------------------------------------------------------------------------------
MatchInfo* MatchesImpl::get_infos()
{
    return &(_infos[0]);
}

//------------------------------------------------------------------------------
const MatchStore& MatchesImpl::get_store() const
{
    return _store;
}

//------------------------------------------------------------------------------
unsigned int MatchesImpl::get_match_count() const
{
    return _count;
}

//------------------------------------------------------------------------------
const char* MatchesImpl::get_match(unsigned int index) const
{
    if (index >= get_match_count())
        return nullptr;

    unsigned int store_id = _infos[index].store_id;
    return _store.get(store_id);
}

//------------------------------------------------------------------------------
const char* MatchesImpl::get_displayable(unsigned int index) const
{
    if (index >= get_match_count())
        return nullptr;

    unsigned int store_id = _infos[index].displayable_store_id;
    if (!store_id)
        store_id = _infos[index].store_id;

    return _store.get(store_id);
}

//------------------------------------------------------------------------------
const char* MatchesImpl::get_aux(unsigned int index) const
{
    if (index >= get_match_count())
        return nullptr;

    if (unsigned int store_id = _infos[index].aux_store_id)
        return _store.get(store_id);

    return nullptr;
}

//------------------------------------------------------------------------------
char MatchesImpl::get_suffix(unsigned int index) const
{
    if (index >= get_match_count())
        return 0;

    return _infos[index].suffix;
}

//------------------------------------------------------------------------------
unsigned int MatchesImpl::get_cell_count(unsigned int index) const
{
    return (index < get_match_count()) ? _infos[index].cell_count : 0;
}

//------------------------------------------------------------------------------
bool MatchesImpl::has_aux() const
{
    return _has_aux;
}

//------------------------------------------------------------------------------
void MatchesImpl::get_match_lcd(StrBase& out) const
{
    int match_count = get_match_count();

    if (match_count <= 0)
        return;

    if (match_count == 1)
    {
        out = get_match(0);
        return;
    }

    out = get_match(0);
    int lcd_length = out.length();

    int cmp_mode = StrCompareScope::current();
    StrCompareScope _(min(cmp_mode, int(StrCompareScope::caseless)));

    for (int i = 1, n = get_match_count(); i < n; ++i)
    {
        const char* match = get_match(i);
        int d = str_compare(match, out.c_str());
        if (d >= 0)
            lcd_length = min(d, lcd_length);
    }

    out.truncate(lcd_length);
}

//------------------------------------------------------------------------------
bool MatchesImpl::is_prefix_included() const
{
    return _prefix_included;
}

//------------------------------------------------------------------------------
void MatchesImpl::reset()
{
    _store.reset();
    _infos.clear();
    _coalesced = false;
    _count = 0;
    _has_aux = false;
    _prefix_included = false;
}

//------------------------------------------------------------------------------
void MatchesImpl::set_prefix_included(bool included)
{
    _prefix_included = included;
}

//------------------------------------------------------------------------------
bool MatchesImpl::add_match(const MatchDesc& desc)
{
    const char* match = desc.match;

    if (_coalesced || match == nullptr || !*match)
        return false;

    int store_id = _store.store_front(match);
    if (store_id < 0)
        return false;

    int displayable_store_id = 0;
    if (desc.displayable != nullptr)
        displayable_store_id = max(0, _store.store_back(desc.displayable));

    int aux_store_id = 0;
    if (_has_aux = (desc.aux != nullptr))
        aux_store_id = max(0, _store.store_back(desc.aux));

    _infos.push_back({
        (unsigned short)store_id,
        (unsigned short)displayable_store_id,
        (unsigned short)aux_store_id,
        0,
        max<unsigned char>(0, desc.suffix),
    });
    ++_count;
    return true;
}

//------------------------------------------------------------------------------
void MatchesImpl::coalesce(unsigned int count_hint)
{
    MatchInfo* infos = &(_infos[0]);

    unsigned int j = 0;
    for (int i = 0, n = int(_infos.size()); i < n && j < count_hint; ++i)
    {
        if (!infos[i].select)
            continue;

        if (i != j)
        {
            MatchInfo temp = infos[j];
            infos[j] = infos[i];
            infos[i] = temp;
        }
        ++j;
    }

    _count = j;
    _coalesced = true;
}
