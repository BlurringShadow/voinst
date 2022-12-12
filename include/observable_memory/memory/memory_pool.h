#pragma once

#include <stdsharp/filesystem/filesystem.h>
#include <stdsharp/containers/containers.h>

#include <foonathan/memory/memory_pool.hpp>
#include <foonathan/memory/container.hpp>

#include <mimalloc.h>

namespace observable_memory
{
    namespace filesystem = ::stdsharp::filesystem;
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

    inline constexpr ::std::size_t default_block_nodes_count = 1024;

    template<typename PoolType = memory::node_pool, typename Allocator = mi_raw_allocator>
    struct memory_pool : memory::memory_pool<PoolType, Allocator>
    {
        using base = memory::memory_pool<PoolType, Allocator>;

        using base::base;

        using typename base::allocator_type;
        using typename base::pool_type;

        using base::min_node_size;
        using base::min_block_size;
        using base::node_size;
        using base::capacity_left;
        using base::next_capacity;
        using base::get_allocator;

        void* allocate_node() { return base::allocate_node(); }

        void* try_allocate_node() noexcept { return base::try_allocate_node(); }

        void* allocate_array(std::size_t n) { return base::allocate_array(n); }

        void* try_allocate_array(std::size_t n) noexcept { return base::try_allocate_array(n); }

        void deallocate_node(void* ptr) noexcept { base::deallocate_node(ptr); }

        bool try_deallocate_node(void* ptr) noexcept { return base::try_deallocate_node(ptr); }

        void deallocate_array(void* ptr, std::size_t n) noexcept { base::deallocate_array(ptr, n); }

        bool try_deallocate_array(void* ptr, std::size_t n) noexcept
        {
            return try_deallocate_array(ptr, n);
        }

        template<typename... Args>
            requires ::std::constructible_from<Allocator, Args...>
        memory_pool(
            const filesystem::bytes node_size,
            const filesystem::bytes block_size,
            Args&&... args
        ):
            base(node_size.size(), block_size.size(), ::std::forward<Args>(args)...)
        {
        }

        template<typename... Args>
            requires ::std::constructible_from<Allocator, Args...>
        memory_pool(
            const filesystem::bytes node_size,
            const ::std::size_t block_node_count = default_block_nodes_count,
            Args&&... args
        ):
            memory_pool(node_size, block_node_count * node_size, ::std::forward<Args>(args)...)
        {
        }

        template<typename OtherPoolType>
        using nested_pool = memory_pool<OtherPoolType, memory::allocator_reference<memory_pool>>;

        template<typename OtherPoolType, typename... Args>
            requires ::std::constructible_from<nested_pool<OtherPoolType>, Args..., memory_pool>
        nested_pool<OtherPoolType> make_nested_pool(Args&&... args)
        {
            return {::std::forward<Args>(args)..., *this};
        }

        template<typename OtherPoolType, typename T>
        auto make_nested_object_pool(
            const ::std::size_t block_object_count = default_block_nodes_count
        )
        {
            constexpr filesystem::bytes obj_size{sizeof(T)};
            return make_nested_pool<OtherPoolType>(obj_size, obj_size * block_object_count);
        }
    };

    using mi_memory_pool = memory_pool<memory::array_pool, mi_raw_allocator>;

    template<typename PoolType>
    using static_pool = memory_pool<PoolType, memory::static_block_allocator>;

    namespace details
    {
        template<template<typename, typename> typename, typename>
        struct default_pool_type;

        template<template<typename, typename> typename Container, typename T>
            requires requires(Container<T, mi_memory_pool> instance) //
        {
            requires ::stdsharp::allocator_aware_container<decltype(instance)>;
            requires !::stdsharp::contiguous_container<decltype(instance)>;
        }
        struct default_pool_type<Container, T> :
            ::std::type_identity< //
                ::std::conditional_t<
                    ::stdsharp::fundamental<T>,
                    memory::small_node_pool,
                    memory::node_pool // clang-format off
                >
            > // clang-format on
        {
        };

        template<template<typename, typename> typename Container, typename T>
            requires requires(Container<T, mi_memory_pool> instance) //
        {
            requires ::stdsharp::allocator_aware_container<decltype(instance)>;
            requires ::stdsharp::contiguous_container<decltype(instance)>;
        }
        struct default_pool_type<Container, T> : ::std::type_identity<memory::array_pool>
        {
        };
    }

    template<template<typename, typename> typename Container, typename T>
    using default_pool_type = typename details::default_pool_type<Container, T>::type;

    template<
        template<typename, typename>
        typename Container,
        typename T,
        typename PoolType = default_pool_type<Container, T> // clang-format off
    > // clang-format on
    inline constexpr ::stdsharp::nodiscard_invocable make_container_from_pool = []< //
        typename ParentPoolType,
        typename Allocator,
        typename nested = typename memory_pool<ParentPoolType, Allocator>:: // clang-format off
            template nested_pool<PoolType>
    >( // clang-format on
        memory_pool<ParentPoolType, Allocator> & pool,
        const ::std::size_t block_object_count = default_block_nodes_count
    )
    {
        using container = Container<T, nested>;

        struct local : container
        {
            nested pool;

            local(nested&& pool): pool(::std::move(pool)), container(pool) {}
        };

        return local{
            pool.template make_nested_object_pool<PoolType, T>(block_object_count) //
        };
    };
}

namespace foonathan::memory
{
    template<typename PoolType, class Allocator>
    struct allocator_traits<::observable_memory::memory_pool<PoolType, Allocator>> :
        allocator_traits<memory_pool<PoolType, Allocator>>
    {
    };
}