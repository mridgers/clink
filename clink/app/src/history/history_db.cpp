// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "history_db.h"
#include "utils/app_context.h"

#include <core/base.h>
#include <core/globber.h>
#include <core/os.h>
#include <core/settings.h>
#include <core/str.h>
#include <core/str_tokeniser.h>

#include <new>
#include <Windows.h>
extern "C" {
#include <readline/history.h>
}

//------------------------------------------------------------------------------
static SettingBool g_shared(
    "history.shared",
    "Share history between instances",
    "",
    false);

static SettingBool g_ignore_space(
    "history.ignore_space",
    "Skip adding lines prefixed with whitespace",
    "Ignore lines that begin with whitespace when adding lines in to\n"
    "the history.",
    true);

static SettingEnum g_dupe_mode(
    "history.dupe_mode",
    "Controls how duplicate entries are handled",
    "If a line is a duplicate of an existing history entry Clink will\n"
    "erase the duplicate when this is set 2. A value of 1 will not add\n"
    "duplicates to the history and a value of 0 will always add lines.\n"
    "Note that history is not deduplicated when reading/writing to disk.",
    "add,ignore,erase_prev",
    2);

static SettingEnum g_expand_mode(
    "history.expand_mode",
    "Sets how command history expansion is applied",
    "The '!' character in an entered line can be interpreted to introduce\n"
    "words from the history. This can be enabled and disable by setting this\n"
    "value to 1 or 0. Values or 2, 3 or 4 will skip any ! character quoted\n"
    "in single, double, or both quotes respectively.",
    "off,on,not_squoted,not_dquoted,not_quoted",
    4);



//------------------------------------------------------------------------------
static int32 history_expand_control(char* line, int32 marker_pos)
{
    int32 setting = g_expand_mode.get();
    if (setting <= 1)
        return (setting <= 0);

    // Is marker_pos inside a quote of some kind?
    int32 in_quote = 0;
    for (int32 i = 0; i < marker_pos && *line; ++i, ++line)
    {
        int32 c = *line;
        if (c == '\'' || c == '\"')
            in_quote = (c == in_quote) ? 0 : c;
    }

    switch (setting)
    {
    case 2: return (in_quote == '\'');
    case 3: return (in_quote == '\"');
    case 4: return (in_quote == '\"' || in_quote == '\'');
    }

    return 0;
}

//------------------------------------------------------------------------------
static void get_file_path(StrBase& out, bool session)
{
    out.clear();

    const auto* app = AppContext::get();
    app->get_history_path(out);

    if (session)
    {
        Str<16> suffix;
        suffix.format("_%d", app->get_id());
        out << suffix;
    }
}

//------------------------------------------------------------------------------
static void* open_file(const char* path)
{
    Wstr<256> wpath(path);
    DWORD share_flags = FILE_SHARE_READ|FILE_SHARE_WRITE;
    void* handle = CreateFileW(wpath.c_str(), GENERIC_READ|GENERIC_WRITE, share_flags,
        nullptr, OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

    return (handle == INVALID_HANDLE_VALUE) ? nullptr : handle;
}



//------------------------------------------------------------------------------
union LineIdImpl
{
    explicit            LineIdImpl()                { outer = 0; }
    explicit            LineIdImpl(uint32 o)  { offset = o; active = 1; }
    explicit            operator bool () const      { return !!outer; }
    struct {
        uint32          offset : 29;
        uint32          bank_index : 2;
        uint32          active : 1;
    };
    HistoryDb::LineId   outer;
};



//------------------------------------------------------------------------------
class BankLock
    : public NoCopy
{
public:
    explicit        operator bool () const;

protected:
                    BankLock() = default;
                    BankLock(void* handle, bool exclusive);
                    ~BankLock();
    void*           _handle = nullptr;
};

//------------------------------------------------------------------------------
BankLock::BankLock(void* handle, bool exclusive)
: _handle(handle)
{
    if (_handle == nullptr)
        return;

    OVERLAPPED overlapped = {};
    int32 flags = exclusive ? LOCKFILE_EXCLUSIVE_LOCK : 0;
    LockFileEx(_handle, flags, 0, ~0u, ~0u, &overlapped);
}

//------------------------------------------------------------------------------
BankLock::~BankLock()
{
    if (_handle != nullptr)
    {
        OVERLAPPED overlapped = {};
        UnlockFileEx(_handle, 0, ~0u, ~0u, &overlapped);
    }
}

//------------------------------------------------------------------------------
BankLock::operator bool () const
{
    return (_handle != nullptr);
}



//------------------------------------------------------------------------------
class ReadLock
    : public BankLock
{
public:
    class FileIter : public NoCopy
    {
    public:
                            FileIter() = default;
                            FileIter(const ReadLock& lock, char* buffer, int32 buffer_size);
        template <int32 S>    FileIter(const ReadLock& lock, char (&buffer)[S]);
        uint32              next(uint32 rollback=0);
        uint32              get_buffer_offset() const   { return _buffer_offset; }
        char*               get_buffer() const          { return _buffer; }
        uint32              get_buffer_size() const     { return _buffer_size; }
        uint32              get_remaining() const       { return _remaining; }

    private:
        char*               _buffer;
        void*               _handle;
        uint32              _buffer_size;
        uint32              _buffer_offset;
        uint32              _remaining;
    };

    class LineIter : public NoCopy
    {
    public:
                            LineIter() = default;
                            LineIter(const ReadLock& lock, char* buffer, int32 buffer_size);
        template <int32 S>    LineIter(const ReadLock& lock, char (&buffer)[S]);
        LineIdImpl          next(StrIter& out);

    private:
        bool                provision();
        FileIter            _file_iter;
        uint32              _remaining = 0;
    };

    explicit                ReadLock() = default;
    explicit                ReadLock(void* handle, bool exclusive=false);
    LineIdImpl              find(const char* line) const;
    template <class T> void find(const char* line, T&& callback) const;
};

//------------------------------------------------------------------------------
ReadLock::ReadLock(void* handle, bool exclusive)
: BankLock(handle, exclusive)
{
}

//------------------------------------------------------------------------------
template <class T> void ReadLock::find(const char* line, T&& callback) const
{
    char buffer[HistoryDb::max_line_length];
    LineIter iter(*this, buffer);

    LineIdImpl id;
    for (StrIter read; id = iter.next(read);)
    {
        if (strncmp(line, read.get_pointer(), read.length()) != 0)
            continue;

        if (line[read.length()] != '\0')
            continue;

        uint32 file_ptr = SetFilePointer(_handle, 0, nullptr, FILE_CURRENT);
        bool abort = callback(id);
        SetFilePointer(_handle, file_ptr, nullptr, FILE_BEGIN);

        if (!abort)
            break;
    }
}

//------------------------------------------------------------------------------
LineIdImpl ReadLock::find(const char* line) const
{
    LineIdImpl id;
    find(line, [&] (LineIdImpl inner_id) {
        id = inner_id;
        return false;
    });
    return id;
}



//------------------------------------------------------------------------------
template <int32 S> ReadLock::FileIter::FileIter(const ReadLock& lock, char (&buffer)[S])
: FileIter(lock, buffer, S)
{
}

//------------------------------------------------------------------------------
ReadLock::FileIter::FileIter(const ReadLock& lock, char* buffer, int32 buffer_size)
: _handle(lock._handle)
, _buffer(buffer)
, _buffer_size(buffer_size)
, _buffer_offset(-buffer_size)
, _remaining(GetFileSize(lock._handle, nullptr))
{
    SetFilePointer(_handle, 0, nullptr, FILE_BEGIN);
    _buffer[0] = '\0';
}

//------------------------------------------------------------------------------
uint32 ReadLock::FileIter::next(uint32 rollback)
{
    if (!_remaining)
        return (_buffer[0] = '\0');

    rollback = min<uint32>(rollback, _buffer_size);
    if (rollback)
        memmove(_buffer, _buffer + _buffer_size - rollback, rollback);

    _buffer_offset += _buffer_size - rollback;

    char* target = _buffer + rollback;
    int32 needed = min(_remaining, _buffer_size - rollback);

    DWORD read = 0;
    ReadFile(_handle, target, needed, &read, nullptr);

    _remaining -= read;
    _buffer_size = read + rollback;
    return _buffer_size;
}



//------------------------------------------------------------------------------
template <int32 S> ReadLock::LineIter::LineIter(const ReadLock& lock, char (&buffer)[S])
: LineIter(lock, buffer, S)
{
}

//------------------------------------------------------------------------------
ReadLock::LineIter::LineIter(const ReadLock& lock, char* buffer, int32 buffer_size)
: _file_iter(lock, buffer, buffer_size)
{
}

//------------------------------------------------------------------------------
bool ReadLock::LineIter::provision()
{
    return !!(_remaining = _file_iter.next(_remaining));
}

//------------------------------------------------------------------------------
LineIdImpl ReadLock::LineIter::next(StrIter& out)
{
    while (_remaining || provision())
    {
        const char* last = _file_iter.get_buffer() + _file_iter.get_buffer_size();
        const char* start = last - _remaining;

        for (; start != last; ++start, --_remaining)
            if (uint32(*start) > 0x1f)
                break;

        const char* end = start;
        for (; end != last; ++end)
            if (uint32(*end) <= 0x1f)
                break;

        if (end == last && start != _file_iter.get_buffer())
        {
            provision();
            continue;
        }

        int32 bytes = int32(end - start);
        _remaining -= bytes;

        if (*start == '|')
            continue;

        new (&out) StrIter(start, int32(end - start));

        uint32 offset = int32(start - _file_iter.get_buffer());
        return LineIdImpl(_file_iter.get_buffer_offset() + offset);
    }

    return LineIdImpl();
}



//------------------------------------------------------------------------------
class WriteLock
    : public ReadLock
{
public:
                    WriteLock() = default;
    explicit        WriteLock(void* handle);
    void            clear();
    void            add(const char* line);
    void            remove(LineIdImpl id);
    void            append(const ReadLock& src);
};

//------------------------------------------------------------------------------
WriteLock::WriteLock(void* handle)
: ReadLock(handle, true)
{
}

//------------------------------------------------------------------------------
void WriteLock::clear()
{
    SetFilePointer(_handle, 0, nullptr, FILE_BEGIN);
    SetEndOfFile(_handle);
}

//------------------------------------------------------------------------------
void WriteLock::add(const char* line)
{
    DWORD written;
    SetFilePointer(_handle, 0, nullptr, FILE_END);
    WriteFile(_handle, line, int32(strlen(line)), &written, nullptr);
    WriteFile(_handle, "\n", 1, &written, nullptr);
}

//------------------------------------------------------------------------------
void WriteLock::remove(LineIdImpl id)
{
    DWORD written;
    SetFilePointer(_handle, id.offset, nullptr, FILE_BEGIN);
    WriteFile(_handle, "|", 1, &written, nullptr);
}

//------------------------------------------------------------------------------
void WriteLock::append(const ReadLock& src)
{
    DWORD written;

    SetFilePointer(_handle, 0, nullptr, FILE_END);

    char buffer[HistoryDb::max_line_length];
    ReadLock::FileIter src_iter(src, buffer);
    while (int32 bytes_read = src_iter.next())
        WriteFile(_handle, buffer, bytes_read, &written, nullptr);
}



//------------------------------------------------------------------------------
class ReadLineIter
{
public:
                            ReadLineIter(const HistoryDb& db, uint32 this_size);
    HistoryDb::LineId       next(StrIter& out);

private:
    bool                    next_bank();
    const HistoryDb&        _db;
    ReadLock                _lock;
    ReadLock::LineIter      _line_iter;
    uint32                  _buffer_size;
    uint32                  _bank_index = 0;
};

//------------------------------------------------------------------------------
ReadLineIter::ReadLineIter(const HistoryDb& db, uint32 this_size)
: _db(db)
, _buffer_size(this_size - sizeof(*this))
{
    next_bank();
}

//------------------------------------------------------------------------------
bool ReadLineIter::next_bank()
{
    while (_bank_index < _db.get_bank_count())
    {
        if (void* bank_handle = _db._bank_handles[_bank_index++])
        {
            char* buffer = (char*)(this + 1);
            _lock.~ReadLock();
            new (&_lock) ReadLock(bank_handle);
            new (&_line_iter) ReadLock::LineIter(_lock, buffer, _buffer_size);
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
HistoryDb::LineId ReadLineIter::next(StrIter& out)
{
    if (_bank_index > _db.get_bank_count())
        return 0;

    do
    {
        if (LineIdImpl ret = _line_iter.next(out))
        {
            ret.bank_index = _bank_index - 1;
            return ret.outer;
        }
    }
    while (next_bank());

    return 0;
}



//------------------------------------------------------------------------------
HistoryDb::Iter::~Iter()
{
    if (impl)
        ((ReadLineIter*)impl)->~ReadLineIter();
}

//------------------------------------------------------------------------------
HistoryDb::LineId HistoryDb::Iter::next(StrIter& out)
{
    return impl ? ((ReadLineIter*)impl)->next(out) : 0;
}



//------------------------------------------------------------------------------
HistoryDb::HistoryDb()
{
    memset(_bank_handles, 0, sizeof(_bank_handles));

    // Create a self-deleting file to used to indicate this session's alive
    Str<280> path;
    get_file_path(path, true);
    path << "~";

    {
        Wstr<280> wpath(path.c_str());
        DWORD flags = FILE_FLAG_DELETE_ON_CLOSE|FILE_ATTRIBUTE_HIDDEN;
        _alive_file = CreateFileW(wpath.c_str(), 0, 0, nullptr, CREATE_ALWAYS, flags, nullptr);
        _alive_file = (_alive_file == INVALID_HANDLE_VALUE) ? nullptr : _alive_file;
    }

    history_inhibit_expansion_function = history_expand_control;

    static_assert(sizeof(LineId) == sizeof(LineIdImpl), "");
}

//------------------------------------------------------------------------------
HistoryDb::~HistoryDb()
{
    // Close alive handle
    CloseHandle(_alive_file);

    // Close all but the master bank. We're going to append to the master one.
    for (int32 i = 1, n = get_bank_count(); i < n; ++i)
        CloseHandle(_bank_handles[i]);

    reap();

    CloseHandle(_bank_handles[bank_master]);
}

//------------------------------------------------------------------------------
void HistoryDb::reap()
{
    // Fold each session found that has no valid alive file.
    Str<280> path;
    get_file_path(path, false);
    path << "_*";

    for (Globber i(path.c_str()); i.next(path);)
    {
        path << "~";
        if (os::get_path_type(path.c_str()) == os::path_type_file)
            if (!os::unlink(path.c_str())) // abandoned alive files will unlink
                continue;

        path.truncate(path.length() - 1);

        int32 file_size = os::get_file_size(path.c_str());
        if (file_size > 0)
        {
            void* src_handle = open_file(path.c_str());
            {
                ReadLock src(src_handle);
                WriteLock dest(_bank_handles[bank_master]);
                if (src && dest)
                    dest.append(src);
            }
            CloseHandle(src_handle);
        }

        os::unlink(path.c_str());
    }
}

//------------------------------------------------------------------------------
void HistoryDb::initialise()
{
    if (_bank_handles[bank_master] != nullptr)
        return;

    Str<280> path;
    get_file_path(path, false);
    _bank_handles[bank_master] = open_file(path.c_str());

    if (g_shared.get())
        return;

    get_file_path(path, true);
    _bank_handles[bank_session] = open_file(path.c_str());

    reap(); // collects orphaned history files.
}

//------------------------------------------------------------------------------
uint32 HistoryDb::get_bank_count() const
{
    int32 count = 0;
    for (void* handle : _bank_handles)
        count += (handle != nullptr);

    return count;
}

//------------------------------------------------------------------------------
void* HistoryDb::get_bank(uint32 index) const
{
    if (index >= get_bank_count())
        return nullptr;

    return _bank_handles[index];
}

//------------------------------------------------------------------------------
template <typename T> void HistoryDb::for_each_bank(T&& callback)
{
    for (int32 i = 0, n = get_bank_count(); i < n; ++i)
    {
        WriteLock lock(get_bank(i));
        if (lock && !callback(i, lock))
            break;
    }
}

//------------------------------------------------------------------------------
template <typename T> void HistoryDb::for_each_bank(T&& callback) const
{
    for (int32 i = 0, n = get_bank_count(); i < n; ++i)
    {
        ReadLock lock(get_bank(i));
        if (lock && !callback(i, lock))
            break;
    }
}

//------------------------------------------------------------------------------
void HistoryDb::load_rl_history()
{
    clear_history();

    char buffer[max_line_length + 1];

    const HistoryDb& const_this = *this;
    const_this.for_each_bank([&] (uint32, const ReadLock& lock)
    {
        StrIter out;
        ReadLock::LineIter iter(lock, buffer, sizeof_array(buffer) - 1);
        while (iter.next(out))
        {
            const char* line = out.get_pointer();
            int32 buffer_offset = int32(line - buffer);
            buffer[buffer_offset + out.length()] = '\0';
            add_history(line);
        }

        return true;
    });
}

//------------------------------------------------------------------------------
void HistoryDb::clear()
{
    for_each_bank([] (uint32, WriteLock& lock)
    {
        lock.clear();
        return true;
    });
}

//------------------------------------------------------------------------------
bool HistoryDb::add(const char* line)
{
    // Ignore empty and/or whitespace prefixed lines?
    if (!line[0] || (g_ignore_space.get() && (line[0] == ' ' || line[0] == '\t')))
        return false;

    // Handle duplicates.
    switch (g_dupe_mode.get())
    {
    case 1:
        // 'ignore'
        if (LineId find_result = find(line))
            return true;
        break;

    case 2:
        // 'erase_prev'
        remove(line);
        break;
    }

    // Add the line.
    void* handle = get_bank(get_bank_count() - 1);
    WriteLock lock(handle);
    if (!lock)
        return false;

    lock.add(line);
    return true;
}

//------------------------------------------------------------------------------
int32 HistoryDb::remove(const char* line)
{
    int32 count = 0;
    for_each_bank([line, &count] (uint32 index, WriteLock& lock)
    {
        lock.find(line, [&] (LineIdImpl id) {
            lock.remove(id);
            return true;
        });

        return true;
    });

    return count;
}

//------------------------------------------------------------------------------
bool HistoryDb::remove(LineId id)
{
    if (!id)
        return false;

    LineIdImpl id_impl;
    id_impl.outer = id;

    void* handle = get_bank(id_impl.bank_index);
    WriteLock lock(handle);
    if (!lock)
        return false;

    lock.remove(id_impl);
    return true;
}

//------------------------------------------------------------------------------
HistoryDb::LineId HistoryDb::find(const char* line) const
{
    LineIdImpl ret;

    for_each_bank([line, &ret] (uint32 index, const ReadLock& lock)
    {
        if (ret = lock.find(line))
            ret.bank_index = index;
        return !ret;
    });

    return ret.outer;
}

//------------------------------------------------------------------------------
HistoryDb::ExpandResult HistoryDb::expand(const char* line, StrBase& out) const
{
    using_history();

    char* expanded = nullptr;
    int32 result = history_expand((char*)line, &expanded);
    if (result >= 0 && expanded != nullptr)
        out.copy(expanded);

    free(expanded);
    return ExpandResult(result);
}

//------------------------------------------------------------------------------
HistoryDb::Iter HistoryDb::read_lines(char* buffer, uint32 size)
{
    Iter ret;
    if (size > sizeof(ReadLineIter))
        ret.impl = uintptr_t(new (buffer) ReadLineIter(*this, size));

    return ret;
}
