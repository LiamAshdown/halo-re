// actor_should_hold_position  (Ghidra: actor_should_hold_position, renamed)
// address 0x4105c0, size 237 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: phase-4 summary "decides whether the actor should currently hold its position,
// forcing a flee for dangerous-weapon threats"; prop.kind 4/5 (the "shared/vault" kinds
// per types/ai.h) forces actor.unknown_3bc (a flag also read/cleared in
// actor_update_melee_combat_action, 0x40cdf0) and clears the movement-pause timer.
// register convention: actor_index in EAX (Ghidra's in_EAX).
// UNSURE: the final branch reseeds the RNG and immediately truncates a float with __ftol,
// but no float computation is visible anywhere in the decompiled C before that truncate --
// this is the same "hidden FPU value" pattern as elsewhere in this module, and the value
// used below (0.0f) is a placeholder, not a reconstruction. Needs the disassembly review
// pass to find the real source (almost certainly `(seed >> 16) * 1.5259022e-05f` scaled by
// some duration constant, per the idiom in src/math/periodic_function_build_noise_table.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern uint32_t random_seed_global; // 0x00719cd0

// blam-cc: EAX -> actor_index, EDX -> definition
// FIXED (objdump 0x41064d..0x41069b): EDX is the actor definition; the hold timer +0x5f4 is
//   trunc(((def +0x84 - def +0x80) * random + def +0x80) * 30). The draft stored 0.
uint8_t actor_should_hold_position(datum_index actor_index, uint8_t *definition)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->unknown_60c == 1) {
        prop *p = (prop *)((uint8_t *)prop_data->data + (self->unknown_610 & 0xffff) * sizeof(prop));
        if (3 < p->kind && p->kind < 6) {
            self->unknown_3bc = 1;
            self->unknown_5f4 = 0;
            return 0;
        }
    }

    if (self->unknown_457 != 0) {
        self->unknown_5f4 = 0;
        return self->unknown_457 == 0;
    }

    {
        float lo = *(float *)(definition + 0x80);
        float hi = *(float *)(definition + 0x84);
        float r;

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        r = (float)(int32_t)(random_seed_global >> 16) * (1.0f / 65536.0f);
        self->unknown_5f4 = (int16_t)(int32_t)(((hi - lo) * r + lo) * 30.0f); // __ftol
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4105c0):

bool FUN_004105c0(void)

{
  short sVar1;
  undefined2 uVar2;
  uint in_EAX;
  int iVar3;

  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(short *)(iVar3 + 0x60c) == 1) {
    sVar1 = *(short *)((*(uint *)(iVar3 + 0x610) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34) +
                      0x24);
    if ((3 < sVar1) && (sVar1 < 6)) {
      *(undefined1 *)(iVar3 + 0x3bc) = 1;
      *(undefined2 *)(iVar3 + 0x5f4) = 0;
      return false;
    }
  }
  if (*(char *)(iVar3 + 0x457) != '\0') {
    *(undefined2 *)(iVar3 + 0x5f4) = 0;
    return *(char *)(iVar3 + 0x457) == '\0';
  }
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  uVar2 = __ftol();
  *(undefined2 *)(iVar3 + 0x5f4) = uVar2;
  return true;
}
#endif
