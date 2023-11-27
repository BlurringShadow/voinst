#include "namespace_alias.h"

namespace voinst
{
    inline constexpr struct
    {
        template<typename T>
        T* operator()(const std::size_t alignment, const std::size_t size , const std::span<T> span) const noexcept
        {
            auto max = std::numeric_limits<std::size_t>::max();
            void* void_ptr = span.data();
            std::align(alignment, size, void_ptr, max);
            return star::pointer_cast<T>(void_ptr);
        }
    } aligned{};
}