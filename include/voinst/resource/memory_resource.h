#pragma once

#include <mimalloc.h>
#include <stdsharp/memory/allocator_traits.h>

#include "../alias.h"

namespace voinst
{
    using heap = mi_heap_t;

    template<std::derived_from<pmr::memory_resource> Resource>
    struct resource_ptr_deleter
    {
        using resource_type = Resource;

        constexpr resource_ptr_deleter(
            resource_type& resource,
            const size_t bytes,
            const size_t align = star::max_alignment_v
        ) noexcept:
            resource(resource), bytes(bytes), align(align)
        {
        }

        std::reference_wrapper<resource_type> resource;
        size_t bytes;
        size_t align;

        constexpr void operator()(void* const p) const noexcept
        {
            resource.get().deallocate(p, bytes, align);
        }

        void operator()(nullptr_t) = delete;
    };

    template<typename Resource>
    resource_ptr_deleter(Resource&, size_t, size_t) -> resource_ptr_deleter<Resource>;

    struct heap_deleter
    {
        void operator()(heap* const p) const noexcept { mi_heap_destroy(p); }
    };

    using heap_ptr = std::unique_ptr<heap, heap_deleter>;

    template<typename T>
    struct allocator
    {
        using value_type = T;

        using propagate_on_container_move_assignment = std::true_type;

        allocator() = default;

        template<typename U>
        constexpr allocator(const allocator<U> /*unused*/) noexcept
        {
        }

        [[nodiscard]] static constexpr auto allocate(const size_t n)
        {
            return mi_new_aligned(n * sizeof(T), alignof(T));
        }

        [[nodiscard]] static constexpr auto reallocate(T* const p, const size_t n)
        {
            return mi_realloc_aligned(p, n * sizeof(T), alignof(T));
        }

        static constexpr void deallocate(T* const p, const size_t n) noexcept
        {
            mi_free_size_aligned(p, n * sizeof(T), alignof(T));
        }

        constexpr bool operator==(const allocator /*unused*/) const noexcept { return true; }
    };

    inline constinit class memory_resource : public pmr::memory_resource
    {
        heap* heap_ = nullptr;

        using allocator = allocator<byte>;

    public:
        constexpr explicit memory_resource(heap* heap = nullptr) noexcept: heap_(heap) {}

        [[nodiscard]] auto heap() const noexcept
        {
            return heap_ == nullptr ? mi_heap_get_default() : heap_;
        }

        auto heap(voinst::heap* const new_heap) noexcept
        {
            const auto old_heap = heap_;
            heap_ = new_heap;
            return old_heap;
        }

    private:
        void* do_allocate(const size_t size, const size_t alignment) override
        {
            return mi_heap_malloc_aligned(heap(), size, alignment);
        }

        void do_deallocate(void* const p, const size_t size, const size_t alignment) override
        {
            mi_free_size_aligned(p, size, alignment);
        }

        [[nodiscard]] bool do_is_equal(const pmr::memory_resource& other) const noexcept override
        {
            const auto p = dynamic_cast<const memory_resource*>(&other);
            return p == nullptr ? false : p->heap_ == heap_;
        }

        [[nodiscard]] auto
            reallocate(void* const p, const size_t size, const size_t alignment) const
        {
            return mi_heap_realloc_aligned(heap(), p, size, alignment);
        }
    } default_resource{}; // NOLINT(*-non-const-global-variables)
}