// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "doskey.h"

#include <core/base.h>
#include <core/settings.h>
#include <core/str.h>
#include <core/str_iter.h>
#include <core/str_tokeniser.h>

//------------------------------------------------------------------------------
static SettingBool g_enhanced_doskey(
    "Doskey.enhanced",
    "Add enhancements to Doskey",
    "Enhanced Doskey adds the expansion of macros that follow '|' and '&'\n"
    "command separators and respects quotes around words when parsing $1...9\n"
    "tags. Note that these features do not apply to Doskey use in Batch files.",
    true);



//------------------------------------------------------------------------------
class WstrStream
{
public:
    typedef wchar_t         TYPE;

    struct RangeDesc
    {
        const TYPE* const   ptr;
        uint32              count;
    };

                            WstrStream();
    void                    operator << (TYPE c);
    void                    operator << (const RangeDesc desc);
    void                    collect(StrImpl<TYPE>& out);
    static RangeDesc        range(TYPE const* ptr, uint32 count);
    static RangeDesc        range(const StrIterImpl<TYPE>& iter);

private:
    void                    grow(uint32 hint=128);
    wchar_t* __restrict     _start;
    wchar_t* __restrict     _end;
    wchar_t* __restrict     _cursor;
};

//------------------------------------------------------------------------------
WstrStream::WstrStream()
: _start(nullptr)
, _end(nullptr)
, _cursor(nullptr)
{
}

//------------------------------------------------------------------------------
void WstrStream::operator << (TYPE c)
{
    if (_cursor >= _end)
        grow();

    *_cursor++ = c;
}

//------------------------------------------------------------------------------
void WstrStream::operator << (const RangeDesc desc)
{
    if (_cursor + desc.count >= _end)
        grow(desc.count);

    for (uint32 i = 0; i < desc.count; ++i, ++_cursor)
        *_cursor = desc.ptr[i];
}

//------------------------------------------------------------------------------
WstrStream::RangeDesc WstrStream::range(const TYPE* ptr, uint32 count)
{
    return { ptr, count };
}

//------------------------------------------------------------------------------
WstrStream::RangeDesc WstrStream::range(const StrIterImpl<TYPE>& iter)
{
    return { iter.get_pointer(), iter.length() };
}

//------------------------------------------------------------------------------
void WstrStream::collect(StrImpl<TYPE>& out)
{
    out.attach(_start, int32(_cursor - _start));
    _start = _end = _cursor = nullptr;
}

//------------------------------------------------------------------------------
void WstrStream::grow(uint32 hint)
{
    hint = (hint + 127) & ~127;
    uint32 size = int32(_end - _start) + hint;
    TYPE* next = (TYPE*)realloc(_start, size * sizeof(TYPE));
    _cursor = next + (_cursor - _start);
    _end = next + size;
    _start = next;
}



//------------------------------------------------------------------------------
DoskeyAlias::DoskeyAlias()
{
    _cursor = _buffer.c_str();
}

//------------------------------------------------------------------------------
void DoskeyAlias::reset()
{
    _buffer.clear();
    _cursor = _buffer.c_str();
}

//------------------------------------------------------------------------------
bool DoskeyAlias::next(WstrBase& out)
{
    if (!*_cursor)
        return false;

    out.copy(_cursor);

    while (*_cursor++);
    return true;
}

//------------------------------------------------------------------------------
DoskeyAlias::operator bool () const
{
    return (*_cursor != 0);
}



//------------------------------------------------------------------------------
Doskey::Doskey(const char* shell_name)
: _shell_name(shell_name)
{
}

//------------------------------------------------------------------------------
bool Doskey::add_alias(const char* alias, const char* text)
{
    Wstr<64> walias(alias);
    Wstr<> wtext(text);
    Wstr<64> wshell(_shell_name);
    return (AddConsoleAliasW(walias.data(), wtext.data(), wshell.data()) == TRUE);
}

//------------------------------------------------------------------------------
bool Doskey::remove_alias(const char* alias)
{
    Wstr<64> walias(alias);
    Wstr<64> wshell(_shell_name);
    return (AddConsoleAliasW(walias.data(), nullptr, wshell.data()) == TRUE);
}

//------------------------------------------------------------------------------
bool Doskey::resolve_impl(const WstrIter& in, WstrStream* out)
{
    // Find the alias for which to retrieve text for.
    WstrTokeniser tokens(in, " ");
    WstrIter token;
    if (!tokens.next(token))
        return false;

    // Legacy Doskey doesn't allow macros that begin with whitespace so if the
    // token does it won't ever resolve as an alias.
    const wchar_t* alias_ptr = token.get_pointer();
    if (!g_enhanced_doskey.get() && alias_ptr != in.get_pointer())
        return false;

    Wstr<32> alias;
    alias.concat(alias_ptr, token.length());

    // Find the alias' text. First check it exists.
    wchar_t unused;
    Wstr<32> wshell(_shell_name);
    if (!GetConsoleAliasW(alias.data(), &unused, sizeof(unused), wshell.data()))
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return false;

    // It does. Allocate space and fetch it.
    Wstr<4> text;
    text.reserve(511);
    GetConsoleAliasW(alias.data(), text.data(), text.size(), wshell.data());

    // Early out if not output location was provided.
    if (out == nullptr)
        return true;

    // Collect the remaining tokens.
    if (g_enhanced_doskey.get())
        tokens.add_quote_pair("\"");

    struct ArgDesc { const wchar_t* ptr; int32 length; };
    FixedArray<ArgDesc, 10> args;
    ArgDesc* desc;
    while (tokens.next(token) && (desc = args.push_back()))
    {
        desc->ptr = token.get_pointer();
        desc->length = int16(token.length());
    }

    // Expand the alias' text into 'out'.
    WstrStream& stream = *out;
    const wchar_t* read = text.c_str();
    for (int32 c = *read; c = *read; ++read)
    {
        if (c != '$')
        {
            stream << c;
            continue;
        }

        c = *++read;
        if (!c)
            break;

        // Convert $x tags.
        switch (c)
        {
        case '$':           stream << '$';  continue;
        case 'g': case 'G': stream << '>';  continue;
        case 'l': case 'L': stream << '<';  continue;
        case 'b': case 'B': stream << '|';  continue;
        case 't': case 'T': stream << '\0'; continue;
        }

        // Unknown tag? Perhaps it is a argument one?
        if (uint32(c - '1') < 9)  c -= '1';
        else if (c == '*')          c = -1;
        else
        {
            stream << '$';
            stream << c;
            continue;
        }

        int32 arg_count = args.size();
        if (!arg_count)
            continue;

        // 'c' is now the arg index or -1 if it is all of them to be inserted.
        if (c < 0)
        {
            const wchar_t* end = in.get_pointer() + in.length();
            const wchar_t* start = args.front()->ptr;
            stream << WstrStream::range(start, int32(end - start));
        }
        else if (c < arg_count)
        {
            const ArgDesc& desc = args.front()[c];
            stream << WstrStream::range(desc.ptr, desc.length);
        }
    }

    return true;
}

//------------------------------------------------------------------------------
void Doskey::resolve(const wchar_t* chars, DoskeyAlias& out)
{
    out.reset();

    WstrStream stream;
    if (g_enhanced_doskey.get())
    {
        WstrIter command;

        // Coarse check to see if there's any aliases to resolve
        {
            bool resolves = false;
            WstrTokeniser commands(chars, "&|");
            commands.add_quote_pair("\"");
            while (commands.next(command))
                if (resolves = resolve_impl(command, nullptr))
                    break;

            if (!resolves)
                return;
        }

        // This line will expand aliases so lets do that.
        {
            const wchar_t* last = chars;
            WstrTokeniser commands(chars, "&|");
            commands.add_quote_pair("\"");
            while (commands.next(command))
            {
                // Copy delimiters into the output buffer verbatim.
                if (int32 delim_length = int32(command.get_pointer() - last))
                    stream << WstrStream::range(last, delim_length);
                last = command.get_pointer() + command.length();

                if (!resolve_impl(command, &stream))
                    stream << WstrStream::range(command);
            }

            // Append any trailing delimiters too.
            while (*last)
                stream << *last++;
        }
    }
    else if (!resolve_impl(WstrIter(chars), &stream))
        return;

    // Double null-terminated as aliases with $T become and Array of commands.
    stream << '\0';
    stream << '\0';

    // Collect the resolve result
    stream.collect(out._buffer);
    out._cursor = out._buffer.c_str();
}
