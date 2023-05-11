// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class SehScope
{
public:
                SehScope();
                ~SehScope();

private:
    void*       _prev_filter;
};
