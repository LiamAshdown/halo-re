// actor_update_special_mode  (Ghidra: actor_update_special_mode, renamed)
// address 0x40d820, size 162 bytes
// name confidence: 0.3   rewrite confidence: 0.95 (verified vs objdump 0x40d820..0x40d8c1; the alert stages get their prop / actor)
// evidence: phase-4 summary "checks whether the actor's current special mode (5, 7, or 8)
// is ready to proceed and, if so, invokes the matching per-mode helper before a common
// cleanup step"; every path that proceeds falls through to
// actor_update_melee_combat_action (0x40cdf0, this module).
// register convention: actor_index in EAX (Ghidra's in_EAX).
// blam-cc: EAX -> actor_index
// UNSURE: mode_data offsets 0x00, 0x01, 0x08 are not otherwise named by types/ai.h for
// these modes. actor_set_target_alert_stage1/actor_set_target_alert_stage2/actor_set_target_alert_stage3 are called with zero visible
// arguments; needs the disassembly review pass to confirm they don't also need actor_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360


// blam-cc: EAX -> actor_index
uint8_t actor_update_special_mode(datum_index actor_index)
{
    actor *self;
    int16_t mode;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    mode = self->mode;

    if (mode == 5) {
        if (self->mode_data.raw[1] == 0) {
            return 0;
        }
        if (*(int16_t *)&self->mode_data.raw[8] == 0) {
            actor_set_target_alert_stage1(self->target_unit_index, actor_index);
        }
    } else if (mode == 7) {
        if (self->mode_data.raw[0] == 0) {
            return 0;
        }
        if (*(int16_t *)&self->mode_data.raw[8] == 0) {
            actor_set_target_alert_stage2(self->target_unit_index, actor_index);
            return actor_update_melee_combat_action(actor_index);
        }
    } else if (mode == 8) {
        if (self->mode_data.raw[0] == 0) {
            return 0;
        }
        actor_set_target_alert_stage3(self->target_unit_index, actor_index);
        return actor_update_melee_combat_action(actor_index);
    } else {
        return 0;
    }
    return actor_update_melee_combat_action(actor_index);
}

#if 0
Original Ghidra decompilation (0x40d820):

uint FUN_0040d820(void)

{
  short sVar1;
  uint in_EAX;
  uint uVar2;
  int iVar3;

  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar1 = *(short *)(iVar3 + 0x6c);
  uVar2 = DAT_00880360 & 0xffffff00;
  if (sVar1 == 5) {
    if (*(char *)(iVar3 + 0x9d) == '\0') {
      return uVar2;
    }
    if (*(short *)(iVar3 + 0xa4) == 0) {
      FUN_0041fb00();
    }
  }
  else {
    if (sVar1 != 7) {
      if (sVar1 != 8) {
        return uVar2;
      }
      if (*(char *)(iVar3 + 0x9c) == '\0') {
        return uVar2;
      }
      FUN_0041fbc0();
      uVar2 = FUN_0040cdf0();
      return uVar2;
    }
    if (*(char *)(iVar3 + 0x9c) == '\0') {
      return uVar2;
    }
    if (*(short *)(iVar3 + 0xa4) == 0) {
      FUN_0041fb60();
      uVar2 = FUN_0040cdf0();
      return uVar2;
    }
  }
  uVar2 = FUN_0040cdf0();
  return uVar2;
}
#endif
