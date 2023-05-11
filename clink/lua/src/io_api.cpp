// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"

#include <core/base.h>
#include <core/str.h>

#include <new.h>

//------------------------------------------------------------------------------
class HandleIo
{
public:
                    HandleIo(HANDLE h) : _handle(h) {}
                    ~HandleIo()         { close(); }
    bool            is_valid() const    { return _handle != nullptr; }
    void            close();

protected:
    HANDLE          _handle;
};

//------------------------------------------------------------------------------
void HandleIo::close()
{
    if (_handle == nullptr)
        return;

    CloseHandle(_handle);
    _handle = nullptr;
}



//------------------------------------------------------------------------------
class HandleReader
    : public HandleIo
{
public:
                        HandleReader(HANDLE h) : HandleIo(h) {}
                        ~HandleReader();
    bool                get(uint32 index, int32& c);
    const char*         get_pointer() const;
    void                consume(uint32 size);
    uint32              read(uint32 size);

private:
    bool                acquire();
    static const int32  BUFFER_SIZE = 8192;
    char*               _buffer = nullptr;
    uint32              _buffer_size = 0;
    uint32              _data_size = 0;
    uint32              _cursor = 0;
};

//------------------------------------------------------------------------------
HandleReader::~HandleReader()
{
    free(_buffer);
}

//------------------------------------------------------------------------------
bool HandleReader::get(uint32 index, int32& c)
{
    index += _cursor;
    while (index >= _data_size)
        if (!acquire())
            return false;

    c = uint32(_buffer[index]);
    return true;
}

//------------------------------------------------------------------------------
const char* HandleReader::get_pointer() const
{
    return _buffer + _cursor;
}

//------------------------------------------------------------------------------
void HandleReader::consume(uint32 size)
{
    _cursor = min(_data_size, _cursor + size);
    if (_cursor == _data_size)
        _cursor = _data_size = 0;
}

//------------------------------------------------------------------------------
uint32 HandleReader::read(uint32 size)
{
    while (_data_size - _cursor < size)
        if (!acquire())
            break;

    return _data_size - _cursor;
}

//------------------------------------------------------------------------------
bool HandleReader::acquire()
{
    if (!is_valid())
        return false;

    uint32 remaining = _buffer_size - _data_size;
    if (remaining == 0)
    {
        remaining = BUFFER_SIZE;
        _buffer_size += BUFFER_SIZE;
        _buffer = (char*)realloc(_buffer, _buffer_size);
    }

    DWORD bytes_read = 0;
    char* dest = _buffer + _data_size;
    BOOL ok = ReadFile(_handle, dest, DWORD(remaining), &bytes_read, nullptr);
    if (ok == FALSE)
    {
        close();
        return false;
    }

    _data_size += bytes_read;
    return true;
}



//------------------------------------------------------------------------------
class HandleWriter
    : public HandleIo
{
public:
                HandleWriter(HANDLE h) : HandleIo(h) {}
    void        write(const char* data, uint32 size);
};

//------------------------------------------------------------------------------
void HandleWriter::write(const char* data, uint32 size)
{
    DWORD written;
    if (WriteFile(_handle, data, DWORD(size), &written, nullptr) == FALSE)
        close();
}



//------------------------------------------------------------------------------
class Popen2Lua
{
public:
                    Popen2Lua(HANDLE job, HANDLE read, HANDLE write);
                    ~Popen2Lua();
    int32           read(lua_State* state);
    int32           lines(lua_State* state);
    int32           write(lua_State* state);

private:
    int32           read_line(lua_State* state, bool include_eol);
    int32           read(lua_State* state, uint32 bytes);
    HANDLE          _job;
    HandleReader    _reader;
    HandleWriter    _writer;
};

//------------------------------------------------------------------------------
Popen2Lua::Popen2Lua(HANDLE job, HANDLE read, HANDLE write)
: _job(job)
, _writer(write)
, _reader(read)
{
}

//------------------------------------------------------------------------------
Popen2Lua::~Popen2Lua()
{
    if (_job != nullptr)
        CloseHandle(_job);
}

//------------------------------------------------------------------------------
int32 Popen2Lua::read_line(lua_State* state, bool include_eol)
{
    uint32 count = 0;
    for (int32 c; _reader.get(count, c); )
    {
        ++count;
        if (c == '\n')
            break;
    }

    if (count == 0 && !_reader.is_valid())
        return 0;

    uint32 size = count;
    const char* data = _reader.get_pointer();

    if (!include_eol)
    {
        size -= size && (data[size - 1] == '\n');
        size -= size && (data[size - 1] == '\r');
    }

    lua_pushlstring(state, data, size);

    _reader.consume(count);
    return 1;
}

//------------------------------------------------------------------------------
int32 Popen2Lua::read(lua_State* state, uint32 bytes)
{
    uint32 size = _reader.read(bytes);
    const char* data = _reader.get_pointer();
    lua_pushlstring(state, data, size);
    _reader.consume(size);
    return 1;
}

//------------------------------------------------------------------------------
int32 Popen2Lua::read(lua_State* state)
{
    if (!_reader.is_valid())
        return 0;

    int32 arg_count = lua_gettop(state);
    if (arg_count >= 2)
    {
        if (lua_isnumber(state, 2))
        {
            uint32 size = uint32(lua_tointeger(state, 2));
            return read(state, size);
        }

        if (lua_isstring(state, 2))
        {
            const char* read_mode = lua_tostring(state, 2);
            if (*read_mode == 'a')  return read(state, ~0u);
            if (*read_mode == 'l')  return read_line(state, false);
            if (*read_mode == 'L')  return read_line(state, true);
            return 0;
        }
    }

    return read_line(state, false);
}

//------------------------------------------------------------------------------
int32 Popen2Lua::lines(lua_State* state)
{
    if (!_reader.is_valid())
        return 0;

    auto impl = [] (lua_State* state) -> int32 {
        int32 self_index = lua_upvalueindex(1);
        auto* self = (Popen2Lua*)lua_touserdata(state, self_index);
        return self->read_line(state, false);
    };

    lua_pushvalue(state, 1);
    lua_pushcclosure(state, impl, 1);
    return 1;
}

//------------------------------------------------------------------------------
int32 Popen2Lua::write(lua_State* state)
{
    if (!_writer.is_valid())
        return 0;

    int32 arg_count = lua_gettop(state);
    if (arg_count < 2 || lua_isnil(state, 2))
    {
        _writer.close();
        return 0;
    }

    size_t bytes;
    const char* data = lua_tolstring(state, 2, &bytes);
    if (data == nullptr)
        return 0;

    _writer.write(data, uint32(bytes));
    return 0;
}



//------------------------------------------------------------------------------
/// -name:  io.popen2
/// -arg:   command:string
/// -ret:   string
static int32 popen2(lua_State* state)
{
    // Get the command line to execute.
    if (lua_gettop(state) < 1 || !lua_isstring(state, 1))
        return 0;

    // Create inheritable pipes
    union pipe_handles
    {
        ~pipe_handles() { for (HANDLE h : handles) { if (h) CloseHandle(h); } }

        struct {
            HANDLE  stdout_read;
            HANDLE  stdout_write;
            HANDLE  stdin_read;
            HANDLE  stdin_write;
        };
        HANDLE      handles[4];
    };

    pipe_handles pipes;

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    for (int32 i = 0; i < sizeof_array(pipes.handles); i += 2)
        if (!CreatePipe(&pipes.handles[i], &pipes.handles[i + 1], &sa, 0))
            return 0;

    // Launch the Process.
    STARTUPINFOW si = { sizeof(si) };
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = pipes.stdout_write;
    si.hStdInput = pipes.stdin_read;
    si.dwFlags = STARTF_USESTDHANDLES;

    PROCESS_INFORMATION  pi;
    Wstr<> command_line;
    command_line = lua_tostring(state, 1);
    BOOL ok = CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, TRUE,
        CREATE_NEW_PROCESS_GROUP|NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
    if (ok == FALSE)
        return 0;

    // Terminate the child's child processes.
    HANDLE job = CreateJobObject(nullptr, nullptr);
    if (job != nullptr)
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit = {};
        JOBOBJECT_BASIC_LIMIT_INFORMATION& basic = limit.BasicLimitInformation;
        basic.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit, sizeof(limit)))
        {
            CloseHandle(job);
            job = nullptr;
        }
        else
            AssignProcessToJobObject(job, pi.hProcess);
    }

    // Create the object that popen2() returns
    void* user_data = lua_newuserdata(state, sizeof(Popen2Lua));
    new (user_data) Popen2Lua(job, pipes.stdout_read, pipes.stdin_write);
    pipes.stdout_read = nullptr;
    pipes.stdin_write = nullptr;

    if (luaL_newmetatable(state, "popen2_mt"))
    {
        lua_createtable(state, 0, 0);

        #define BIND_METHOD(name) do {                                  \
                auto name##_thunk = [] (lua_State* state) -> int32 {      \
                    auto* self = (Popen2Lua*)lua_touserdata(state, 1); \
                    return self ? self->name(state) : 0;                \
                };                                                      \
                lua_pushliteral(state, #name);                          \
                lua_pushcfunction(state, name##_thunk);                 \
                lua_rawset(state, -3);                                  \
            } while (false)
        BIND_METHOD(read);
        BIND_METHOD(lines);
        BIND_METHOD(write);
        #undef BIND_METHOD

        lua_setfield(state, -2, "__index");

        auto gc_thunk = [] (lua_State* state) -> int32 {
            auto* self = (Popen2Lua*)lua_touserdata(state, 1);
            self->~Popen2Lua();
            return 0;
        };
        lua_pushcfunction(state, gc_thunk);
        lua_setfield(state, -2, "__gc");
    }

    lua_setmetatable(state, -2);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
}

//------------------------------------------------------------------------------
void io_lua_initialise(LuaState& lua)
{
    struct {
        const char* name;
        int32       (*method)(lua_State*);
    } methods[] = {
        { "popen2", &popen2 },
    };

    lua_State* state = lua.get_state();

    lua_getglobal(state, "io");

    for (const auto& method : methods)
    {
        lua_pushstring(state, method.name);
        lua_pushcfunction(state, method.method);
        lua_rawset(state, -3);
    }

    lua_pop(state, 1);
}
