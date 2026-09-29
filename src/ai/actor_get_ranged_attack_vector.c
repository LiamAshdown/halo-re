// actor_get_ranged_attack_vector  (Ghidra: actor_get_ranged_attack_vector; named from out/phase2/results/ai_02.json)
// address 0x420970, size 404 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x420970..0x420b03; offsets probed)
// evidence: out/phase2/results/ai_02.json -- for a vehicle unit (target prop.is_parented set)
//   it reads the controlled unit's aiming_vector (types/units.h, unit+0x23c) into the output
//   vector; otherwise, if the unit already has an owning actor (target.owner_actor_index !=
//   none), it defers to actor_get_cached_wander_position (0x4281f0, "writes one of two cached 3-float positions
//   into the output vector via EDX; returns whether a position was produced" per
//   out/phase4/ai_functions.md); when neither applies it instead scans the actor's perceived
//   ally list to test whether the output vector's direction would cross an ally within a
//   dot-product tolerance of 0.5, normalizing the ally offset through
//   vector3d_normalize_with_length.
// register convention: EAX -> target_prop_index, ECX -> actor_index; param_1 (real_vector3d*)
//   is Ghidra's recognized stack parameter, the output vector.
//   // blam-cc: EAX -> target_prop_index, ECX -> actor_index, stack -> out_vector
//
// UNSURE: the ally-scan dot product is computed against out_vector directly, even though it
// was just filled from unit.aiming_vector (a direction) on the branch that reaches this code --
// preserved exactly, matching out/phase4's own "output vector" framing.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *prop_data;   // 0x008802c0
extern data_array *object_data; // 0x008603b0

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990


// blam-cc: EAX -> target_prop_index, ECX -> actor_index, stack -> out_vector
// Produces an aim/attack direction for the actor's controlled unit (vehicle hardpoint or
// fallback path) and, in the fallback case, reports whether that line of fire is clear of
// nearby allies.
uint8_t actor_get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index, real_vector3d *out_vector)
{
    actor *self;
    prop *target;
    object *unit_obj;
    unit_data *unit;
    datum_index prop_index;
    prop *ally;
    real_vector3d delta;
    float length;
    float dot;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    if (target->has_parent != 0) {
        return 0;
    }

    if (target->is_parented == 0) {
        if (target->owner_actor_index != k_datum_index_none) {
            return actor_get_cached_wander_position(target->owner_actor_index, out_vector); // FIXED: EAX = prop +0x1c
        }
        return 0;
    }

    unit_obj = ((object_header *)object_data->data)[target->object_index & 0xffff].data;
    unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    *out_vector = unit->aiming_vector;

    if (target->unknown_12f == 0 && (int8_t)self->tally.unit_props > 0) {
        prop_index = self->first_prop;
        while (prop_index != k_datum_index_none) {
            ally = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
            prop_index = ally->next_in_actor;

            if ((1 < ally->kind && ally->kind < 4) && ally->is_unit != 0) {
                delta.i = ally->last_known_position.x - target->last_known_position.x;
                delta.j = ally->last_known_position.y - target->last_known_position.y;
                delta.k = ally->last_known_position.z - target->last_known_position.z;
                length = vector3d_normalize_with_length(&delta);

                if (length > 0.0f) {
                    dot = delta.i * out_vector->i + delta.j * out_vector->j + delta.k * out_vector->k;
                    if (dot > 0.5f) {
                        return 1;
                    }
                }
            }
        }
        return 0;
    }

    // UNSURE: this fallback returns whatever raw byte was in target.unknown_12f (cVar6 in the
    // original), not a fixed 0/1 -- preserved exactly rather than coerced to bool.
    return target->unknown_12f;
}

#if 0
Original Ghidra decompilation (0x420970):

uint FUN_00420970(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  char cVar6;
  short sVar7;
  int iVar8;
  float fVar9;
  float10 fVar10;
  int iVar11;
  uint in_EAX;
  uint uVar12;
  int iVar13;
  uint uVar14;
  undefined2 extraout_var;
  uint in_ECX;
  int iVar15;
  uint uVar16;
  float10 fVar17;

  iVar11 = DAT_00880360;
  iVar15 = (in_ECX & 0xffff) * 0x724;
  uVar12 = (in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  iVar13 = *(int *)(DAT_00880360 + 0x34);
  uVar14 = uVar12 & 0xffffff00;
  if (*(char *)(uVar12 + 0x14) != '\0') {
    return uVar14;
  }
  if (*(char *)(uVar12 + 0x12e) == '\0') {
    if (*(int *)(uVar12 + 0x1c) != -1) {
      uVar12 = FUN_004281f0();
      return uVar12;
    }
  }
  else {
    cVar6 = *(char *)(uVar12 + 0x12f);
    iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(uVar12 + 0x18) & 0xffff) * 0xc);
    *param_1 = *(float *)(iVar8 + 0x23c);
    param_1[1] = *(float *)(iVar8 + 0x240);
    fVar9 = *(float *)(iVar8 + 0x244);
    param_1[2] = fVar9;
    iVar8 = DAT_008802c0;
    uVar14 = CONCAT31((int3)((uint)fVar9 >> 8),cVar6);
    if ((cVar6 == '\0') && ('\0' < *(char *)(iVar13 + iVar15 + 0x1ec))) {
      uVar16 = *(uint *)(*(int *)(iVar11 + 0x34) + 0x50 + iVar15);
      while (uVar16 != 0xffffffff) {
        iVar13 = (uVar16 & 0xffff) * 0x138;
        sVar7 = *(short *)(iVar13 + 0x24 + *(int *)(iVar8 + 0x34));
        uVar14 = iVar13 + *(int *)(iVar8 + 0x34);
        uVar16 = *(uint *)(uVar14 + 8);
        if (((1 < sVar7) && (sVar7 < 4)) && (*(char *)(uVar14 + 0x60) != '\0')) {
          fVar9 = *(float *)(uVar14 + 0xbc);
          fVar1 = *(float *)(uVar12 + 0xbc);
          fVar2 = *(float *)(uVar14 + 0xc0);
          fVar3 = *(float *)(uVar12 + 0xc0);
          fVar4 = *(float *)(uVar14 + 0xc4);
          fVar5 = *(float *)(uVar12 + 0xc4);
          fVar17 = (float10)vector3d_normalize_with_length();
          fVar10 = (float10)0.0;
          uVar14 = CONCAT22(extraout_var,
                            (ushort)(fVar17 < fVar10) << 8 |
                            (ushort)(NAN(fVar17) || NAN(fVar10)) << 10 |
                            (ushort)(fVar17 == fVar10) << 0xe);
          if (fVar17 < fVar10 == 0 && (fVar17 == fVar10) == 0) {
            fVar9 = (fVar9 - fVar1) * *param_1 +
                    (fVar2 - fVar3) * param_1[1] + (fVar4 - fVar5) * param_1[2];
            uVar14 = CONCAT22(extraout_var,
                              (ushort)(fVar9 < 0.5) << 8 | (ushort)NAN(fVar9) << 10 |
                              (ushort)(fVar9 == 0.5) << 0xe);
            if (fVar9 < 0.5 == 0 && (fVar9 == 0.5) == 0) {
              return CONCAT31((int3)(uVar14 >> 8),1);
            }
          }
        }
      }
      uVar14 = uVar14 & 0xffffff00;
    }
  }
  return uVar14;
}
#endif
