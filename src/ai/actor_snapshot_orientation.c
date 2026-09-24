// actor_snapshot_orientation  (Ghidra: actor_snapshot_orientation, already named)
// address 0x4294d0, size 160 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/ai.h actor.facing/facing_unknown_180/facing_unknown_18c (0x174/0x180/0x18c)
//   copied verbatim (9 consecutive dwords) to snapshot_facing/snapshot_unknown_708/
//   snapshot_unknown_714 (0x6fc/0x708/0x714); actor.flags(0x6d0)/override_target(0x720)/
//   queued_look_vector(0x6e0)/unknown_6ec, all cited by this exact address in the header.
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern const real_vector3d *global_origin3d_pointer; // 0x00696714

// blam-cc: EAX -> actor_index
// Snapshots the actor's current orientation basis (facing and its two companion vectors)
// into the secondary snapshot block, resets flags and override_target to zero, seeds
// queued_look_vector from the shared origin vector, and marks the snapshot's status word
// unset.
void actor_snapshot_orientation(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    self->snapshot_facing = self->facing;
    self->snapshot_unknown_708 = self->facing_unknown_180;
    self->snapshot_unknown_714 = self->facing_unknown_18c;

    self->flags = 0;
    self->override_target = 0;
    self->queued_look_vector = *global_origin3d_pointer;
    self->unknown_6ec = -1;
}

#if 0
Original Ghidra decompilation (0x4294d0):

void actor_snapshot_orientation(void)

{
  undefined *puVar1;
  uint in_EAX;
  int iVar2;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *(undefined4 *)(iVar2 + 0x6fc) = *(undefined4 *)(iVar2 + 0x174);
  *(undefined4 *)(iVar2 + 0x700) = *(undefined4 *)(iVar2 + 0x178);
  *(undefined4 *)(iVar2 + 0x704) = *(undefined4 *)(iVar2 + 0x17c);
  *(undefined4 *)(iVar2 + 0x708) = *(undefined4 *)(iVar2 + 0x180);
  *(undefined4 *)(iVar2 + 0x70c) = *(undefined4 *)(iVar2 + 0x184);
  *(undefined4 *)(iVar2 + 0x710) = *(undefined4 *)(iVar2 + 0x188);
  *(undefined4 *)(iVar2 + 0x714) = *(undefined4 *)(iVar2 + 0x18c);
  *(undefined4 *)(iVar2 + 0x718) = *(undefined4 *)(iVar2 + 400);
  *(undefined4 *)(iVar2 + 0x71c) = *(undefined4 *)(iVar2 + 0x194);
  puVar1 = PTR_DAT_00696714;
  *(undefined4 *)(iVar2 + 0x6d0) = 0;
  *(undefined4 *)(iVar2 + 0x720) = 0;
  *(undefined4 *)(iVar2 + 0x6e0) = *(undefined4 *)puVar1;
  *(undefined4 *)(iVar2 + 0x6e4) = *(undefined4 *)(puVar1 + 4);
  *(undefined4 *)(iVar2 + 0x6e8) = *(undefined4 *)(puVar1 + 8);
  *(undefined2 *)(iVar2 + 0x6ec) = 0xffff;
  return;
}
#endif
