#pragma once

#include <stdsharp/containers/actions.h>

#include "aligned.h"
#include "voinst/iterator.h"

namespace voinst::details
{
    template<typename Rng>
    struct rng_buffer_resource_traits
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

            pmr::vector<mem_rng> frees_{1, empty_rng};

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
            default_policy() = default;

            constexpr explicit default_policy(pmr::memory_resource* const upstream):
                frees_(1, empty_rng, upstream)
            {
                frees_.reserve(2);
            }

            constexpr auto upstream() const noexcept { return frees_.get_allocator().resource(); }

            constexpr void*
                operator()(const size_t bytes, const size_t alignment, Rng& buffer)
            {
                const auto data = std::ranges::data(buffer);
                const auto size = std::ranges::size(buffer);

                if(auto& front = frees_.front(); front == empty_rng) front = {data, data + size};

                std::span<star::byte> allocation_candidate{};
                const auto end = frees_.end();
                auto candidate_iter = end;

                for(auto it = frees_.begin(); it != end; ++it)
                {
                    auto& free = *it;
                    if(free.empty()) continue;

                    const auto& span = align(alignment, bytes, std::span{free.begin, free.end});

                    if(span.empty() ||
                       (candidate_iter != frees_.cend()) && (free.size() >= candidate_iter->size()))
                        continue;

                    allocation_candidate = span;
                    candidate_iter = it;
                }

                if(allocation_candidate.empty()) return size;

                allocate_impl(candidate_iter, allocation_candidate);
                return allocation_candidate.data();
            }

            constexpr bool operator()(
                void* const ptr,
                const size_t bytes,
                const size_t alignment,
                Rng& buffer
            ) noexcept
            {
                Expects(is_align(alignment, bytes, ptr));
                const auto byte_ptr = star::pointer_cast<star::byte>(ptr);

                if(const auto data = std::ranges::data(buffer);
                   !is_iter_in(data, data + std::ranges::size(buffer), byte_ptr))
                    return false;

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
                        front.begin = deallocated.begin;
                        return true;
                    }
                }
                else if(try_append(found - 1, found, deallocated)) return true;

                frees_.emplace(found, deallocated);

                return true;
            }
        }; // NOLINTEND(*-pointer-arithmetic)
    };
}

namespace voinst
{
    template<
        std::ranges::contiguous_range Rng,
        typename Policy = details::rng_buffer_resource_traits<Rng>::default_policy>
        requires requires(const size_t s, Rng& storage) {
            requires std::ranges::sized_range<Rng>;
            requires star::
                invocable_r<Policy&, void*, decltype(s), decltype(s), decltype(storage)>;
            requires std::
                predicate<Policy&, void* const, decltype(s), decltype(s), decltype(storage)>;
        }
    class rng_buffer_resource : pmr::memory_resource
    {
        Rng buffer_{};
        Policy policy_{};

    protected:
        constexpr void* do_allocate(const size_t bytes, const size_t alignment) override
        {
            const auto ptr = std::invoke(policy_, bytes, alignment, buffer_);
            return ptr == nullptr ? upstream_resource()->allocate(bytes, alignment) : ptr;
        }

        constexpr void
            do_deallocate(void* const ptr, const size_t bytes, const size_t alignment)
                override
        {
            if(!std::invoke(policy_, ptr, bytes, alignment, buffer_))
                upstream_resource()->deallocate(ptr, bytes, alignment);
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return star::to_void_pointer(this) == &other;
        }

    public:
        using policy = Policy;

        rng_buffer_resource() = default;

        constexpr explicit rng_buffer_resource(const gsl::not_null<pmr::memory_resource*> upstream)
            requires std::constructible_from<Policy, pmr::memory_resource*>
            : policy_(upstream.get())
        {
        }

        rng_buffer_resource(const rng_buffer_resource&) = delete;
        rng_buffer_resource(rng_buffer_resource&&) = delete;
        rng_buffer_resource& operator=(const rng_buffer_resource&) = delete;
        rng_buffer_resource& operator=(rng_buffer_resource&&) = delete;
        ~rng_buffer_resource() override = default;

        [[nodiscard]] constexpr auto upstream_resource() const noexcept
        {
            return policy_.upstream();
        }

        [[nodiscard]] constexpr auto& buffer() const noexcept { return buffer_; }

        [[nodiscard]] constexpr auto& buffer() noexcept { return buffer_; }
    };
}