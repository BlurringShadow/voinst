#pragma once

#include <foonathan/memory/memory_pool_type.hpp>
#include <mimalloc.h>
#include <stdsharp/filesystem/filesystem.h>
#include <foonathan/memory/memory_pool.hpp>
#include <foonathan/memory/std_allocator.hpp>

#include <foonathan/memory/container.hpp>

namespace observable_memory
{
    namespace memory = ::foonathan::memory;

    struct mi_raw_allocator
    {
        using is_stateful = std::bool_constant<false>;

        static inline void* allocate_node(std::size_t size, std::size_t alignment)
        {
            return mi_new_aligned(size, alignment);
        }

        static inline void
            deallocate_node(void* node, std::size_t size, std::size_t alignment) noexcept
        {
            mi_free_size_aligned(node, size, alignment);
        }

        static inline void*
            allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            return allocate_node(count * size, alignment);
        }

        static inline void deallocate_array(
            void* ptr,
            std::size_t count,
            std::size_t size,
            std::size_t alignment
        ) noexcept
        {
            deallocate_node(ptr, count * size, alignment);
        }

        static constexpr ::std::size_t max_alignment() noexcept { return MI_ALIGNMENT_MAX; }
    };

    struct mi_memory_pool : memory::memory_pool<memory::array_pool, mi_raw_allocator>
    {
        template<typename T>
        using std_allocator = memory::std_allocator<T, mi_memory_pool>;

        template<typename T>
        std_allocator<T> get_std_allocator()
        {
            return *this;
        }

        template<typename PoolType>
        using nested_pool =
            memory::memory_pool<PoolType, memory::allocator_reference<mi_memory_pool>>;

        template<typename PoolType>
        nested_pool<PoolType>
            get_nested_pool(const ::std::size_t node_size, const ::std::size_t block_size)
        {
            return {node_size, block_size, *this};
        }

        template<typename PoolType>
        using container_alloc = nested_pool<PoolType>;

        template<typename T>
        auto make_vector()
        {
            return ::std::vector<T, nested_pool<memory::array_pool>>(
                get_nested_pool<memory::array_pool>()
            );
        }
    };

    template<typename PoolType>
    using static_pool = memory::memory_pool<PoolType, memory::static_block_allocator>;

    inline auto foo()
    {
        namespace filesystem = ::stdsharp::filesystem;
        using namespace stdsharp::literals;

        mi_memory_pool pool{
            foonathan::memory::list_node_size<int>::value,
            filesystem::bytes{4_KiB}.size() //
        };

        return make_nested_pool<memory::array_pool>(pool, sizeof(int), sizeof(int));
    }
}