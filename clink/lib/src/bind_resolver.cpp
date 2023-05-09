// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "bind_resolver.h"
#include "binder.h"

#include <core/base.h>
#include <core/str.h>

#include <new>

//------------------------------------------------------------------------------
BindResolver::Binding::Binding(BindResolver* resolver, int node_index)
: m_outer(resolver)
, m_node_index(node_index)
{
    const Binder& binder = m_outer->m_binder;
    const auto& Node = binder.get_node(m_node_index);

    m_module = Node.module;
    m_depth = max<unsigned char>(1, Node.depth);
    m_id = Node.id;
}

//------------------------------------------------------------------------------
BindResolver::Binding::operator bool () const
{
    return (m_outer != nullptr);
}

//------------------------------------------------------------------------------
EditorModule* BindResolver::Binding::get_module() const
{
    if (m_outer == nullptr)
        return nullptr;

    const Binder& binder = m_outer->m_binder;
    return binder.get_module(m_module);
}

//------------------------------------------------------------------------------
unsigned char BindResolver::Binding::get_id() const
{
    if (m_outer == nullptr)
        return 0xff;

    return m_id;
}

//------------------------------------------------------------------------------
void BindResolver::Binding::get_chord(StrBase& chord) const
{
    if (m_outer == nullptr)
        return;

    chord.clear();
    chord.concat(m_outer->m_keys + m_outer->m_tail, m_depth);
}

//------------------------------------------------------------------------------
void BindResolver::Binding::claim()
{
    if (m_outer != nullptr)
        m_outer->claim(*this);
}



//------------------------------------------------------------------------------
BindResolver::BindResolver(const Binder& binder)
: m_binder(binder)
{
}

//------------------------------------------------------------------------------
void BindResolver::set_group(int group)
{
    if (unsigned(group) - 1 >= sizeof_array(m_binder.m_nodes) - 1)
        return;

    if (m_group == group || !m_binder.get_node(group - 1).is_group)
        return;

    m_group = group;
    m_node_index = group;
    m_pending_input = true;
}

//------------------------------------------------------------------------------
int BindResolver::get_group() const
{
    return m_group;
}

//------------------------------------------------------------------------------
void BindResolver::reset()
{
    int group = m_group;

    new (this) BindResolver(m_binder);

    m_group = group;
    m_node_index = m_group;
}

//------------------------------------------------------------------------------
bool BindResolver::step(unsigned char key)
{
    if (m_key_count >= sizeof_array(m_keys))
    {
        reset();
        return false;
    }

    m_keys[m_key_count] = key;
    ++m_key_count;

    return step_impl(key);
}

//------------------------------------------------------------------------------
bool BindResolver::step_impl(unsigned char key)
{
    int next = m_binder.find_child(m_node_index, key);
    if (!next)
        return true;

    m_node_index = next;
    return (m_binder.get_node(next).child == 0);
}

//------------------------------------------------------------------------------
BindResolver::Binding BindResolver::next()
{
    // Push remaining Input through the tree.
    if (m_pending_input)
    {
        m_pending_input = false;

        unsigned int keys_remaining = m_key_count - m_tail;
        if (!keys_remaining || keys_remaining >= sizeof_array(m_keys))
        {
            reset();
            return Binding();
        }

        for (int i = m_tail, n = m_key_count; i < n; ++i)
            if (step_impl(m_keys[i]))
                break;
    }

    // Go along this depth's nodes looking for valid binds.
    while (m_node_index)
    {
        const Binder::Node& Node = m_binder.get_node(m_node_index);

        // Move iteration along to the next Node.
        int node_index = m_node_index;
        m_node_index = Node.next;

        // Check to see if where we're currently at a Node in the tree that is
        // a valid bind (at the point of call).
        int key_index = m_tail + Node.depth - 1;
        if (Node.bound && (!Node.key || Node.key == m_keys[key_index]))
            return Binding(this, node_index);
    }

    // We can't get any further traversing the tree with the Input provided.
    reset();
    return Binding();
}

//------------------------------------------------------------------------------
void BindResolver::claim(Binding& Binding)
{
    m_tail += Binding.m_depth;
    m_node_index = m_group;
    m_pending_input = true;
}
