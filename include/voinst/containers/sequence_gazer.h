#pragma once

#include <stdsharp/containers/concepts.h>

#include "../alias.h"

namespace voinst
{

    template<typename Iter>
    struct remove_event
    {
        Iter begin;
        Iter end;
    };

    template<typename Iter>
    struct update_event
    {
        Iter begin;
        Iter end;
    };
}