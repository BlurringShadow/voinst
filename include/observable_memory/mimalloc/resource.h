#pragma once

#include <memory>

#include <stdsharp/concepts/concepts.h>

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

                bool operator==(const mi_memory_resource&) const = default;
            };
        }

        class memory_resource_adapter : public pmr::memory_resource
        {
            template<typename T>
            class impl : public pmr::memory_resource, T
            {
            public:
                using T::T;

                [[nodiscard]] constexpr T& get_resource() noexcept { return *this; }

                [[nodiscard]] constexpr const T& get_resource() const noexcept { return *this; }

                constexpr bool operator==(const memory_resource_adapter& other) const noexcept
                {
                    return do_is_equal(other);
                }

            protected:
                [[nodiscard]] constexpr void*
                    do_allocate(::std::size_t bytes, ::std::size_t alignment) override
                {
                    return T::do_allocate(bytes, alignment);
                }

                constexpr void
                    do_deallocate(void* p, ::std::size_t bytes, ::std::size_t alignment) noexcept
                    override
                {
                    T::do_deallocate(p, bytes, alignment);
                }

                [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) const //
                    noexcept override
                {
                    auto casted = dynamic_cast<const impl*>(&other);

                    return casted == nullptr ? false : get_resource() == casted->get_resource();
                }
            };

            template<typename T>
                requires ::std::constructible_from<impl<::std::remove_cvref_t<T>>, T>
            static auto construct_impl(T&& resource)
            {
                return ::std::
                    unique_ptr<pmr::memory_resource, mi_stl_allocator<pmr::memory_resource>>{};
            }

            decltype(construct_impl(0)) impl_;

        public:
            template<typename T>
                requires ::std::constructible_from<impl<::std::remove_cvref_t<T>>, T>
            constexpr memory_resource_adapter(T&& resource):
                impl_( //
                    ::std::make_unique<impl<::std::remove_cvref_t<T>>>( //
                        ::std::forward<T>(resource)
                    )
                )
            {
            }

            [[nodiscard]] constexpr pmr::memory_resource& get_resource() noexcept { return *impl_; }

            [[nodiscard]] constexpr const pmr::memory_resource& get_resource() const noexcept
            {
                return *impl_;
            }

            [[nodiscard]] constexpr bool operator==(const memory_resource_adapter& other
            ) const noexcept
            {
                return *impl_ == *other.impl_;
            }

            [[nodiscard]] constexpr T& get_resource() noexcept { return *this; }

            [[nodiscard]] constexpr const T& get_resource() const noexcept { return *this; }

            constexpr bool operator==(const memory_resource_adapter& other) const noexcept
            {
                return do_is_equal(other);
            }

        protected:
            [[nodiscard]] constexpr void*
                do_allocate(::std::size_t bytes, ::std::size_t alignment) override
            {
                return T::do_allocate(bytes, alignment);
            }

            constexpr void
                do_deallocate(void* p, ::std::size_t bytes, ::std::size_t alignment) noexcept
                override
            {
                T::do_deallocate(p, bytes, alignment);
            }

            [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) const //
                noexcept override
            {
                auto casted = dynamic_cast<const memory_resource_adapter*>(&other);

                return casted == nullptr ? false : get_resource() == casted->get_resource();
            }
        };

        [[nodiscard]] inline auto& get_default_resource() noexcept
        {
            static memory_resource_adapter<details::mi_memory_resource> resource{};
            return resource;
        }
    }
}