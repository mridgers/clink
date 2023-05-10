// Copyright (c) 2021 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class Instruction
{
public:
    enum
    {
        rel_offset_shift    = 8,
        rel_size_shift      = 16,
    };

                        Instruction() = default;
    explicit            operator bool () const  { return get_length() != 0; }
    bool                is_relative() const     { return (_value > 0xff); }
    int                 get_rel_size() const    { return (_value >> rel_size_shift) & 0xff; }
    int                 get_rel_offset() const  { return (_value >> rel_offset_shift) & 0xff; }
    int                 get_length() const      { return _value & 0xff; }
    void                copy(const unsigned char* from, unsigned char* to) const;

private:
    friend Instruction  disassemble(const unsigned char* ptr);
                        Instruction(int value) : _value(value) {}
    int                 _value = 0;
};

//------------------------------------------------------------------------------
class InstructionIter
{
public:
                            InstructionIter(const void* data);
    Instruction             next();

private:
    const unsigned char*    _data;
};
