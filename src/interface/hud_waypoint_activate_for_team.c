// hud_waypoint_activate_for_team  (Ghidra: FUN_004af1b0, renamed in the phase-4 review)
// address 0x4af1b0, size 122 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: rewritten from objdump 0x4af1b0..0x4af229 in the phase-4 review (the first rewrite
// had no iterator and passed 0 for every register argument). Walks the players data_array
// with an inline iterator (signature data ^ 'reti') and, for every local player whose team
// (+0x20) equals the team argument, calls hud_waypoint_activate_for_player with EAX the player
// datum (iterator index), EBX the target (this function EAX) and DX the kind.
// register convention: EAX target; four stack arguments.
//   // blam-cc: target -> EAX
// reconciled: R16 the local iterator shadow struct (int32 next_index) is now types/memory.h data_iterator; the binary stores next_index as a WORD

#include <string.h>
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
extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void hud_waypoint_activate_for_player(datum_index player_index, datum_index target, int16_t kind,
                                             int16_t arrow_index, float vertical_offset); // 0x4af0d0, blam-cc: EAX player_index, EBX target, DX kind

// blam-cc: target -> EAX
void hud_waypoint_activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind,
                                    float vertical_offset)
{
    data_iterator iterator;
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0; // the binary writes only the low word
    iterator.index = (datum_index)-1;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    for (p = (player *)data_iterator_next(&iterator); p != 0;
         p = (player *)data_iterator_next(&iterator)) {
        if (p->local_player_index != -1 && (int32_t)team == p->team) {
            hud_waypoint_activate_for_player(iterator.index, target, kind, arrow_index, vertical_offset);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4af1b0):

void FUN_004af1b0(undefined4 param_1,short param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if ((*(short *)(iVar1 + 2) != -1) && ((int)param_2 == *(int *)(iVar1 + 0x20))) {
      FUN_004af0d0(param_1,param_4);
    }
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
