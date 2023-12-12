#pragma once

#include <thread>
#include <unordered_set>
#include <stdsharp/synchronizer.h>
#include <stdsharp/lazy.h>

#include <mimalloc.h>

#include "alias.h"

namespace voinst
{
    struct deleter
    {
        void operator()(void* const p) const noexcept { mi_free(p); }

        void operator()(nullptr_t) = delete;
    };

    template<typename T>
    struct allocator
    {
        using value_type = T;

        using is_always_equal = std::true_type;

        allocator() = default;

        template<typename U>
        constexpr allocator(const allocator<U> /*unused*/) noexcept
        {
        }

        [[nodiscard]] constexpr auto allocate(const size_t n) const
        {
            return mi_new_aligned(n * sizeof(T), alignof(T));
        }

        constexpr void deallocate(T* const p, const size_t n) const noexcept
        {
            mi_free_size_aligned(p, n * sizeof(T), alignof(T));
        }

        constexpr bool operator==(const allocator /*unused*/) const noexcept { return true; }
    };
}

namespace voinst
{
    class memory_resource : public std::pmr::memory_resource
    {
        using heap = mi_heap_t;

        struct heap_deleter
        {
            void operator()(heap* const p) const noexcept { mi_heap_destroy(p); }
        };

        using thread_id = std::thread::id;
        using heap_ptr = std::unique_ptr<heap, heap_deleter>;

        struct heap_getter
        {
            heap_ptr operator()() const { return heap_ptr{mi_heap_new()}; }
        };

        using lazy_heap = star::lazy_value<heap_getter>;

        star::synchronizer<std::unordered_map<
            thread_id,
            heap_ptr,
            std::hash<thread_id>,
            std::ranges::equal_to,
            allocator<std::pair<const thread_id, heap_ptr>>>>
            heaps_{};
    };

    inline auto& get_default_resource()
    {
        static constinit memory_resource mem_rsc{};
        return mem_rsc;
    }
}