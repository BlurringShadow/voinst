#pragma once

#include <stdsharp/containers/actions.h>

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

            pmr::vector<mem_rng> frees_;

            constexpr auto allocate_impl(const auto it, const std::span<star::byte>& candidate)
            {
                auto& free = *it;
                mem_rng left_mem_rng{free.begin, candidate.begin()};
                mem_rng right_mem_rng{candidate.end(), free.end};

                if(left_mem_rng.empty())
                {
                    free = right_mem_rng;
                    return;
                }

                if(right_mem_rng.empty())
                {
                    free = left_mem_rng;
                    return;
                }

                insert_free(it, left_mem_rng, right_mem_rng);
            }

            constexpr auto insert_free(
                const auto it,
                const mem_rng& left_mem_rng,
                const mem_rng& right_mem_rng
            )
            {
                constexpr auto squeeze = [&](const auto f_begin, const auto f_end)
                {
                    const auto found = std::ranges::find_if(
                        f_begin,
                        f_end,
                        [](const auto& rng) { return rng.empty(); }
                    );

                    if(found == f_end) return false;

                    std::ranges::move_backward(f_begin, found, found);
                    *f_begin = right_mem_rng;
                    *(f_begin - 1) = *left_mem_rng;

                    return true;
                };

                if(squeeze(std::reverse_iterator{it}, frees_.crend()) ||
                   squeeze(it + 1, frees_.cend()))
                    return;

                *it = left_mem_rng;
                frees_.insert(it + 1, right_mem_rng);
            }

            static constexpr auto try_append_back(const mem_rng& src, mem_rng& dst)
            {
                if(src.begin != dst.end) return false;
                dst.end = src.end;
                return true;
            }

            static constexpr auto try_append_front(const mem_rng& src, mem_rng& dst)
            {
                if(src.end != dst.begin) return false;
                dst.begin = src.begin;
                return true;
            }

            static constexpr auto try_append(const auto prev, const auto next, const mem_rng& src)
            {
                if(!try_append_back(src, *prev)) return try_append_front(src, *next);
                if(auto& b = next->begin; src.end == b) b = next->end;
                return true;
            }

        public:
            constexpr explicit default_policy(pmr::memory_resource* const upstream):
                frees_(1, empty_rng, upstream)
            {
                frees_.reserve(2);
            }

            constexpr auto upstream() const noexcept { return frees_.get_allocator().resource(); }

            constexpr std::size_t operator()(
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            )
            {
                const auto data = std::assume_aligned<star::max_alignment_v>(storage.data());

                if(auto& front = frees_.front(); front == empty_rng) front = {data, data + Size};

                std::span<star::byte> allocation_candidate{};
                const auto end = frees_.end();
                auto candidate_iter = end;

                for(auto it = frees_.begin(); it != end; ++it)
                {
                    auto& free = *it;
                    if(free.empty()) continue;

                    const auto& aligned = align(alignment, bytes, std::span{free.begin, free.end});

                    if(aligned.empty() ||
                       (candidate_iter != frees_.cend()) && (free.size() >= candidate_iter->size()))
                        continue;

                    allocation_candidate = aligned;
                    candidate_iter = it;
                }

                if(allocation_candidate.empty()) return Size;

                allocate_impl(candidate_iter, allocation_candidate);
                return allocation_candidate - data;
            }

            constexpr void operator()(
                void* const ptr,
                const std::size_t bytes,
                const std::size_t alignment,
                std::array<star::byte, Size>& storage
            ) noexcept
            {
                Expects(is_align(alignment, bytes, ptr));

                const auto byte_ptr = star::pointer_cast<star::byte>(ptr);
                const mem_rng deallocated{byte_ptr, byte_ptr + bytes};
                const auto found = std::ranges::lower_bound(
                    frees_,
                    deallocated.end,
                    {},
                    [](const auto& rng) { return rng.begin; }
                );

                if(found == frees_.cbegin())
                {
                    if(auto& front = frees_.front(); front.begin == deallocated.end)
                    {
                        Expects(deallocated.begin >= storage.data());
                        front.begin = deallocated.begin;
                        return;
                    }
                }
                else if(try_append(found - 1, found, deallocated)) return;

                frees_.emplace(found, deallocated);
            }
        }; // NOLINTEND(*-pointer-arithmetic)
    };
}

namespace voinst
{
    template<
        std::size_t Size,
        std::constructible_from<pmr::memory_resource*> Policy =
            details::static_memory_resource_traits<Size>::default_policy>
        requires std::invocable<
                     Policy&,
                     const std::size_t,
                     const std::size_t,
                     std::array<star::byte, Size>&> &&
        std::invocable<
                     Policy&,
                     void* const,
                     const std::size_t,
                     const std::size_t,
                     std::array<star::byte, Size>&>
    class static_memory_resource : pmr::memory_resource
    {
        alignas(std::max_align_t) std::array<star::byte, Size> storage_{};
        Policy policy_;

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
            std::invoke(policy_, ptr, bytes, alignment, storage_);
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return star::to_void_pointer(this) == &other;
        }

    public:
        constexpr explicit static_memory_resource(
            const gsl::not_null<pmr::memory_resource*> upstream = pmr::get_default_resource()
        ):
            policy_(upstream.get())
        {
        }

        static_memory_resource(const static_memory_resource&) = delete;
        static_memory_resource(static_memory_resource&&) = delete;
        static_memory_resource& operator=(const static_memory_resource&) = delete;
        static_memory_resource& operator=(static_memory_resource&&) = delete;
        ~static_memory_resource() override = default;

        [[nodiscard]] constexpr auto upstream_resource() const noexcept
        {
            return policy_.upstream();
        }
    };
}