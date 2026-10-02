// actor_get_idle_facing_range  (Ghidra: actor_get_idle_facing_range, renamed)
// address 0x4150f0, size 84 bytes
// name confidence: 0.4   rewrite confidence: 0.95 (VERIFIED against objdump 0x4150f0..0x415143)
// evidence: phase-4 summary "returns a pointer into the actor's unit structure holding the
// aim-pitch limit pair appropriate for the actor's current posture" -- actually returns a
// pointer into the actor's Actor tag data: the three candidate offsets (0xdc, 0xf4, 0x10c)
// are exactly Actor.noncombat_idle_facing / guard_idle_facing / combat_idle_facing
// (offsetof against types/tags.h, #pragma pack(1)), each a float[2] pair, selected by
// actor.unknown_3fc.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index
// UNSURE: actor.unknown_3fc's own meaning (a posture/behavior-state selector: 2 -> guard,
// 3..4 -> combat, else -> noncombat) is not established beyond this use.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> actor_index
float *actor_get_idle_facing_range(datum_index actor_index)
{
    actor *self;
    Actor *definition;
    int16_t state;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    state = self->look_posture;
    definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    if (state == 2) {
        return definition->guard_idle_facing;
    }
    if (2 < state && state < 5) {
        return definition->combat_idle_facing;
    }
    return definition->noncombat_idle_facing;
}

#if 0
Original Ghidra decompilation (0x4150f0):

int FUN_004150f0(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar1 = *(short *)(iVar2 + 0x3fc);
  iVar2 = *(int *)((*(uint *)(iVar2 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (sVar1 == 2) {
    return iVar2 + 0xf4;
  }
  if ((2 < sVar1) && (sVar1 < 5)) {
    return iVar2 + 0x10c;
  }
  return iVar2 + 0xdc;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
