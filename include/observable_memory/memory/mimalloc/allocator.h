#pragma once

#include <mimalloc.h>
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include <stdsharp/memory/memory.h>

#include "resource.h"

namespace observable_memory::mimalloc
{
    struct deleter
    {
        void operator()(void* p) const noexcept { mi_free(p); }

        constexpr void operator()(::std::nullptr_t) = delete;
    };

    template<typename T>
    using allocator = mi_stl_allocator<T>;
}