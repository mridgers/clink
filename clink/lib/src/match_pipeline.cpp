// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "match_pipeline.h"
#include "line_state.h"
#include "match_generator.h"
#include "match_pipeline.h"
#include "matches_impl.h"

#include <core/array.h>
#include <core/str_compare.h>
#include <terminal/ecma48_iter.h>

#include <algorithm>

//------------------------------------------------------------------------------
static unsigned int normal_selector(
    const char* needle,
    const MatchStore& Store,
    MatchInfo* infos,
    int count)
{
    int select_count = 0;
    for (int i = 0; i < count; ++i)
    {
        const char* name = Store.get(infos[i].store_id);
        int j = str_compare(needle, name);
        infos[i].select = (j < 0 || !needle[j]);
        ++select_count;
    }

    return select_count;
}

//------------------------------------------------------------------------------
static void alpha_sorter(const MatchStore& Store, MatchInfo* infos, int count)
{
    auto predicate = [&] (const MatchInfo& lhs, const MatchInfo& rhs) {
        const char* l = Store.get(lhs.store_id);
        const char* r = Store.get(rhs.store_id);
        return (stricmp(l, r) < 0);
    };

    std::sort(infos, infos + count, predicate);
}



//------------------------------------------------------------------------------
MatchPipeline::MatchPipeline(MatchesImpl& matches)
: m_matches(matches)
{
}

//------------------------------------------------------------------------------
void MatchPipeline::reset() const
{
    m_matches.reset();
}

//------------------------------------------------------------------------------
void MatchPipeline::generate(
    const LineState& state,
    const Array<MatchGenerator*>& generators) const
{
    MatchBuilder builder(m_matches);
    for (auto* generator : generators)
        if (generator->generate(state, builder))
            break;
}

//------------------------------------------------------------------------------
void MatchPipeline::fill_info() const
{
    int count = m_matches.get_info_count();
    if (!count)
        return;

    MatchInfo* info = m_matches.get_infos();
    for (int i = 0; i < count; ++i, ++info)
    {
        const char* displayable = m_matches.get_displayable(i);
        info->cell_count = cell_count(displayable);
    }
}

//------------------------------------------------------------------------------
void MatchPipeline::select(const char* needle) const
{
    int count = m_matches.get_info_count();
    if (!count)
        return;

    unsigned int selected_count = 0;
    selected_count = normal_selector(needle, m_matches.get_store(),
        m_matches.get_infos(), count);

    m_matches.coalesce(selected_count);
}

//------------------------------------------------------------------------------
void MatchPipeline::sort() const
{
    int count = m_matches.get_match_count();
    if (!count)
        return;

    alpha_sorter(m_matches.get_store(), m_matches.get_infos(), count);
}
