#pragma once

#include "aligned.h"

namespace voinst::details
{
    template<std::size_t Size>
    struct static_memory_resource_traits
    {
        class default_policy // NOLINTBEGIN(*-reinterpret-*, *-pointer-arithmetic)
        {
            struct allocation_info
            {
                std::size_t padding{};
                std::size_t size{};

                auto allocation_start() noexcept
                {
                    return reinterpret_cast<std::byte*>(this) + sizeof(allocation_info) + padding;
                }

                auto allocation_end() noexcept { return allocation_start() + size; }

                std::size_t available{};

                auto available_span() noexcept { return std::span{allocation_end(), available}; }

                auto next_info(const star::byte* const end) noexcept
                {
                    const auto next = allocation_end() + available;
                    return next < end ? //
                        std::launder(reinterpret_cast<allocation_info*>(next)) :
                        nullptr;
                }
            };

        public:
            constexpr auto operator()(
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
            {
                const auto data = std::assume_aligned<star::max_alignment_v>(storage.data());

                for(auto info_ptr = std::launder(reinterpret_cast<allocation_info*>(data));
                    info_ptr != nullptr;
                    info_ptr = info_ptr->next_info())
                {
                    const auto info_span = align(
                        sizeof(allocation_info),
                        alignof(allocation_info),
                        info_ptr->available_span()
                    );
                    if(info_span.empty()) continue;

                    const auto allocated_span = align(alignment, bytes, info_span);

                    if(!allocated_span.empty()) continue;

                    ::new(info_span.data()) allocation_info{
                        .padding = allocated_span.data() - info_span.data(),
                        .size = bytes,
                        .available = allocated_span.size() - bytes,
                    };

                    info_ptr->available -= info_span.size();

                    return allocated_span.data() - data;
                }

                return Size;
            }

            constexpr void operator()(
                const void* const ptr,
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
            {
                const auto data = std::assume_aligned<star::max_alignment_v>(storage.data());
                auto info_ptr = std::launder(reinterpret_cast<allocation_info*>(data));

                for(; info_ptr != nullptr; info_ptr = info_ptr->next_info())
                    if(info_ptr->allocation_start() == ptr) break;

                Expects(info_ptr != nullptr);
                Expects(info_ptr->size == bytes);
                std::assume
            }
        }; // NOLINTEND(*-reinterpret-*, *-pointer-arithmetic)
    };
}

namespace voinst
{
    template<
        std::size_t Size,
        typename Policy = details::static_memory_resource_traits<Size>::default_policy>
    class static_memory_resource : pmr::memory_resource
    {
        alignas(std::max_align_t) std::array<star::byte, Size> storage_{};
        Policy policy_{};

    protected:
        constexpr void* do_allocate(const std::size_t bytes, const std::size_t alignment) override
        {
            auto index = std::invoke(policy_, bytes, alignment, storage_);

            if(index == storage_.size()) throw std::bad_alloc{};

            return &storage_[index];
        }

        constexpr void
            do_deallocate(void* const ptr, const std::size_t bytes, const std::size_t alignment)
                override
        {
            star::byte* data_ptr = nullptr;

            if(std::is_constant_evaluated())
                for(std::size_t i = 0; i < Size; ++i)
                {
                    auto cur_ptr = &storage_[i];
                    if(cur_ptr == ptr)
                    {
                        data_ptr = cur_ptr;
                        break;
                    }
                }
            else
            {
                Expects(star::is_between(ptr, storage_.data(), &storage_.back()));
                data_ptr = star::pointer_cast<star::byte>(ptr);
            }

            std::invoke(policy_, data_ptr, bytes, alignment, storage_);
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return star::to_void_pointer(this) == &other;
        }
    };
}