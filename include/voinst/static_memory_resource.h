#pragma once

#include "aligned.h"
#include <range/v3/view/chunk_by.hpp>

namespace voinst::details
{
    template<std::size_t Size>
    struct static_memory_resource_traits
    {
        class default_policy
        {
            std::array<bool, Size> used_{};

            auto aligned_search(
                const std::size_t bytes,
                const std::size_t alignment,
                std::byte* const data
            ) noexcept
            {
                const auto used_begin = used_.begin();
                const auto used_end = used_.end();
                for(auto candidate_begin = std::ranges::find(used_, false);
                    candidate_begin != used_end;)
                {
                    const auto candidate_end = std::ranges::find(candidate_begin, used_end, true);
                    const auto aligned_span = aligned(
                        alignment,
                        bytes,
                        {data + (candidate_begin - used_begin), candidate_end - candidate_begin}
                    );

                    if(aligned_span.empty())
                    {
                        if(candidate_end == used_end) break;
                        candidate_begin = std::ranges::find(candidate_end + 1, used_end, false);
                    }
                    else return aligned_span.data() - data;
                }

                return used_.size();
            }

        public:
            constexpr auto operator()(
                const std::size_t bytes,
                const std::size_t alignment,
                std::byte* const data
            ) noexcept
            {
                const auto index = std::is_constant_evaluated() ? //
                    std::ranges::search_n(used_, bytes, false) :
                    aligned_search(bytes, alignment, data);

                std::ranges::fill_n(used_.begin() + index, star::auto_cast(bytes), true);

                return index;
            }

            constexpr void operator()(
                const std::size_t idx,
                const std::size_t bytes,
                const std::size_t /*unused*/
            ) noexcept
            {
                std::ranges::fill_n(used_.begin() + idx, star::auto_cast(bytes), false);
            }
        };
    };
}

namespace voinst
{
    template<std::size_t Size, typename Policy>
    class static_memory_resource : pmr::memory_resource
    {
        std::array<star::byte, Size> storage_{};
        Policy policy_{};

    protected:
        constexpr void* do_allocate(std::size_t bytes, const std::size_t alignment) override
        {
            auto index = std::invoke(policy_, bytes, alignment, storage_.data());

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

            std::invoke(policy_, data_ptr - storage_.data(), bytes, alignment);
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return star::to_void_pointer(this) == &other;
        }
    };
}