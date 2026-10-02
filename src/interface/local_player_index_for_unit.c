// local_player_index_for_unit  (Ghidra: FUN_004940a0, unnamed)
// address 0x4940a0, size 72 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "Finds which local player is controlling the
// given unit object."; same local_players[0]/player_data walk as
// first_person_weapon_interface_tick.c, this time comparing player::unit against the argument
// instead of reading it. The decompiled loop only ever tests index 0 (the `0 < sVar2` guard
// exits for any other value before the match is checked, and index 1 unconditionally returns
// -1), matching "retail PC has one local player slot" from types/game.h.
// register convention: unit_index in EDI (unaff_EDI). // blam-cc: unit_index=EDI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480, "players"

// Finds which local player currently controls unit_index, returning -1 if none does.
int32_t local_player_index_for_unit(datum_index unit_index)
{
    int32_t i;
    player *record;

    for (i = 0; i < 1; i++) {
        if (local_player_globals->local_players[i] == (datum_index)0xffffffff) {
            continue;
        }
        record = (player *)((char *)player_data->data +
                             (local_player_globals->local_players[i] & 0xffff) * sizeof(player));
        if (record->unit == unit_index) {
            return i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4940a0):

int FUN_004940a0(void)

{
  uint uVar1;
  short sVar2;
  int iVar3;
  int unaff_EDI;

  iVar3 = 0;
  while ((((sVar2 = (short)iVar3, sVar2 == -1 || (0 < sVar2)) ||
          (uVar1 = *(uint *)(DAT_0087a478 + 4 + sVar2 * 4), uVar1 == 0xffffffff)) ||
         (*(int *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34)) != unaff_EDI)))
  {
    iVar3 = iVar3 + 1;
    if (0 < (short)iVar3) {
      return CONCAT22((short)((uint)iVar3 >> 0x10),0xffff);
    }
  }
  return iVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
