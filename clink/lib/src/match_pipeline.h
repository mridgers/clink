// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class LineState;
class MatchGenerator;
class MatchesImpl;
template <typename T> class Array;

//------------------------------------------------------------------------------
class MatchPipeline
{
public:
                        MatchPipeline(MatchesImpl& matches);
    void                reset() const;
    void                generate(const LineState& state, const Array<MatchGenerator*>& generators) const;
    void                fill_info() const;
    void                select(const char* needle) const;
    void                sort() const;

private:
    MatchesImpl&        m_matches;
};
