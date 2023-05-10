// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "ecma48_iter.h"

#include <core/base.h>
#include <core/str_tokeniser.h>

//------------------------------------------------------------------------------
extern "C" int32 wcwidth(int32);



//------------------------------------------------------------------------------
uint32 cell_count(const char* in)
{
    uint32 count = 0;

    Ecma48State state;
    Ecma48Iter iter(in, state);
    while (const Ecma48Code& code = iter.next())
    {
        if (code.get_type() != Ecma48Code::type_chars)
            continue;

        StrIter inner_iter(code.get_pointer(), code.get_length());
        while (int32 c = inner_iter.next())
            count += wcwidth(c);
    }

    return count;
}

//------------------------------------------------------------------------------
static bool in_range(int32 value, int32 left, int32 right)
{
    return (uint32(right - value) <= uint32(right - left));
}



//------------------------------------------------------------------------------
enum
{
    ecma48_state_unknown = 0,
    ecma48_state_char,
    ecma48_state_esc,
    ecma48_state_esc_st,
    ecma48_state_csi_p,
    ecma48_state_csi_f,
    ecma48_state_cmd_str,
    ecma48_state_char_str,
};



//------------------------------------------------------------------------------
bool Ecma48Code::decode_csi(CsiBase& base, int32* params, uint32 max_params) const
{
    if (get_type() != type_c1 || get_code() != c1_csi)
        return false;

    /* CSI P ... P I .... I F */
    StrIter iter(get_pointer(), get_length());

    // Skip CSI
    if (iter.next() == 0x1b)
        iter.next();

    // Is the parameter string tagged as private/experimental?
    if (base.private_use = in_range(iter.peek(), 0x3c, 0x3f))
        iter.next();

    // Extract parameters.
    base.intermediate = 0;
    base.final = 0;
    int32 param = 0;
    uint32 count = 0;
    bool trailing_param = false;
    while (int32 c = iter.next())
    {
        if (in_range(c, 0x30, 0x3b))
        {
            trailing_param = true;

            if (c == 0x3b)
            {
                if (count < max_params)
                    params[count++] = param;

                param = 0;
            }
            else if (c != 0x3a) // Blissfully gloss over ':' part of spec.
                param = (param * 10) + (c - 0x30);
        }
        else if (in_range(c, 0x20, 0x2f))
            base.intermediate = char(c);
        else if (!in_range(c, 0x3c, 0x3f))
            base.final = char(c);
    }

    if (trailing_param)
        if (count < max_params)
            params[count++] = param;

    base.param_count = char(count);
    return true;
}

//------------------------------------------------------------------------------
bool Ecma48Code::get_c1_str(StrBase& out) const
{
    if (get_type() != type_c1 || get_code() == c1_csi)
        return false;

    StrIter iter(get_pointer(), get_length());

    // Skip announce
    if (iter.next() == 0x1b)
        iter.next();

    const char* start = iter.get_pointer();

    // Skip until terminator
    while (int32 c = iter.peek())
    {
        if (c == 0x9c || c == 0x1b)
            break;

        iter.next();
    }

    out.clear();
    out.concat(start, int32(iter.get_pointer() - start));
    return true;
}



//------------------------------------------------------------------------------
Ecma48Iter::Ecma48Iter(const char* s, Ecma48State& state, int32 len)
: _iter(s, len)
, _code(state.code)
, _state(state)
{
}

//------------------------------------------------------------------------------
const Ecma48Code& Ecma48Iter::next()
{
    _code._str = _iter.get_pointer();

    const char* copy = _iter.get_pointer();
    bool done = true;
    while (1)
    {
        int32 c = _iter.peek();
        if (!c)
        {
            if (_state.state != ecma48_state_char)
            {
                _code._length = 0;
                return _code;
            }

            break;
        }

        switch (_state.state)
        {
        case ecma48_state_char:     done = next_char(c);     break;
        case ecma48_state_char_str: done = next_char_str(c); break;
        case ecma48_state_cmd_str:  done = next_cmd_str(c);  break;
        case ecma48_state_csi_f:    done = next_csi_f(c);    break;
        case ecma48_state_csi_p:    done = next_csi_p(c);    break;
        case ecma48_state_esc:      done = next_esc(c);      break;
        case ecma48_state_esc_st:   done = next_esc_st(c);   break;
        case ecma48_state_unknown:  done = next_unknown(c);  break;
        }

        if (_state.state != ecma48_state_char)
        {
            while (copy != _iter.get_pointer())
            {
                _state.buffer[_state.count] = *copy++;
                _state.count += (_state.count < sizeof_array(_state.buffer) - 1);
            }
        }

        if (done)
            break;
    }

    if (_state.state != ecma48_state_char)
    {
        _code._str = _state.buffer;
        _code._length = _state.count;
    }
    else
        _code._length = int32(_iter.get_pointer() - _code.get_pointer());

    _state.reset();

    return _code;
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_c1()
{
    // Convert c1 code to its 7-bit version.
    _code._code = (_code._code & 0x1f) | 0x40;

    switch (_code.get_code())
    {
        case 0x50: /* dcs */
        case 0x5d: /* osc */
        case 0x5e: /* pm  */
        case 0x5f: /* apc */
            _state.state = ecma48_state_cmd_str;
            return false;

        case 0x5b: /* Csi */
            _state.state = ecma48_state_csi_p;
            return false;

        case 0x58: /* sos */
            _state.state = ecma48_state_char_str;
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_char(int32 c)
{
    if (in_range(c, 0x00, 0x1f))
    {
        _code._type = Ecma48Code::type_chars;
        return true;
    }

    _iter.next();
    return false;
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_char_str(int32 c)
{
    _iter.next();

    if (c == 0x1b)
    {
        _state.state = ecma48_state_esc_st;
        return false;
    }

    return (c == 0x9c);
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_cmd_str(int32 c)
{
    if (c == 0x1b)
    {
        _iter.next();
        _state.state = ecma48_state_esc_st;
        return false;
    }
    else if (c == 0x9c)
    {
        _iter.next();
        return true;
    }
    else if (in_range(c, 0x08, 0x0d) || in_range(c, 0x20, 0x7e))
    {
        _iter.next();
        return false;
    }

    // Reset
    _code._str = _iter.get_pointer();
    _code._length = 0;
    _state.reset();
    return false;
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_csi_f(int32 c)
{
    if (in_range(c, 0x20, 0x2f))
    {
        _iter.next();
        return false;
    }
    else if (in_range(c, 0x40, 0x7e))
    {
        _iter.next();
        return true;
    }

    // Reset
    _code._str = _iter.get_pointer();
    _code._length = 0;
    _state.reset();
    return false;
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_csi_p(int32 c)
{
    if (in_range(c, 0x30, 0x3f))
    {
        _iter.next();
        return false;
    }

    _state.state = ecma48_state_csi_f;
    return next_csi_f(c);
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_esc(int32 c)
{
    _iter.next();

    if (in_range(c, 0x40, 0x5f))
    {
        _code._type = Ecma48Code::type_c1;
        _code._code = c;
        return next_c1();
    }
    else if (in_range(c, 0x60, 0x7f))
    {
        _code._type = Ecma48Code::type_icf;
        _code._code = c;
        return true;
    }

    _state.state = ecma48_state_char;
    return false;
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_esc_st(int32 c)
{
    if (c == 0x5c)
    {
        _iter.next();
        return true;
    }

    _code._str = _iter.get_pointer();
    _code._length = 0;
    _state.reset();
    return false;
}

//------------------------------------------------------------------------------
bool Ecma48Iter::next_unknown(int32 c)
{
    _iter.next();

    if (c == 0x1b)
    {
        _state.state = ecma48_state_esc;
        return false;
    }
    else if (in_range(c, 0x00, 0x1f))
    {
        _code._type = Ecma48Code::type_c0;
        _code._code = c;
        return true;
    }
    else if (in_range(c, 0x80, 0x9f))
    {
        _code._type = Ecma48Code::type_c1;
        _code._code = c;
        return next_c1();
    }

    _code._type = Ecma48Code::type_chars;
    _state.state = ecma48_state_char;
    return false;
}
