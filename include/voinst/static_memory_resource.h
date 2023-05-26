#pragma once

#include "namespace_alias.h"

namespace voinst
{
    template<::std::size_t Size>
    class static_memory_resource : pmr::memory_resource
    {
        ::stdsharp::static_memory_resource<Size> rsc_;

    protected:
        constexpr void* do_allocate(std::size_t bytes, const std::size_t) override
        {
            return rsc_.allocate(bytes);
        }

        constexpr void do_deallocate(void* ptr, const std::size_t size, const std::size_t) override
        {
            rsc_.deallocate(ptr, size);
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return star::to_void_pointer(this) == &other;
        }

    public:
        constexpr void release() noexcept { rsc_.release(); }
    };
}