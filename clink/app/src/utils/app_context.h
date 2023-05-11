// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/singleton.h>

class StrBase;

//------------------------------------------------------------------------------
class AppContext
    : public Singleton<const AppContext>
{
public:
    struct Desc
    {
                Desc();
        bool    quiet = false;
        bool    log = true;
        bool    inherit_id = false;
        char    state_dir[509]; // = {}; (this crashes cl.exe v18.00.21005.1)
    };

                AppContext(const Desc& desc);
    int32       get_id() const;
    bool        is_logging_enabled() const;
    bool        is_quiet() const;
    void        get_binaries_dir(StrBase& out) const;
    void        get_state_dir(StrBase& out) const;
    void        get_log_path(StrBase& out) const;
    void        get_settings_path(StrBase& out) const;
    void        get_history_path(StrBase& out) const;
    void        update_env() const;

private:
    Desc        _desc;
    int32       _id;
};
