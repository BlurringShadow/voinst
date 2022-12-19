#include "observable_memory/mimalloc/resource.h"
#include "test.h"

using namespace std;
using namespace observable_memory;

SCENARIO("mimalloc memory resource", "[mimalloc]") // NOLINT
{
    GIVEN("a default resource")
    {
        auto& resource = mimalloc::get_default_resource();

        THEN("allocate 16 bytes from resource")
        {
            void* ptr = resource.allocate(16, 16);

            AND_THEN("deallocate 16 bytes from resource")
            {
                resource.deallocate(ptr, 16, 16); //
            }
        }
    }

    GIVEN("construct a synchronized pool resource from default resource")
    {
        pmr::synchronized_pool_resource pool_resource{&mimalloc::get_default_resource()};

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