// Copyright (c) Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"

#include <core/linear_allocator.h>

//------------------------------------------------------------------------------
TEST_CASE("LinearAllocator: basic")
{
    LinearAllocator allocator(8);
    REQUIRE(allocator.alloc(1) != nullptr);
    REQUIRE(allocator.alloc(7) != nullptr);
    REQUIRE(allocator.alloc(1) == nullptr);
}

//------------------------------------------------------------------------------
TEST_CASE("LinearAllocator: invalid")
{
    LinearAllocator allocator(8);
    REQUIRE(allocator.alloc(0) == nullptr);
    REQUIRE(allocator.alloc(9) == nullptr);
}

//------------------------------------------------------------------------------
TEST_CASE("LinearAllocator: calloc")
{
    LinearAllocator allocator(sizeof(int32) * 8);
    REQUIRE(allocator.calloc<int32>(0) == nullptr);
    REQUIRE(allocator.calloc<int32>() != nullptr);
    REQUIRE(allocator.calloc<int32>(7) != nullptr);
    REQUIRE(allocator.calloc<int32>(1) == nullptr);
}
