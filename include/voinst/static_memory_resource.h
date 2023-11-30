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
                std::size_t ptr_index{};
                std::size_t size{};
                std::size_t next_info_index{};
            };

            auto find_first_available(
                std::size_t info_index,
                const std::size_t bytes,
                std::byte* const storage
            )
            {
                for(; info_index < Size;)
                {
                    const auto& info = *reinterpret_cast<allocation_info*>(storage + info_index);
                    const auto next_info_index = info.next_info_index;
                    const auto next_ptr_index = next_info_index < Size ?
                        std::launder(reinterpret_cast<allocation_info*>(storage + next_info_index))->ptr_index :
                        Size;

                    if(next_ptr_index - info.ptr_index >= bytes + info.size) break;

                    info_index = next_info_index;
                }

                return info_index;
            }

            auto aligned_search(
                const std::size_t bytes,
                const std::size_t alignment,
                std::byte* const storage
            ) noexcept
            {
                const auto data = std::assume_aligned<star::max_alignment_v>(storage);

                for(std::size_t i = 0;;)
                {
                    const auto info_index = find_first_available(i, bytes, storage);

                    if(info_index == Size) break;

                    const auto& info = *std::launder(reinterpret_cast<allocation_info*>(storage + info_index));

                    const auto aligned_span = aligned(
                        alignment,
                        bytes,
                        {storage + info.ptr_index + info.size, candidate_end - candidate_begin}
                    );

                    if(aligned_span.empty())
                    {
                        if(info_index == Size) break;
                        candidate_begin = std::ranges::find(candidate_end + 1, Size, false);
                    }
                    else return aligned_span.data() - storage;
                }

                return used_.size();
            }

        public:
            constexpr auto operator()(
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& data
            ) noexcept
            {
                const auto index = std::is_constant_evaluated() ? //
                    std::ranges::search_n(used_, bytes, false) :
                    aligned_search(bytes, alignment, data.data());

                std::ranges::fill_n(used_.begin() + index, star::auto_cast(bytes), true);

                return index;
            }

            constexpr void
                operator()(void* const ptr, const std::size_t bytes, const std::size_t /*unused*/, std::array<star::byte, Size>&) noexcept
            {
                std::ranges::fill_n(used_.begin() + idx, star::auto_cast(bytes), false);
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