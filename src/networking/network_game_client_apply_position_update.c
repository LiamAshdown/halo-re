// network_game_client_apply_position_update  (Ghidra: FUN_004dff70, unnamed)
// address 0x4dff70, size 264 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Applies a received position/orientation
// delta packet to the local object via FUN_00473390, after validating the packet's tick and
// delta-count fields against the destination's tag data (datum_get)." 0x006b1460 matches
// types/game.h's `machine_to_player[16]`; the datum_get idiom (index/salt check against
// player_data) matches src/memory/datum_get.c exactly; player+0x11c matches types/game.h's
// player::unknown_11c.
// register convention: all four parameters are genuine stack (cdecl) parameters per Ghidra's
// own signature for this function.
// blam-cc: stack -> state, packet, param_3 (unused), object
// UNSURE: `state` (param_1) has no named type; its two touched fields (a tick at +0x04 and a
// machine-index byte pair at +0x0c) line up with the 0x34-byte network_machine::connect_state
// block that network_game_client_apply_received_update.c stages into a local copy before
// calling this function with no visible arguments -- so `state` is very likely a pointer into
// (or the base of) that staged copy, but connect_state itself is documented as fully opaque
// in out/phase4/networking_types_notes.md, so no field names are invented for it here; raw
// offsets are used instead. param_3 is never read in the original.
// UNSURE: FUN_00473390's real parameter types are not established here; `object` is passed
// through unmodified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern datum_index machine_to_player[16]; // 0x006b1460
extern data_array *player_data; // 0x0087a480, stride 0x200 (game module)
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, memory module
extern void FUN_00473390(uint32_t *delta, void *object); // other module (UNSURE)

// Applies one position/orientation delta record from `packet` onto `object`, but only if the
// packet's tick is not older than the last one recorded in `state`, its delta-item count is 0
// or 1, and `state`'s machine-index slot resolves (through machine_to_player and player_data)
// to a player that currently has a live unit.
void network_game_client_apply_position_update(uint8_t *state, uint32_t *packet, void *param_3, void *object)
{
    int16_t delta_count;
    uint32_t delta[16];
    int32_t i;
    uint32_t *src;
    datum_index player_datum;
    player *plr;

    (void)param_3;
    if (*(uint32_t *)(state + 4) > (*packet & 0x7fffffff)) {
        return;
    }
    delta_count = *(int16_t *)((uint8_t *)packet + 6);
    if (delta_count < 0 || delta_count >= 2) {
        return;
    }

    for (i = 0; i < 8; i = i + 1) {
        delta[i] = 0;
    }
    if (delta_count > 0) {
        src = packet + 2;
        for (i = 0; i < 8; i = i + 1) {
            delta[i] = src[i];
        }
    }

    player_datum = machine_to_player[*(uint16_t *)(state + 0xc)];
    if (player_datum == (datum_index)0xffffffff) {
        return;
    }
    plr = (player *)datum_get(player_datum, player_data);
    if (plr == 0 || plr->unit == (datum_index)0xffffffff) {
        return;
    }

    FUN_00473390(delta, object);
    *(uint32_t *)(state + 4) = *packet & 0x7fffffff;
    for (i = 0; i < 8; i = i + 1) {
        delta[8 + i] = delta[i];
    }
    plr = (player *)datum_get(player_datum, player_data);
    if (plr != 0) {
        plr->unknown_11c = delta[8] & 0x4d0;
    }
}

#if 0
Original Ghidra decompilation (0x4dff70):

void FUN_004dff70(int param_1,uint *param_2,undefined4 param_3,undefined4 param_4)

{
  short sVar1;
  int iVar2;
  uint *puVar3;
  uint *puVar4;
  uint local_40 [16];

  if (((*(uint *)(param_1 + 4) <= (*param_2 & 0x7fffffff)) &&
      (sVar1 = *(short *)((int)param_2 + 6), -1 < sVar1)) && (sVar1 < 2)) {
    local_40[1] = 0;
    local_40[2] = 0;
    local_40[3] = 0;
    local_40[4] = 0;
    local_40[5] = 0;
    local_40[6] = 0;
    local_40[7] = 0;
    local_40[0] = 0;
    if (0 < sVar1) {
      puVar3 = param_2 + 2;
      puVar4 = local_40;
      for (iVar2 = ((int)sVar1 & 0x7ffffffU) << 3; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
    }
    if ((((&DAT_006b1460)[*(ushort *)(param_1 + 0xc)] != -1) && (iVar2 = datum_get(), iVar2 != 0))
       && (*(int *)(iVar2 + 0x34) != -1)) {
      FUN_00473390(local_40,param_4);
      *(uint *)(param_1 + 4) = *param_2 & 0x7fffffff;
      puVar3 = local_40;
      puVar4 = local_40 + 8;
      for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
      iVar2 = datum_get();
      if (iVar2 != 0) {
        *(uint *)(iVar2 + 0x11c) = local_40[8] & 0x4d0;
      }
    }
  }
  return;
}
#endif
