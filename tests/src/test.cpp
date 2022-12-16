#include "test.h"
#include <iostream>
#include <sstream>

void mi_redirect_to_catch2()
{
    mi_register_output(
        [](const char* in, void*)
        {
            using namespace stdsharp::literals;

            static thread_local enum { info, warning, error } level;

            static thread_local std::string buffer;

            constexpr auto prefix = "mimalloc: "sv;

            std::string_view msg = in;

            buffer += msg;

            if(msg.starts_with(prefix))
            {
                msg.remove_prefix(prefix.size());
                if(msg.starts_with("warning"))
                {
                    msg.remove_prefix(sizeof("warning") - 1);
                    level = warning;
                }
                else if(msg.starts_with("error"))
                {
                    msg.remove_prefix(sizeof("error") - 1);
                    level = error;
                }
                else level = info;
            }

            if(buffer.ends_with('\n'))
            {
                buffer.insert(0, "mimalloc: ");

                switch(level)
                {
                case info:
                {
                    INFO(buffer);
                    break;
                }
                case warning:
                {
                    WARN(buffer);
                    break;
                }
                case error:
                {
                    FAIL(buffer);
                    break;
                }
                }
            }

            buffer.clear();
        },
        nullptr
    );
}
