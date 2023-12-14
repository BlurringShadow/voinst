#include "voinst/resource/rng_buffer_resource.h"
#include "test.h"

using namespace std;
using namespace voinst;

SCENARIO("range buffer resource", "[rng_buffer_resource]") // NOLINT
{
    GIVEN("array resource")
    {
        array_resource<4> resource;

        THEN("array resource should not be equal")
        {
            array_resource<4> another_resource;
            REQUIRE(resource != another_resource);
        }

        THEN("test best fit allocate")
        {
            auto p0 = resource.allocate(2, 1);
            auto p1 = resource.allocate(1, 1);
            auto p2 = resource.allocate(1, 1);

            resource.deallocate(p0, 2);
            resource.deallocate(p2, 1);

            auto p3 = resource.allocate(1, 1);
            auto p4 = resource.allocate(2, 1);

            resource.deallocate(p1, 1);
            resource.deallocate(p3, 1);
            resource.deallocate(p4, 2);
        }
    }
}