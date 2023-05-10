// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include "str.h"
#include "singleton.h"

//------------------------------------------------------------------------------
#define LOG(...)    Logger::info(__FUNCTION__, __LINE__, __VA_ARGS__)
#define ERR(...)    Logger::error(__FUNCTION__, __LINE__, __VA_ARGS__)

//------------------------------------------------------------------------------
class Logger
    : public Singleton<Logger>
{
public:
    virtual         ~Logger();
    static void     info(const char* function, int32 line, const char* fmt, ...);
    static void     error(const char* function, int32 line, const char* fmt, ...);

protected:
    virtual void    emit(const char* function, int32 line, const char* fmt, va_list args) = 0;
};

//------------------------------------------------------------------------------
class FileLogger
    : public Logger
{
public:
                    FileLogger(const char* log_path);
    virtual void    emit(const char* function, int32 line, const char* fmt, va_list args) override;

private:
    Str<256>        _log_path;
};
