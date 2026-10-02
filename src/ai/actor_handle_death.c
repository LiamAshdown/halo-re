// actor_handle_death  (Ghidra: actor_handle_death, renamed)
// address 0x40dd50, size 198 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against 0x40dd50 (order record layout, EBX target / order registers, mode 4))
// evidence: phase-4 summary "one-time actor death handling that clears its current threat
// and transitions it into mode 4 (death handling)"; builds a scratch block, calls
// actor_consider_target_candidate and actor_check_melee_target_reachable ("clears the current threat"), then
// commits mode 4 (_actor_mode_death) with the block as mode_data.
// register convention: actor_index in EAX (param_1), param_2/param_3 recognized as normal
// byte parameters by Ghidra (stack).
// UNSURE: the 0x30-byte local block is zeroed then has bytes written at +4, +5, +8 (as
// int16 -1) and +0x1c (the actor's previous target_unit_index); its first few bytes line
// up with actor_order's target_index/parameter fields but the target_unit_index write at
// +0x1c does not fit that struct, so it is kept here as a raw local buffer rather than
// asserting it is an actor_order.
// UNSURE: after actor_check_melee_target_reachable(actor_index) is called, the code re-tests the int16 at +8 of
// the local block (already set to -1 immediately before), which only makes sense if
// actor_consider_target_candidate and/or actor_check_melee_target_reachable take a hidden pointer to this block
// in addition to actor_index. Declared here taking that pointer as a second parameter;
// needs the disassembly review pass to confirm which register carries it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

extern uint16_t actor_consider_target_candidate(datum_index actor_index,
                                                datum_index candidate_prop_index); // 0x4208a0, this module;
// stack -> actor_index, EBX -> candidate_prop_index. The EBX value at 0x40ddcb is the same
// previous-target prop handle the code has just written to the order record at +0x2c.
extern void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order); // 0x403f00, stack, EBX order
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module

uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3)
{
    actor *self;
    uint8_t local_data[0x30];
    int32_t previous_target;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->order_committed != 0) {
        return 0;
    }

    previous_target = self->target_unit_index;

    for (uint32_t i = 0; i < sizeof(local_data) / 4; i++) {
        ((uint32_t *)local_data)[i] = 0;
    }
    *(int16_t *)(local_data + 0xc) = 0;      // local_84, redundant with the zero loop
    *(int16_t *)(local_data + 0) = 0;        // local_90._0_2_, redundant with the zero loop
    *(int16_t *)(local_data + 8) = -1;       // local_88
    local_data[4] = param_2;                 // local_8c
    local_data[5] = param_3;                 // local_8b
    *(int32_t *)(local_data + 0x1c) = previous_target; // local_74

    if (previous_target != -1) {
        actor_consider_target_candidate(actor_index, (datum_index)previous_target);
    }
    if (self->swarm == 0) {
        actor_check_melee_target_reachable(actor_index, (int16_t *)local_data); // 0x40ddde: EBX = the local order
        if (*(int16_t *)(local_data + 8) != -1) {
            actor_set_mode(actor_index, 4, local_data);
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40dd50):

undefined4 FUN_0040dd50(uint param_1,undefined1 param_2,undefined1 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 local_90;
  undefined1 local_8c;
  undefined1 local_8b;
  short local_88;
  undefined2 local_84;
  int local_74;

  iVar3 = *(int *)(DAT_00880360 + 0x34) + (param_1 & 0xffff) * 0x724;
  if (*(char *)(iVar3 + 0x160) == '\0') {
    iVar1 = *(int *)(iVar3 + 0x270);
    puVar4 = &local_90;
    for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    local_84 = 0;
    local_90._0_2_ = 0;
    local_88 = -1;
    local_8c = param_2;
    local_8b = param_3;
    local_74 = iVar1;
    if (iVar1 != -1) {
      actor_consider_target_candidate(param_1);
    }
    if (*(char *)(iVar3 + 6) == '\0') {
      FUN_00403f00(param_1);
      if (local_88 != -1) {
        actor_set_mode(param_1,4,&local_90);
        return 1;
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
