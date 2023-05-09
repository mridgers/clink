// Copyright (c) 2021 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "inst_iter.h"

//------------------------------------------------------------------------------
enum
{
    /* these are packed into nibbles of op_table */
    code_noparam              = 0x0,
    code_imm8                 = 0x1,
    code_imm16                = 0x2,
    code_imm16_imm8           = 0x3,
    code_immaddr              = 0x4,
    code_imm16_32             = 0x5,
    code_imm16_32_64          = 0x6, // x64 only
    code_imm16_imm16_32       = 0x6, // x86 only, dupes imm16/32/64
    code_rel8                 = 0x7,
    code_rel16_32             = 0x8,
    code_modrm                = 0x9,
    code_modrm_imm8           = 0xa,
    code_modrm_imm16_32       = 0xb,
    code_modrm_f6_01_imm8     = 0xc, // f6, with modrm.reg == 0 or 1
    code_modrm_f7_01_imm16_32 = 0xd, // f7, with modrm.reg == 0 or 1
    code_prefix               = 0xe,
    code_unknown              = 0xf,
};

//------------------------------------------------------------------------------
static const unsigned char op_table[] =
{
#if _M_X64
    //10    32    54    76    98    ba    dc    fe  // x64
    0x99, 0x99, 0x51, 0xff, 0x99, 0x99, 0x51, 0xf0, // 00
    0x99, 0x99, 0x51, 0xff, 0x99, 0x99, 0x51, 0xff, // 10
    0x99, 0x99, 0x51, 0xfe, 0x99, 0x99, 0x51, 0xfe, // 20
    0x99, 0x99, 0x51, 0x0e, 0x99, 0x99, 0x51, 0x0e, // 30
    0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, // 40
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 50
    0xff, 0x9f, 0xee, 0xee, 0xb5, 0xa1, 0x00, 0x00, // 60
    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, // 70
    0xba, 0xaf, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, // 80
    0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, // 90
    0x44, 0x44, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, // a0
    0x11, 0x11, 0x11, 0x11, 0x66, 0x66, 0x66, 0x66, // b0
    0xaa, 0x02, 0xff, 0xba, 0x03, 0x02, 0x10, 0x00, // c0
    0x99, 0x99, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, // d0
    0x77, 0x77, 0x11, 0x11, 0x88, 0x7f, 0x00, 0x00, // e0
    0x0e, 0xee, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x99, // f0
#else
    //10    32    54    76    98    ba    dc    fe  // x86
    0x99, 0x99, 0x51, 0x00, 0x99, 0x99, 0x51, 0xf0, // 00
    0x99, 0x99, 0x51, 0x00, 0x99, 0x99, 0x51, 0x00, // 10
    0x99, 0x99, 0x51, 0x0e, 0x99, 0x99, 0x51, 0x0e, // 20
    0x99, 0x99, 0x51, 0x0e, 0x99, 0x99, 0x51, 0x0e, // 30
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 40
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 50
    0x00, 0x99, 0xee, 0xee, 0xb5, 0xa1, 0x00, 0x00, // 60
    0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, // 70
    0xba, 0xaa, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, // 80
    0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, // 90
    0x44, 0x44, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00, // a0
    0x11, 0x11, 0x11, 0x11, 0x55, 0x55, 0x55, 0x55, // b0
    0xaa, 0x02, 0x99, 0xba, 0x03, 0x02, 0x10, 0x00, // c0
    0x99, 0x99, 0x11, 0x00, 0xff, 0xff, 0xff, 0xff, // d0
    0x77, 0x77, 0x11, 0x11, 0x88, 0x76, 0x00, 0x00, // e0
    0x0e, 0xee, 0x00, 0xdc, 0x00, 0x00, 0x00, 0x99, // f0
#endif // _M_X64
};

//------------------------------------------------------------------------------
static int lookup(int value)
{
    int index = value >> 1;
    int odd_shift = (value & 0x01) << 2;
    return (op_table[index] >> odd_shift) & 0x0f;
}

//------------------------------------------------------------------------------
static int disassemble_modrm(const unsigned char* cursor)
{
    int modrm = *cursor;

    if (modrm >= 0300)
        return 1;

#if _M_X64
    if ((modrm & 0307) == 0005)
        return -5;
#endif

    int length = 1;
    if ((modrm & 0307) == 0005) length += 4; // disp32
    if ((modrm & 0300) == 0100) length += 1; // +disp8
    if ((modrm & 0300) == 0200) length += 4; // +disp32
    if ((modrm & 0007) == 0004)              // sib
    {
        length += 1;
        int sib = *(cursor + 1);
        if ((sib & 0007) == 0005)
        {
            switch (modrm & 0300)
            {
            case 0000:
            //case 0200:
                length += 4;
                break;
            /*
            case 0100:
                length += 1;
                break;
            */
            }
        }
    }

    return length;
}

//------------------------------------------------------------------------------
static Instruction disassemble(const unsigned char* ptr)
{
    const unsigned char* cursor = ptr;
    
    auto end = [&] () -> Instruction {
        return {int(intptr_t(cursor - ptr))};
    };

    // prefixes
    int code;
    int imm_shift = 1;
    while (true)
    {
        int op = *cursor;
        ++cursor;

        code = lookup(op);
        if (code != code_prefix)
            break;

        if ((op & 0xf0) == 0x40) // rex.w
            imm_shift = 1 << ((op & 0x08) >> 3);

        else if (op == 0x66) // op-size
            imm_shift = 0;
    };

    // two-byte op code
    // if (cursor[-1] == 0x0f)
    //   /* ...! */

    if (code == code_unknown)
        return {0};

    // simple immediates
    if (code <= code_imm16_imm8)
    {
        cursor += code;
        return end();
    }

    // less-simple immediates
    if (code <= code_imm16_32_64)
    {
        switch (code)
        {
#if _M_X64
        case code_immaddr:        cursor += 8;                         break;
        case code_imm16_32_64:    cursor += 2 << imm_shift;            break;
#else
        case code_immaddr:        cursor += 4;                         break;
        case code_imm16_imm16_32: cursor += 2; /* fallthrough */
#endif
        case code_imm16_32:       cursor += 2 << int(imm_shift != 0);  break;
        }
        return end();
    }

    // ip-relative immediates
    if (code <= code_rel16_32)
    {
        const unsigned char* old_cursor = cursor;

        int mask = (code_rel8 - code);
        int shift = 1 + (imm_shift != 0);
        cursor += int(1 << (shift & mask));

        int rel_offset = int(intptr_t(old_cursor - ptr));
        int rel_size = int(intptr_t(cursor - old_cursor));
        cursor += (rel_offset << Instruction::rel_offset_shift);
        cursor += (rel_size << Instruction::rel_size_shift);

        return end();
    }

    int modrm = *cursor;

    int modrm_length = disassemble_modrm(cursor);
#if _M_X64
    if (modrm_length < 0)
    {
        int rel_offset = int(intptr_t(cursor + 1 - ptr)); // +1 to skip modrm
        int rel_size = 4;
        cursor += (rel_offset << Instruction::rel_offset_shift);
        cursor += (rel_size << Instruction::rel_size_shift);
        modrm_length = -modrm_length;
    }
#endif
    cursor += modrm_length;
    /* cursor cannot be dereferenced after this point */

    // Special case for f6/f7, reg=0 or 1.
    switch (code)
    {
    case code_modrm_f6_01_imm8:
    case code_modrm_f7_01_imm16_32:
        if ((modrm & 0070) > 0010)
            return end();
        code -= (code_modrm_f6_01_imm8 - code_modrm_imm8);
    }

    if (code == code_modrm_imm8)
        cursor += 1;

    else if (code == code_modrm_imm16_32)
        cursor += 2 << int(imm_shift != 0);

    return end();
}



//------------------------------------------------------------------------------
void Instruction::copy(const unsigned char* from, unsigned char* to) const
{
    memcpy(to, from, get_length());

    if (!is_relative())
        return;

    // Note it is the caller's responsibility to make sure there's enough bits
    // to adjust the relative offset to the new location.

    from += get_rel_offset();
    to += get_rel_offset();
    ptrdiff_t distance = ptrdiff_t(to) - ptrdiff_t(from);

    switch (get_rel_size())
    {
    case 4:
        memcpy(to, from, 4);
        *(int*)to += int(distance);
        break;

    case 2:
        memcpy(to, from, 2);
        *(short*)to += short(distance);
        break;

    case 1:
        memcpy(to, from, 1);
        *(char*)to += char(distance);
        break;
    }
}



//------------------------------------------------------------------------------
InstructionIter::InstructionIter(const void* data)
: m_data((const unsigned char*)data)
{
}

//------------------------------------------------------------------------------
Instruction InstructionIter::next()
{
    Instruction ret = disassemble(m_data);
    m_data += ret.get_length();
    return ret;
}
