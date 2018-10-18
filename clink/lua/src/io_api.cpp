// Copyright (c) 2018 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "lua_state.h"

#include <core/base.h>
#include <core/str.h>

#include <new.h>

//------------------------------------------------------------------------------
class handle_io
{
public:
                    handle_io(HANDLE h) : m_handle(h) {}
                    ~handle_io()        { close(); }
    bool            is_valid() const    { return m_handle != nullptr; }
    void            close();

protected:
    HANDLE          m_handle;
};

//------------------------------------------------------------------------------
void handle_io::close()
{
    if (m_handle == nullptr)
        return;

    CloseHandle(m_handle);
    m_handle = nullptr;
}



//------------------------------------------------------------------------------
class handle_reader
    : public handle_io
{
public:
                        handle_reader(HANDLE h) : handle_io(h) {}
                        ~handle_reader();
    bool                get(unsigned int index, int& c);
    const char*         get_pointer() const;
    void                consume(unsigned int size);
    unsigned int        read(unsigned int size);

private:
    bool                aquire();
    static const int    BUFFER_SIZE = 8192;
    char*               m_buffer = nullptr;
    unsigned int        m_buffer_size = 0;
    unsigned int        m_data_size = 0;
    unsigned int        m_cursor = 0;
};

//------------------------------------------------------------------------------
handle_reader::~handle_reader()
{
    free(m_buffer);
}

//------------------------------------------------------------------------------
bool handle_reader::get(unsigned int index, int& c)
{
    index += m_cursor;
    while (index >= m_data_size)
        if (!aquire())
            return false;

    c = unsigned(m_buffer[index]);
    return true;
}

//------------------------------------------------------------------------------
const char* handle_reader::get_pointer() const
{
    return m_buffer + m_cursor;
}

//------------------------------------------------------------------------------
void handle_reader::consume(unsigned int size)
{
    m_cursor = min(m_data_size, m_cursor + size);
    if (m_cursor == m_data_size)
        m_cursor = m_data_size = 0;
}

//------------------------------------------------------------------------------
unsigned int handle_reader::read(unsigned int size)
{
    while (m_data_size - m_cursor < size)
        if (!aquire())
            break;

    return m_data_size - m_cursor;
}

//------------------------------------------------------------------------------
bool handle_reader::aquire()
{
    if (!is_valid())
        return false;

    unsigned int remaining = m_buffer_size - m_data_size;
    if (remaining == 0)
    {
        remaining = BUFFER_SIZE;
        m_buffer_size += BUFFER_SIZE;
        m_buffer = (char*)realloc(m_buffer, m_buffer_size);
    }

    DWORD bytes_read = 0;
    char* dest = m_buffer + m_data_size;
    BOOL ok = ReadFile(m_handle, dest, DWORD(remaining), &bytes_read, nullptr);
    if (ok == FALSE)
    {
        close();
        return false;
    }

    m_data_size += bytes_read;
    return true;
}



//------------------------------------------------------------------------------
class handle_writer
    : public handle_io
{
public:
                handle_writer(HANDLE h) : handle_io(h) {}
    void        write(const char* data, unsigned int size);
};

//------------------------------------------------------------------------------
void handle_writer::write(const char* data, unsigned int size)
{
    DWORD written;
    if (WriteFile(m_handle, data, DWORD(size), &written, nullptr) == FALSE)
        close();
}



//------------------------------------------------------------------------------
class popen2_lua
{
public:
                    popen2_lua(HANDLE job, HANDLE read, HANDLE write);
                    ~popen2_lua();
    int             read(lua_State* state);
    int             lines(lua_State* state);
    int             write(lua_State* state);

private:
    int             read_line(lua_State* state, bool include_eol);
    int             read(lua_State* state, unsigned int bytes);
    HANDLE          m_job;
    handle_reader   m_reader;
    handle_writer   m_writer;
};

//------------------------------------------------------------------------------
popen2_lua::popen2_lua(HANDLE job, HANDLE read, HANDLE write)
: m_job(job)
, m_writer(write)
, m_reader(read)
{
}

//------------------------------------------------------------------------------
popen2_lua::~popen2_lua()
{
    if (m_job != nullptr)
        CloseHandle(m_job);
}

//------------------------------------------------------------------------------
int popen2_lua::read_line(lua_State* state, bool include_eol)
{
    unsigned int count = 0;
    for (int c; m_reader.get(count, c); )
    {
        ++count;
        if (c == '\n')
            break;
    }

    if (count == 0 && !m_reader.is_valid())
        return 0;

    unsigned int size = count;
    const char* data = m_reader.get_pointer();

    if (!include_eol)
    {
        size -= size && (data[size - 1] == '\n');
        size -= size && (data[size - 1] == '\r');
    }

    lua_pushlstring(state, data, size);

    m_reader.consume(count);
    return 1;
}

//------------------------------------------------------------------------------
int popen2_lua::read(lua_State* state, unsigned int bytes)
{
    unsigned int size = m_reader.read(bytes);
    const char* data = m_reader.get_pointer();
    lua_pushlstring(state, data, size);
    m_reader.consume(size);
    return 1;
}

//------------------------------------------------------------------------------
int popen2_lua::read(lua_State* state)
{
    if (!m_reader.is_valid())
        return 0;

    int arg_count = lua_gettop(state);
    if (arg_count >= 2)
    {
        if (lua_isnumber(state, 2))
        {
            unsigned int size = unsigned(lua_tointeger(state, 2));
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
int popen2_lua::lines(lua_State* state)
{
    if (!m_reader.is_valid())
        return 0;

    auto impl = [] (lua_State* state) -> int {
        int self_index = lua_upvalueindex(1);
        void* self = lua_touserdata(state, self_index);
        return self ? ((popen2_lua*)self)->read_line(state, false) : 0;
    };

    lua_pushvalue(state, 1);
    lua_pushcclosure(state, impl, 1);
    return 1;
}

//------------------------------------------------------------------------------
int popen2_lua::write(lua_State* state)
{
    if (!m_writer.is_valid())
        return 0;

    int arg_count = lua_gettop(state);
    if (arg_count < 2 || lua_isnil(state, 2))
    {
        m_writer.close();
        return 0;
    }

    size_t bytes;
    const char* data = lua_tolstring(state, 2, &bytes);
    if (data == nullptr)
        return 0;

    m_writer.write(data, unsigned(bytes));
    return 0;
}



//------------------------------------------------------------------------------
/// -name:  io.popen2
/// -arg:   command:string
/// -ret:   string
static int popen2(lua_State* state)
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
    for (int i = 0; i < sizeof_array(pipes.handles); i += 2)
        if (!CreatePipe(&pipes.handles[i], &pipes.handles[i + 1], &sa, 0))
            return 0;

    // Launch the process.
    STARTUPINFOW si = { sizeof(si) };
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.hStdOutput = pipes.stdout_write;
    si.hStdInput = pipes.stdin_read;
    si.dwFlags = STARTF_USESTDHANDLES;

    PROCESS_INFORMATION  pi;
    wstr<> command_line;
    command_line = lua_tostring(state, 1);
    BOOL ok = CreateProcessW(nullptr, command_line.data(), nullptr, nullptr, TRUE,
        NORMAL_PRIORITY_CLASS|CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
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
    void* user_data = lua_newuserdata(state, sizeof(popen2_lua));
    new (user_data) popen2_lua(job, pipes.stdout_read, pipes.stdin_write);
    pipes.stdout_read = nullptr;
    pipes.stdin_write = nullptr;

    if (luaL_newmetatable(state, "popen2_mt"))
    {
        lua_createtable(state, 0, 0);

        #define BIND_METHOD(name) do {                                  \
                auto name##_thunk = [] (lua_State* state) -> int {      \
                    auto* self = (popen2_lua*)lua_touserdata(state, 1); \
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

        auto gc_thunk = [] (lua_State* state) -> int {
            auto* self = (popen2_lua*)lua_touserdata(state, 1);
            self->~popen2_lua();
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
void io_lua_initialise(lua_state& lua)
{
    struct {
        const char* name;
        int         (*method)(lua_State*);
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
