#pragma once

#include <stdsharp/utility/auto_cast.h>
#include <unordered_set>

#include <mimalloc-new-delete.h>

#include "../../namespace_alias.h"

namespace observable_memory::mimalloc
{
    class allocation
    {
        std::size_t size_;
        std::size_t alignment_;
        void* ptr_;

    public:
        allocation(const std::size_t bytes, const std::align_val_t alignment, void* const ptr):
            size_(bytes), alignment_(stdsharp::auto_cast(alignment)), ptr_(ptr)
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
        scoped_allocation(
            const std::size_t bytes,
            const std::align_val_t alignment,
            const bool allocate = true
        ):
            allocation(
                bytes,
                alignment,
                allocate ? mi_new_aligned(bytes, stdsharp::auto_cast(alignment)) : nullptr
            )
        {
        }

        void allocate()
        {
            if(get() == nullptr) { get() = mi_new_aligned(size(), alignment()); }
        }

        ~scoped_allocation()
        {
            if(get() != nullptr) mi_free_size_aligned(get(), size(), alignment());
        }

        scoped_allocation(const scoped_allocation&) = delete;
        scoped_allocation(scoped_allocation&&) = default;
        scoped_allocation& operator=(const scoped_allocation&) = delete;
        scoped_allocation& operator=(scoped_allocation&&) = default;
    };

    class memory_resource : pmr::memory_resource
    {
    public:
        memory_resource(const memory_resource&) = delete;
        memory_resource(memory_resource&&) = default;
        memory_resource& operator=(const memory_resource&) = delete;
        memory_resource& operator=(memory_resource&&) = default;

        void release() noexcept { allocations_.clear(); }

    protected:
        void* do_allocate(const std::size_t bytes, const std::size_t alignment) override
        {
            return allocations_.emplace(bytes, alignment).first->get();
        }

        void do_deallocate(
            void* const p,
            const std::size_t bytes,
            const std::size_t alignment
        ) noexcept override
        {
            allocations_.erase(allocation{p, bytes, alignment});
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return static_cast<const void*>(this) == &other;
        }

    private:
        std::unordered_set<scoped_allocation> allocations_;
    };
}

namespace std
{
    template<>
    struct hash<::observable_memory::mimalloc::allocation>
    {
        [[nodiscard]] constexpr size_t
            operator()(const ::observable_memory::mimalloc::allocation& alloc) const noexcept
        {
            return hash<void*>{}(alloc.get());
        }
    };
}