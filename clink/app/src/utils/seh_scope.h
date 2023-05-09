// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class SehScope
{
public:
                SehScope();
                ~SehScope();

private:
    void*       m_prev_filter;
};
