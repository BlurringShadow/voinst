#include <algorithm>
#include <span>

#include "observable_memory/memory/mimalloc/allocator.h"
#include "test.h"

using namespace std;
using namespace observable_memory;

SCENARIO("mimalloc deleter", "[mimalloc]") // NOLINT
{
    GIVEN("a unique pointer using mimalloc deleter, ")
    {
        unique_ptr<int, mimalloc::deleter> ptr{};

        THEN("allocate a int using mimalloc")
        {
            mi_stl_allocator<int> allocator;
            ptr.reset(allocator.allocate(1));

            AND_THEN("assign 42 to the int")
            {
                *ptr = 42;
                REQUIRE(*ptr == 42);
            }
        }
    }
}

SCENARIO("mimalloc allocator", "[mimalloc]") // NOLINT
{
    GIVEN("a vector of ints")
    {
        using data_t = vector<int>;

        const auto& data_vec = GENERATE(data_t{42}, data_t{13, 2}, data_t{5, 6});

        INFO(fmt::format("vector size: {}, values: {}", data_vec.size(), data_vec));

        GIVEN("a default int allocator")
        {
            mimalloc::allocator<int> allocator;

            AND_GIVEN("a vector of ints")
            {
                INFO(fmt::format("vector size: {}, values: {}", data_vec.size(), data_vec));

                THEN("allocate same size of ints from resource, and construct a span of ints")
                {
                    const span<int> span{allocator.allocate(data_vec.size()), data_vec.size()};

                    AND_THEN("assign the values to the span")
                    {
                        std::ranges::copy(data_vec, span.begin());

                        REQUIRE(std::ranges::equal(span, data_vec));

                        AND_THEN("deallocate the pointer")
                        {
                            allocator.deallocate(span.data(), span.size());
                        }
                    }
                }
            }
        }

        GIVEN("my_vec, a vector using the default int allocator")
        {
            vector<int, mimalloc::allocator<int>> my_vec;

            THEN("assign vec to my_vec")
            {
                my_vec.resize(data_vec.size());
                std::ranges::copy(data_vec, my_vec.begin());

                REQUIRE(std::ranges::equal(my_vec, data_vec));
            }
        }

        GIVEN("a proxy allocator using the default int std allocator")
        {
            allocator<int> allocator;

            const mimalloc::proxy_allocator proxy_allocator{allocator};

            THEN("allocate same size of ints from resource, and construct a span of ints")
            {
                const span<int> span{proxy_allocator.allocate(data_vec.size()), data_vec.size()};

                AND_THEN("assign the values to the span")
                {
                    std::ranges::copy(data_vec, span.begin());

                    REQUIRE(std::ranges::equal(span, data_vec));

                    AND_THEN("deallocate the pointer")
                    {
                        proxy_allocator.deallocate(span.data(), span.size());
                    }
                }
            }
        }
    }
}