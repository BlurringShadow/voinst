#pragma once

#include <stdsharp/containers/concepts.h>

#include "../namespace_alias.h"

namespace voinst
{
    template<typename T>
    struct stargazer;

    template<star::sequence_container Container>
    class stargazer<Container>
    {
        Container container_;

        using iterator = typename Container::iterator;
        using const_iterator = typename Container::const_iterator;

    public:
        template<typename... Args>
            requires std::constructible_from<Container, Args...>
        constexpr stargazer(Args&&... args) //
            noexcept(star::nothrow_constructible_from<Container, Args...>):
            container_(cpp_forward(args)...)
        {
        }

        struct event
        {
            const_iterator begin;
            const_iterator end;
            bool is_new;
        };
    };
}