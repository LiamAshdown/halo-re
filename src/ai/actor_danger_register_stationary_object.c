// actor_danger_register_stationary_object  (Ghidra: actor_danger_register_stationary_object; named from out/phase2/results/ai_02.json)
// address 0x41ea60, size 557 bytes
// name confidence: 0.45   rewrite confidence: 0.2
// evidence: out/phase2/results/ai_02.json -- checks an object's flag byte and object velocity,
//   then registers it into the actor's danger record (danger_type=3) if closer/newer than the
//   current one. Matches actor.danger_* fields and object.velocity in types/ai.h /
//   types/objects.h; the tag-data float at +4 is Object.bounding_radius (types/tags.h), which
//   fixes the earlier assumed byte layout.
// register convention: EAX is a caller-owned position block (Ghidra's in_EAX, saved into EDI
//   at 0x41ea75 and tested at 0x41eb0c); actor_index, object_index and the byte are genuine
//   stack parameters. When the EAX block is null the function builds its own with
//   actor_get_firing_positions.
//   // blam-cc: EAX -> reference, stack -> actor_index, object_index, unknown_byte
//
// UNSURE, substantially: careful bracket-tracing of the decompilation shows the registration
// path is taken when the object's squared velocity is *above* 0.0011111111 (roughly
// 0.033 units/tick), not below it -- the opposite of phase2's "near-zero velocity... landed
// object" framing. This rewrite follows the traced control flow exactly; the naming/summary
// may describe the wrong branch.
// UNSURE: the tag-data flag byte at tag+0x2f0 does not correspond to any field this batch's
// headers name (types/tags.h's Object is only 0x17c bytes; the concrete subtype -- Equipment,
// Garbage, a Weapon -- is not established, and 0x2f0 does not land on an obviously-named field
// in the Item-derived candidates checked). Left as a raw offset from the tag data pointer.
// UNSURE: the value stored into actor.danger_unknown_290 is read from the object at byte
// offset 0x324 -- the same offset types/units.h names unit_data.driver_unit_index -- but this
// object is not established to be a unit; kept as a raw offset, not that field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"

// sqrt/fabs are single x87 instructions (FSQRT/FABS) in the original code, which
// Ghidra renders as the pseudo-functions SQRT()/ABS(); declared locally instead of via
// <math.h> because -I types shadows that header name with types/math.h.
extern double sqrt(double x); // FSQRT
static float sqrt_f(float x) { return (float)sqrt((double)x); }

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900
extern void actor_get_firing_positions(datum_index actor_index, uint32_t *out_block, real_point3d *query_point); // 0x41c1e0, this batch
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX

// blam-cc: stack -> actor_index, object_index, unknown_byte
// Detects a nearby object whose velocity is above a small threshold (see UNSURE above) and
// records it as the actor's current danger if more urgent than what's already tracked. Returns
// 1 if registered, 0 if rejected.
uint8_t actor_danger_register_stationary_object(const float *reference, datum_index actor_index,
                                                datum_index object_index, uint8_t unknown_byte)
{
    actor *self;
    object *obj;
    uint8_t *tag_data;
    float bounding_radius;
    float velocity_sq;
    real_point3d fetched_position;
    uint32_t local_positions[14];
    const float *block;
    float px, py, pz;
    float dx, dy, dz;
    float distance;
    float threshold_distance;
    int16_t existing_type;
    uint32_t *clear;
    int32_t i;
    int32_t driver_field;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->active_unit_index != k_datum_index_none) {
        return 0;
    }

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    tag_data = (uint8_t *)tag_instances[obj->definition_tag & 0xffff].data;

    if ((int8_t)tag_data[0x2f0] < 0) {
        velocity_sq = obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                      obj->velocity.i * obj->velocity.i;

        if (velocity_sq > 0.0011111111f) {
            object_get_position(&fetched_position, object_index);
            px = fetched_position.x;
            py = fetched_position.y;
            pz = fetched_position.z;

            // FIXED: the EAX block is only rebuilt when the caller did not supply one
            // (objdump 0x41eb0c: `test edi,edi / jne`). The first rewrite always rebuilt it.
            block = reference;
            if (block == (const float *)0) {
                actor_get_firing_positions(actor_index, local_positions, &fetched_position);
                block = (const float *)local_positions;
            }

            // FIXED: the coordinate pairing was crossed. objdump 0x41eb23 subtracts
            // block+0xc from position.x, block+0x10 from position.y and block+0x14 from
            // position.z; the first rewrite paired x with +0x10 and y with +0xc.
            dx = px - *(float *)((const uint8_t *)block + 0xc);
            dy = py - *(float *)((const uint8_t *)block + 0x10);
            dz = pz - *(float *)((const uint8_t *)block + 0x14);
            distance = sqrt_f(dx * dx + dy * dy + dz * dz);

            bounding_radius = *(float *)(tag_data + 4);
            threshold_distance = bounding_radius + 10.0f;

            if (threshold_distance <= distance) {
                return 0;
            }

            existing_type = self->danger_type;
            if (existing_type < 3 ||
                (existing_type == 3 && self->danger_object_index != object_index &&
                 distance < self->danger_unknown_2d4)) {
                clear = (uint32_t *)&self->danger_type;
                for (i = 0x1b; i != 0; i--) {
                    *clear = 0;
                    clear++;
                }

                self->danger_type = 3;
                self->danger_object_index = object_index;
                driver_field = *(int32_t *)((uint8_t *)obj + 0x324);
                self->danger_unknown_290 = driver_field;
                self->danger_unknown_294 = bounding_radius;
                // FIXED: 0x298 takes Ghidra's local_44 (== position.x at [esp+0x10]) and
                // 0x29c takes local_40 (== position.y); the first rewrite swapped them.
                self->danger_unknown_298 = px;
                self->danger_unknown_29c = py;
                self->danger_unknown_2a0 = pz;
                self->danger_unknown_2a4 = *(uint32_t *)&obj->velocity.i;
                self->danger_unknown_2a8 = *(uint32_t *)&obj->velocity.j;
                self->danger_unknown_286 = unknown_byte;
                self->danger_unknown_2ac = *(uint32_t *)&obj->velocity.k;
                self->danger_unknown_284 = 0x14;
                self->danger_unknown_282 = 0;

                if (driver_field != -1) {
                    // 0x41ec4a: CX = the driver's team, DX = the actor's
                    if (teams_are_enemies(*(int16_t *)((uint8_t *)((object_header *)object_data->data)[driver_field & 0xffff].data + 0xb8),
                                          *(int16_t *)((uint8_t *)self + 0x3e)) == 0) {
                        self->danger_unknown_282 = 1;
                    }
                }
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x41ea60):

uint FUN_0041ea60(uint param_1,uint param_2,undefined1 param_3)

{
  float fVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  float fVar6;
  char extraout_AL;
  undefined1 *in_EAX;
  int iVar7;
  uint uVar8;
  undefined2 extraout_var_00;
  undefined2 extraout_var_01;
  undefined2 uVar10;
  undefined3 uVar9;
  undefined3 extraout_var;
  int iVar11;
  undefined4 *puVar12;
  float local_44;
  float local_40;
  float local_3c;
  undefined1 local_38 [56];

  iVar11 = (param_1 & 0xffff) * 0x724;
  uVar8 = *(uint *)(iVar11 + 0x158 + *(int *)(DAT_00880360 + 0x34));
  iVar11 = iVar11 + *(int *)(DAT_00880360 + 0x34);
  if (uVar8 != 0xffffffff) {
    return uVar8 & 0xffffff00;
  }
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  iVar7 = (*puVar3 & 0xffff) * 0x20;
  iVar4 = *(int *)(iVar7 + 0x14 + DAT_0087bc14);
  uVar8 = CONCAT31((int3)((uint)iVar7 >> 8),*(char *)(iVar4 + 0x2f0));
  if (*(char *)(iVar4 + 0x2f0) < '\0') {
    fVar6 = (float)puVar3[0x1c] * (float)puVar3[0x1c] +
            (float)puVar3[0x1b] * (float)puVar3[0x1b] + (float)puVar3[0x1a] * (float)puVar3[0x1a];
    uVar8 = CONCAT22((short)((uint)iVar7 >> 0x10),
                     (ushort)(fVar6 < 0.0011111111) << 8 | (ushort)NAN(fVar6) << 10 |
                     (ushort)(fVar6 == 0.0011111111) << 0xe);
    if (fVar6 < 0.0011111111 == 0 && (fVar6 == 0.0011111111) == 0) {
      object_get_position();
      uVar10 = extraout_var_00;
      if (in_EAX == (undefined1 *)0x0) {
        actor_get_firing_positions();
        in_EAX = local_38;
        uVar10 = extraout_var_01;
      }
      fVar6 = SQRT((local_40 - *(float *)(in_EAX + 0x10)) * (local_40 - *(float *)(in_EAX + 0x10)) +
                   (local_44 - *(float *)(in_EAX + 0xc)) * (local_44 - *(float *)(in_EAX + 0xc)) +
                   (local_3c - *(float *)(in_EAX + 0x14)) * (local_3c - *(float *)(in_EAX + 0x14)));
      fVar1 = *(float *)(iVar4 + 4) + 10.0;
      uVar8 = CONCAT22(uVar10,(ushort)(fVar1 < fVar6) << 8 |
                              (ushort)(NAN(fVar1) || NAN(fVar6)) << 10 |
                              (ushort)(fVar1 == fVar6) << 0xe);
      if (fVar1 >= fVar6 && (fVar1 == fVar6) == 0) {
        sVar2 = *(short *)(iVar11 + 0x280);
        uVar8 = CONCAT22(uVar10,sVar2);
        if (sVar2 < 3) {
LAB_0041ebaa:
          puVar12 = (undefined4 *)(iVar11 + 0x280);
          for (iVar7 = 0x1b; iVar7 != 0; iVar7 = iVar7 + -1) {
            *puVar12 = 0;
            puVar12 = puVar12 + 1;
          }
          *(undefined2 *)(iVar11 + 0x280) = 3;
          *(uint *)(iVar11 + 0x28c) = param_2;
          *(uint *)(iVar11 + 0x290) = puVar3[0xc9];
          *(undefined4 *)(iVar11 + 0x294) = *(undefined4 *)(iVar4 + 4);
          *(float *)(iVar11 + 0x298) = local_44;
          *(float *)(iVar11 + 0x29c) = local_40;
          *(float *)(iVar11 + 0x2a0) = local_3c;
          *(uint *)(iVar11 + 0x2a4) = puVar3[0x1a];
          uVar8 = puVar3[0x1b];
          *(uint *)(iVar11 + 0x2a8) = uVar8;
          uVar5 = puVar3[0x1c];
          uVar9 = (undefined3)(uVar8 >> 8);
          *(undefined1 *)(iVar11 + 0x286) = param_3;
          *(uint *)(iVar11 + 0x2ac) = uVar5;
          *(undefined2 *)(iVar11 + 0x284) = 0x14;
          *(undefined2 *)(iVar11 + 0x282) = 0;
          if (*(int *)(iVar11 + 0x290) != -1) {
            FUN_0045bd50();
            uVar9 = extraout_var;
            if (extraout_AL == '\0') {
              *(undefined2 *)(iVar11 + 0x282) = 1;
            }
          }
          return CONCAT31(uVar9,1);
        }
        if ((sVar2 == 3) && (*(uint *)(iVar11 + 0x28c) != param_2)) {
          fVar1 = *(float *)(iVar11 + 0x2d4);
          uVar8 = CONCAT22(uVar10,(ushort)(fVar6 < fVar1) << 8 |
                                  (ushort)(NAN(fVar6) || NAN(fVar1)) << 10 |
                                  (ushort)(fVar6 == fVar1) << 0xe);
          if (fVar6 < fVar1) goto LAB_0041ebaa;
        }
      }
      return uVar8 & 0xffffff00;
    }
  }
  return uVar8 & 0xffffff00;
}
#endif
