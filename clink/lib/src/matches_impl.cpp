// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "matches_impl.h"

#include <core/base.h>
#include <core/str.h>
#include <core/str_compare.h>

//------------------------------------------------------------------------------
MatchBuilder::MatchBuilder(Matches& matches)
: m_matches(matches)
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
    return ((MatchesImpl&)m_matches).add_match(desc);
}

//------------------------------------------------------------------------------
void MatchBuilder::set_prefix_included(bool included)
{
    return ((MatchesImpl&)m_matches).set_prefix_included(included);
}



//------------------------------------------------------------------------------
const char* MatchStore::get(unsigned int id) const
{
    id <<= alignment_bits;
    return (id < m_size) ? (m_ptr + id) : nullptr;
}



//------------------------------------------------------------------------------
MatchesImpl::StoreImpl::StoreImpl(unsigned int size)
: m_front(0)
, m_back(size)
{
    m_size = size;
    m_ptr = (char*)malloc(size);
}

//------------------------------------------------------------------------------
MatchesImpl::StoreImpl::~StoreImpl()
{
    free(m_ptr);
}

//------------------------------------------------------------------------------
void MatchesImpl::StoreImpl::reset()
{
    m_back = m_size;
    m_front = 0;
}

//------------------------------------------------------------------------------
int MatchesImpl::StoreImpl::store_front(const char* str)
{
    unsigned int size = get_size(str);
    unsigned int next = m_front + size;
    if (next > m_back)
        return -1;

    StrBase(m_ptr + m_front, size).copy(str);

    unsigned int ret = m_front;
    m_front = next;
    return ret >> alignment_bits;
}

//------------------------------------------------------------------------------
int MatchesImpl::StoreImpl::store_back(const char* str)
{
    unsigned int size = get_size(str);
    unsigned int next = m_back - size;
    if (next < m_front)
        return -1;

    m_back = next;
    StrBase(m_ptr + m_back, size).copy(str);

    return m_back >> alignment_bits;
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
: m_store(min(store_size, 0x10000u))
{
    m_infos.reserve(1023);
}

//------------------------------------------------------------------------------
unsigned int MatchesImpl::get_info_count() const
{
    return int(m_infos.size());
}

//------------------------------------------------------------------------------
MatchInfo* MatchesImpl::get_infos()
{
    return &(m_infos[0]);
}

//------------------------------------------------------------------------------
const MatchStore& MatchesImpl::get_store() const
{
    return m_store;
}

//------------------------------------------------------------------------------
unsigned int MatchesImpl::get_match_count() const
{
    return m_count;
}

//------------------------------------------------------------------------------
const char* MatchesImpl::get_match(unsigned int index) const
{
    if (index >= get_match_count())
        return nullptr;

    unsigned int store_id = m_infos[index].store_id;
    return m_store.get(store_id);
}

//------------------------------------------------------------------------------
const char* MatchesImpl::get_displayable(unsigned int index) const
{
    if (index >= get_match_count())
        return nullptr;

    unsigned int store_id = m_infos[index].displayable_store_id;
    if (!store_id)
        store_id = m_infos[index].store_id;

    return m_store.get(store_id);
}

//------------------------------------------------------------------------------
const char* MatchesImpl::get_aux(unsigned int index) const
{
    if (index >= get_match_count())
        return nullptr;

    if (unsigned int store_id = m_infos[index].aux_store_id)
        return m_store.get(store_id);

    return nullptr;
}

//------------------------------------------------------------------------------
char MatchesImpl::get_suffix(unsigned int index) const
{
    if (index >= get_match_count())
        return 0;

    return m_infos[index].suffix;
}

//------------------------------------------------------------------------------
unsigned int MatchesImpl::get_cell_count(unsigned int index) const
{
    return (index < get_match_count()) ? m_infos[index].cell_count : 0;
}

//------------------------------------------------------------------------------
bool MatchesImpl::has_aux() const
{
    return m_has_aux;
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
    return m_prefix_included;
}

//------------------------------------------------------------------------------
void MatchesImpl::reset()
{
    m_store.reset();
    m_infos.clear();
    m_coalesced = false;
    m_count = 0;
    m_has_aux = false;
    m_prefix_included = false;
}

//------------------------------------------------------------------------------
void MatchesImpl::set_prefix_included(bool included)
{
    m_prefix_included = included;
}

//------------------------------------------------------------------------------
bool MatchesImpl::add_match(const MatchDesc& desc)
{
    const char* match = desc.match;

    if (m_coalesced || match == nullptr || !*match)
        return false;

    int store_id = m_store.store_front(match);
    if (store_id < 0)
        return false;

    int displayable_store_id = 0;
    if (desc.displayable != nullptr)
        displayable_store_id = max(0, m_store.store_back(desc.displayable));

    int aux_store_id = 0;
    if (m_has_aux = (desc.aux != nullptr))
        aux_store_id = max(0, m_store.store_back(desc.aux));

    m_infos.push_back({
        (unsigned short)store_id,
        (unsigned short)displayable_store_id,
        (unsigned short)aux_store_id,
        0,
        max<unsigned char>(0, desc.suffix),
    });
    ++m_count;
    return true;
}

//------------------------------------------------------------------------------
void MatchesImpl::coalesce(unsigned int count_hint)
{
    MatchInfo* infos = &(m_infos[0]);

    unsigned int j = 0;
    for (int i = 0, n = int(m_infos.size()); i < n && j < count_hint; ++i)
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

    m_count = j;
    m_coalesced = true;
}
