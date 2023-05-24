#pragma once

#include <unordered_map>
#include <memory_resource>

#include <boost/container/pmr/resource_adaptor.hpp>
#include <mimalloc.h>
#include <mimalloc-new-delete.h>

namespace observable_memory
{
    namespace pmr = ::std::pmr;
    namespace bpmr = ::boost::container::pmr;

    using boost_mem_src = bpmr::memory_resource;
}

namespace observable_memory::mimalloc
{
    class memory_resource : pmr::memory_resource
    {
    public:
        memory_resource(const memory_resource&) = delete;
        memory_resource(memory_resource&&) = default;
        memory_resource& operator=(const memory_resource&) = delete;
        memory_resource& operator=(memory_resource&&) = default;

        void release() noexcept
        {
            release_impl();
            allocations_.clear();
        }

        ~memory_resource() noexcept override { release_impl(); }

    protected:
        void* do_allocate(std::size_t bytes, std::size_t alignment) override
        {
            auto ptr = impl_.allocate(bytes, alignment);

            allocations_.emplace(ptr, alloc_info{bytes, alignment});

            return ptr;
        }

        void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override
        {
            impl().deallocate(p, bytes, alignment);
            allocations_.erase(p);
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return static_cast<const void*>(this) == &other;
        }

    private:
        void release_impl() noexcept
        {
            for(const auto [ptr, info] : allocations_) impl_.deallocate(ptr, info.size);
        }

        mi_stl_allocator<char> impl_;

        struct alloc_info
        {
            size_t size;
            size_t alignment;
        };

        ::std::unordered_map<void*, alloc_info> allocations_;
    };

}