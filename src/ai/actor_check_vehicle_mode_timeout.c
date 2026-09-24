// actor_check_vehicle_mode_timeout  (Ghidra: actor_check_vehicle_mode_timeout, renamed)
// address 0x428270, size 76 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/ai.h actor.mode(0x6c)==_actor_mode_vehicle(10) and actor.mode_data (the
//   per-mode union at 0x9c..0x11f); offsets 0xa0/0xa7/0xac used here fall inside that union
//   at mode_data[4]/[0xb]/[0x10] and have no individual names since they are only meaningful
//   in vehicle mode. Phase-4 summary: "For an actor in vehicle mode with a particular
//   sub-state, checks whether a per-actor tick counter has exceeded a global time
//   threshold."
// register convention: ECX -> actor_index (EAX is decompiler return-value scratch, not a
//   real input -- its low byte is never read).
//   // blam-cc: ECX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data; // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

// blam-cc: ECX -> actor_index
uint8_t actor_check_vehicle_mode_timeout(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->mode == _actor_mode_vehicle &&
        *(int16_t *)&self->mode_data[4] == 3 &&
        self->mode_data[0xb] != 0) {
        int32_t deadline = *(int32_t *)&self->mode_data[0x10] + 0x1e;
        return (int32_t)game_time->game_time <= deadline;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x428270):

uint FUN_00428270(void)

{
  uint in_EAX;
  uint uVar1;
  uint in_ECX;
  int iVar2;

  iVar2 = (in_ECX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar1 = in_EAX & 0xffffff00;
  if (((*(short *)(iVar2 + 0x6c) == 10) && (*(short *)(iVar2 + 0xa0) == 3)) &&
     (*(char *)(iVar2 + 0xa7) != '\0')) {
    iVar2 = *(int *)(iVar2 + 0xac) + 0x1e;
    uVar1 = CONCAT31((int3)((uint)iVar2 >> 8),*(int *)(DAT_006f1d6c + 0xc) <= iVar2);
  }
  return uVar1;
}
#endif
