#include "voinst/resource/memory_resource.h"
#include "test.h"

using namespace std;
using namespace voinst;

using memory_resource = voinst::memory_resource;

SCENARIO("mimalloc memory resource", "[memory_resource]") // NOLINT
{
    GIVEN("resources")
    {
        memory_resource resource;
        pmr::synchronized_pool_resource syn_resource;

        THEN("all default resource should be equal")
        {
            memory_resource another_resource;
            REQUIRE(resource == another_resource);
        }

        THEN("default resource should not be equal to synchronized_pool_resource")
        {
            REQUIRE(resource != syn_resource);
        }
    }

    GIVEN("resource, a default resource")
    {
        memory_resource resource;

        THEN("allocate 16 bytes from resource")
        {
            void* const ptr = resource.allocate(16, 16);

            AND_THEN("deallocate 16 bytes from resource")
            {
                resource.deallocate(ptr, 16, 16); //
            }
        }
    }

    GIVEN("pool_resource, get a default resource")
    {
        pmr::unsynchronized_pool_resource resource{&default_resource};

        THEN("allocate 16 bytes from  resource")
        {
            void* const ptr = resource.allocate(16, 16);

            AND_THEN("deallocate 16 bytes from  resource")
            {
                resource.deallocate(ptr, 16, 16); //
            }
        }
    }
}