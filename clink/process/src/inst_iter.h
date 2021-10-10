// Copyright (c) 2021 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class instruction
{
public:
    enum
    {
        rel_offset_shift    = 8,
        rel_size_shift      = 16,
    };

                        instruction() = default;
    explicit            operator bool () const  { return get_length() != 0; }
    bool                is_relative() const     { return (m_value > 0xff); }
    int                 get_rel_size() const    { return (m_value >> rel_size_shift) & 0xff; }
    int                 get_rel_offset() const  { return (m_value >> rel_offset_shift) & 0xff; }
    int                 get_length() const      { return m_value & 0xff; }
    void                copy(const unsigned char* from, unsigned char* to) const;

private:
    friend instruction  disassemble(const unsigned char* ptr);
                        instruction(int value) : m_value(value) {}
    int                 m_value = 0;
};

//------------------------------------------------------------------------------
class instruction_iter
{
public:
                            instruction_iter(const void* data);
    instruction             next();

private:
    const unsigned char*    m_data;
};
