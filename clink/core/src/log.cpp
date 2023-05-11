// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "log.h"

#include <stdarg.h>

//------------------------------------------------------------------------------
Logger::~Logger()
{
}

//------------------------------------------------------------------------------
void Logger::info(const char* function, int32 line, const char* fmt, ...)
{
    Logger* instance = Logger::get();
    if (instance == nullptr)
        return;

    va_list args;
    va_start(args, fmt);
    instance->emit(function, line, fmt, args);
    va_end(args);
}

//------------------------------------------------------------------------------
void Logger::error(const char* function, int32 line, const char* fmt, ...)
{
    Logger* instance = Logger::get();
    if (instance == nullptr)
        return;

    DWORD last_error = GetLastError();

    va_list args;
    va_start(args, fmt);

    Logger::info(function, line, "*** ERROR ***");
    instance->emit(function, line, fmt, args);
    Logger::info(function, line, "(last error = %d)", last_error);

    va_end(args);
}



//------------------------------------------------------------------------------
FileLogger::FileLogger(const char* log_path)
{
    _log_path << log_path;
}

//------------------------------------------------------------------------------
void FileLogger::emit(const char* function, int32 line, const char* fmt, va_list args)
{
    FILE* file;

    file = fopen(_log_path.c_str(), "at");
    if (file == nullptr)
        return;

    Str<24> func_name;
    func_name << function;

    DWORD pid = GetCurrentProcessId();

    Str<256> buffer;
    buffer.format("%04x %-24s %4d ", pid, func_name.c_str(), line);
    fputs(buffer.c_str(), file);
    vfprintf(file, fmt, args);
    fputs("\n", file);

    fclose(file);
}
