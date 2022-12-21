#pragma once

#if __cpp_lib_polymorphic_allocator >= 201902L
    #include <memory_resource>
#else
    #include <boost/container/pmr/synchronized_pool_resource.hpp>
    #include <boost/container/pmr/unsynchronized_pool_resource.hpp>
    #include <boost/container/pmr/polymorphic_allocator.hpp>
#endif

#include <mimalloc.h>

namespace observable_memory
{
    namespace pmr = ::

#if __cpp_lib_polymorphic_allocator >= 201902L
        std::pmr
#else
        boost::container::pmr
#endif
        ;
}

namespace observable_memory::mimalloc
{
    struct deleter
    {
        constexpr void operator()(void* p) const noexcept { mi_free(p); }

        constexpr void operator()(::std::nullptr_t) = delete;
    };

    struct memory_resource : pmr::memory_resource
    {
        static constexpr ::std::size_t max_align = MI_ALIGNMENT_MAX;

        [[nodiscard]] void*
            do_allocate(::std::size_t bytes, ::std::size_t alignment = max_align) override
        {
            return mi_new_aligned(bytes, alignment);
        }

        void do_deallocate(void* p, ::std::size_t bytes, ::std::size_t alignment) noexcept override
        {
            mi_free_size_aligned(p, bytes, alignment);
        }

        [[nodiscard]] constexpr bool do_is_equal( //
            const pmr::memory_resource& other
        ) const noexcept override
        {
            return dynamic_cast<const memory_resource*>(&other) != nullptr;
        }

        bool operator==(const memory_resource&) const = default;
    };

    [[nodiscard]] inline auto& get_resource() noexcept
    {
        static memory_resource resource{};
        return resource;
    }
}
