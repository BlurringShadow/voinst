#pragma once

#include "resource.h"

namespace observable_memory::mimalloc
{
    template<typename T>
    using default_allocator = pmr::polymorphic_allocator<T>;

    template<typename T>
    [[nodiscard]] default_allocator<int> get_default_allocator() noexcept
    {
        return {&get_default_resource()};
    }
}