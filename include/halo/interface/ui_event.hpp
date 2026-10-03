/**
 * @file include/halo/interface/ui_event.hpp
 * Field access of the widget input event record: four 16-bit words, [0] the event kind, [1] the controller, [2] the code in its
 * low byte and the key state in its high byte, [3] the value.
 */
#pragma once

#include <stdint.h>

namespace halo::interface {

/** The key state byte of an input event (1 while the key is down), the high byte of word 2. */
inline int8_t event_state(const int16_t *event) {
    return static_cast<int8_t>(static_cast<uint16_t>(event[2]) >> 8);
}

/** The code byte of an input event (a character or a button index), the low byte of word 2. */
inline uint8_t event_code(const int16_t *event) {
    return static_cast<uint8_t>(event[2]);
}

}  // namespace halo::interface
