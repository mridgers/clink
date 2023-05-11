// Copyright (c) Martin Ridgers
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
static uint32 normal_selector(
    const char* needle,
    const MatchStore& Store,
    MatchInfo* infos,
    int32 count)
{
    int32 select_count = 0;
    for (int32 i = 0; i < count; ++i)
    {
        const char* name = Store.get(infos[i].store_id);
        int32 j = str_compare(needle, name);
        infos[i].select = (j < 0 || !needle[j]);
        ++select_count;
    }

    return select_count;
}

//------------------------------------------------------------------------------
static void alpha_sorter(const MatchStore& Store, MatchInfo* infos, int32 count)
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
: _matches(matches)
{
}

//------------------------------------------------------------------------------
void MatchPipeline::reset() const
{
    _matches.reset();
}

//------------------------------------------------------------------------------
void MatchPipeline::generate(
    const LineState& state,
    const Array<MatchGenerator*>& generators) const
{
    MatchBuilder builder(_matches);
    for (auto* generator : generators)
        if (generator->generate(state, builder))
            break;
}

//------------------------------------------------------------------------------
void MatchPipeline::fill_info() const
{
    int32 count = _matches.get_info_count();
    if (!count)
        return;

    MatchInfo* info = _matches.get_infos();
    for (int32 i = 0; i < count; ++i, ++info)
    {
        const char* displayable = _matches.get_displayable(i);
        info->cell_count = cell_count(displayable);
    }
}

//------------------------------------------------------------------------------
void MatchPipeline::select(const char* needle) const
{
    int32 count = _matches.get_info_count();
    if (!count)
        return;

    uint32 selected_count = 0;
    selected_count = normal_selector(needle, _matches.get_store(),
        _matches.get_infos(), count);

    _matches.coalesce(selected_count);
}

//------------------------------------------------------------------------------
void MatchPipeline::sort() const
{
    int32 count = _matches.get_match_count();
    if (!count)
        return;

    alpha_sorter(_matches.get_store(), _matches.get_infos(), count);
}
