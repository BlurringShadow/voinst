#include "voinst/memory_resource.h"
#include "test.h"

using namespace std;
using namespace voinst;

using memory_resource = voinst::memory_resource;

SCENARIO("mimalloc memory resource", "[mimalloc]") // NOLINT
{
    GIVEN("resource, another_resource, syn_resource, two default resource and a "
          "synchronized_pool_resource")
    {
        memory_resource resource;
        memory_resource another_resource;
        voinst::pmr::synchronized_pool_resource syn_resource;

        THEN("all default resource should be not equal or same")
        {
            REQUIRE(resource != another_resource);
        }

        THEN("default resource should not be equal to synchronized_pool_resource")
        {
            REQUIRE_FALSE(resource == syn_resource);
            REQUIRE(resource != syn_resource);
        }
    }

    GIVEN("resource, a default resource")
    {
        memory_resource resource;

        THEN("allocate 16 bytes from resource")
        {
            void* ptr = resource.allocate(16, 16);

            AND_THEN("deallocate 16 bytes from resource")
            {
                resource.deallocate(ptr, 16, 16); //
            }
        }
    }

    GIVEN("pool_resource, construct a synchronized pool resource from default resource")
    {
        memory_resource resource;
        voinst::pmr::synchronized_pool_resource pool_resource{&resource};

        THEN("allocate 16 bytes from pool resource")
        {
            void* ptr = pool_resource.allocate(16, 16);

            AND_THEN("deallocate 16 bytes from pool resource")
            {
                pool_resource.deallocate(ptr, 16, 16); //
            }
        }
    }
}