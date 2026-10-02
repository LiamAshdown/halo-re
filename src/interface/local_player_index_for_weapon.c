// local_player_index_for_weapon  (Ghidra: FUN_00494010, unnamed)
// address 0x494010, size 135 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md "Finds which local player currently has the given
// weapon object equipped."; resolves the controlled unit the same way as
// local_player_index_for_unit.c, then compares unit->weapons[unit->current_weapon_index]
// (types/units.h, 0x2f2/0x2f8) against the argument. Loop bound is `< 1`, matching every other
// single-local-player loop in this module.
// register convention: weapon_index is the one Ghidra-recognized stack parameter (param_1).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480, "players"
extern data_array *object_data; // 0x008603b0, "objects"

// Finds which local player currently has weapon_index equipped as its unit's active weapon,
// returning -1 if none does.
int32_t local_player_index_for_weapon(datum_index weapon_index)
{
    int32_t i;
    player *record;
    object_header *header;
    unit_data *u;
    int16_t slot;

    for (i = 0; i < 1; i++) {
        if (local_player_globals->local_players[i] == (datum_index)0xffffffff) {
            continue;
        }
        record = (player *)((char *)player_data->data +
                             (local_player_globals->local_players[i] & 0xffff) * sizeof(player));
        if (record->unit == (datum_index)0xffffffff) {
            continue;
        }
        header = &((object_header *)object_data->data)[record->unit & 0xffff];
        u = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
        slot = u->current_weapon_index;
        if (slot != -1 && weapon_index == u->weapons[slot]) {
            return i;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x494010):

int FUN_00494010(int param_1)

{
  uint uVar1;
  int iVar2;
  short sVar3;
  int iVar4;

  iVar4 = 0;
  do {
    sVar3 = (short)iVar4;
    if ((((sVar3 != -1) && (sVar3 < 1)) &&
        (uVar1 = *(uint *)(DAT_0087a478 + 4 + sVar3 * 4), uVar1 != 0xffffffff)) &&
       (uVar1 = *(uint *)((uVar1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34),
       uVar1 != 0xffffffff)) {
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
      sVar3 = *(short *)(iVar2 + 0x2f2);
      if ((sVar3 != -1) && (param_1 == *(int *)(iVar2 + 0x2f8 + sVar3 * 4))) {
        return iVar4;
      }
    }
    iVar4 = iVar4 + 1;
    if (0 < (short)iVar4) {
      return CONCAT22((short)((uint)iVar4 >> 0x10),0xffff);
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
