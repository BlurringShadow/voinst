#include "observable_memory/mimalloc/resource.h"
#include "test.h"

using namespace std;
using namespace observable_memory;

SCENARIO("mimalloc memory resource", "[mimalloc]") // NOLINT
{
    GIVEN("resource, another_resource, two default resource")
    {
        auto& resource = mimalloc::get_resource();
        auto& another_resource = mimalloc::get_resource();

        THEN("all default resource should be equal and same")
        {
            REQUIRE(resource == another_resource);
            REQUIRE_FALSE(resource != another_resource);
        }
    }

    GIVEN("resource, a default resource")
    {
        auto& resource = mimalloc::get_resource();

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
        pmr::synchronized_pool_resource pool_resource{&mimalloc::get_resource()};

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