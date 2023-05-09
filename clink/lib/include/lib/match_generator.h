// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class LineState;
class MatchBuilder;

//------------------------------------------------------------------------------
class MatchGenerator
{
public:
    virtual bool    generate(const LineState& line, MatchBuilder& Builder) = 0;
    virtual int     get_prefix_length(const LineState& line) const = 0;

private:
};

MatchGenerator& file_match_generator();
