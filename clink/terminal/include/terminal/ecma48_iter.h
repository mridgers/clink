// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/str_iter.h>

//------------------------------------------------------------------------------
uint32 cell_count(const char*);

//------------------------------------------------------------------------------
class Ecma48Code
{
public:
    enum Type : uint8
    {
        type_none,
        type_chars,
        type_c0,
        type_c1,
        type_icf
    };

    enum : uint8
    {
        c0_nul, c0_soh, c0_stx, c0_etx, c0_eot, c0_enq, c0_ack, c0_bel,
        c0_bs,  c0_ht,  c0_lf,  c0_vt,  c0_ff,  c0_cr,  c0_so,  c0_si,
        c0_dle, c0_dc1, c0_dc2, c0_dc3, c0_dc4, c0_nak, c0_syn, c0_etb,
        c0_can, c0_em,  c0_sub, c0_esc, c0_fs,  c0_gs,  c0_rs,  c0_us,
    };

    enum : uint8
    {
        c1_apc          = 0x5f,
        c1_csi          = 0x5b,
        c1_dcs          = 0x50,
        c1_osc          = 0x5d,
        c1_pm           = 0x5e,
        c1_sos          = 0x58,
    };

    struct CsiBase
    {
        char                final;
        char                intermediate;
        bool                private_use;
        uint8               param_count;
        int32               params[1];
        int32               get_param(int32 index, int32 fallback=0) const;
    };

    template <int32 PARAM_N>
    struct Csi : public CsiBase
    {
        static const int32  max_param_count = PARAM_N;

    private:
        int32               buffer[PARAM_N - 1];
    };

    explicit                operator bool () const { return !!get_length(); }
    const char*             get_pointer() const    { return _str; }
    uint32                  get_length() const     { return _length; }
    Type                    get_type() const       { return _type; }
    uint32                  get_code() const       { return _code; }
    template <int32 S> bool   decode_csi(Csi<S>& out) const;
    bool                    get_c1_str(StrBase& out) const;

private:
    friend class            Ecma48Iter;
    friend class            Ecma48State;
                            Ecma48Code() = default;
                            Ecma48Code(Ecma48Code&) = delete;
                            Ecma48Code(Ecma48Code&&) = delete;
    void                    operator = (Ecma48Code&) = delete;
    bool                    decode_csi(CsiBase& base, int32* params, uint32 max_params) const;
    const char*             _str;
    uint16                  _length;
    Type                    _type;
    uint8                   _code;
};

//------------------------------------------------------------------------------
inline int32 Ecma48Code::CsiBase::get_param(int32 index, int32 fallback) const
{
    if (uint32(index) < uint32(param_count))
        return *(params + index);

    return fallback;
}

//------------------------------------------------------------------------------
template <int32 S>
bool Ecma48Code::decode_csi(Csi<S>& csi) const
{
    return decode_csi(csi, csi.params, S);
}



//------------------------------------------------------------------------------
class Ecma48State
{
public:
                        Ecma48State()   { reset(); }
    void                reset()         { state = count = 0; }

private:
    friend class        Ecma48Iter;
    Ecma48Code          code;
    int32               state;
    int32               count;
    char                buffer[64];
};



//------------------------------------------------------------------------------
class Ecma48Iter
{
public:
                        Ecma48Iter(const char* s, Ecma48State& state, int32 len=-1);
    const Ecma48Code&   next();

private:
    bool                next_c1();
    bool                next_char(int32 c);
    bool                next_char_str(int32 c);
    bool                next_cmd_str(int32 c);
    bool                next_csi_f(int32 c);
    bool                next_csi_p(int32 c);
    bool                next_esc(int32 c);
    bool                next_esc_st(int32 c);
    bool                next_unknown(int32 c);
    StrIter             _iter;
    Ecma48Code&         _code;
    Ecma48State&        _state;
};
