#pragma once

#include "aligned.h"

namespace voinst::details
{
    struct static_memory_resource_traits
    {
        static constexpr struct
        {
            constexpr auto operator()(
                const auto& used,
                const std::size_t bytes,
                const std::size_t alignment,
                std::byte* const data
            ) const noexcept
            {
                return std::is_constant_evaluated() ? //
                    std::ranges::search_n(used, bytes, false) :
                    aligned_search(used, bytes, alignment, data);
            }

            static auto aligned_search(
                const auto& used,
                const std::size_t bytes,
                const std::size_t alignment,
                std::byte* const data
            ) noexcept // NOLINTBEGIN(*-pointer-arithmetic)
            {
                for(std::size_t i = 0; i < used.size();)
                {
                    const auto used_iter = used.begin() + i;
                    const auto available_end = std::ranges::find(used_iter, used.end(), true);
                    const auto aligned_ptr =
                        aligned(alignment, bytes, {data + i, available_end - used_iter});

                    if(aligned_ptr == nullptr)
                        i = std::ranges::find(available_end, used.end(), false) - used.begin();
                    else return aligned_ptr - data;
                }

                return used.size();
            } // NOLINTEND(*-pointer-arithmetic)
        } default_policy{};
    };
}

namespace voinst
{
    template<std::size_t Size>
    class static_memory_resource : pmr::memory_resource
    {
        std::array<star::byte, Size> storage_{};
        std::array<bool, Size> used_{};

    protected:
        constexpr void* do_allocate(std::size_t bytes, const std::size_t alignment) override
        {
            return allocate(
                bytes,
                alignment,
                details::static_memory_resource_traits::default_policy
            );
        }

        constexpr void
            do_deallocate(void* const ptr, const std::size_t bytes, const std::size_t /*unused*/)
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

            std::ranges::fill_n(data_ptr, star::auto_cast(bytes), false);
        }

        [[nodiscard]] constexpr bool do_is_equal(const pmr::memory_resource& other) //
            const noexcept override
        {
            return star::to_void_pointer(this) == &other;
        }

    public:
        constexpr void* allocate(
            std::size_t bytes,
            const std::size_t alignment,
            star::invocable_r<
                const decltype(used_)&,
                std::size_t,
                std::size_t,
                std::size_t,
                star::byte*> auto&& policy
        )
        {
            auto index = std::invoke(
                cpp_forward(policy),
                std::as_const(used_),
                bytes,
                alignment,
                storage_.data()
            );

            if(index == storage_.size()) throw std::bad_alloc{};

            std::ranges::fill_n(used_.begin() + index, star::auto_cast(bytes), true);

            return &storage_[index];
        }
    };
}