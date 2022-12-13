#pragma once

#include <catch2/catch_message.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <ctll/fixed_string.hpp>
#include <ctre.hpp>

#include <stdsharp/type_traits/core_traits.h>

#include <fmt/core.h>

#include <mimalloc.h>

void mi_redirect_to_catch2();

template<typename T>
constexpr ::std::string_view type() noexcept
{
    return __func__; // NOLINT
}