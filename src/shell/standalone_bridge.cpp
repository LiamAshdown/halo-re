/**
 * @file src/shell/standalone_bridge.cpp
 * halo::shell wrappers over the standalone loader services.
 */

#include "halo/shell/standalone.hpp"

namespace halo::shell {

void standalone_log(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    halo::standalone::log_formatted(format, ap);
    va_end(ap);
}

int standalone_devmode(void)
{
    return halo::standalone::devmode_enabled() ? 1 : 0;
}

}  // namespace halo::shell
