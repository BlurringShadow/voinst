#include "observable_memory/mimalloc/resource.h"

#include "test.h"

using namespace std;
using namespace observable_memory;

SCENARIO("mimalloc memory resource", "[mimalloc]") // NOLINT
{
    // mi_redirect_to_catch2();

    GIVEN("a default resource")
    {
        auto& resource = mimalloc::get_default_resource();

        void* ptr = nullptr;

        THEN("allocate 16 bytes from resource") ptr = resource.allocate(16);

        THEN("deallocate 16 bytes from resource") resource.deallocate(ptr, 16);
    }
}
