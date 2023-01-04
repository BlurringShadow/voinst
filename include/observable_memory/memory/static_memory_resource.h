#pragma once

#include <boost/container/pmr/resource_adaptor.hpp>

#include <stdsharp/memory/static_allocator.h>

namespace observable_memory
{
    template<::std::size_t Size>
    using static_memory_resource = pmr::resource_adaptor<::stdsharp::static_memory_resource<char>>;
}