#include "observable_memory/mimalloc/resource.h"
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

SCENARIO("mimalloc memory resource", "[mimalloc]") // NOLINT
{
    GIVEN("resource, another_resource, syn_resource, two default resource and a "
          "synchronized_pool_resource")
    {
        auto& resource = mimalloc::get_resource();
        auto& another_resource = mimalloc::get_resource();
        pmr::synchronized_pool_resource syn_resource;

        THEN("all default resource should be equal and same")
        {
            REQUIRE(resource == another_resource);
            REQUIRE_FALSE(resource != another_resource);
        }

        THEN("default resource should not be equal to synchronized_pool_resource")
        {
            REQUIRE_FALSE(resource == syn_resource);
            REQUIRE(resource != syn_resource);
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