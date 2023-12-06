#pragma once

#include "aligned.h"

namespace voinst::details
{
    template<std::size_t Size>
    struct static_memory_resource_traits
    {
        class default_policy // NOLINTBEGIN(*-reinterpret-*, *-pointer-arithmetic)
        {
            struct free_rng
            {
                star::byte* begin = nullptr;
                star::byte* end = nullptr;

                constexpr auto size() const noexcept { return end - begin; }
            };

            std::span<free_rng> frees_{};

            static constexpr auto sizeof_free_rng = sizeof(free_rng);

            constexpr void init(const star::byte* const data) noexcept
            {
                frees_ = {
                    ::new(data) free_rng{
                        data,
                        Size,
                    },
                    1
                };
            }

            constexpr auto
                try_insert_allocated(const std::size_t i, const free_rng& new_info) noexcept
            {
            }

        public:
            default_policy() = default;
            default_policy(const default_policy&) = default;
            default_policy(default_policy&&) = default;
            default_policy& operator=(const default_policy&) = default;
            default_policy& operator=(default_policy&&) = default;

            constexpr ~default_policy() { std::ranges::destroy(frees_); }

            constexpr auto operator()(
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
                requires(Size >= sizeof(free_rng))
            {
                const auto begin = std::assume_aligned<star::max_alignment_v>(storage.data());
                const auto end = begin + Size;
                star::byte* candidate{};
                std::size_t target_index = Size;

                if(frees_.empty()) init(begin);

                for(const auto& [index, free] : frees_ | ranges::views::enumerate)
                {
                    const auto& aligned = align(alignment, bytes, std::span{free.begin, free.end});

                    if(aligned.empty() || (target_index != Size) && (free.size() >= frees_[target_index].size())) continue;

                    candidate = aligned.data();
                    target_index = index;
                }

                return pre_info == nullptr ? Size :
                                             try_set_allocated(
                                                 *pre_info,
                                                 *next_info,
                                                 free_rng{candidate_ptr, candidate_ptr + bytes},
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
                auto info_ptr = std::launder(reinterpret_cast<free_rng*>(data));

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