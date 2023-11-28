#include "namespace_alias.h"

namespace voinst
{
    inline constexpr struct
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
    } aligned{};
}