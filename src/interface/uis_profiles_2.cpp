/**
 * Profile list and profile selection behaviour of the UI.
 */

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#include "halo/interface/uis_profiles.hpp"
#include "halo/saved_games/api.hpp"

extern "C" {
extern uint16_t new_profile_name_buffer_006b37f4[0xc];
extern int16_t new_profile_name_entry_player_00692b00;
extern uint8_t new_profile_name_flag_0071916e;
extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind);
extern uint8_t ui_new_profile_name_entry_open(void *widget, int16_t *event, uint8_t *out_handled);
extern int32_t saved_player_profile_slots_handle;
extern void saved_item_select(int32_t profile_index);
}

namespace halo::ui {

/**
 * Original UI routine; see docs/original/interface/ui_new_profile_name_entry_open.c.txt for the recovery notes.
 *
 * @address 0x4a1940
 */
uint8_t UiProfiles::new_profile_name_entry_open(void *widget, int16_t *event, uint8_t *out_handled)
{
    uint16_t default_name[128];

    (void)widget;
    (void)out_handled;
    halo::saved_games::saved_game_allocate_new_slot(default_name);
    wcsncpy((wchar_t *)new_profile_name_buffer_006b37f4, (const wchar_t *)default_name, 0xb);
    new_profile_name_buffer_006b37f4[0xb] = 0;
    new_profile_name_entry_player_00692b00 = event[1];
    new_profile_name_flag_0071916e = 0;
    virtual_keyboard_open(new_profile_name_buffer_006b37f4, 0x18, 8);
    return 1;
}

/**
 * Original UI routine; see docs/original/interface/ui_profile_select_or_create.c.txt for the recovery notes.
 *
 * @address 0x4a29a0
 */
uint8_t UiProfiles::profile_select_or_create(void *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t count = 1;
    int32_t slot;

    halo::saved_games::saved_game_enumerate_by_type(0, &slot, 0, (uint16_t *)&count);
    if (count > 0) {
        saved_item_select(saved_player_profile_slots_handle);
        return 1;
    }
    ui_new_profile_name_entry_open(widget, event, out_handled);
    new_profile_name_flag_0071916e = 1;
    return 0;
}

}
