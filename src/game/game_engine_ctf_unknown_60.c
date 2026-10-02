// game_engine_ctf_unknown_60  (not a Ghidra function; the ctf game engine definition's +0x60 slot (unknown_60); no C existed, so that
//   stored pointer trapped as unlisted_469270)
// address 0x469270, size 139 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469270..0x4692fa: false only as the server when the unit's player may not pick
//   up the weapon: the item is a live weapon that must be readied, its flag bit 6 (+0x22c) is clear, and its owner
//   team (+0xb8) is the player's team; true otherwise (also without a player or item).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"
#include "items.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack
extern uint32_t weapon_must_be_readied(datum_index item_index); // 0x4c2ea0, blam-cc: EAX

uint8_t game_engine_ctf_unknown_60(datum_index unit_index, datum_index item_index)
{
    datum_index player = player_index_from_unit_index(unit_index);
    uint8_t *weapon;

    if (player == 0xffffffff || item_index == 0xffffffff || network_game_mode != 2) {
        return 1;
    }
    weapon = (uint8_t *)object_try_and_get(item_index, 4);
    if (weapon != 0 && (uint8_t)weapon_must_be_readied(item_index) != 0 && (weapon[0x22c] & 0x40) == 0 &&
        ((struct weapon_object *)weapon)->base.owner_team == *(int32_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0x20)) {
        return 0;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
