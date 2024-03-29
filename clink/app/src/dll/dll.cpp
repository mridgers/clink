// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "host/host_cmd.h"
#include "utils/app_context.h"
#include "utils/seh_scope.h"
#include "version.h"

#include <core/base.h>
#include <core/globber.h>
#include <core/log.h>
#include <core/os.h>
#include <core/path.h>
#include <core/settings.h>
#include <core/str.h>
#include <core/str_tokeniser.h>

//------------------------------------------------------------------------------
const char* g_clink_header =
    "Clink v" CLINK_VERSION_STR " / "
    "Copyright (c) Martin Ridgers\n"
    "http://mridgers.github.io/clink\n"
    ;



//------------------------------------------------------------------------------
static Host* g_host = nullptr;



//------------------------------------------------------------------------------
static void success()
{
    if (!AppContext::get()->is_quiet())
        puts(g_clink_header);
}

//------------------------------------------------------------------------------
static void failed()
{
    Str<280> buffer;
    AppContext::get()->get_state_dir(buffer);
    fprintf(stderr, "Failed to load Clink.\nSee log for details (%s).\n", buffer.c_str());
}

//------------------------------------------------------------------------------
static bool get_host_name(StrBase& out)
{
    Str<MAX_PATH> buffer;
    if (GetModuleFileName(nullptr, buffer.data(), buffer.size()) == buffer.size())
        return false;

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        return false;

    return path::get_name(buffer.c_str(), out);
}

//------------------------------------------------------------------------------
static void shutdown_clink()
{
    SehScope seh;

    if (g_host != nullptr)
    {
        g_host->shutdown();
        delete g_host;
        g_host = nullptr;
    }

    if (Logger* Logger = Logger::get())
        delete Logger;

    delete AppContext::get();
}

//------------------------------------------------------------------------------
bool __stdcall initialise_clink(const AppContext::Desc& app_desc)
{
    SehScope seh;

    auto* app_ctx = new AppContext(app_desc);

    // Start a log file.
    if (app_ctx->is_logging_enabled())
    {
        Str<256> log_path;
        app_ctx->get_log_path(log_path);
        new FileLogger(log_path.c_str());
    }

    // What Process is the DLL loaded into?
    Str<64> host_name;
    if (!get_host_name(host_name))
        return false;

    LOG("Host Process is '%s'", host_name.c_str());

    // Search for a supported Host.
    struct {
        const char* name;
        Host*       (*creator)();
    } hosts[] = {
        { "cmd.exe", []() -> Host* { return new HostCmd(); } },
    };

    for (int32 i = 0; i < sizeof_array(hosts); ++i)
        if (stricmp(host_name.c_str(), hosts[i].name) == 0)
            if (g_host = (hosts[i].creator)())
                break;

    // Bail out if this isn't a supported Host.
    if (g_host == nullptr)
    {
        LOG("Unknown Host.");
        return false;
    }

    // Validate and initialise.
    if (!g_host->validate())
    {
        LOG("Host validation failed.");
        return false;
    }

    if (!g_host->initialise())
    {
        failed();
        return false;
    }

    atexit(shutdown_clink);

    success();
    return true;
}
