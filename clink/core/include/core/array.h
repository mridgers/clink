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
                    IterImpl(U* u) : _u(u)               {}
        void        operator ++ ()                         { _u += D; }
        U&          operator * () const                    { return *_u; }
        U&          operator -> () const                   { return *_u; }
        bool        operator != (const IterImpl& i) const { return i._u != _u; }

    private:
        U*          _u;
    };

    typedef IterImpl<1, T>         Iter;
    typedef IterImpl<1, T const>   Citer;
    typedef IterImpl<-1, T>        Riter;
    typedef IterImpl<-1, T const>  RCiter;

                    Array(T* ptr, unsigned int size, unsigned int capacity=0);
    Iter            begin()          { return _ptr; }
    Iter            end()            { return _ptr + _size; }
    Citer           begin() const    { return _ptr; }
    Citer           end() const      { return _ptr + _size; }
    Riter           rbegin()         { return _ptr + _size - 1; }
    Riter           rend()           { return _ptr - 1; }
    RCiter          rbegin() const   { return _ptr + _size - 1; }
    RCiter          rend() const     { return _ptr - 1; }
    unsigned int    size() const     { return _size; }
    unsigned int    capacity() const { return _capacity; }
    bool            empty() const    { return !_size; }
    bool            full() const     { return (_size == _capacity); }
    T const*        front() const    { return _ptr; }
    T*              front()          { return _ptr; }
    T const*        back() const     { return empty() ? nullptr : (_ptr + _size - 1); }
    T*              back()           { return empty() ? nullptr : (_ptr + _size - 1); }
    T*              push_back()      { return full() ? nullptr : (_ptr + _size++); }
    void            clear();
    T const*        operator [] (unsigned int index) const;

protected:
    T*              _ptr;
    unsigned int    _size;
    unsigned int    _capacity;
};

//------------------------------------------------------------------------------
template <typename T>
Array<T>::Array(T* ptr, unsigned int size, unsigned int capacity)
: _ptr(ptr)
, _size(size)
, _capacity(capacity ? capacity : size)
{
}

//------------------------------------------------------------------------------
template <typename T>
T const* Array<T>::operator [] (unsigned int index) const
{
    return (index >= capacity()) ? nullptr : (_ptr + index);
}

//------------------------------------------------------------------------------
template <typename T>
void Array<T>::clear()
{
    for (auto iter : *this)
        iter.~T();

    _size = 0;
}



//------------------------------------------------------------------------------
template <typename T, unsigned int SIZE>
class FixedArray
    : public Array<T>
{
public:
                FixedArray() : Array<T>(_buffer, 0, SIZE) {}

private:
    T           _buffer[SIZE];
};
