// actor_combat_status_should_hold  (Ghidra: actor_combat_status_should_hold, renamed)
// address 0x40d520, size 93 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump.)
// evidence: phase-4 summary "determines whether the actor should stay at its current
// combat status level rather than escalating, based on morale and scripted command
// restrictions"; reads actor.mode_data.raw bytes at +8 and +5 (mode-specific), the "vitality
// grade" at +0x6e and the scripted-restriction counter at +0x1e4.
// register convention: actor_index in EAX (no declared parameter, Ghidra's in_EAX);
// param_1/param_2 are the two declared int16 parameters, passed on the stack.
// blam-cc: EAX -> actor_index, stack -> param_1, param_2
// UNSURE: the two mode_data bytes (offsets 0x08 and 0x05 within the union) are not named
// by types/ai.h for any specific mode; kept as raw offsets into actor.mode_data.raw.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, stack -> threshold_a, threshold_b
uint8_t actor_combat_status_should_hold(datum_index actor_index, int16_t threshold_a, int16_t threshold_b)
{
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->mode_data.raw[8] != 0) {
        return (uint8_t)(threshold_b <= self->combat_status);
    }
    if (0 < *(int16_t *)&self->mode_data.raw[0] && self->combat_status < threshold_a &&
        (self->post_combat_action < 1 || self->mode_data.raw[5] != 0)) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x40d520):

int FUN_0040d520(short param_1,short param_2)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  uint3 uVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724;
  iVar2 = iVar1 + *(int *)(DAT_00880360 + 0x34);
  uVar3 = (uint3)((uint)iVar2 >> 8);
  if (*(char *)(iVar1 + 0xa4 + *(int *)(DAT_00880360 + 0x34)) != '\0') {
    return CONCAT31(uVar3,param_2 <= *(short *)(iVar2 + 0x6e));
  }
  if (((0 < *(short *)(iVar2 + 0x9c)) && (*(short *)(iVar2 + 0x6e) < param_1)) &&
     ((*(short *)(iVar2 + 0x1e4) < 1 || (*(char *)(iVar2 + 0xa1) != '\0')))) {
    return (uint)uVar3 << 8;
  }
  return CONCAT31(uVar3,1);
}
#endif
