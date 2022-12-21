#pragma once

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

#define ALLOC_MEMBER_FUN(attr, name)                                                   \
    template<typename... Args>                                                         \
        requires requires(T t) { traits::name(t, ::std ::declval<Args>()...); }        \
    constexpr auto name(Args&&... args)                                                \
        const noexcept(noexcept(traits::name(this->get(), ::std::declval<Args>()...))) \
    {                                                                                  \
        return traits::name(this->get(), ::std::forward<Args>(args)...);               \
    }

        ALLOC_MEMBER_FUN([[nodiscard]], allocate)
        ALLOC_MEMBER_FUN(, deallocate)
        ALLOC_MEMBER_FUN([[nodiscard]], max_size)
        ALLOC_MEMBER_FUN([[nodiscard]], construct)
        ALLOC_MEMBER_FUN(, destroy)
        ALLOC_MEMBER_FUN([[nodiscard]], select_on_container_copy_construction)

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
            return ::std::ranges::equal_to{}(this->get(), alloc);
        }
    };

    template<typename T>
    proxy_allocator(T&) -> proxy_allocator<T>;

    template<typename T>
    class allocator : public proxy_allocator<pmr::polymorphic_allocator<T>>
    {
        [[nodiscard]] static constexpr auto& get_instance() noexcept
        {
            static pmr::polymorphic_allocator<T> alloc{&get_resource()};
            return alloc;
        }

    public:
        constexpr allocator() noexcept:
            proxy_allocator<pmr::polymorphic_allocator<T>>(get_instance())
        {
        }

        using is_always_equal = ::std::true_type;
    };
}