// unit_update_steering_deviation_effects  (Ghidra: FUN_00574f30; renamed from the phase2
//   proposal)
// address 0x574f30, size 559 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary)
// rewrite confidence: 0.25
// evidence: types/objects.h object.velocity (0x068), .bounding_center (0x0a0); types/tags.h
//   Vehicle.material_effects (tag_id at absolute 0x3cc); the physics.tag_id-at-0x8c idiom
//   (contact-point count at Physics+0x74); damage_data field mapping follows the pattern
//   established in unit_cause_melee_damage.c, except here "origin" is left at its zero-fill
//   default and "direction" carries the velocity-deviation vector.
// register convention: unit object index in EAX (param_1); a reference direction pointer in
//   ECX (in_ECX); a contact-point array pointer in EAX (in_EAX, aliasing the object index
//   register at the point it is used, per Ghidra -- almost certainly wrong, see UNSURE).
//   // blam-cc: EAX -> unit_index, ECX -> reference_direction, ? -> contact_points
// UNSURE: in_EAX is used both as the unit object index (via param_1, a stack copy Ghidra
//   tracked separately) and, later, as a raw contact-point array base -- these cannot both be
//   the same register's value, so the array pointer is modeled as a separate parameter here.
// UNSURE: globals_tag_data+0x18c+0x48 (a second effect tag from the same "fall damage table"
//   block referenced by unit_apply_fall_damage.c) is not named in any header.
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *globals_tag_data;   // 0x00746fa0

extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t param_3,
                                 int16_t param_4, int16_t param_5, uint32_t param_6); // 0x4ee5e0
extern datum_index sound_start_at_object_marker(datum_index effect_index, void *position, float intensity,
                                 uint32_t flag); // 0x543ce0, UNSURE signature
extern double sqrt(double x);

// Applies a steering-deviation based damage/light-intensity effect while at least one of the
// unit's ground-contact markers is active.
void unit_update_steering_deviation_effects(uint32_t unit_index, real_vector3d *reference_direction,
                                             uint8_t *contact_points)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t *fall_table = *(uint8_t **)(globals_tag_data + 0x18c);
    int32_t impact_effect_tag = *(int32_t *)(fall_table + 0x48);

    if (impact_effect_tag == -1 && *(int32_t *)((uint8_t *)tag + 0x3cc) == -1) {
        return;
    }

    {
        real_vector3d deviation;
        double length;

        deviation.i = obj->velocity.i - reference_direction->i;
        deviation.j = obj->velocity.j - reference_direction->j;
        deviation.k = obj->velocity.k - reference_direction->k;
        length = sqrt((double)(deviation.i * deviation.i + deviation.j * deviation.j +
                                deviation.k * deviation.k));

        if (length > 0.02) {
            uint8_t *physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;
            int32_t count = *(int32_t *)(physics_tag + 0x74);
            int32_t i = 0;

            while ((contact_points[i * 0x130] & 2) == 0) {
                i++;
                if (i >= count) {
                    return;
                }
            }

            {
                float fraction = (float)((length - 0.02) * 45.454544);
                float clamped = (fraction < 0.0f) ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);

                if (impact_effect_tag != -1) {
                    damage_data dd = {0};
                    dd.team_index = -1;
                    dd.responsible_player = k_datum_index_none;
                    dd.responsible_object = k_datum_index_none;
                    *(int16_t *)&dd.location_cluster_index = -1;
                    dd.epicentre = obj->bounding_center;
                    dd.direction = deviation;
                    dd.random_blend = clamped;
                    dd.multiplier = 1.0f;
                    dd.material_type = -1;
                    dd.damage_effect_tag = impact_effect_tag;
                    object_apply_damage(&dd, unit_index, -1, -1, -1, 0);
                }

                if (*(int32_t *)((uint8_t *)tag + 0x3cc) != -1) {
                    sound_start_at_object_marker(*(int32_t *)((uint8_t *)tag + 0x3cc), (void *)0xffffffff, clamped, 0);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x574f30):

void FUN_00574f30(uint param_1)

{
  uint *puVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  int in_EAX;
  int iVar8;
  float *in_ECX;
  int iVar9;
  short sVar10;
  int *piVar11;
  float local_68;
  int local_58 [4];
  undefined2 local_48;
  undefined2 local_40;
  uint local_3c;
  uint local_38;
  uint local_34;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  undefined4 local_14;
  undefined2 local_c;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = *(int *)(*(int *)(DAT_00746fa0 + 0x18c) + 0x48);
  if ((iVar3 != -1) || (*(int *)(iVar2 + 0x3cc) != -1)) {
    fVar4 = (float)puVar1[0x1a] - *in_ECX;
    fVar5 = (float)puVar1[0x1b] - in_ECX[1];
    fVar6 = (float)puVar1[0x1c] - in_ECX[2];
    fVar7 = SQRT(fVar4 * fVar4 + fVar5 * fVar5 + fVar6 * fVar6);
    if (0.02 < fVar7) {
      iVar9 = *(int *)(*(int *)((*(uint *)(iVar2 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                      0x74);
      sVar10 = 0;
      if (0 < iVar9) {
        iVar8 = 0;
        while ((*(byte *)(iVar8 * 0x130 + in_EAX) & 2) == 0) {
          sVar10 = sVar10 + 1;
          iVar8 = (int)sVar10;
          if (iVar9 <= iVar8) {
            return;
          }
        }
        local_68 = (fVar7 - 0.02) * 45.454544;
        if (iVar3 != -1) {
          piVar11 = local_58;
          for (iVar9 = 0x15; iVar9 != 0; iVar9 = iVar9 + -1) {
            *piVar11 = 0;
            piVar11 = piVar11 + 1;
          }
          local_c = 0xffff;
          local_58[2] = 0xffffffff;
          local_58[3] = 0xffffffff;
          local_48 = 0xffff;
          local_40 = 0xffff;
          local_14 = 0x3f800000;
          if (0.0 <= local_68) {
            local_18 = local_68;
            if (1.0 < local_68) {
              local_18 = 1.0;
            }
          }
          else {
            local_18 = 0.0;
          }
          local_3c = puVar1[0x28];
          local_38 = puVar1[0x29];
          local_34 = puVar1[0x2a];
          local_58[0] = iVar3;
          local_24 = fVar4;
          local_20 = fVar5;
          local_1c = fVar6;
          object_apply_damage(local_58,param_1,0xffffffff,0xffffffff,0xffffffff,0);
        }
        if (*(int *)(iVar2 + 0x3cc) != -1) {
          if (0.0 <= local_68) {
            if (1.0 < local_68) {
              local_68 = 1.0;
            }
          }
          else {
            local_68 = 0.0;
          }
          FUN_00543ce0(*(int *)(iVar2 + 0x3cc),0xffffffff,local_68,0);
        }
      }
    }
  }
  return;
}
#endif
