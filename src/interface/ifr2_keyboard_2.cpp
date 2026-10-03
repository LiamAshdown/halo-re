#include "halo/interface/ifr2_keyboard.hpp"
#include "halo/interface/engine_state.hpp"
#include "saved_games.h"
#include "input.h"

extern "C" input_event_queue input_event_queue_active;
#include "crt.h"
#include <wchar.h>
#include <string.h>

#ifdef interface
#undef interface
#endif

extern "C" {
extern virtual_keyboard_globals virtual_keyboard;
extern uint8_t controls_input_capture_flags;
extern datum_index tag_lookup(tag_group group, char *path);
extern int32_t time_query_performance_counter_ms(void);
extern void widget_play_sound_effect(int16_t effect_id);
extern void **keyboard_device;
extern uint8_t key_frames[0x6d];
extern uint8_t key_release_pending[0x6d];
}

namespace halo::interface {

/**
 * Opens the on-screen virtual keyboard for a caller-supplied wide-string buffer, clamping maximum_length to
 * 0x40 bytes, choosing the large or small UI prompt tag by length, and resetting the DirectInput keyboard
 * device's buffered input if one is active. Returns 0 without doing anything if the keyboard is already open
 * or its strings tag failed to load. blam-cc: ESI -> destination, stack -> maximum_length, field_kind
 *
 * @address 0x4a89a0
 */
uint8_t VirtualKeyboard::open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind)
{
    if (virtual_keyboard.active != 0 || virtual_keyboard.strings_tag_data == 0) {
        return 0;
    }

    memset(input_event_queue_active.events, 0, sizeof(input_event_queue_active.events));

    virtual_keyboard.caret = 0;
    virtual_keyboard.unknown_0a = 0;
    virtual_keyboard.active = 1;
    virtual_keyboard.destination = destination;
    virtual_keyboard.destination_end = destination + wcslen((const wchar_t *)destination);
    virtual_keyboard.maximum_length = (maximum_length > 0x3f) ? 0x40 : (int16_t)maximum_length;
    virtual_keyboard.selection_start = -1;
    virtual_keyboard.open_time = time_query_performance_counter_ms();
    virtual_keyboard.field_kind = field_kind;
    virtual_keyboard.unknown_01 = 0;
    virtual_keyboard.unknown_02 = 0;
    virtual_keyboard.unknown_03 = 0;
    virtual_keyboard.opened = 1;
    virtual_keyboard.validation_mode = 1;
    wcsncpy((wchar_t *)virtual_keyboard.text, (const wchar_t *)destination, 0x20);
    virtual_keyboard.text[31] = 0;
    virtual_keyboard.committed = 0;
    virtual_keyboard.large_ui_tag = tag_lookup(0x666f6e74  , (char *)"ui\\large_ui");
    virtual_keyboard.small_ui_tag = tag_lookup(0x666f6e74  , (char *)((maximum_length < 0x33) ? "ui\\large_ui" : "ui\\small_ui"));
    widget_play_sound_effect(2);

    controls_input_capture_flags |= 4;

    if (keyboard_device != 0) {
        int32_t minus_one = -1;
        void **vtable = *(void ***)keyboard_device;
        ((directinput_set_property_fn)vtable[0x28 / 4])(keyboard_device, 0x14, 0, &minus_one, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }
    return 1;
}

} // namespace halo::interface

extern "C" {

uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind)
{
    return halo::interface::VirtualKeyboard::open(destination, maximum_length, field_kind);
}

}
