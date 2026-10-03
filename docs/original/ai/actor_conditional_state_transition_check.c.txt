// actor_conditional_state_transition_check  (Ghidra: actor_conditional_state_transition_check, renamed)
// address 0x40d7a0, size 114 bytes
// name confidence: 0.45   rewrite confidence: 0.9 (checked against objdump 0x40d7a0..0x40d810)
// evidence: phase-4 summary "conditionally re-runs the combat state-transition check, but
// only while the actor is already in the relevant combat sub-state"; gated on
// mode == _actor_mode_vehicle (10) and a mode_data sub-state at +4.
// register convention: actor_index in ESI (Ghidra's unaff_ESI).
// blam-cc: ESI -> actor_index
// UNSURE: mode_data offsets 0x04, 0x07, 0x08, 0x29 (vehicle-mode data) are not otherwise
// named by types/ai.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern char actor_evaluate_combat_state_transition(uint32_t actor_index); // 0x40c620, other half of this session

// blam-cc: ESI -> actor_index
uint8_t actor_conditional_state_transition_check(datum_index actor_index)
{
    actor *self;
    int16_t sub_state;
    uint8_t flag;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->mode != _actor_mode_vehicle) {
        return 0;
    }
    sub_state = *(int16_t *)&self->mode_data.raw[4];
    if (sub_state == 2 || sub_state == 3) {
        if (self->mode_data.raw[7] != 0 || self->mode_data.raw[8] != 0) {
            return actor_evaluate_combat_state_transition(actor_index);
        }
        flag = self->mode_data.raw[0x29];
    } else {
        if (sub_state != 4 && sub_state != 5) {
            return 0;
        }
        flag = self->mode_data.raw[0x29];
    }
    if (flag == 0) {
        return 0;
    }
    return actor_evaluate_combat_state_transition(actor_index);
}

#if 0
Original Ghidra decompilation (0x40d7a0):

uint FUN_0040d7a0(void)

{
  char cVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  uint unaff_ESI;

  iVar4 = (unaff_ESI & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar3 = DAT_00880360 & 0xffffff00;
  if (*(short *)(iVar4 + 0x6c) != 10) {
    return uVar3;
  }
  sVar2 = *(short *)(iVar4 + 0xa0);
  if ((sVar2 == 2) || (sVar2 == 3)) {
    if ((*(char *)(iVar4 + 0xa3) != '\0') || (*(char *)(iVar4 + 0xa4) != '\0')) goto LAB_0040d7ea;
    cVar1 = *(char *)(iVar4 + 0xc5);
  }
  else {
    if ((sVar2 != 4) && (sVar2 != 5)) {
      return uVar3;
    }
    cVar1 = *(char *)(iVar4 + 0xc5);
  }
  if (cVar1 == '\0') {
    return uVar3;
  }
LAB_0040d7ea:
  uVar3 = actor_evaluate_combat_state_transition();
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
