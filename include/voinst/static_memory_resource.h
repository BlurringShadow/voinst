#pragma once

#include "aligned.h"

namespace voinst::details
{
    template<std::size_t Size>
    struct static_memory_resource_traits
    {
        class default_policy // NOLINTBEGIN(*-reinterpret-*, *-pointer-arithmetic)
        {
            static constexpr struct mem_rng
            {
                star::byte* begin = nullptr;
                star::byte* end = nullptr;

                constexpr auto size() const noexcept { return end - begin; }

                constexpr auto empty() const noexcept { return end == begin; }
            } empty_rng{};

            std::span<mem_rng> frees_{};

            static constexpr auto sizeof_mem_rng = sizeof(mem_rng);

            constexpr auto try_allocate(std::size_t i, const mem_rng& take_space)
            {
                auto& info = frees_[i];
                mem_rng left_info{info.begin, take_space.begin};
                mem_rng right_info{take_space.end, info.end};

                if(left_info.size() == 0)
                {
                    info = right_info;
                    return true;
                }

                if(right_info.size() == 0)
                {
                    info = left_info;
                    return true;
                }

                return try_insert_free(i, left_info, right_info);
            }

            constexpr auto
                try_insert_free(std::size_t i, const mem_rng& left_info, const mem_rng& right_info)
            {
                auto it = std::ranges::find_if(
                    frees_.subspan(i + 1),
                    [](const auto& f) { return f.empty(); }
                );

                if(it == frees_.end())
                    return try_reallocate_frees(i, left_info, right_info);

                for(auto next_i = i + 1;; i = next_i, ++next_i)
                {
                    if(next_i == frees_.size() || frees_[next_i].begin == nullptr)
                    {
                        frees_[i] = empty_rng;
                        break;
                    }

                    frees_[next_i] = frees_[i];
                }

                auto& info = frees_[i];
                std::span new_frees{frees_.data(), frees_.size() + 1};

                info = right_info;

                std::ranges::move_backward(frees_.subspan(i), new_frees.end());

                info = left_info;
                frees_ = new_frees;
            }

            constexpr auto try_reallocate_frees(
                const std::size_t i,
                const mem_rng& left_info,
                const mem_rng& right_info
            )
            {
                constexpr auto alignof_free_rng = alignof(mem_rng);
                const auto actual_size = (frees_.size() + 1) * sizeof_mem_rng;
                const std::ranges::single_view right_info_view = (right_info);
                auto& info = frees_[i];

                info = left_info;

                auto [candidate, target_index] =
                    best_fit(frees_.size() * 2 * sizeof_mem_rng, alignof_free_rng, right_info_view);

                if(candidate == nullptr)
                {
                    std::tie(candidate, target_index) = best_fit(
                        (frees_.size() + 1) * sizeof_mem_rng,
                        alignof_free_rng,
                        right_info_view
                    );

                    if(candidate == nullptr)
                    {
                        info = {left_info.begin, right_info.end};
                        return false;
                    }
                }

                {
                    auto& target_info = frees_[target_index];
                    frees_[target_index].begin += actual_size;

                    if(frees_) }

                std::span new_frees{reinterpret_cast<mem_rng*>(candidate), frees_.size() + 1};

                std::ranges::uninitialized_move(
                    ranges::views::concat(
                        frees_.subspan(0, i + 1),
                        right_info_view,
                        frees_.subspan(i + 1)
                    ),
                    new_frees.begin()
                );

                frees_ = new_frees;

                return true;
            }

            static constexpr auto best_fit(
                const std::size_t bytes,
                const std::size_t alignment,
                const auto& rng,
                mem_rng candidate_free = empty_rng
            )
            {
                star::byte* candidate = nullptr;
                std::size_t candidate_index = 0;

                for(const auto& [index, free] : rng | ranges::views::enumerate)
                {
                    const auto& aligned = align(alignment, bytes, std::span{free.begin, free.end});

                    if(aligned.empty() ||
                       !candidate_free.empty() && (free.size() >= candidate_free.size()))
                        continue;

                    candidate = aligned.data();
                    candidate_index = index;
                }

                return std::pair{candidate, candidate_index};
            }

        public:
            constexpr auto operator()(
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
                requires(Size >= sizeof(mem_rng))
            {
                const auto data = std::assume_aligned<star::max_alignment_v>(storage.data());

                if(frees_.empty())
                {
                    if(Size < sizeof_mem_rng) return Size;
                    frees_ = {
                        ::new(data) mem_rng{
                            data + sizeof_mem_rng,
                            Size - sizeof_mem_rng,
                        },
                        1
                    };
                }

                const auto [candidate, i] = best_fit(bytes, alignment, frees_);

                return candidate != nullptr && try_allocate(i, {candidate, candidate + bytes}) ?
                    Size :
                    candidate - data;
            }

            constexpr void operator()(
                const void* const ptr,
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
            {
                const auto data = std::assume_aligned<star::max_alignment_v>(storage.data());
                auto info_ptr = std::launder(reinterpret_cast<mem_rng*>(data));

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