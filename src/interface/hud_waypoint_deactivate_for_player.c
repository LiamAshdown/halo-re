// hud_waypoint_deactivate_for_player  (Ghidra: FUN_004af230, renamed in the phase-4 review)
// address 0x4af230, size 123 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4af230..0x4af2aa. Frees the slot of {kind, target} in the record of the
// player local index: kind bits to 0xf, target and arrow to -1. The first rewrite always used
// record 0 (equal on retail, one local player).
// register convention: EAX player, EDI target, SI kind.
//   // blam-cc: player_index -> EAX, target -> EDI, kind -> SI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern data_array *player_data;            // 0x0087a480
extern hud_waypoint_state *hud_waypoints;  // 0x006b3a44

// blam-cc: player_index -> EAX, target -> EDI, kind -> SI
void hud_waypoint_deactivate_for_player(datum_index player_index, datum_index target, int16_t kind)
{
    hud_waypoint *waypoints;
    int16_t local_player_index;
    int16_t i;

    if (player_index == (datum_index)-1) {
        return;
    }
    local_player_index = ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->local_player_index;
    if (local_player_index < 0 || local_player_index >= 1 || target == (datum_index)-1) {
        return;
    }
    waypoints = hud_waypoints[local_player_index].waypoints;
    for (i = 0; i < 4; i++) {
        hud_waypoint *waypoint = &waypoints[i];
        if ((int16_t)(waypoint->type << 12) >> 12 == kind && waypoint->object_index == target) {
            waypoint->type |= 0xf;
            waypoint->object_index = (datum_index)-1;
            waypoint->arrow_index = -1;
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4af230):

void FUN_004af230(void)

{
  undefined2 *puVar1;
  uint in_EAX;
  int iVar2;
  short sVar3;
  short unaff_SI;
  int unaff_EDI;

  if ((((in_EAX != 0xffffffff) &&
       (sVar3 = *(short *)((in_EAX & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)),
       -1 < sVar3)) && (sVar3 < 1)) && (unaff_EDI != -1)) {
    iVar2 = sVar3 * 0x30 + DAT_006b3a44;
    sVar3 = 0;
    while ((puVar1 = (undefined2 *)(iVar2 + sVar3 * 0xc),
           (short)(*(short *)(iVar2 + 2 + sVar3 * 0xc) << 0xc) >> 0xc != unaff_SI ||
           (*(int *)(puVar1 + 4) != unaff_EDI))) {
      sVar3 = sVar3 + 1;
      if (3 < sVar3) {
        return;
      }
    }
    *(byte *)(puVar1 + 1) = *(byte *)(puVar1 + 1) | 0xf;
    *(undefined4 *)(puVar1 + 4) = 0xffffffff;
    *puVar1 = 0xffff;
  }
  return;
}
#endif
