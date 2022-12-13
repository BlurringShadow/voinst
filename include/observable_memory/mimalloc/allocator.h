#pragma once

#include "resource.h"

namespace observable_memory::mimalloc
{
    template<typename T>
    auto& get_default_allocator() noexcept
    {
        static pmr::polymorphic_allocator<T> alloc{get_default_resource()};
        return alloc;
    }
}