// players_rebind_local_player_after_load  (not a Ghidra function; game state after-load proc 10)
// address 0x4765f0, size 359 bytes
// name confidence: 0.3  rewrite confidence: 0.8
// evidence: game_state_after_load_procs[10] (0x0069e7dc) holds 0x4765f0.
// objdump 0x4765f0..0x476756: the wanted local slot is profile_slot_id[0] (0x00714dde; -1 means 0). Unless
//   that slot is below 1 and already has a player (player_globals +0x04), and only with exactly one player
//   (0x006894b8 == 1), the first local slot 0..3 that is in range (< 1) and holds a player is moved to the
//   wanted slot: the old slot's player gets local_player_index (+0x02) -1 and the slot -1; the old slot's
//   player control record (player_control_globals 0x006b145c +0x10, 0x40 each) is reset (zeroed, +0x00 and
//   +0x28 -1, +0x20/+0x22/+0x24 -1, +0x26 0, pitch limits +0x38 -1.4906585 / +0x3c 1.4906585, +0x08/+0x0a
//   0); game_set_local_player (ECX player, SI slot) and game_engine_init_player_look_state_from_object
//   (EDX unit, AX slot) rebind it; the hud weapon interface records (0x00719430: 0x28 each at +0, 0x50
//   each at +0x28) and hud unit meters (0x0071942c, 0x58 each) are copied from the old slot to the new.
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include <string.h>

extern player_globals *local_player_globals;      // 0x0087a478
extern data_array *player_data;                   // 0x0087a480
extern int16_t profile_slot_id[];                 // 0x00714dde
extern int16_t game_player_count_word;            // 0x006894b8
extern uint8_t *player_control_globals_ptr;       // 0x006b145c
extern uint8_t *hud_weapon_interface_globals;     // 0x00719430
extern uint8_t *hud_unit_meters;                  // 0x0071942c

extern void game_set_local_player(datum_index player_handle, int16_t local_player_index); // 0x474d50, ECX, SI
extern void game_engine_init_player_look_state_from_object(datum_index unit, int16_t local_player_index); // 0x470e80, EDX, AX

void players_rebind_local_player_after_load(void)
{
    int16_t local = profile_slot_id[0];
    int16_t slot;
    datum_index handle = k_datum_index_none;
    player *entry;
    uint8_t *control;

    if (local == -1) {
        local = 0;
    }
    if (local < 1 && local_player_globals->local_players[local] != k_datum_index_none) {
        return;
    }
    if (game_player_count_word != 1) {
        return;
    }
    for (slot = 0; slot < 4; slot++) {
        if (slot < 1) {
            handle = local_player_globals->local_players[slot];
            if (handle != k_datum_index_none) {
                break;
            }
        }
    }
    if (slot >= 4) {
        return;
    }

    entry = (player *)((uint8_t *)player_data->data + (handle & 0xffff) * 0x200);
    {
        datum_index old = local_player_globals->local_players[slot];

        if (old != k_datum_index_none) {
            ((player *)((uint8_t *)player_data->data + (old & 0xffff) * 0x200))->local_player_index = -1;
        }
        local_player_globals->local_players[slot] = k_datum_index_none;
    }

    control = player_control_globals_ptr + 0x10 + slot * 0x40;
    memset(control, 0, 0x40);
    *(int32_t *)(control + 0x00) = -1;
    *(int16_t *)(control + 0x20) = -1;
    *(int16_t *)(control + 0x22) = -1;
    *(int16_t *)(control + 0x24) = -1;
    *(int32_t *)(control + 0x28) = -1;
    control[0x26] = 0;
    *(uint32_t *)(control + 0x3c) = 0x3fbf0243;
    *(uint32_t *)(control + 0x38) = 0xbfbf0243;
    *(int16_t *)(control + 0x08) = 0;
    *(int16_t *)(control + 0x0a) = 0;

    game_set_local_player(handle, local);
    game_engine_init_player_look_state_from_object(entry->unit, local);

    memmove(hud_weapon_interface_globals + local * 0x28, hud_weapon_interface_globals + slot * 0x28, 0x28);
    memmove(hud_weapon_interface_globals + 0x28 + local * 0x50, hud_weapon_interface_globals + 0x28 + slot * 0x50, 0x50);
    memmove(hud_unit_meters + local * 0x58, hud_unit_meters + slot * 0x58, 0x58);
}
