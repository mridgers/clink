// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "binder.h"
#include "bind_resolver.h"

#include <core/base.h>
#include <core/str_hash.h>

#include <algorithm>

//------------------------------------------------------------------------------
template <int32 SIZE> static bool translate_chord(const char* chord, char (&out)[SIZE])
{
    // '\M-x'           = alt-x
    // '\C-x' or '^x'   = ctrl-x
    // '\e[t'           = ESC [ t (aka CSI t)
    // 'abc'            = abc

    int32 i = 0;
    for (; i < (SIZE - 1) && *chord; ++i, ++chord)
    {
        if (*chord != '\\' && *chord != '^')
        {
            out[i] = *chord;
            continue;
        }

        if (*chord == '^')
        {
            if (!*++chord)
            {
                out[i] = '^';
                --chord;
            }
            else
                out[i] = *chord & 0x1f;
            continue;
        }

        ++chord;
        switch (*chord)
        {
        case '\0':
            out[i] = '\\';
            --chord;
            continue;

        case 'M':
            if (chord[1] != '-' || !chord[2])
                return false;

            out[i] = '\x1b';

            ++chord; // move to '-'
            if (chord[1] == 'C' && chord[2] == '-') // 'C-' following?
            {
                ++i;
                chord += 3; // move past '-C-'

                if (i >= (SIZE - 1) || !*chord)
                    return false;

                out[i] = *chord & 0x1f;
            }
            continue;

        case 'C':
            if (chord[1] != '-' || !chord[2])
                return false;

            chord += 2;
            out[i] = *chord & 0x1f;
            continue;

        // Some escape sequences for convenience.
        case 'e':   out[i] = '\x1b';    break;
        case 't':   out[i] = '\t';      break;
        case 'n':   out[i] = '\n';      break;
        case 'r':   out[i] = '\r';      break;
        case '0':   out[i] = '\0';      break;
        default:    out[i] = *chord;    break;
        }
    }

    out[i] = '\0';
    return true;
}



//------------------------------------------------------------------------------
Binder::Binder()
{
    // Initialise the default group.
    _nodes[0] = { 1 };
    _nodes[1] = { 0 };
    _next_node = 2;

    static_assert(sizeof(Node) == sizeof(GroupNode), "Size assumption");
}

//------------------------------------------------------------------------------
int32 Binder::get_group(const char* name)
{
    if (name == nullptr || name[0] == '\0')
        return 1;

    uint32 hash = str_hash(name);

    int32 index = get_group_node(0)->next;
    while (index)
    {
        const GroupNode* Node = get_group_node(index);
        if (*(int32*)(Node->hash) == hash)
            return index + 1;

        index = Node->next;
    }

    return -1;
}

//------------------------------------------------------------------------------
int32 Binder::create_group(const char* name)
{
    if (name == nullptr || name[0] == '\0')
        return -1;

    int32 index = alloc_nodes(2);
    if (index < 0)
        return -1;

    // Create a new group Node;
    GroupNode* group = get_group_node(index);
    *(int32*)(group->hash) = str_hash(name);
    group->is_group = 1;

    // Link the new Node into the front of the list.
    GroupNode* master = get_group_node(0);
    group->next = master->next;
    master->next = index;

    // Next Node along is the group's root.
    ++index;
    _nodes[index] = {};
    return index;
}

//------------------------------------------------------------------------------
bool Binder::bind(
    uint32 group,
    const char* chord,
    EditorModule& module,
    uint8 id)
{
    // Validate Input
    if (group >= sizeof_array(_nodes))
        return false;

    // Translate from ASCII representation to actual keys.
    char translated[64];
    if (!translate_chord(chord, translated))
        return false;

    chord = translated;

    // Store the module pointer
    int32 module_index = add_module(module);
    if (module_index < 0)
        return false;

    // Add the chord of keys into the Node graph.
    int32 depth = 0;
    int32 head = group;
    for (; *chord; ++chord, ++depth)
        if (!(head = insert_child(head, *chord)))
            return false;

    --chord;

    // If the insert point is already bound we'll duplicate the Node at the end
    // of the list. Also check if this is a duplicate of the existing bind.
    Node* bindee = _nodes + head;
    if (bindee->bound)
    {
        int32 check = head;
        while (check > head)
        {
            if (bindee->key == *chord && bindee->module == module_index && bindee->id == id)
                return true;

            check = bindee->next;
            bindee = _nodes + check;
        }

        head = append(head, *chord);
    }

    if (!head)
        return false;

    bindee = _nodes + head;
    bindee->module = module_index;
    bindee->bound = 1;
    bindee->depth = depth;
    bindee->id = id;

    return true;
}

//------------------------------------------------------------------------------
int32 Binder::insert_child(int32 parent, uint8 key)
{
    if (int32 child = find_child(parent, key))
        return child;

    return add_child(parent, key);
}

//------------------------------------------------------------------------------
int32 Binder::find_child(int32 parent, uint8 key) const
{
    const Node* Node = _nodes + parent;

    int32 index = Node->child;
    for (; index > parent; index = Node->next)
    {
        Node = _nodes + index;
        if (Node->key == key)
            return index;
    }

    return 0;
}

//------------------------------------------------------------------------------
int32 Binder::add_child(int32 parent, uint8 key)
{
    int32 child = alloc_nodes();
    if (child < 0)
        return 0;

    Node addee = {};
    addee.key = key;

    int32 current_child = _nodes[parent].child;
    if (current_child < parent)
    {
        addee.next = parent;
        _nodes[parent].child = child;
    }
    else
    {
        int32 tail = find_tail(current_child);
        addee.next = parent;
        _nodes[tail].next = child;
    }

    _nodes[child] = addee;
    return child;
}

//------------------------------------------------------------------------------
int32 Binder::find_tail(int32 head)
{
    while (_nodes[head].next > head)
        head = _nodes[head].next;

    return head;
}

//------------------------------------------------------------------------------
int32 Binder::append(int32 head, uint8 key)
{
    int32 index = alloc_nodes();
    if (index < 0)
        return 0;

    int32 tail = find_tail(head);

    Node addee = {};
    addee.key = key;
    addee.next = _nodes[tail].next;
    _nodes[tail].next = index;

    _nodes[index] = addee;
    return index;
}

//------------------------------------------------------------------------------
const Binder::Node& Binder::get_node(uint32 index) const
{
    if (index < sizeof_array(_nodes))
        return _nodes[index];

    static const Node zero = {};
    return zero;
}

//------------------------------------------------------------------------------
Binder::GroupNode* Binder::get_group_node(uint32 index)
{
    if (index < sizeof_array(_nodes))
        return (GroupNode*)(_nodes + index);

    return nullptr;
}

//------------------------------------------------------------------------------
int32 Binder::alloc_nodes(uint32 count)
{
    int32 next = _next_node + count;
    if (next > sizeof_array(_nodes))
        return -1;

    _next_node = next;
    return _next_node - count;
}

//------------------------------------------------------------------------------
int32 Binder::add_module(EditorModule& module)
{
    for (int32 i = 0, n = _modules.size(); i < n; ++i)
        if (*(_modules[i]) == &module)
            return i;

    if (EditorModule** slot = _modules.push_back())
    {
        *slot = &module;
        return int32(slot - _modules.front());
    }

    return -1;
}

//------------------------------------------------------------------------------
EditorModule* Binder::get_module(uint32 index) const
{
    auto b = _modules[index];
    return b ? *b : nullptr;
}
