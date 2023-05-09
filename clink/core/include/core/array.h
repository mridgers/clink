// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
template <typename T>
class Array
{
    /* This class is really rather poor */

public:
    template <int D, typename U>
    class IterImpl
    {
    public:
                    IterImpl(U* u) : m_u(u)               {}
        void        operator ++ ()                         { m_u += D; }
        U&          operator * () const                    { return *m_u; }
        U&          operator -> () const                   { return *m_u; }
        bool        operator != (const IterImpl& i) const { return i.m_u != m_u; }

    private:
        U*          m_u;
    };

    typedef IterImpl<1, T>         Iter;
    typedef IterImpl<1, T const>   Citer;
    typedef IterImpl<-1, T>        Riter;
    typedef IterImpl<-1, T const>  RCiter;

                    Array(T* ptr, unsigned int size, unsigned int capacity=0);
    Iter            begin()          { return m_ptr; }
    Iter            end()            { return m_ptr + m_size; }
    Citer           begin() const    { return m_ptr; }
    Citer           end() const      { return m_ptr + m_size; }
    Riter           rbegin()         { return m_ptr + m_size - 1; }
    Riter           rend()           { return m_ptr - 1; }
    RCiter          rbegin() const   { return m_ptr + m_size - 1; }
    RCiter          rend() const     { return m_ptr - 1; }
    unsigned int    size() const     { return m_size; }
    unsigned int    capacity() const { return m_capacity; }
    bool            empty() const    { return !m_size; }
    bool            full() const     { return (m_size == m_capacity); }
    T const*        front() const    { return m_ptr; }
    T*              front()          { return m_ptr; }
    T const*        back() const     { return empty() ? nullptr : (m_ptr + m_size - 1); }
    T*              back()           { return empty() ? nullptr : (m_ptr + m_size - 1); }
    T*              push_back()      { return full() ? nullptr : (m_ptr + m_size++); }
    void            clear();
    T const*        operator [] (unsigned int index) const;

protected:
    T*              m_ptr;
    unsigned int    m_size;
    unsigned int    m_capacity;
};

//------------------------------------------------------------------------------
template <typename T>
Array<T>::Array(T* ptr, unsigned int size, unsigned int capacity)
: m_ptr(ptr)
, m_size(size)
, m_capacity(capacity ? capacity : size)
{
}

//------------------------------------------------------------------------------
template <typename T>
T const* Array<T>::operator [] (unsigned int index) const
{
    return (index >= capacity()) ? nullptr : (m_ptr + index);
}

//------------------------------------------------------------------------------
template <typename T>
void Array<T>::clear()
{
    for (auto iter : *this)
        iter.~T();

    m_size = 0;
}



//------------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
class FixedArray
    : public Array<T>
{
public:
                FixedArray() : Array<T>(m_buffer, 0, SIZE) {}

private:
    T           m_buffer[SIZE];
};
