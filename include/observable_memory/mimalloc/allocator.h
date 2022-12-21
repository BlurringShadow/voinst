#pragma once

#include <stdsharp/functional/operations.h>
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
    template<typename T>
    struct proxy_allocator : ::std::reference_wrapper<T>, private ::stdsharp::allocator_traits<T>
    {
        using ::std::reference_wrapper<T>::reference_wrapper;

        using traits = ::stdsharp::allocator_traits<T>;

        using typename traits::pointer;
        using typename traits::const_pointer;
        using typename traits::void_pointer;
        using typename traits::const_void_pointer;
        using typename traits::value_type;
        using typename traits::size_type;
        using typename traits::difference_type;
        using typename traits::propagate_on_container_copy_assignment;
        using typename traits::propagate_on_container_move_assignment;
        using typename traits::propagate_on_container_swap;
        using typename traits::is_always_equal;

        template<typename U>
        struct rebind
        {
            using other = proxy_allocator<U>;
        };

#define ALLOC_MEMBER_FUN(name)                                                         \
    template<typename... Args>                                                         \
        requires requires(T t) { traits::name(t, ::std ::declval<Args>()...); }        \
    constexpr auto name(Args&&... args)                                                \
        const noexcept(noexcept(traits::name(this->get(), ::std::declval<Args>()...))) \
    {                                                                                  \
        return traits::name(this->get(), ::std::forward<Args>(args)...);               \
    }

        ALLOC_MEMBER_FUN(allocate)
        ALLOC_MEMBER_FUN(deallocate)
        ALLOC_MEMBER_FUN(max_size)
        ALLOC_MEMBER_FUN(construct)
        ALLOC_MEMBER_FUN(destroy)
        ALLOC_MEMBER_FUN(select_on_container_copy_construction)

#undef ALLOC_MEMBER_FUN

        template<typename... Args>
            requires requires(T t) { t.allocate_at_least(::std::declval<Args>()...); }
        constexpr auto allocate_at_least(Args&&... args) const
            noexcept(noexcept(this->get().allocate_at_least(::std::declval<Args>()...)))
        {
            return this->get().allocate_at_least(::std::forward<Args>(args)...);
        }

        template<typename OtherAlloc>
            requires requires(proxy_allocator instance, OtherAlloc other) { instance == other; }
        constexpr bool operator==(const proxy_allocator<OtherAlloc>& alloc) const
            noexcept(noexcept(*this == alloc.get()))
        {
            return *this == alloc.get();
        }

        template<typename OtherAlloc>
            requires ::std::invocable<::std::ranges::equal_to, T, OtherAlloc>
        constexpr bool operator==(const OtherAlloc& alloc) const
            noexcept(::stdsharp::nothrow_invocable<::std::ranges::equal_to, T, OtherAlloc>)
        {
            return ::stdsharp::equal_to_v(this->get(), alloc);
        }
    };

    template<typename T>
    proxy_allocator(T&) -> proxy_allocator<T>;

    template<typename T>
    struct get_allocator_fn
    {
        [[nodiscard]] constexpr auto operator()() const noexcept
        {
            static pmr::polymorphic_allocator<T> alloc{&get_resource()};
            return proxy_allocator{alloc};
        }
    };

    template<typename T>
    inline constexpr get_allocator_fn<T> get_allocator{};

    template<typename T>
    using allocator = ::std::invoke_result_t<get_allocator_fn<T>>;

    template<template<typename...> typename Container, typename T>
    struct get_container_fn
    {
    private:
        template<template<typename...> typename>
        struct apply_to_container;

#define APPLY_TO_CONTAINER(name)                   \
    template<>                                     \
    struct apply_to_container<::std::name>         \
    {                                              \
        using type = ::std::name<T, allocator<T>>; \
    };

        APPLY_TO_CONTAINER(vector)
        APPLY_TO_CONTAINER(deque)
        APPLY_TO_CONTAINER(forward_list)
        APPLY_TO_CONTAINER(list)
        APPLY_TO_CONTAINER(set)
        APPLY_TO_CONTAINER(map)
        APPLY_TO_CONTAINER(unordered_set)
        APPLY_TO_CONTAINER(unordered_map)

#undef APPLY_TO_CONTAINER

    public:
        using container = typename apply_to_container<Container>::type;
        using alloc_t = allocator<T>;

        template<typename... Args>
            requires ::std::constructible_from<container, Args..., alloc_t>
        [[nodiscard]] constexpr auto operator()(Args&&... args) const
            noexcept(::stdsharp::nothrow_constructible_from<container, Args..., alloc_t>)
        {
            return container{::std::forward<Args>(args)..., get_allocator<T>()};
        }
    };

    template<template<typename...> typename Container, typename T>
    inline constexpr get_container_fn<Container, T> get_container{};
}