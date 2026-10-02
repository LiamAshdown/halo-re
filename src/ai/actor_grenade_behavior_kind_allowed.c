// actor_grenade_behavior_kind_allowed  (Ghidra: actor_grenade_behavior_kind_allowed, renamed)
// address 0x40f670, size 139 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: phase-4 summary "checks mode- and timing-based gating rules to decide whether
// a particular grenade behavior kind is currently allowed for the actor."
// register convention: actor_index in EAX (Ghidra's in_EAX), kind on the stack (param_1).
// blam-cc: EAX -> actor_index, stack -> kind
// UNSURE: actor.unknown_60c/0x27c/0x278/0x161 are not otherwise named by types/ai.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, stack -> kind
uint8_t actor_grenade_behavior_kind_allowed(datum_index actor_index, int16_t kind)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (kind == 1) {
        return self->firing_target_type == 1 && 7 < self->target_combat_status;
    }
    if (kind == 2) {
        return self->firing_target_type == 0 && 4 < self->target_combat_status &&
               self->target_alive != 0 && 0x4a < self->ticks_since_engaged;
    }
    if (kind == 3) {
        return self->firing_target_type == 1 && 7 < self->target_combat_status && self->vehicle_gunner != 0;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40f670):

int FUN_0040f670(short param_1)

{
  uint in_EAX;
  int iVar1;
  uint3 uVar2;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar2 = (uint3)((uint)iVar1 >> 8);
  if (param_1 == 1) {
    if ((*(short *)(iVar1 + 0x60c) == 1) && (7 < *(short *)(iVar1 + 0x268))) {
LAB_0040f6b4:
      return CONCAT31(uVar2,1);
    }
  }
  else if (param_1 == 2) {
    if ((((*(short *)(iVar1 + 0x60c) == 0) && (4 < *(short *)(iVar1 + 0x268))) &&
        (*(char *)(iVar1 + 0x27c) != '\0')) && (0x4a < *(int *)(iVar1 + 0x278))) {
      return CONCAT31(uVar2,1);
    }
  }
  else {
    if (param_1 != 3) {
      return (uint)uVar2 << 8;
    }
    if (((*(short *)(iVar1 + 0x60c) == 1) && (7 < *(short *)(iVar1 + 0x268))) &&
       (*(char *)(iVar1 + 0x161) != '\0')) goto LAB_0040f6b4;
  }
  return (uint)uVar2 << 8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
