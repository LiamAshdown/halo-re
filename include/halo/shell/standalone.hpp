/**
 * @file include/halo/shell/standalone.hpp
 * Services of the standalone loader (standalone/loader.cpp) as seen from the engine: the standalone log and developer mode.
 * The loader functions live in halo::standalone; halo::shell wraps them for the engine modules.
 */
#pragma once

#include <stdarg.h>

namespace halo::standalone {

/** Writes one formatted line to the standalone log. */
void log_formatted(const char *format, va_list ap);

/** True when developer mode is enabled (-devmode or HALO_DEVMODE). */
bool devmode_enabled();

}  // namespace halo::standalone

namespace halo::shell {

/** printf-style line to the standalone log. */
void standalone_log(const char *format, ...);

/** 1 when developer mode is enabled, else 0. */
int standalone_devmode(void);

}  // namespace halo::shell
