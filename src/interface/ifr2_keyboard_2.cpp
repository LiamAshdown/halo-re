#include "halo/interface/ifr2_keyboard.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/interface/engine_state.hpp"
#include "saved_games.h"
#include "input.h"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"

static auto &input_event_queue_active = halo::link::ref<input_event_queue>(halo::ui::vars().input_event_queue_active);
#include "crt.h"
#include <wchar.h>
#include <string.h>
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/interface/records.hpp"
#include "halo/interface/com_object.hpp"

#ifdef interface
#undef interface
#endif

static auto &virtual_keyboard = halo::link::ref<virtual_keyboard_globals>(halo::ui::vars().virtual_keyboard);
static auto &controls_input_capture_flags = halo::link::ref<uint8_t>(halo::ui::vars().controls_input_capture_flags);
static auto &keyboard_device = halo::link::ref<void **>(halo::ui::vars().keyboard_device);
static auto &key_frames = halo::link::ref<uint8_t [0x6d]>(halo::ui::vars().key_frames);
static auto &key_release_pending = halo::link::ref<uint8_t [0x6d]>(halo::ui::vars().key_release_pending);

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
    virtual_keyboard.open_time = halo::cseries::time_query_performance_counter_ms();
    virtual_keyboard.field_kind = field_kind;
    virtual_keyboard.unknown_01 = 0;
    virtual_keyboard.unknown_02 = 0;
    virtual_keyboard.unknown_03 = 0;
    virtual_keyboard.opened = 1;
    virtual_keyboard.validation_mode = 1;
    wcsncpy((wchar_t *)virtual_keyboard.text, (const wchar_t *)destination, 0x20);
    virtual_keyboard.text[31] = 0;
    virtual_keyboard.committed = 0;
    virtual_keyboard.large_ui_tag = halo::interface::lookup_tag(halo::fourcc('f', 'o', 'n', 't'), halo::tag_paths::large_ui);
    virtual_keyboard.small_ui_tag = halo::interface::lookup_tag(halo::fourcc('f', 'o', 'n', 't'), (maximum_length < 0x33) ? halo::tag_paths::large_ui : halo::tag_paths::small_ui);
    halo::interface::widget_play_sound_effect(2);

    controls_input_capture_flags |= 4;

    if (keyboard_device != 0) {
        int32_t minus_one = -1;
        void **vtable = halo::interface::com_vtable(keyboard_device);
        ((directinput_set_property_fn)vtable[0x28 / 4])(keyboard_device, 0x14, 0, &minus_one, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }
    return 1;
}

} // namespace halo::interface

namespace halo::interface {

uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind)
{
    return halo::interface::VirtualKeyboard::open(destination, maximum_length, field_kind);
}

}
