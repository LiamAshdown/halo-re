// hud_waypoint_deactivate_for_team  (Ghidra: FUN_004af2b0, renamed in the phase-4 review)
// address 0x4af2b0, size 112 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: rewritten from objdump 0x4af2b0..0x4af31f in the phase-4 review (the first rewrite
// passed 0 for every register argument). For every local player of the team, calls
// hud_waypoint_deactivate_for_player with EAX the player datum, EDI the target and SI the
// kind (this function EAX).
// register convention: EAX kind; two stack arguments.
//   // blam-cc: kind -> EAX
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
extern void hud_waypoint_deactivate_for_player(datum_index player_index, datum_index target,
                                               int16_t kind); // 0x4af230, blam-cc: EAX player_index, EDI target, SI kind

// blam-cc: kind -> EAX
void hud_waypoint_deactivate_for_team(int16_t kind, int16_t team, datum_index target)
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
            hud_waypoint_deactivate_for_player(iterator.index, target, kind);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4af2b0):

void FUN_004af2b0(short param_1)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if ((*(short *)(iVar1 + 2) != -1) && ((int)param_1 == *(int *)(iVar1 + 0x20))) {
      FUN_004af230();
    }
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
