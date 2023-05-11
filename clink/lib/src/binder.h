// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/array.h>

class EditorModule;

//------------------------------------------------------------------------------
class Binder
{
public:
                        Binder();
    int32               get_group(const char* name=nullptr);
    int32               create_group(const char* name);
    bool                bind(uint32 group, const char* chord, EditorModule& module, uint8 id);

private:
    static const int32  link_bits = 9;
    static const int32  module_bits = 6;

    struct Node
    {
        uint16          is_group    : 1;
        uint16          next        : link_bits;
        uint16          module      : module_bits;

        uint16          child       : link_bits;
        uint16          depth       : 4;
        uint16          bound       : 1;
        uint16                      : 2;

        uint8           key;
        uint8           id;
    };

    struct GroupNode
    {
        uint16          is_group    : 1;
        uint16          next        : link_bits;
        uint16                      : 6;
        uint16          hash[2];
    };

    typedef FixedArray<EditorModule*, (1 << module_bits)> Modules;

    friend class        BindResolver;
    int32               insert_child(int32 parent, uint8 key);
    int32               find_child(int32 parent, uint8 key) const;
    int32               add_child(int32 parent, uint8 key);
    int32               find_tail(int32 head);
    int32               append(int32 head, uint8 key);
    const Node&         get_node(uint32 index) const;
    GroupNode*          get_group_node(uint32 index);
    int32               alloc_nodes(uint32 count=1);
    int32               add_module(EditorModule& module);
    EditorModule*       get_module(uint32 index) const;
    Modules             _modules;
    Node                _nodes[1 << link_bits];
    uint32              _next_node;
};
