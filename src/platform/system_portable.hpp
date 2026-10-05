#pragma once

/**
 * @file src/platform/system_portable.hpp
 * Plain-computation stand-ins for Windows system services (src/platform/system_portable.cpp).
 */

#include <stdint.h>

namespace halo::platform::portable {

/** SHA-1 of size bytes. */
void sha1(const uint8_t *data, uint32_t size, uint8_t digest[20]);

/** Windows-1252 to UTF-16 and back, with MultiByteToWideChar / WideCharToMultiByte's counting rules. */
int32_t ansi_to_wide(const char *text, int32_t length, uint16_t *out, int32_t capacity);
int32_t wide_to_ansi(const uint16_t *text, int32_t length, char *out, int32_t capacity);

}  // namespace halo::platform::portable
