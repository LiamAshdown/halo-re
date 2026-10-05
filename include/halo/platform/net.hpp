/**
 * @file include/halo/platform/net.hpp
 * Networking helpers independent of the operating system.
 */
#pragma once

#include <cstdint>
#include <cstdio>

namespace halo::platform {

/** address (host byte order, first octet in the high byte) as dotted-quad text in a static buffer, as inet_ntoa. */
inline char *ipv4_text(uint32_t address)
{
    static char text[16];

    snprintf(text, sizeof(text), "%u.%u.%u.%u", address >> 24, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff);
    return text;
}

}  // namespace halo::platform
