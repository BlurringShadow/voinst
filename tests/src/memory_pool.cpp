#include "observable_memory/memory/memory_pool.h"

#include "test.h"

using namespace std;
using namespace observable_memory;
using namespace stdsharp::literals;

namespace filesystem = ::observable_memory::filesystem;

SCENARIO("memory pool constructible", "[memory_pool]") // NOLINT
{
    STATIC_REQUIRE(constructible_from<mi_memory_pool, size_t, size_t>);
    STATIC_REQUIRE(constructible_from<mi_memory_pool, ::filesystem::kibibytes, size_t>);
    STATIC_REQUIRE( //
        constructible_from<
            mi_memory_pool,
            ::filesystem::kibibytes,
            ::filesystem::kibibytes // clang-format off
        > // clang-format on
    );
}

SCENARIO("mimalloc memory pool for container", "[memory_pool]") // NOLINT
{
    GIVEN("a memory pool")
    {
        mi_memory_pool pool(4_KiB, 16_KiB);

        THEN("created a vector from pool")
        {
            auto&& vec = make_container_from_pool<memory::vector, int>(pool);

            STATIC_REQUIRE(same_as<decltype(vec.pool)::pool_type, memory::array_pool>);

            // THEN("it is empty") REQUIRE(vec.empty());

            // THEN("emplace values")
            // {
            //     vec.emplace_back(1);
            //     vec.emplace_back(2);
            //     vec.emplace_back(3);
            // }
        }
    }
}