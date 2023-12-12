#pragma once

#include "alias.h"

namespace voinst
{
    inline constexpr struct is_iter_in_fn
    {
        template<typename I, std::sentinel_for<I> S>
            requires std::sentinel_for<I, I>
        constexpr bool operator()(I begin, const S& end, const I& in) const noexcept
        {
            if(!std::is_constant_evaluated()) return begin <= in && in < end;

            for(; begin != end; ++begin)
                if(begin == in) return true;
            return false;
        }

        template<std::ranges::range R>
            requires std::sentinel_for<std::ranges::iterator_t<R>, std::ranges::iterator_t<R>>
        constexpr bool operator()(R&& r, const star::const_iterator_t<R>& in) const noexcept
        {
            return (*this)(std::ranges::cbegin(r), std::ranges::cend(r), in);
        }
    } is_iter_in{};
}