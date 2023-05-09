// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <assert.h>

//------------------------------------------------------------------------------
template <class T>
class Singleton
{
public:
                Singleton();
                ~Singleton();
    static T*   get();

private:
    static T*&  get_store();
};

//------------------------------------------------------------------------------
template <class T> Singleton<T>::Singleton()
{
    assert(get_store() == nullptr);
    get_store() = (T*)this;
}

//------------------------------------------------------------------------------
template <class T> Singleton<T>::~Singleton()
{
    get_store() = nullptr;
}

//------------------------------------------------------------------------------
template <class T> T* Singleton<T>::get()
{
    return get_store();
}

//------------------------------------------------------------------------------
template <class T> T*& Singleton<T>::get_store()
{
    static T* instance;
    return instance;
}
