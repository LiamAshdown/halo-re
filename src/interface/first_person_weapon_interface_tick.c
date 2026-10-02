// first_person_weapon_interface_tick  (Ghidra: FUN_004923d0, unnamed)
// address 0x4923d0, size 96 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4923d0..0x49242f)
// evidence: out/phase4/interface_functions.md "Per-frame entry point for local player 0's
// first-person weapon interface: re-initializes the cached weapon-interface block when the
// controlled unit changed, then runs its tick update." Reads player_globals::local_players[0]
// (types/game.h) to find the current player index, then player_data[index].unit (0x34,
// "unit at +0x34" per types/hs.h's own note on the same array) to find the controlled object;
// compares it against first_person_weapon_interfaces[0].unit_index (types/interface.h).
// register convention: no register-passed arguments; hard-codes local player slot 0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals;          // 0x0087a478
extern data_array *player_data;                       // 0x0087a480, "players"
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

extern void first_person_weapon_interface_initialize(int16_t local_player_index); // 0x493c60, this module (movsx di at 0x493c6f)
extern void first_person_weapon_update(int32_t local_player_index); // 0x493150, this module

// Local player 0's per-frame first-person weapon entry point: if the current player slot is
// empty, does nothing; otherwise re-initializes the cached weapon-interface record whenever the
// controlled unit changed (or the record has no weapon yet), then runs the main weapon update.
void first_person_weapon_interface_tick(void)
{
    player *record;
    datum_index unit_index;
    first_person_weapon_interface *fp;

    fp = &first_person_weapon_interfaces[0];

    if (local_player_globals->local_players[0] != (datum_index)0xffffffff) {
        record = (player *)((char *)player_data->data +
                             (local_player_globals->local_players[0] & 0xffff) * sizeof(player));
        unit_index = record->unit;

        if (fp->unit_index != unit_index) {
            fp->unknown_30[0x20] = 0; // 0x50, inside the unnamed 0x30..0x87 scratch block
            fp->unit_index = unit_index;
            first_person_weapon_interface_initialize(0);
        }
        if (fp->weapon_index == (datum_index)0xffffffff) {
            first_person_weapon_interface_initialize(0);
        }
        first_person_weapon_update(0);
    }
}

#if 0
Original Ghidra decompilation (0x4923d0):

void FUN_004923d0(void)

{
  int iVar1;
  int iVar2;

  iVar2 = DAT_006b2d98;
  if (*(uint *)(DAT_0087a478 + 4) != 0xffffffff) {
    iVar1 = *(int *)((*(uint *)(DAT_0087a478 + 4) & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34)
                    + 0x34);
    if (*(int *)(DAT_006b2d98 + 4) != iVar1) {
      *(undefined1 *)(DAT_006b2d98 + 0x50) = 0;
      *(int *)(iVar2 + 4) = iVar1;
      FUN_00493c60(0);
    }
    if (*(int *)(iVar2 + 8) == -1) {
      FUN_00493c60(0);
    }
    first_person_weapon_update(0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
