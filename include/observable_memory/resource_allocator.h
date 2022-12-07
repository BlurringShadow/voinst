#pragma once

#include <mimalloc.h>

namespace observable_memory
{
    template<typename T>
    struct allocator : mi_stl_allocator<T>
    {
    };
}