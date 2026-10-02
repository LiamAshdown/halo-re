// hud_waypoint_activate_for_player  (Ghidra: FUN_004af0d0, renamed in the phase-4 review)
// address 0x4af0d0, size 206 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4af0d0..0x4af1a3. The per-player record is hud_waypoints + local player
// index * 0x30 (the first rewrite always used record 0; the index is 0 on retail anyway). An
// existing slot with the same kind (sign-extended low 4 bits) and target takes the new arrow
// and offset; otherwise the last free slot (kind 0xf) is filled; with no free slot nothing
// happens. Called by hud_waypoint_activate_for_team (0x4af1b0).
// register convention: EAX player, EBX target, DX kind; two stack arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data;            // 0x0087a480
extern hud_waypoint_state *hud_waypoints;  // 0x006b3a44

// Shows waypoint arrow arrow_index over target (a flag, object or custom waypoint by kind) for
// the local player of player_index.
// blam-cc: EAX -> player_index, EBX -> target, DX -> kind, stack -> arrow_index, vertical_offset
void hud_waypoint_activate_for_player(datum_index player_index, datum_index target, int16_t kind,
                                      int16_t arrow_index, float vertical_offset)
{
    hud_waypoint *waypoints;
    int16_t local_player_index;
    int16_t free_slot;
    int16_t i;

    if (player_index == (datum_index)-1) {
        return;
    }
    local_player_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->local_player_index;
    if (local_player_index < 0 || local_player_index >= 1 || target == (datum_index)-1 || arrow_index == -1) {
        return;
    }
    waypoints = hud_waypoints[local_player_index].waypoints;
    free_slot = -1;
    for (i = 0; i < 4; i++) {
        hud_waypoint *waypoint = &waypoints[i];
        int16_t slot_kind = (int16_t)(waypoint->type << 12) >> 12;

        if (slot_kind == kind && waypoint->object_index == target) {
            waypoint->arrow_index = arrow_index;
            waypoint->vertical_offset = vertical_offset;
            return;
        }
        if (slot_kind == -1) {
            free_slot = i;
        }
    }
    if (free_slot != -1) {
        hud_waypoint *waypoint = &waypoints[free_slot];
        waypoint->object_index = target;
        waypoint->arrow_index = arrow_index;
        waypoint->type = (int16_t)(waypoint->type ^ ((waypoint->type ^ kind) & 0xf));
        waypoint->vertical_offset = vertical_offset;
    }
}

#if 0
Original Ghidra decompilation (0x4af0d0):

void FUN_004af0d0(short param_1,undefined4 param_2)

{
  short *psVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  short in_DX;
  int unaff_EBX;
  short sVar4;
  short sVar5;

  sVar4 = -1;
  if ((((in_EAX != 0xffffffff) &&
       (sVar5 = *(short *)((in_EAX & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)),
       -1 < sVar5)) && (sVar5 < 1)) && ((unaff_EBX != -1 && (param_1 != -1)))) {
    iVar3 = sVar5 * 0x30 + DAT_006b3a44;
    sVar5 = 0;
    do {
      psVar1 = (short *)(iVar3 + sVar5 * 0xc);
      sVar2 = (short)(psVar1[1] << 0xc) >> 0xc;
      if ((sVar2 == in_DX) && (*(int *)(psVar1 + 4) == unaff_EBX)) {
        *psVar1 = param_1;
        *(undefined4 *)(psVar1 + 2) = param_2;
        return;
      }
      if (sVar2 == -1) {
        sVar4 = sVar5;
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < 4);
    if (sVar4 != -1) {
      psVar1 = (short *)(iVar3 + sVar4 * 0xc);
      *(int *)(psVar1 + 4) = unaff_EBX;
      *psVar1 = param_1;
      psVar1[1] = psVar1[1] ^ (byte)(*(byte *)(psVar1 + 1) ^ (byte)in_DX) & 0xf;
      *(undefined4 *)(psVar1 + 2) = param_2;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
