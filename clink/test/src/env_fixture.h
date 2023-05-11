// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class EnvFixture
{
public:
                    EnvFixture(const char** env);
                    ~EnvFixture();

protected:
    void            convert_eq_to_null(wchar_t* env_strings);
    void            clear();
    wchar_t*        _env_strings;
};
