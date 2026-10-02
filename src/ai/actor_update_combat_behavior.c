// actor_update_combat_behavior  (Ghidra: actor_update_combat_behavior, renamed)
// address 0x40d610, size 374 bytes
// name confidence: 0.45   rewrite confidence: 0.9 (checked against objdump 0x40d610..0x40d785; +0xa4 is a word)
// evidence: phase-4 summary "runs the combat-behavior update appropriate to the actor's
// archetype, choosing between the state-transition check and the melee/combat decision
// logic"; switches on actor_mode_definitions[self->mode].combat_grade and dispatches to
// actor_evaluate_combat_state_transition (0x40c620, other half of this session) or
// actor_update_melee_combat_action (0x40cdf0, this module).
// register convention: actor_index in EDI (Ghidra's unaff_EDI), param_1/param_2 on the
// stack.
// blam-cc: EDI -> actor_index, stack -> param_1, param_2
// UNSURE: actor_evaluate_combat_state_transition and actor_update_melee_combat_action are
// called with zero visible arguments at every site in this function; almost certainly
// actor_index still held in EDI. Needs the disassembly review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;                            // 0x00880360
extern data_array *prop_data;                             // 0x008802c0
extern actor_mode_definition actor_mode_definitions[16];  // 0x00655254

extern char actor_evaluate_combat_state_transition(uint32_t actor_index); // 0x40c620, other half of this session
extern uint8_t actor_update_melee_combat_action(datum_index actor_index);       // 0x40cdf0, this module

// blam-cc: EDI -> actor_index, stack -> param_1, param_2
uint8_t actor_update_combat_behavior(datum_index actor_index, uint8_t param_1, uint8_t param_2)
{
    actor *self;
    uint8_t result;
    uint8_t use_param_1;
    prop *target_prop;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    result = 0;
    use_param_1 = 1;
    if (param_2 == 0) {
        use_param_1 = param_1;
    }

    switch (actor_mode_definitions[self->mode].combat_grade) {
    case 1:
    case 2:
        if (use_param_1 == 0) goto use_default;
        if (self->combat_status < 5) {
            if (self->combat_status < 2 && self->mode != 2 &&
                (self->combat_status != 0 || (self->stood_down == 0 && self->post_combat_action < 1))) {
                goto use_default;
            }
            goto call_melee_combat_action;
        }
        result = actor_evaluate_combat_state_transition(actor_index);
        break;
    case 3:
        if (use_param_1 == 0 || self->combat_status < 4) {
            if (1 < self->combat_status) {
                if (self->target_unit_index == (datum_index)k_datum_index_none) {
                    goto use_default;
                }
                target_prop = (prop *)((uint8_t *)prop_data->data + (self->target_unit_index & 0xffff) * sizeof(prop));
                if (self->target_unit_index == self->pursuit_target_prop_index &&
                    (target_prop->noticed_a != 0 || (self->mode == 5 && *(int16_t *)(self->mode_data.raw + 8) == 0)) &&
                    (target_prop->noticed_b != 0 ||
                     ((self->mode == 5 && *(int16_t *)(self->mode_data.raw + 8) == 0) ||
                      (self->mode == 7 && *(int16_t *)(self->mode_data.raw + 8) == 0)))) {
                    goto use_default;
                }
            }
        call_melee_combat_action:
            result = actor_update_melee_combat_action(actor_index);
        } else {
            result = actor_evaluate_combat_state_transition(actor_index);
        }
        break;
    case 4:
        if (self->combat_status < 4) goto call_melee_combat_action;
        result = actor_evaluate_combat_state_transition(actor_index);
        break;
    default:
        goto use_default;
    }
    if (result != 0) {
        return result;
    }
use_default:
    if (param_2 != 0) {
        result = actor_update_melee_combat_action(actor_index);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x40d610):

uint FUN_0040d610(char param_1,char param_2)

{
  int iVar1;
  short sVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  char cVar6;
  uint unaff_EDI;

  iVar1 = (unaff_EDI & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar3 = *(short *)(iVar1 + 0x6c) * 0x38;
  uVar4 = uVar3 & 0xffffff00;
  cVar6 = '\x01';
  if (param_2 == '\0') {
    cVar6 = param_1;
  }
  switch(*(undefined2 *)(&DAT_00655258 + uVar3)) {
  case 1:
  case 2:
    if (cVar6 == '\0') goto switchD_0040d655_default;
    sVar2 = *(short *)(iVar1 + 0x6e);
    if (sVar2 < 5) {
      if (((sVar2 < 2) && (*(short *)(iVar1 + 0x6c) != 2)) &&
         ((sVar2 != 0 || ((*(char *)(iVar1 + 0x1c8) == '\0' && (*(short *)(iVar1 + 0x1e4) < 1))))))
      goto switchD_0040d655_default;
      goto LAB_0040d767;
    }
    uVar4 = actor_evaluate_combat_state_transition();
    break;
  case 3:
    if ((cVar6 == '\0') || (*(short *)(iVar1 + 0x6e) < 4)) {
      if ((1 < *(short *)(iVar1 + 0x6e)) &&
         ((uVar3 = *(uint *)(iVar1 + 0x270), uVar3 == 0xffffffff ||
          (((iVar5 = (uVar3 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34),
            uVar3 == *(uint *)(iVar1 + 0x3c0) &&
            ((*(char *)(iVar5 + 0xb9) != '\0' ||
             ((*(short *)(iVar1 + 0x6c) == 5 && (*(short *)(iVar1 + 0xa4) == 0)))))) &&
           ((*(char *)(iVar5 + 0xba) != '\0' ||
            (((*(short *)(iVar1 + 0x6c) == 5 && (*(short *)(iVar1 + 0xa4) == 0)) ||
             ((*(short *)(iVar1 + 0x6c) == 7 && (*(short *)(iVar1 + 0xa4) == 0))))))))))))
      goto switchD_0040d655_default;
LAB_0040d767:
      uVar4 = FUN_0040cdf0();
    }
    else {
      uVar4 = actor_evaluate_combat_state_transition();
    }
    break;
  case 4:
    if (*(short *)(iVar1 + 0x6e) < 4) goto LAB_0040d767;
    uVar4 = actor_evaluate_combat_state_transition();
    break;
  default:
    goto switchD_0040d655_default;
  }
  if ((char)uVar4 != '\0') {
    return uVar4;
  }
switchD_0040d655_default:
  if (param_2 != '\0') {
    uVar4 = FUN_0040cdf0();
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
