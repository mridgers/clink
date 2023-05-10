// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

class Binder;
class EditorModule;
class StrBase;

//------------------------------------------------------------------------------
class BindResolver
{
public:
    class Binding
    {
    public:
        explicit        operator bool () const;
        EditorModule*   get_module() const;
        unsigned char   get_id() const;
        void            get_chord(StrBase& chord) const;
        void            claim();

    private:
        friend class    BindResolver;
                        Binding() = default;
                        Binding(BindResolver* resolver, int node_index);
        BindResolver*   _outer = nullptr;
        unsigned short  _node_index;
        unsigned char   _module;
        unsigned char   _depth;
        unsigned char   _id;
    };

                        BindResolver(const Binder& binder);
    void                set_group(int group);
    int                 get_group() const;
    bool                step(unsigned char key);
    Binding             next();
    void                reset();

private:
    void                claim(Binding& Binding);
    bool                step_impl(unsigned char key);
    const Binder&       _binder;
    unsigned short      _node_index = 1;
    unsigned short      _group = 1;
    bool                _pending_input = false;
    unsigned char       _tail = 0;
    unsigned char       _key_count = 0;
    char                _keys[8];
};
