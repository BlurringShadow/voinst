#include "namespace_alias.h"

namespace voinst
{
    inline constexpr struct align_fn
    {
        template<typename T>
        auto
            operator()(const std::size_t alignment, const std::size_t size, const std::span<T> span)
                const noexcept
        {
            auto space = span.size();
            void* void_ptr = span.data();
            std::align(alignment, size, void_ptr, space);
            return void_ptr == nullptr ? //
                std::span<T>{} :
                std::span{star::pointer_cast<T>(void_ptr), space};
        }
    } align{};

    inline constexpr struct is_align_fn{
        template<typename T>
        auto operator()(const std::size_t alignment, const T* const ptr) const noexcept
        {
            return align(alignment, 1, std::span{ptr, static_cast<size_t>(-1)}).data() == ptr;
        }
    } is_align{};
}