// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

//------------------------------------------------------------------------------
class LinearAllocator
{
public:
                          LinearAllocator(int size);
                          LinearAllocator(void* buffer, int size);
                          ~LinearAllocator();
    void*                 alloc(int size);
    template <class T> T* calloc(int count=1);

private:
    char*                 _buffer;
    int                   _used;
    int                   _max;
    bool                  _owned;
};

//------------------------------------------------------------------------------
inline LinearAllocator::LinearAllocator(int size)
: _buffer((char*)malloc(size))
, _used(0)
, _max(size)
, _owned(true)
{
}

//------------------------------------------------------------------------------
inline LinearAllocator::LinearAllocator(void* buffer, int size)
: _buffer((char*)buffer)
, _used(0)
, _max(size)
, _owned(false)
{
}

//------------------------------------------------------------------------------
inline LinearAllocator::~LinearAllocator()
{
    if (_owned)
        free(_buffer);
}

//------------------------------------------------------------------------------
inline void* LinearAllocator::alloc(int size)
{
    if (size == 0)
        return nullptr;

    int used = _used + size;
    if (used > _max)
        return nullptr;

    char* ptr = _buffer + _used;
    _used = used;
    return ptr;
}

//------------------------------------------------------------------------------
template <class T> T* LinearAllocator::calloc(int count)
{
    return (T*)(alloc(sizeof(T) * count));
}
