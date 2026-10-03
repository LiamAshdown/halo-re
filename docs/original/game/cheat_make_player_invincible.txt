// cheat_make_player_invincible  (Ghidra: FUN_0045a720; renamed per symbols/review_queue.txt)
// address 0x45a720, size 114 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: types/game.h player_globals::local_players (0x0087a478+4), player::unit (0x34);
//   types/units.h unit_data::unknown_37c/flags. Same field writes as
//   cheat_make_selected_object_invincible.c (0x45a6c0), applied to a specific local player's
//   unit instead of a debug-selected object.
// register convention: local-player slot in AX (in_AX; only slot 0 is ever valid, matching
//   k_maximum_local_players == 1).
//   // blam-cc: in_AX -> local_player_slot

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data;              // 0x008603b0

// Forces local player `local_player_slot`'s controlled unit's shield-like field to full and sets
// its invincibility-style flag bits.
void cheat_make_player_invincible(int16_t local_player_slot)
    // blam-cc: in_AX -> local_player_slot
{
    datum_index player_index;
    datum_index unit_index;
    object *unit_obj;
    unit_data *unit;

    if (-1 < local_player_slot && local_player_slot < 1 && local_player_slot != -1) {
        player_index = local_player_globals->local_players[local_player_slot];
        if (player_index != k_datum_index_none) {
            unit_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player)))->unit;
            unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
            unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
            unit->active_camouflage_power = 1.0f;
            if ((unit->flags & 0x10) != 0) {
                unit->flags = unit->flags | 0x20;
            }
            unit->flags = unit->flags | 0x10;
        }
    }
}

#if 0
Original Ghidra decompilation (0x45a720), from tools/pack.py 0x45a720:

void FUN_0045a720(void)

{
  uint uVar1;
  int iVar2;
  short in_AX;

  if ((((-1 < in_AX) && (in_AX < 1)) && (in_AX != -1)) &&
     (uVar1 = *(uint *)(DAT_0087a478 + 4 + in_AX * 4), uVar1 != 0xffffffff)) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                    (*(uint *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) &
                    0xffff) * 0xc);
    *(undefined4 *)(iVar2 + 0x37c) = 0x3f800000;
    if ((*(uint *)(iVar2 + 0x204) & 0x10) != 0) {
      *(uint *)(iVar2 + 0x204) = *(uint *)(iVar2 + 0x204) | 0x20;
    }
    *(uint *)(iVar2 + 0x204) = *(uint *)(iVar2 + 0x204) | 0x10;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
