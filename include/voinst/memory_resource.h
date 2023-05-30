#pragma once

#include <unordered_set>

#include <mimalloc.h>

#include "namespace_alias.h"

namespace voinst
{
    struct deleter
    {
        void operator()(void* const p) const noexcept { mi_free(p); }

        void operator()(std::nullptr_t) = delete;
    };

    template<typename T>
    struct allocator
    {
        using value_type = T;

        using is_always_equal = std::true_type;

        allocator() = default;

        template<typename U>
        constexpr allocator(const allocator<U>) noexcept
        {
        }

        [[nodiscard]] constexpr auto
            allocate(const std::size_t n, const void* const hint = nullptr) const
        {
            const auto size = n * sizeof(T);

            return hint == nullptr ?
                star::pointer_cast<value_type>(mi_new_aligned(size, alignof(T))) :
                star::pointer_cast<value_type>( //
                    mi_realloc_aligned(
                        const_cast<void*>(hint), // NOLINT(*-const-cast)
                        size,
                        alignof(T)
                    )
                );
        }

        constexpr void deallocate(T* const p, const std::size_t) const noexcept { mi_free(p); }

        constexpr bool operator==(const allocator) const noexcept { return true; }
    };

    class allocation
    {
        std::size_t size_;
        std::size_t alignment_;
        void* ptr_;

    public:
        allocation(const std::size_t bytes, const std::align_val_t alignment, void* const ptr):
            size_(bytes), alignment_(star::auto_cast(alignment)), ptr_(ptr)
        {
        }

        [[nodiscard]] constexpr void* get() const noexcept { return ptr_; }

        [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }

        [[nodiscard]] constexpr std::size_t alignment() const noexcept { return alignment_; }

        [[nodiscard]] bool operator==(const allocation& other) const = default;

    protected:
        [[nodiscard]] constexpr auto& get() noexcept { return ptr_; }
    };

    class scoped_allocation : public allocation
    {
    public:
        using allocation::allocation;

        scoped_allocation(
            const std::size_t bytes,
            const std::align_val_t alignment,
            const bool allocate = true
        ):
            allocation(
                bytes,
                alignment,
                allocate ? mi_new_aligned(bytes, star::auto_cast(alignment)) : nullptr
            )
        {
        }

        void allocate()
        {
            auto& ptr = get();
            if(ptr == nullptr) ptr = mi_new_aligned(size(), alignment());
        }

        ~scoped_allocation()
        {
            auto& ptr = get();
            if(ptr != nullptr) mi_free(std::exchange(ptr, nullptr));
        }

        scoped_allocation(const scoped_allocation&) = delete;
        scoped_allocation(scoped_allocation&&) = default;
        scoped_allocation& operator=(const scoped_allocation&) = delete;
        scoped_allocation& operator=(scoped_allocation&&) = default;
    };
}

namespace std
{
    template<>
    struct hash<::voinst::allocation>
    {
        [[nodiscard]] size_t operator()(const ::voinst::allocation& alloc) const noexcept
        {
            return hash<const void*>{}(alloc.get());
        }
    };

    template<>
    struct hash<::voinst::scoped_allocation> : hash<::voinst::allocation>
    {
        using is_transparent = void;
    };
}

namespace voinst
{
    namespace details
    {
        template<
            typename CharAlloc,
            typename Alloc = star::allocator_traits<CharAlloc>:: //
            template rebind_alloc<star::all_aligned> // clang-format off
        > // clang-format on
        class resource_adaptor_impl : public pmr::memory_resource, star::value_wrapper<Alloc>
        {
            using allocator_type = Alloc;

            using base = star::value_wrapper<allocator_type>;

            using base::v;

            using traits = star::allocator_traits<allocator_type>;

        public:
            resource_adaptor_impl() = default;

            constexpr resource_adaptor_impl(const CharAlloc& alloc) noexcept: base(alloc) {}

            [[nodiscard]] constexpr CharAlloc get_allocator() const noexcept { return v; }

        private:
            static constexpr auto to_aligned_size(const std::size_t bytes) noexcept
            {
                constexpr auto max_align_v = star::max_alignment_v;
                constexpr auto expr0 = max_align_v - 1;
                return (bytes + expr0) & ~expr0;
            }

            [[nodiscard]] constexpr void*
                do_allocate(const std::size_t bytes, const std::size_t) override
            {
                return star::auto_cast(traits::allocate(v, to_aligned_size(bytes)));
            }

            constexpr void
                do_deallocate(void* const p, const std::size_t bytes, const std::size_t) noexcept
                override
            {
                traits::deallocate(v, static_cast<star::all_aligned*>(p), to_aligned_size(bytes));
            }

            [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
                const noexcept override
            {
                if(static_cast<const pmr::memory_resource*>(this) == &other) return true;

                const auto ptr = dynamic_cast<const resource_adaptor_impl*>(&other);

                if(ptr == nullptr) return false;

                if constexpr(traits::is_always_equal::value) return true;
                else return v == ptr->v;
            }
        };
    }

    template<star::allocator_req Alloc>
    using resource_adaptor = details:: //
        resource_adaptor_impl<typename star::allocator_traits<Alloc>::template rebind_alloc<char>>;

    using memory_resource = resource_adaptor<allocator<char>>;

    inline auto& get_default_resource()
    {
        static constinit memory_resource mem_rsc{};
        return mem_rsc;
    }
}