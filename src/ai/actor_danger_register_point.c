// actor_danger_register_point  (Ghidra: actor_danger_register_point; named from out/phase2/results/ai_02.json)
// address 0x41ec90, size 259 bytes
// name confidence: 0.45   rewrite confidence: 0.35
// evidence: out/phase2/results/ai_02.json -- compares an incoming danger radius (param_1) and
//   distance (param_2) against the actor's existing danger record priority (+0x2d4), and if it
//   wins, zeroes and refills the +0x280 danger block (danger_type=1) with the source object's
//   position/velocity. Matches actor.danger_type/danger_object_index/danger_unknown_2d4/etc.
//   and object.velocity in types/ai.h and types/objects.h.
// register convention: EAX -> actor_index, EDX -> source_object_index; radius, distance,
//   accept_flag and unknown_byte are Ghidra's recognized stack parameters.
//   // blam-cc: EAX -> actor_index, EDX -> source_object_index,
//   //   stack -> radius, distance, accept_flag, unknown_byte
//
// UNSURE: every early-return path in the original packs raw x87 FPU comparison-flag bits
// (C0/C2/C3) into the upper bytes of the return value via an incompletely resolved
// float-compare -- Ghidra's own decompilation never reads those bits back (the real path
// masks them off with `& 0xffffff00`, and the register register convention here is a byte
// return elsewhere in this trio). Only the low byte (0 = rejected, 1 = registered) is
// meaningful and is what this rewrite returns.
// UNSURE: Ghidra shows the position-fetch call with no visible arguments; the natural
// destination inside the just-cleared danger block is danger_center, which is what is written
// here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900

// blam-cc: EAX -> actor_index, EDX -> source_object_index,
//   stack -> radius, distance, accept_flag, unknown_byte
// Registers a point-based threat (e.g. an explosion or projectile) as the actor's current
// danger if it outranks the one already tracked. Returns 1 if registered, 0 if rejected either
// because the threat is out of range (radius+10 <= distance) or a higher-priority danger is
// already active.
uint8_t actor_danger_register_point(datum_index actor_index, datum_index source_object_index,
                                    float radius, float distance, char accept_flag, uint8_t unknown_byte)
{
    actor *self;
    object *source_obj;
    float threshold;
    int16_t existing_type;
    uint8_t should_register;
    uint32_t *clear;
    int32_t i;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    threshold = radius + 10.0f;

    if (threshold < distance || threshold == distance) {
        return 0;
    }

    existing_type = self->danger_type;
    should_register = 0;
    if (existing_type <= 0) {
        should_register = 1;
    } else if (existing_type == 1 && self->danger_object_index != source_object_index &&
               distance < self->danger_unknown_2d4) {
        should_register = 1;
    }

    if (should_register == 0) {
        return 0;
    }

    source_obj = ((object_header *)object_data->data)[source_object_index & 0xffff].data;

    clear = (uint32_t *)&self->danger_type;
    for (i = 0x1b; i != 0; i--) {
        *clear = 0;
        clear++;
    }

    self->danger_unknown_294 = radius;
    self->danger_type = 1;
    self->danger_object_index = source_object_index;

    object_get_position(&self->danger_center, source_object_index);

    self->danger_unknown_2a4 = *(uint32_t *)&source_obj->velocity.i;
    self->danger_unknown_2a8 = *(uint32_t *)&source_obj->velocity.j;
    self->danger_unknown_2ac = *(uint32_t *)&source_obj->velocity.k;
    self->danger_unknown_284 = 6;
    self->danger_unknown_286 = unknown_byte;
    self->danger_unknown_282 = (int16_t)(accept_flag == 0);
    return 1;
}

#if 0
Original Ghidra decompilation (0x41ec90):

uint FUN_0041ec90(float param_1,float param_2,char param_3,undefined1 param_4)

{
  float fVar1;
  short sVar2;
  int iVar3;
  undefined4 uVar4;
  uint in_EAX;
  undefined2 uVar6;
  uint uVar5;
  int iVar7;
  uint in_EDX;
  int iVar8;
  undefined4 *puVar9;

  fVar1 = param_1 + 10.0;
  uVar6 = (undefined2)((uint)DAT_00880360 >> 0x10);
  iVar8 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (fVar1 < param_2 || (fVar1 == param_2) != 0) {
    return CONCAT22(uVar6,(ushort)(fVar1 < param_2) << 8 |
                          (ushort)(NAN(fVar1) || NAN(param_2)) << 10 |
                          (ushort)(fVar1 == param_2) << 0xe);
  }
  sVar2 = *(short *)(iVar8 + 0x280);
  uVar5 = CONCAT22(uVar6,sVar2);
  if (0 < sVar2) {
    if ((sVar2 == 1) && (*(uint *)(iVar8 + 0x28c) != in_EDX)) {
      fVar1 = *(float *)(iVar8 + 0x2d4);
      uVar5 = CONCAT22(uVar6,(ushort)(param_2 < fVar1) << 8 |
                             (ushort)(NAN(param_2) || NAN(fVar1)) << 10 |
                             (ushort)(param_2 == fVar1) << 0xe);
      if (param_2 < fVar1) goto LAB_0041ecff;
    }
    return uVar5 & 0xffffff00;
  }
LAB_0041ecff:
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
  puVar9 = (undefined4 *)(iVar8 + 0x280);
  for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar9 = 0;
    puVar9 = puVar9 + 1;
  }
  *(float *)(iVar8 + 0x294) = param_1;
  *(undefined2 *)(iVar8 + 0x280) = 1;
  *(uint *)(iVar8 + 0x28c) = in_EDX;
  object_get_position();
  *(undefined4 *)(iVar8 + 0x2a4) = *(undefined4 *)(iVar3 + 0x68);
  *(undefined4 *)(iVar8 + 0x2a8) = *(undefined4 *)(iVar3 + 0x6c);
  uVar4 = *(undefined4 *)(iVar3 + 0x70);
  *(undefined4 *)(iVar8 + 0x2ac) = uVar4;
  *(undefined2 *)(iVar8 + 0x284) = 6;
  *(undefined1 *)(iVar8 + 0x286) = param_4;
  *(ushort *)(iVar8 + 0x282) = (ushort)(param_3 == '\0');
  return CONCAT31((int3)((uint)uVar4 >> 8),1);
}
#endif
