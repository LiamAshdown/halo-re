#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * On-screen virtual keyboard state machine.
 */
class VirtualKeyboard {
public:
    VirtualKeyboard() = delete;

    static void backspace();
    static uint8_t character_is_legal(int32_t validation_mode, uint8_t character);
    static uint8_t close();
    static void draw_text(Rectangle2D *bounds);
    static int32_t initialize();
    static uint8_t open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind);
    static void process_input();
    static void render();

private:
    static void vk_clear_text(void);
    static uint8_t vk_trim_trailing_whitespace(void);
    static void virtual_keyboard_set_text_state(int16_t column);
};

} // namespace halo::interface
