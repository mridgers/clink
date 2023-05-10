// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#include <core/array.h>

class EditorModule;

//------------------------------------------------------------------------------
class Binder
{
public:
                        Binder();
    int                 get_group(const char* name=nullptr);
    int                 create_group(const char* name);
    bool                bind(unsigned int group, const char* chord, EditorModule& module, unsigned char id);

private:
    static const int    link_bits = 9;
    static const int    module_bits = 6;

    struct Node
    {
        unsigned short  is_group    : 1;
        unsigned short  next        : link_bits;
        unsigned short  module      : module_bits;

        unsigned short  child       : link_bits;
        unsigned short  depth       : 4;
        unsigned short  bound       : 1;
        unsigned short              : 2;

        unsigned char   key;
        unsigned char   id;
    };

    struct GroupNode
    {
        unsigned short  is_group    : 1;
        unsigned short  next        : link_bits;
        unsigned short              : 6;
        unsigned short  hash[2];
    };

    typedef FixedArray<EditorModule*, (1 << module_bits)> Modules;

    friend class        BindResolver;
    int                 insert_child(int parent, unsigned char key);
    int                 find_child(int parent, unsigned char key) const;
    int                 add_child(int parent, unsigned char key);
    int                 find_tail(int head);
    int                 append(int head, unsigned char key);
    const Node&         get_node(unsigned int index) const;
    GroupNode*          get_group_node(unsigned int index);
    int                 alloc_nodes(unsigned int count=1);
    int                 add_module(EditorModule& module);
    EditorModule*       get_module(unsigned int index) const;
    Modules             _modules;
    Node                _nodes[1 << link_bits];
    unsigned int        _next_node;
};
