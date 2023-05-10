// Copyright (c) 2017 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/str_iter.h>
#include <lib/line_editor.h>
#include <terminal/terminal.h>
#include <terminal/terminal_in.h>
#include <terminal/terminal_out.h>

#include <Windows.h>
#include <xmmintrin.h>

//------------------------------------------------------------------------------
struct Handle
{
           Handle()                 = default;
           Handle(const Handle&)    = delete;
           Handle(const Handle&&)   = delete;
           Handle(HANDLE h)         { value = (h == INVALID_HANDLE_VALUE) ? nullptr : h; }
           ~Handle()                { close(); }
           operator HANDLE () const { return value; }
    void   operator = (HANDLE h)    { close(); value = h; }
    void   close()                  { if (value != nullptr) CloseHandle(value); }
    HANDLE value = nullptr;
};



//------------------------------------------------------------------------------
class TestEditor
{
public:
    void            start(const char* prompt);
    void            end();
    void            press_keys(const char* keys);

private:
    Terminal        _terminal;
    LineEditor*     _editor;
    Handle          _thread;
};

//------------------------------------------------------------------------------
void TestEditor::start(const char* prompt)
{
    _terminal = terminal_create();

    LineEditor::Desc desc = {};
    desc.input = _terminal.in;
    desc.output = _terminal.out;
    desc.prompt = prompt;
    _editor = line_editor_create(desc);

    auto thread = [] (void* param) -> DWORD {
        auto* self = (TestEditor*)param;
        char c;
        self->_editor->edit(&c, sizeof(c));
        return 0;
    };

    _thread = CreateThread(nullptr, 0, thread, this, 0, nullptr);
}

//------------------------------------------------------------------------------
void TestEditor::end()
{
    press_keys("\n");
    WaitForSingleObject(_thread, INFINITE);
    line_editor_destroy(_editor);
    terminal_destroy(_terminal);
}

//------------------------------------------------------------------------------
void TestEditor::press_keys(const char* keys)
{
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD written;

    INPUT_RECORD input_record = { KEY_EVENT };
    KEY_EVENT_RECORD& key_event = input_record.Event.KeyEvent;
    key_event.bKeyDown = TRUE;
    while (int32 c = *keys++)
    {
        key_event.uChar.UnicodeChar = wchar_t(c);
        WriteConsoleInput(handle, &input_record, 1, &written);
    }
}



//------------------------------------------------------------------------------
class TestConsole
{
public:
            TestConsole();
            ~TestConsole();
    void    resize(int32 columns) { resize(columns, _rows); }
    void    resize(int32 columns, int32 rows);
    int32   get_columns() const { return _columns; }
    int32   get_rows() const    { return _rows; }

private:
    Handle  _handle;
    HANDLE  _prev_handle;
    int32   _columns;
    int32   _rows;
};

//------------------------------------------------------------------------------
TestConsole::TestConsole()
: _prev_handle(GetStdHandle(STD_OUTPUT_HANDLE))
{
    _handle = CreateConsoleScreenBuffer(GENERIC_WRITE|GENERIC_READ, 3, nullptr,
        CONSOLE_TEXTMODE_BUFFER, nullptr);

    COORD cursor_pos = { 0, 0 };
    SetConsoleCursorPosition(_handle, cursor_pos);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(_handle , &csbi);
    _columns = csbi.dwSize.X;
    _rows = csbi.dwSize.Y;

    SetConsoleActiveScreenBuffer(_handle);
    SetStdHandle(STD_OUTPUT_HANDLE, _handle);
}

//------------------------------------------------------------------------------
TestConsole::~TestConsole()
{
    SetStdHandle(STD_OUTPUT_HANDLE, _prev_handle);
}

//------------------------------------------------------------------------------
void TestConsole::resize(int32 columns, int32 rows)
{
    SMALL_RECT rect = { 0, 0, int16(columns - 1), int16(rows - 1) };
    COORD size = { int16(columns), int16(rows) };

    SetConsoleWindowInfo(_handle, TRUE, &rect);
    SetConsoleScreenBufferSize(_handle, size);
    SetConsoleWindowInfo(_handle, TRUE, &rect);

    _columns = columns;
    _rows = rows;
}



//------------------------------------------------------------------------------
class Stepper
{
public:
                    Stepper(int32 timeout_ms);
                    ~Stepper();
    bool            step();

private:
    enum State
    {
        state_running,
        state_step,
        state_paused,
        state_quit,
    };

    void            run_input_thread();
    Handle          _input_thread;
    volatile State  _state;
    int32           _timeout_ms;
};

//------------------------------------------------------------------------------
Stepper::Stepper(int32 timeout_ms)
: _state(state_running)
, _timeout_ms(timeout_ms)
{
    auto thunk = [] (void* param) -> DWORD {
        auto* self = (Stepper*)param;
        self->run_input_thread();
        return 0;
    };

    _input_thread = CreateThread(nullptr, 0, thunk, this, 0, nullptr);
}

//------------------------------------------------------------------------------
Stepper::~Stepper()
{
    TerminateThread(_input_thread, 0);
}

//------------------------------------------------------------------------------
void Stepper::run_input_thread()
{
/*
    HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    while (true)
    {
        DWORD count;
        INPUT_RECORD record;
        if (!ReadConsoleInputW(stdin_handle, &record, 1, &count))
        {
            _state = state_quit;
            break;
        }

        if (record.EventType != KEY_EVENT)
            continue;

        const auto& key_event = record.Event.KeyEvent;
        if (!key_event.bKeyDown)
            continue;

        switch (key_event.wVirtualKeyCode)
        {
        case VK_ESCAPE: _state = state_quit; break;
        case VK_SPACE:  _state = state_step; break;
        }
    }
*/
}

//------------------------------------------------------------------------------
bool Stepper::step()
{
    switch (_state)
    {
    case state_paused:  while (_state == state_paused) Sleep(10); break;
    case state_running: Sleep(_timeout_ms); break;
    case state_step:    _state = state_paused; break;
    }

    return (_state != state_quit);
}



//------------------------------------------------------------------------------
class Runner
{
public:
                        Runner();
    void                go();

private:
    bool                step();
    void                ecma48_test();
    void                line_test();
    TestConsole         _console;
    Stepper             _stepper;
};

//------------------------------------------------------------------------------
Runner::Runner()
: _stepper(200)
{
    srand(0xa9e);
    _console.resize(80, 25);
}

//------------------------------------------------------------------------------
void Runner::go()
{
    ecma48_test();
    line_test();
}

//------------------------------------------------------------------------------
bool Runner::step()
{
    return _stepper.step();
}

//------------------------------------------------------------------------------
void Runner::ecma48_test()
{
#define CSI(x) "\x1b[" #x
    Terminal terminal = terminal_create();
    TerminalOut& output = *terminal.out;
    output.begin();

    // Clear screen after
    output.write(CSI(41m) CSI(3;3H) "X" CSI(J), -1);
    if (!step())
        return;

    // Clear screen before
    output.write(CSI(42m) CSI(5;3H) "X\b\b" CSI(1J), -1);
    if (!step())
        return;

    // Clear screen all
    output.write(CSI(43m) CSI(2J), -1);
    if (!step())
        return;

    // Clear line after
    output.write(CSI(44m) CSI(4;4H) "X" CSI(K), -1);
    if (!step())
        return;

    // Clear line before
    output.write(CSI(45m) CSI(5;4H) "X\b\b" CSI(1K), -1);
    if (!step())
        return;

    // All line
    output.write("\n" CSI(46m) CSI(2K), -1);
    if (!step())
        return;

    output.write(CSI(0m) CSI(1;1H) CSI(J), -1);
    output.end();
    terminal_destroy(terminal);
#undef CSI
}

//------------------------------------------------------------------------------
void Runner::line_test()
{
    TestEditor editor;

#if 1
    editor.start("prompt\n->$ ");
#else
    editor.start("prompt$ ");
#endif // 0
    const char word[] = "9876543210";
    for (int32 i = 0; i < 100;)
    {
        editor.press_keys("_");
        int32 j = rand() % (sizeof(word) - 2);
        editor.press_keys(word + j);
        i += sizeof(word) - j;
    }

    int32 columns = _console.get_columns();
    for (; columns > 16 && step(); --columns)
        _console.resize(columns);

    for (; columns < 60 && step(); ++columns)
        _console.resize(columns);

    step();
    editor.end();
}



//------------------------------------------------------------------------------
int32 draw_test(int32, char**)
{
    Runner().go();
    return 0;
}
