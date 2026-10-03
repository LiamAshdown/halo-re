/**
 * @file include/halo/interface/wide_text.hpp
 * Typed access to the UTF-16 text the interface code passes around: wide string literals and the text block a widget owns.
 */
#pragma once

#include <stdint.h>

#include "interface.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/** A wide string literal as the UTF-16 unit pointer the text API takes (wchar_t is 16 bits wide here). */
inline const uint16_t *wide(const wchar_t *literal) {
    return reinterpret_cast<const uint16_t *>(literal);
}

/** The UTF-16 text block of a text widget or label (its `text` slot). */
inline uint16_t *widget_text(const widget_instance *widget) {
    return static_cast<uint16_t *>(widget->text);
}

}  // namespace halo::interface
