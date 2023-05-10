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
        child->_parent = this;
        if (Section* tail = _child)
        {
            for (; tail->_sibling != nullptr; tail = tail->_sibling);
            tail->_sibling = child;
        }
        else
            _child = child;
    }

    void enter(Section*& tree_iter, const char* name) {
        _name = name;

        if (_parent == nullptr)
            if (Section* parent = get_outer_store())
                parent->add_child(this);

        for (; tree_iter->_child != nullptr; tree_iter = tree_iter->_child)
            tree_iter->_active = true;
        tree_iter->_active = true;

        get_outer_store() = this;
    }

    static Section*&    get_outer_store() { static Section* section; return section; }
    void                leave() { get_outer_store() = _parent; }
    explicit            operator bool () { return _active; }
    const char*         _name = "";
    Section*            _parent = nullptr;
    Section*            _child = nullptr;
    Section*            _sibling = nullptr;
    uint32              _assert_count = 0;
    bool                _active = false;

    struct Scope
    {
                        Scope(Section*& tree_iter, Section& section, const char* name) : _section(&section) { _section->enter(tree_iter, name); }
                        ~Scope() { _section->leave(); }
        explicit        operator bool () { return bool(*_section); }
        Section*        _section;
    };
};

//------------------------------------------------------------------------------
struct Test
{
    typedef void        (test_func)(Section*&);
    static Test*&       get_head() { static Test* head; return head; }
    static Test*&       get_tail() { static Test* tail; return tail; }
    Test*               _next = nullptr;
    test_func*          _func;
    const char*         _name;

    Test(const char* name, test_func* func)
    : _name(name)
    , _func(func)
    {
        if (get_head() == nullptr)
            get_head() = this;

        if (Test* tail = get_tail())
            tail->_next = this;
        get_tail() = this;
    }
};

//------------------------------------------------------------------------------
inline bool run(const char* prefix="")
{
    int32 fail_count = 0;
    int32 test_count = 0;
    int32 assert_count = 0;

    for (Test* Test = Test::get_head(); Test != nullptr; Test = Test->_next)
    {
        // Cheap lower-case prefix Test.
        const char* a = prefix, *b = Test->_name;
        for (; (*a & ~0x20) == (*b & ~0x20); ++a, ++b);
        if (*a)
            continue;

        ++test_count;
        printf("......... %s", Test->_name);

        Section root;

        try
        {
            Section* tree_iter = &root;
            do
            {
                Section::Scope x = Section::Scope(tree_iter, root, Test->_name);

                (Test->_func)(tree_iter);

                for (Section* parent; parent = tree_iter->_parent; tree_iter = parent)
                {
                    tree_iter->_active = false;
                    if (tree_iter = tree_iter->_sibling)
                        break;
                }
            }
            while (tree_iter != &root);
        }
        catch (...)
        {
            ++fail_count;
            assert_count += root._assert_count;
            continue;
        }

        assert_count += root._assert_count;
        puts("\rok ");
    }

    printf("\n tests:%d  failed:%d  asserts:%d\n", test_count, fail_count, assert_count);

    return (fail_count == 0);
}

//------------------------------------------------------------------------------
inline void fail(const char* expr, const char* file, int32 line)
{
    Section* failed_section = clatch::Section::get_outer_store();

    puts("\n");
    printf(" expr; %s\n", expr);
    printf("where; %s(%d)\n", file, line);
    printf("trace; ");
    for (; failed_section != nullptr; failed_section = failed_section->_parent)
        printf("%s\n       ", failed_section->_name);
    puts("");

    throw std::exception();
}

//------------------------------------------------------------------------------
template <typename CALLBACK>
void fail(const char* expr, const char* file, int32 line, CALLBACK&& cb)
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
        for (; _clatch_s->_parent != nullptr; _clatch_s = _clatch_s->_parent);\
        ++_clatch_s->_assert_count;\
        \
        if (!(expr))\
            clatch::fail(#expr, __FILE__, __LINE__, ##__VA_ARGS__);\
    } while (0)
