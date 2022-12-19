#pragma once

#include <stdsharp/concepts/concepts.h>

#if __cpp_lib_polymorphic_allocator >= 201902L
    #include <memory_resource>
#else
    #include <boost/container/pmr/synchronized_pool_resource.hpp>
    #include <boost/container/pmr/unsynchronized_pool_resource.hpp>
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
    namespace mimalloc
    {
        namespace details
        {
            struct mi_memory_resource
            {
                static constexpr ::std::size_t max_align = MI_ALIGNMENT_MAX;

                static void* do_allocate(::std::size_t bytes, ::std::size_t alignment)
                {
                    return mi_new_aligned(bytes, alignment);
                }

                static void
                    do_deallocate(void* p, ::std::size_t bytes, ::std::size_t alignment) noexcept
                {
                    mi_free_size_aligned(p, bytes, alignment);
                }

                [[nodiscard]] constexpr bool operator==(const mi_memory_resource) const noexcept
                {
                    return true;
                }
            };
        }

        template<typename T>
        class memory_resource_adapter : public pmr::memory_resource, public T
        {
        protected:
            void* do_allocate(::std::size_t bytes, ::std::size_t alignment) override
            {
                return T::do_allocate(bytes, alignment);
            }

            void do_deallocate(void* p, ::std::size_t bytes, ::std::size_t alignment) noexcept
                override
            {
                T::do_deallocate(p, bytes, alignment);
            }

            [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) const //
                noexcept override
            {
                return static_cast<const T&>(*this) ==
                    static_cast<const T&>(dynamic_cast<const memory_resource_adapter&>(other));
            }
        };

        inline auto& get_default_resource() noexcept
        {
            static memory_resource_adapter<details::mi_memory_resource> resource{};
            return resource;
        }
    }
}