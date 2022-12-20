#include <algorithm>
#include <span>

#include "observable_memory/mimalloc/allocator.h"
#include "test.h"

using namespace std;
using namespace observable_memory;

SCENARIO("mimalloc allocator", "[mimalloc]") // NOLINT
{
    GIVEN("a default int allocator")
    {
        auto allocator = mimalloc::get_default_allocator<int>();

        AND_GIVEN("a vector of ints")
        {
            auto vec = GENERATE(vector<int>{42}, vector<int>{13, 2}, vector<int>{5, 6});

            INFO(fmt::format("vector size: {}, values: {}", vec.size(), vec));

            THEN("allocate same size of ints from resource, and construct a span of ints")
            {
                span<int> span{allocator.allocate(vec.size()), vec.size()};

                AND_THEN("assign the values to the span")
                {
                    ranges::copy(vec, span.begin());

                    REQUIRE(ranges::equal(span, vec));

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
        vector<int, mimalloc::default_allocator<int>> my_vec;

        AND_GIVEN("vec, a vector of ints")
        {
            auto vec = GENERATE(vector<int>{42}, vector<int>{13, 2}, vector<int>{5, 6});

            INFO(fmt::format("vector size: {}, values: {}", vec.size(), vec));

            THEN("assign vec to my_vec")
            {
                my_vec.resize(vec.size());
                ranges::copy(vec, my_vec.begin());

                REQUIRE(ranges::equal(my_vec, vec));
            }
        }
    }
}