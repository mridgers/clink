// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//------------------------------------------------------------------------------
inline int32  str_len(const char* s)                                   { return int32(strlen(s)); }
inline int32  str_len(const wchar_t* s)                                { return int32(wcslen(s)); }
inline void str_ncat(char* d, const char* s, size_t n)               { strncat(d, s, n); }
inline void str_ncat(wchar_t* d, const wchar_t* s, size_t n)         { wcsncat(d, s, n); }
inline int32  str_cmp(const char* l, const char* r)                    { return strcmp(r, l); }
inline int32  str_cmp(const wchar_t* l, const wchar_t* r)              { return wcscmp(r, l); }
inline int32  str_icmp(const char* l, const char* r)                   { return stricmp(r, l); }
inline int32  str_icmp(const wchar_t* l, const wchar_t* r)             { return wcsicmp(r, l); }
inline int32  vsnprint(char* d, int32 n, const char* f, va_list a)       { return vsnprintf(d, n, f, a); }
inline int32  vsnprint(wchar_t* d, int32 n, const wchar_t* f, va_list a) { return _vsnwprintf(d, n, f, a); }
inline const char*    str_chr(const char* s, int32 c)                  { return strchr(s, c); }
inline const wchar_t* str_chr(const wchar_t* s, int32 c)               { return wcschr(s, wchar_t(c)); }
inline const char*    str_rchr(const char* s, int32 c)                 { return strrchr(s, c); }
inline const wchar_t* str_rchr(const wchar_t* s, int32 c)              { return wcsrchr(s, wchar_t(c)); }

uint32 char_count(const char*);
uint32 char_count(const wchar_t*);

//------------------------------------------------------------------------------
template <typename TYPE>
class StrImpl
{
public:
    typedef TYPE        char_t;

                        StrImpl(TYPE* data, uint32 size);
                        StrImpl(const StrImpl&) = delete;
                        StrImpl(const StrImpl&&) = delete;
                        ~StrImpl();
    void                attach(TYPE* data, uint32 size);
    bool                reserve(uint32 size);
    TYPE*               data();
    const TYPE*         c_str() const;
    uint32              size() const;
    bool                is_growable() const;
    uint32              length() const;
    uint32              char_count() const;
    void                clear();
    bool                empty() const;
    void                truncate(uint32 length);
    int32               first_of(int32 c) const;
    int32               last_of(int32 c) const;
    bool                equals(const TYPE* rhs) const;
    bool                iequals(const TYPE* rhs) const;
    bool                copy(const TYPE* src);
    bool                concat(const TYPE* src, int32 n=-1);
    bool                format(const TYPE* format, ...);
    TYPE                operator [] (uint32 i) const;
    StrImpl&            operator << (const TYPE* rhs);
    template <int32 I>
    StrImpl&            operator << (const TYPE (&rhs)[I]);
    StrImpl&            operator << (const StrImpl& rhs);
    void                operator = (const StrImpl&) = delete;

protected:
    void                set_growable(bool state=true);

private:
    typedef uint16 ushort;

    void                free_data();
    TYPE*               _data;
    ushort              _size : 15;
    ushort              _growable : 1;
    mutable ushort      _length : 15;
    ushort              _owns_ptr : 1;
};

//------------------------------------------------------------------------------
template <typename TYPE>
StrImpl<TYPE>::StrImpl(TYPE* data, uint32 size)
: _data(data)
, _size(size)
, _growable(0)
, _owns_ptr(0)
, _length(0)
{
}

//------------------------------------------------------------------------------
template <typename TYPE>
StrImpl<TYPE>::~StrImpl()
{
    free_data();
}

//------------------------------------------------------------------------------
template <typename TYPE>
void StrImpl<TYPE>::attach(TYPE* data, uint32 size)
{
    if (is_growable())
    {
        free_data();
        _data = data;
        _size = size;
        _owns_ptr = 1;
    }
    else
    {
        clear();
        concat(data, size);
    }
}

//------------------------------------------------------------------------------
template <typename TYPE>
void StrImpl<TYPE>::set_growable(bool state)
{
    _growable = state ? 1 : 0;
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::reserve(uint32 new_size)
{
    ++new_size;
    if (_size >= new_size)
        return true;

    if (!is_growable())
        return false;

    new_size = (new_size + 63) & ~63;

    TYPE* new_data = (TYPE*)malloc(new_size * sizeof(TYPE));
    memcpy(new_data, c_str(), _size * sizeof(TYPE));

    free_data();

    _data = new_data;
    _size = new_size;
    _owns_ptr = 1;
    return true;
}

//------------------------------------------------------------------------------
template <typename TYPE>
void StrImpl<TYPE>::free_data()
{
    if (_owns_ptr)
        free(_data);
}

//------------------------------------------------------------------------------
template <typename TYPE>
TYPE* StrImpl<TYPE>::data()
{
    _length = 0;
    return _data;
}

//------------------------------------------------------------------------------
template <typename TYPE>
const TYPE* StrImpl<TYPE>::c_str() const
{
    return _data;
}

//------------------------------------------------------------------------------
template <typename TYPE>
uint32 StrImpl<TYPE>::size() const
{
    return _size;
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::is_growable() const
{
    return _growable;
}

//------------------------------------------------------------------------------
template <typename TYPE>
uint32 StrImpl<TYPE>::length() const
{
    if (!_length & !empty())
        _length = str_len(c_str());

    return _length;
}

//------------------------------------------------------------------------------
template <typename TYPE>
uint32 StrImpl<TYPE>::char_count() const
{
    return ::char_count(c_str());
}

//------------------------------------------------------------------------------
template <typename TYPE>
void StrImpl<TYPE>::clear()
{
    _data[0] = '\0';
    _length = 0;
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::empty() const
{
    return (c_str()[0] == '\0');
}

//------------------------------------------------------------------------------
template <typename TYPE>
void StrImpl<TYPE>::truncate(uint32 pos)
{
    if (pos >= _size)
        return;

    _data[pos] = '\0';
    _length = pos;
}

//------------------------------------------------------------------------------
template <typename TYPE>
int32 StrImpl<TYPE>::first_of(int32 c) const
{
    const TYPE* r = str_chr(c_str(), c);
    return r ? int32(r - c_str()) : -1;
}

//------------------------------------------------------------------------------
template <typename TYPE>
int32 StrImpl<TYPE>::last_of(int32 c) const
{
    const TYPE* r = str_rchr(c_str(), c);
    return r ? int32(r - c_str()) : -1;
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::equals(const TYPE* rhs) const
{
    return (str_cmp(c_str(), rhs) == 0);
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::iequals(const TYPE* rhs) const
{
    return (str_icmp(c_str(), rhs) == 0);
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::copy(const TYPE* src)
{
    clear();
    return concat(src);
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::concat(const TYPE* src, int32 n)
{
    if (src == nullptr)
        return false;

    if (n < 0)
        n = str_len(src);

    int32 len = length();
    reserve(len + n);

    int32 remaining = _size - len - 1;

    bool truncated = (remaining < n);
    if (!truncated)
        remaining = n;

    if (remaining > 0)
    {
        str_ncat(_data + len, src, remaining);
        _length += remaining;
    }

    return !truncated;
}

//------------------------------------------------------------------------------
template <typename TYPE>
bool StrImpl<TYPE>::format(const TYPE* format, ...)
{
    va_list args;
    va_start(args, format);
    uint32 ret = vsnprint(_data, _size, format, args);
    va_end(args);

    _data[_size - 1] = '\0';
    _length = 0;

    return (ret <= _size);
}

//------------------------------------------------------------------------------
template <typename TYPE>
TYPE StrImpl<TYPE>::operator [] (uint32 i) const
{
    return (i < length()) ? c_str()[i] : 0;
}

//------------------------------------------------------------------------------
template <typename TYPE>
StrImpl<TYPE>& StrImpl<TYPE>::operator << (const TYPE* rhs)
{
    concat(rhs);
    return *this;
}

//------------------------------------------------------------------------------
template <typename TYPE> template <int32 I>
StrImpl<TYPE>& StrImpl<TYPE>::operator << (const TYPE (&rhs)[I])
{
    concat(rhs, I);
    return *this;
}

//------------------------------------------------------------------------------
template <typename TYPE>
StrImpl<TYPE>& StrImpl<TYPE>::operator << (const StrImpl& rhs)
{
    concat(rhs.c_str());
    return *this;
}



//------------------------------------------------------------------------------
template <typename T> class StrIterImpl;

int32 to_utf8(class StrBase& out, const wchar_t* utf16);
int32 to_utf8(class StrBase& out, StrIterImpl<wchar_t>& iter);
int32 to_utf8(char* out, int32 max_count, const wchar_t* utf16);
int32 to_utf8(char* out, int32 max_count, StrIterImpl<wchar_t>& iter);

int32 to_utf16(class WstrBase& out, const char* utf8);
int32 to_utf16(class WstrBase& out, StrIterImpl<char>& iter);
int32 to_utf16(wchar_t* out, int32 max_count, const char* utf8);
int32 to_utf16(wchar_t* out, int32 max_count, StrIterImpl<char>& iter);



//------------------------------------------------------------------------------
class StrBase : public StrImpl<char>
{
public:
    template <int32 I> StrBase(char (&data)[I]) : StrImpl<char>(data, I) {}
                     StrBase(char* data, int32 size) : StrImpl<char>(data, size) {}
                     StrBase(const StrBase&)          = delete;
                     StrBase(const StrBase&&)         = delete;
    int32            from_utf16(const wchar_t* utf16)  { clear(); return to_utf8(*this, utf16); }
    void             operator = (const char* value)    { copy(value); }
    void             operator = (const wchar_t* value) { from_utf16(value); }
    void             operator = (const StrBase& rhs)  = delete;
};

class WstrBase : public StrImpl<wchar_t>
{
public:
    template <int32 I> WstrBase(char (&data)[I]) : StrImpl<wchar_t>(data, I) {}
                     WstrBase(wchar_t* data, int32 size) : StrImpl<wchar_t>(data, size) {}
                     WstrBase(const WstrBase&)         = delete;
                     WstrBase(const WstrBase&&)        = delete;
    int32            from_utf8(const char* utf8)        { clear(); return to_utf16(*this, utf8); }
    void             operator = (const wchar_t* value)  { copy(value); }
    void             operator = (const char* value)     { from_utf8(value); }
    void             operator = (const WstrBase&)       = delete;
};



//------------------------------------------------------------------------------
template <int32 COUNT=128, bool GROWABLE=true>
class Str : public StrBase
{
public:
                Str() : StrBase(_data, COUNT)     { clear(); set_growable(GROWABLE); }
    explicit    Str(const char* value) : Str()      { copy(value); }
    explicit    Str(const wchar_t* value) : Str()   { from_utf16(value); }
                Str(const Str&) = delete;
                Str(const Str&&) = delete;
    using       StrBase::operator =;

private:
    char        _data[COUNT];
};

template <int32 COUNT=128, bool GROWABLE=true>
class Wstr : public WstrBase
{
public:
                Wstr() : WstrBase(_data, COUNT)   { clear(); set_growable(GROWABLE); }
    explicit    Wstr(const wchar_t* value) : Wstr() { copy(value); }
    explicit    Wstr(const char* value) : Wstr()    { from_utf8(value); }
                Wstr(const Wstr&) = delete;
                Wstr(const Wstr&&) = delete;
    using       WstrBase::operator =;

private:
    wchar_t     _data[COUNT];
};



//------------------------------------------------------------------------------
inline uint32 char_count(const char* ptr)
{
    uint32 count = 0;
    while (int32 c = *ptr++)
        // 'count' is increased if the top two MSBs of c are not 10xxxxxx
        count += (c & 0xc0) != 0x80;

    return count;
}

inline uint32 char_count(const wchar_t* ptr)
{
    uint32 count = 0;
    while (uint16 c = *ptr++)
    {
        ++count;

        if ((c & 0xfc00) == 0xd800)
            if ((*ptr & 0xfc00) == 0xdc00)
                ++ptr;
    }

    return count;
}
