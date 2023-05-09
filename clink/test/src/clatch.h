// Copyright (c) 2016 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include <stdio.h>
#include <stdlib.h>
#include <exception>

namespace clatch {

//------------------------------------------------------------------------------
struct Section
{
    void add_child(Section* child) {
        child->m_parent = this;
        if (Section* tail = m_child)
        {
            for (; tail->m_sibling != nullptr; tail = tail->m_sibling);
            tail->m_sibling = child;
        }
        else
            m_child = child;
    }

    void enter(Section*& tree_iter, const char* name) {
        m_name = name;

        if (m_parent == nullptr)
            if (Section* parent = get_outer_store())
                parent->add_child(this);

        for (; tree_iter->m_child != nullptr; tree_iter = tree_iter->m_child)
            tree_iter->m_active = true;
        tree_iter->m_active = true;

        get_outer_store() = this;
    }

    static Section*&    get_outer_store() { static Section* section; return section; }
    void                leave() { get_outer_store() = m_parent; }
    explicit            operator bool () { return m_active; }
    const char*         m_name = "";
    Section*            m_parent = nullptr;
    Section*            m_child = nullptr;
    Section*            m_sibling = nullptr;
    unsigned int        m_assert_count = 0;
    bool                m_active = false;

    struct Scope
    {
                        Scope(Section*& tree_iter, Section& section, const char* name) : m_section(&section) { m_section->enter(tree_iter, name); }
                        ~Scope() { m_section->leave(); }
        explicit        operator bool () { return bool(*m_section); }
        Section*        m_section;
    };
};

//------------------------------------------------------------------------------
struct Test
{
    typedef void        (test_func)(Section*&);
    static Test*&       get_head() { static Test* head; return head; }
    static Test*&       get_tail() { static Test* tail; return tail; }
    Test*               m_next = nullptr;
    test_func*          m_func;
    const char*         m_name;

    Test(const char* name, test_func* func)
    : m_name(name)
    , m_func(func)
    {
        if (get_head() == nullptr)
            get_head() = this;

        if (Test* tail = get_tail())
            tail->m_next = this;
        get_tail() = this;
    }
};

//------------------------------------------------------------------------------
inline bool run(const char* prefix="")
{
    int fail_count = 0;
    int test_count = 0;
    int assert_count = 0;

    for (Test* Test = Test::get_head(); Test != nullptr; Test = Test->m_next)
    {
        // Cheap lower-case prefix Test.
        const char* a = prefix, *b = Test->m_name;
        for (; (*a & ~0x20) == (*b & ~0x20); ++a, ++b);
        if (*a)
            continue;

        ++test_count;
        printf("......... %s", Test->m_name);

        Section root;

        try
        {
            Section* tree_iter = &root;
            do
            {
                Section::Scope x = Section::Scope(tree_iter, root, Test->m_name);

                (Test->m_func)(tree_iter);

                for (Section* parent; parent = tree_iter->m_parent; tree_iter = parent)
                {
                    tree_iter->m_active = false;
                    if (tree_iter = tree_iter->m_sibling)
                        break;
                }
            }
            while (tree_iter != &root);
        }
        catch (...)
        {
            ++fail_count;
            assert_count += root.m_assert_count;
            continue;
        }

        assert_count += root.m_assert_count;
        puts("\rok ");
    }

    printf("\n tests:%d  failed:%d  asserts:%d\n", test_count, fail_count, assert_count);

    return (fail_count == 0);
}

//------------------------------------------------------------------------------
inline void fail(const char* expr, const char* file, int line)
{
    Section* failed_section = clatch::Section::get_outer_store();

    puts("\n");
    printf(" expr; %s\n", expr);
    printf("where; %s(%d)\n", file, line);
    printf("trace; ");
    for (; failed_section != nullptr; failed_section = failed_section->m_parent)
        printf("%s\n       ", failed_section->m_name);
    puts("");

    throw std::exception();
}

//------------------------------------------------------------------------------
template <typename CALLBACK>
void fail(const char* expr, const char* file, int line, CALLBACK&& cb)
{
    puts("\n");
    cb();
    fail(expr, file, line);
}

} // namespace clatch

//------------------------------------------------------------------------------
#define CLATCH_IDENT__(d, b) _clatch_##d##_##b
#define CLATCH_IDENT_(d, b)  CLATCH_IDENT__(d, b)
#define CLATCH_IDENT(d)      CLATCH_IDENT_(d, __LINE__)

#define TEST_CASE(name)\
    static void CLATCH_IDENT(test_func)(clatch::Section*&);\
    static clatch::Test CLATCH_IDENT(Test)(name, CLATCH_IDENT(test_func));\
    static void CLATCH_IDENT(test_func)(clatch::Section*& _clatch_tree_iter)

#define SECTION(name)\
    static clatch::Section CLATCH_IDENT(Section);\
    if (clatch::Section::Scope CLATCH_IDENT(Scope) = clatch::Section::Scope(_clatch_tree_iter, CLATCH_IDENT(Section), name))


#define REQUIRE(expr, ...)\
    do {\
        auto* _clatch_s = clatch::Section::get_outer_store();\
        for (; _clatch_s->m_parent != nullptr; _clatch_s = _clatch_s->m_parent);\
        ++_clatch_s->m_assert_count;\
        \
        if (!(expr))\
            clatch::fail(#expr, __FILE__, __LINE__, ##__VA_ARGS__);\
    } while (0)
