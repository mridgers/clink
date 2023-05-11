// Copyright (c) Martin Ridgers
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
        uint8           get_id() const;
        void            get_chord(StrBase& chord) const;
        void            claim();

    private:
        friend class    BindResolver;
                        Binding() = default;
                        Binding(BindResolver* resolver, int32 node_index);
        BindResolver*   _outer = nullptr;
        uint16          _node_index;
        uint8           _module;
        uint8           _depth;
        uint8           _id;
    };

                        BindResolver(const Binder& binder);
    void                set_group(int32 group);
    int32               get_group() const;
    bool                step(uint8 key);
    Binding             next();
    void                reset();

private:
    void                claim(Binding& Binding);
    bool                step_impl(uint8 key);
    const Binder&       _binder;
    uint16              _node_index = 1;
    uint16              _group = 1;
    bool                _pending_input = false;
    uint8               _tail = 0;
    uint8               _key_count = 0;
    char                _keys[8];
};
