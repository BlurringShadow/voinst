#include "test.h"

void mi_redirect_to_catch2()
{
    mi_register_output(
        [](const char* in, void*)
        {
            using namespace stdsharp::literals;

            std::string_view msg = in;

            constexpr ctll::fixed_string prefix = "mimalloc: ";

            if(!ctre::starts_with<prefix>(msg)) return;

            msg.remove_prefix(prefix.size());
            if(ctre::starts_with<"warning">(msg)) { WARN(msg); }
            else if(ctre::starts_with<"error">(msg)) { FAIL(msg); }
            else { INFO(msg); }
        },
        nullptr
    );
}
