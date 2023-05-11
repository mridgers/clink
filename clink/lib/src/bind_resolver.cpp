// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "bind_resolver.h"
#include "binder.h"

#include <core/base.h>
#include <core/str.h>

#include <new>

//------------------------------------------------------------------------------
BindResolver::Binding::Binding(BindResolver* resolver, int32 node_index)
: _outer(resolver)
, _node_index(node_index)
{
    const Binder& binder = _outer->_binder;
    const auto& Node = binder.get_node(_node_index);

    _module = Node.module;
    _depth = max<uint8>(1, Node.depth);
    _id = Node.id;
}

//------------------------------------------------------------------------------
BindResolver::Binding::operator bool () const
{
    return (_outer != nullptr);
}

//------------------------------------------------------------------------------
EditorModule* BindResolver::Binding::get_module() const
{
    if (_outer == nullptr)
        return nullptr;

    const Binder& binder = _outer->_binder;
    return binder.get_module(_module);
}

//------------------------------------------------------------------------------
uint8 BindResolver::Binding::get_id() const
{
    if (_outer == nullptr)
        return 0xff;

    return _id;
}

//------------------------------------------------------------------------------
void BindResolver::Binding::get_chord(StrBase& chord) const
{
    if (_outer == nullptr)
        return;

    chord.clear();
    chord.concat(_outer->_keys + _outer->_tail, _depth);
}

//------------------------------------------------------------------------------
void BindResolver::Binding::claim()
{
    if (_outer != nullptr)
        _outer->claim(*this);
}



//------------------------------------------------------------------------------
BindResolver::BindResolver(const Binder& binder)
: _binder(binder)
{
}

//------------------------------------------------------------------------------
void BindResolver::set_group(int32 group)
{
    if (uint32(group) - 1 >= sizeof_array(_binder._nodes) - 1)
        return;

    if (_group == group || !_binder.get_node(group - 1).is_group)
        return;

    _group = group;
    _node_index = group;
    _pending_input = true;
}

//------------------------------------------------------------------------------
int32 BindResolver::get_group() const
{
    return _group;
}

//------------------------------------------------------------------------------
void BindResolver::reset()
{
    int32 group = _group;

    new (this) BindResolver(_binder);

    _group = group;
    _node_index = _group;
}

//------------------------------------------------------------------------------
bool BindResolver::step(uint8 key)
{
    if (_key_count >= sizeof_array(_keys))
    {
        reset();
        return false;
    }

    _keys[_key_count] = key;
    ++_key_count;

    return step_impl(key);
}

//------------------------------------------------------------------------------
bool BindResolver::step_impl(uint8 key)
{
    int32 next = _binder.find_child(_node_index, key);
    if (!next)
        return true;

    _node_index = next;
    return (_binder.get_node(next).child == 0);
}

//------------------------------------------------------------------------------
BindResolver::Binding BindResolver::next()
{
    // Push remaining Input through the tree.
    if (_pending_input)
    {
        _pending_input = false;

        uint32 keys_remaining = _key_count - _tail;
        if (!keys_remaining || keys_remaining >= sizeof_array(_keys))
        {
            reset();
            return Binding();
        }

        for (int32 i = _tail, n = _key_count; i < n; ++i)
            if (step_impl(_keys[i]))
                break;
    }

    // Go along this depth's nodes looking for valid binds.
    while (_node_index)
    {
        const Binder::Node& Node = _binder.get_node(_node_index);

        // Move iteration along to the next Node.
        int32 node_index = _node_index;
        _node_index = Node.next;

        // Check to see if where we're currently at a Node in the tree that is
        // a valid bind (at the point of call).
        int32 key_index = _tail + Node.depth - 1;
        if (Node.bound && (!Node.key || Node.key == _keys[key_index]))
            return Binding(this, node_index);
    }

    // We can't get any further traversing the tree with the Input provided.
    reset();
    return Binding();
}

//------------------------------------------------------------------------------
void BindResolver::claim(Binding& Binding)
{
    _tail += Binding._depth;
    _node_index = _group;
    _pending_input = true;
}
