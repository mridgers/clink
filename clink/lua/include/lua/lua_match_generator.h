// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <lib/match_generator.h>

class LuaState;

//------------------------------------------------------------------------------
class LuaMatchGenerator
    : public MatchGenerator
{
public:
                    LuaMatchGenerator(LuaState& state);
    virtual         ~LuaMatchGenerator();

private:
    virtual bool    generate(const LineState& line, MatchBuilder& Builder) override;
    virtual int32   get_prefix_length(const LineState& line) const override;
    void            initialise();
    void            print_error(const char* error) const;
    void            lua_pushlinestate(const LineState& line);
    bool            load_script(const char* script);
    void            load_scripts(const char* path);
    LuaState&       _state;
};
