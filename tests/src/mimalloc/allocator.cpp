#include <algorithm>
#include <initializer_list>
#include <span>

#include "observable_memory/mimalloc/allocator.h"
#include "test.h"

using namespace std;
using namespace observable_memory;

SCENARIO("mimalloc allocator", "[mimalloc]") // NOLINT
{
    using data_t = initializer_list<int>;

    GIVEN("a default int allocator")
    {
        auto allocator = mimalloc::get_allocator<int>();

        AND_GIVEN("a vector of ints")
        {
            auto vec = GENERATE(data_t{42}, data_t{13, 2}, data_t{5, 6});

            INFO(fmt::format("vector size: {}, values: {}", vec.size(), vec));

            THEN("allocate same size of ints from resource, and construct a span of ints")
            {
                span<int> span{allocator.allocate(vec.size()), vec.size()};

                AND_THEN("assign the values to the span")
                {
                    std::ranges::copy(vec, span.begin());

                    REQUIRE(std::ranges::equal(span, vec));

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
        auto my_vec = mimalloc::get_container<vector, int>();

        AND_GIVEN("vec, a vector of ints")
        {
            auto vec = GENERATE(data_t{42}, data_t{13, 2}, data_t{5, 6});

            INFO(fmt::format("vector size: {}, values: {}", vec.size(), vec));

            THEN("assign vec to my_vec")
            {
                my_vec.resize(vec.size());
                std::ranges::copy(vec, my_vec.begin());

                REQUIRE(std::ranges::equal(my_vec, vec));
            }
        }
    }
}