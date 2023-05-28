#pragma once

#include <unordered_set>

#include <mimalloc-new-delete.h>

#include "namespace_alias.h"

namespace voinst
{
    struct deleter
    {
        void operator()(void* p) const noexcept { mi_free(p); }

        constexpr void operator()(std::nullptr_t) = delete;
    };

    template<typename T>
    struct allocator
    {
        using value_type = T;

        template<typename U>
        struct rebind
        {
            using other = allocator<U>;
        };

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
        [[nodiscard]] constexpr size_t operator()(const ::voinst::allocation& alloc) const noexcept
        {
            return hash<void*>{}(alloc.get());
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
    class memory_resource : public pmr::memory_resource
    {
        std::unordered_set<
            scoped_allocation,
            std::hash<::voinst::scoped_allocation>,
            std::ranges::equal_to,
            allocator<scoped_allocation> // clang-format off
        > // clang-format on
            allocations_;

    public:
        void release() noexcept { allocations_.clear(); }

    protected:
        [[nodiscard]] void*
            do_allocate(const std::size_t bytes, const std::size_t alignment) override
        {
            return allocations_.emplace(bytes, star::auto_cast(alignment)).first->get();
        }

        void do_deallocate(
            void* const p,
            const std::size_t bytes,
            const std::size_t alignment
        ) noexcept override
        {
            allocations_.erase(allocations_.find(allocation{bytes, star::auto_cast(alignment), p}));
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return star::to_void_pointer(this) == &other;
        }
    };
}