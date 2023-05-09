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
        BindResolver*   m_outer = nullptr;
        unsigned short  m_node_index;
        unsigned char   m_module;
        unsigned char   m_depth;
        unsigned char   m_id;
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
    const Binder&       m_binder;
    unsigned short      m_node_index = 1;
    unsigned short      m_group = 1;
    bool                m_pending_input = false;
    unsigned char       m_tail = 0;
    unsigned char       m_key_count = 0;
    char                m_keys[8];
};
