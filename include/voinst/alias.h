#pragma once

#include <memory_resource>
#include <stdsharp/memory/memory.h>

namespace voinst
{
    namespace std = ::std;
    namespace pmr = std::pmr;
    namespace star = ::stdsharp;
    namespace gsl = ::gsl;
    namespace ranges = ::ranges;
    namespace ctre = ::ctre;

    using size_t = std::size_t;
    using nullptr_t = std::nullptr_t;
    using byte = star::byte;
}