// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#pragma once

#define sizeof_array(x)     (sizeof(x) / sizeof(x[0]))
#define AS_STR(x)           AS_STR_IMPL(x)
#define AS_STR_IMPL(x)      #x

#if defined(_WIN32)
#   define PLATFORM_WINDOWS
#else
#   error Unsupported platform.
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#   define threadlocal      __declspec(thread)
#else
#   define threadlocal      thread_local
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900
#   define align_to(x)       __declspec(align(x))
#else
#   define align_to(x)      alignas(x)
#endif

#if defined(_M_AMD64) || defined(__x86_64__)
#   define ARCHITECTURE     64
#elif defined(_M_IX86) || defined(__i386)
#   define ARCHITECTURE     86
#else
#   error Unknown architecture
#endif

#undef min
template <class A> A min(A a, A b) { return (a < b) ? a : b; }

#undef max
template <class A> A max(A a, A b) { return (a > b) ? a : b; }

#undef clamp
template <class A> A clamp(A v, A m, A M) { return min(max(v, m), M); }

//------------------------------------------------------------------------------
#include <cstdint>
using int8  = int8_t;  using uint8  = uint8_t;
using int16 = int16_t; using uint16 = uint16_t;
using int32 = int32_t; using uint32 = uint32_t;
using int64 = int64_t; using uint64 = uint64_t;

//------------------------------------------------------------------------------
struct NoCopy
{
            NoCopy() = default;
            ~NoCopy() = default;

private:
            NoCopy(const NoCopy&) = delete;
            NoCopy(const NoCopy&&) = delete;
    void    operator = (const NoCopy&) = delete;
    void    operator = (const NoCopy&&) = delete;
};
