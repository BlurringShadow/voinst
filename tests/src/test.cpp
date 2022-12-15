#include "test.h"

void mi_redirect_to_catch2()
{
    mi_register_output(
        [](const char* in, void*)
        {
            using namespace stdsharp::literals;

            std::string_view msg = in;

            constexpr auto prefix = "mimalloc: "sv;

            if(!msg.starts_with(prefix)) return;

            msg.remove_prefix(prefix.size());
            if(msg.starts_with("warning: "))
            {
                msg.remove_prefix("warning: "sv.size());
                WARN(msg);
            }
            else if(msg.starts_with("error: "))
            {
                msg.remove_prefix("error: "sv.size());
                FAIL(msg);
            }
            else INFO(msg);
        },
        nullptr
    );
}
