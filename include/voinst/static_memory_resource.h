#pragma once

#include "aligned.h"

namespace voinst::details
{
    template<std::size_t Size>
    struct static_memory_resource_traits
    {
        class default_policy // NOLINTBEGIN(*-reinterpret-*, *-pointer-arithmetic)
        {
            struct allocated_rng
            {
                star::byte* begin = nullptr;
                star::byte* end = nullptr;
            };

            static constexpr auto info_type_size = sizeof(allocated_rng);

            std::span<allocated_rng> allocations_{};

            constexpr auto first_allocate(
                const std::size_t bytes,
                const std::size_t alignment,
                star::byte* const data
            ) noexcept
            {
                constexpr auto initialize_size = (Size >= info_type_size * 2) ? 2 : 1;
                constexpr auto start_offset = info_type_size * initialize_size;
                const auto span =
                    align(alignment, bytes, std::span{data + start_offset, Size - start_offset});

                if(span.empty()) return Size;

                allocations_ = {
                    ::new(data) allocated_rng[initialize_size]{
                        {data, data + start_offset},
                        {span.data(), span.data() + bytes}
                    },
                    initialize_size
                };

                return span.data() - data;
            }

            static constexpr try_set_allocated(
                allocated_rng& pre_info,
                allocated_rng& next_info,
                const allocated_rng& new_info,
                star::byte* const data
            ) noexcept
            {
                if(pre_info == nullptr) return Size;

                auto& next_info = &pre_info + 1;
                if(pre_info) }

        public:
            default_policy() = default;
            default_policy(const default_policy&) = default;
            default_policy(default_policy&&) = default;
            default_policy& operator=(const default_policy&) = default;
            default_policy& operator=(default_policy&&) = default;

            constexpr ~default_policy() noexcept { std::ranges::destroy(allocations_); }

            constexpr auto operator()(
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
                requires(Size >= sizeof(allocated_rng))
            {
                const auto begin = std::assume_aligned<star::max_alignment_v>(storage.data());
                const auto end = begin + Size;

                if(allocations_ == nullptr) return first_allocate(bytes, alignment, begin);

                allocated_rng* pre_info = nullptr;
                allocated_rng next_info = nullptr;
                star::byte* candidate_ptr = nullptr;

                for( //
                    std::size_t overflowed = -1;
                    auto&& window : std::views::single(allocated_rng{begin, begin}) |
                        ::ranges::views::concat(
                                        allocations_,
                                        std::views::single(allocated_rng{end, end})
                        ) |
                        ::ranges::views::sliding(2) //
                )
                {
                    auto& pre = ranges::front(window);
                    auto& next = ranges::back(window);

                    std::span available{pre.end, next.begin};
                    const auto aligned_span = align(alignment, bytes, available);

                    if(aligned_span.empty()) continue;

                    const auto cur_overflowed = available.size() - bytes;

                    if(overflowed <= cur_overflowed) continue;

                    pre_info = &pre;
                    next_info = &next;
                    candidate_ptr = aligned_span.data();
                    overflowed = cur_overflowed;

                    if(overflowed == 0) break;
                }

                return pre_info == nullptr ?
                    Size :
                    try_set_allocated(
                        *pre_info,
                        allocated_rng{candidate_ptr, candidate_ptr + bytes},
                        begin
                    );
            }

            constexpr void operator()(
                const void* const ptr,
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
            {
                const auto data = std::assume_aligned<star::max_alignment_v>(storage.data());
                auto info_ptr = std::launder(reinterpret_cast<allocated_rng*>(data));

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