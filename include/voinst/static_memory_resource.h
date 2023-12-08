#pragma once

#include "aligned.h"

namespace voinst::details
{
    template<std::size_t Size>
    struct static_memory_resource_traits
    {
        class default_policy // NOLINTBEGIN(*-pointer-arithmetic)
        {
            static constexpr struct mem_rng
            {
                star::byte* begin = nullptr;
                star::byte* end = nullptr;

                constexpr auto size() const noexcept { return end - begin; }

                constexpr auto empty() const noexcept { return end == begin; }
            } empty_rng{};

            std::vector<mem_rng> frees_{};

            static constexpr auto sizeof_mem_rng = sizeof(mem_rng);

            constexpr auto try_allocate_free(const std::size_t i, const std::span<star::byte>& candidate)
            {
                auto& free = frees_[i];
                mem_rng left_mem_rng{free.begin, candidate.begin()};
                mem_rng right_mem_rng{candidate.end(), free.end};

                if(left_mem_rng.empty())
                {
                    free = right_mem_rng;
                    return true;
                }

                if(right_mem_rng.empty())
                {
                    free = left_mem_rng;
                    return true;
                }

                return try_insert_free(i, left_mem_rng, right_mem_rng);
            }

            constexpr auto try_insert_free(
                std::size_t i,
                const mem_rng& left_mem_rng,
                const mem_rng& right_mem_rng
            )
            {
                const auto it = frees_.begin() + i;
                auto cur = it;
                auto adjacent_it = frees_.end();

                for(;; --cur)
                {
                    if(cur->empty())
                    {
                        for(adjacent_it = cur + 1; adjacent_it < it; ++adjacent_it, ++cur)
                            *cur = *adjacent_it;
                        *cur = left_mem_rng;
                        *adjacent_it = right_mem_rng;
                        return true;
                    }

                    if(cur == frees_.begin()) break;
                }

                for(cur = it; cur != frees_.end(); ++cur)
                    if(cur->empty())
                    {
                        for(adjacent_it = cur - 1; adjacent_it > it; --adjacent_it, --cur)
                            *cur = *adjacent_it;
                        *adjacent_it = left_mem_rng;
                        *cur = right_mem_rng;
                        return true;
                    }

                Expects(false);
            }

            static constexpr auto best_fit(
                const std::size_t bytes,
                const std::size_t alignment,
                const auto& rng,
                mem_rng candidate_free = empty_rng
            )
            {
                std::span<star::byte> candidate{};
                std::size_t candidate_index = 0;

                for(const auto& [index, free] : rng | ranges::views::enumerate)
                {
                    if(free.empty()) continue;

                    const auto& aligned = align(alignment, bytes, std::span{free.begin, free.end});

                    if(aligned.empty() ||
                       !candidate_free.empty() && (free.size() >= candidate_free.size()))
                        continue;

                    candidate = aligned;
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
                            data + Size,
                        },
                        1
                    };
                }

                const auto [candidate, i] = best_fit(bytes, alignment, frees_);

                return !candidate.empty() && try_allocate_free(i, {candidate, candidate + bytes}) ?
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