// actor_queue_search_position  (Ghidra: actor_queue_search_position; named from out/phase2/results/ai_02.json)
// address 0x421af0, size 199 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x421af0..0x421bb6.)
// evidence: out/phase2/results/ai_02.json -- only updates when the actor is not already deep
//   in combat (awareness_level<3) and the new priority is >= the stored one
//   (actor.search_priority), then records up to two optional position/velocity vectors plus
//   several scalar fields, all named in types/ai.h's actor struct (search_unknown_318.. through
//   search_unknown_348). Called throughout the module with a constant duration of 90 ticks
//   whenever a new point of interest is noticed.
// register convention: EAX -> actor_index, ECX -> position (nullable), DX -> priority, ESI ->
//   velocity (nullable, unaff_ESI); param_1..param_6 are Ghidra's recognized stack parameters.
//   // blam-cc: EAX -> actor_index, ECX -> position, EDX -> priority, ESI -> velocity,
//   //   stack -> unknown_324, unknown_328, unknown_33c, unknown_340, unknown_344, unknown_348

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index, ECX -> position, EDX -> priority, ESI -> velocity,
//   stack -> unknown_324, unknown_328, unknown_33c, unknown_340, unknown_344, unknown_348
// Records a candidate 'investigate/search' position for the actor if its priority is at least
// as high as any currently queued one, replacing the stored position and/or velocity (each
// optional) and the caller-supplied scalar fields.
void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority,
                                 real_vector3d *velocity, uint32_t unknown_324, uint32_t unknown_328,
                                 uint32_t unknown_33c, uint32_t unknown_340, uint32_t unknown_344,
                                 uint8_t unknown_348)
{
    actor *self;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->awareness_level < 3 && self->search_priority <= priority) {
        self->search_priority = priority;

        if (position == (real_point3d *)0) {
            self->search_unknown_314 = 0;
        } else {
            self->search_unknown_314 = 1;
            self->search_unknown_318 = ((uint32_t *)position)[0];
            self->search_unknown_31c = ((uint32_t *)position)[1];
            self->search_unknown_320 = ((uint32_t *)position)[2];
            self->search_unknown_324 = unknown_324;
            self->search_unknown_328 = unknown_328;
        }

        if (velocity == (real_vector3d *)0) {
            self->search_unknown_32c = 0;
        } else {
            self->search_unknown_32c = 1;
            self->search_unknown_330 = ((uint32_t *)velocity)[0];
            *(uint32_t *)&self->unknown_334 = ((uint32_t *)velocity)[1];
            self->search_unknown_338 = ((uint32_t *)velocity)[2];
        }

        self->search_unknown_340 = unknown_340;
        self->search_unknown_344 = unknown_344;
        self->search_unknown_348 = unknown_348;
        self->search_unknown_33c = unknown_33c;
    }
}

#if 0
Original Ghidra decompilation (0x421af0):

void FUN_00421af0(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined1 param_6)

{
  uint in_EAX;
  int iVar1;
  undefined4 *in_ECX;
  short in_DX;
  undefined4 *unaff_ESI;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((*(short *)(iVar1 + 0x6a) < 3) && (*(short *)(iVar1 + 0x312) <= in_DX)) {
    *(short *)(iVar1 + 0x312) = in_DX;
    if (in_ECX == (undefined4 *)0x0) {
      *(undefined1 *)(iVar1 + 0x314) = 0;
    }
    else {
      *(undefined1 *)(iVar1 + 0x314) = 1;
      *(undefined4 *)(iVar1 + 0x318) = *in_ECX;
      *(undefined4 *)(iVar1 + 0x31c) = in_ECX[1];
      *(undefined4 *)(iVar1 + 800) = in_ECX[2];
      *(undefined4 *)(iVar1 + 0x324) = param_1;
      *(undefined4 *)(iVar1 + 0x328) = param_2;
    }
    if (unaff_ESI == (undefined4 *)0x0) {
      *(undefined1 *)(iVar1 + 0x32c) = 0;
    }
    else {
      *(undefined1 *)(iVar1 + 0x32c) = 1;
      *(undefined4 *)(iVar1 + 0x330) = *unaff_ESI;
      *(undefined4 *)(iVar1 + 0x334) = unaff_ESI[1];
      *(undefined4 *)(iVar1 + 0x338) = unaff_ESI[2];
    }
    *(undefined4 *)(iVar1 + 0x340) = param_4;
    *(undefined4 *)(iVar1 + 0x344) = param_5;
    *(undefined1 *)(iVar1 + 0x348) = param_6;
    *(undefined4 *)(iVar1 + 0x33c) = param_3;
  }
  return;
}
#endif
