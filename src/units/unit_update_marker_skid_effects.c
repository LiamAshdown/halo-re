// unit_update_marker_skid_effects  (Ghidra: FUN_00575460; renamed from the phase2 proposal)
// address 0x575460, size 475 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary)
// rewrite confidence: 0.85 (REWRITTEN from objdump 0x575460..0x57563a)
// evidence: types/tags.h Vehicle.effect (tag_id at absolute 0x3dc); types/objects.h
//   object.location_leaf_index (0x098); the physics.tag_id-at-0x8c idiom (contact-point count
//   at Physics+0x74, per-node record base at Physics+0x78, stride 0x80, matching the sibling
//   unit_update_marker_traction_effects.c).
// register convention: unit object index in EAX (in_EAX); a contact-point array pointer as the
//   stack parameter (param_1, stride 0x130, matching unit_update_ground_contact_counter.c and
//   unit_update_steering_deviation_effects.c).
//   // blam-cc: EAX -> unit_index, stack -> contact_points
// UNSURE: the contact-point record's fields at +0x54/+0x58/+0x5c (a local velocity vector) and
//   +0x70 (an index passed on to material_effects_play_at_marker) are not named anywhere in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type,
    int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param,
    real_point3d *position, real_vector3d *offset); // 0x453490, EAX, stack x4, EDX, EDI
extern double sqrt(double x);

// REWRITTEN from objdump. For each contact point (stride 0x130) with flag bit 1 whose velocity (+0x54..+0x5c)
//   is faster than 0.03: intensity = clamp((speed - 0.03) * 4.5454545, 0, 1); position = contact +0x04 + normal
//   (+0x60) * (contact +0x74 - physics node +0x68 + 0.003); offset = velocity * (0.8660254 / speed) + normal * 0.5;
//   then material_effects_play_at_marker(tag +0x3dc, 9 + (node +0x24 & 1), contact +0x70, &obj +0x98, intensity,
//   &position, &offset). The draft passed the effect-type index as the tag and left EDX/EDI unset.
// blam-cc: EAX -> unit_index, stack -> contact_points
void unit_update_marker_skid_effects(uint32_t unit_index, uint8_t *contact_points)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *physics_tag;
    int32_t count;
    int16_t i;

    if (*(int32_t *)(tag + 0x3dc) == -1) {
        return;
    }
    physics_tag = (uint8_t *)tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    count = *(int32_t *)(physics_tag + 0x74);
    for (i = 0; (int32_t)i < count; i++) {
        uint8_t *contact = contact_points + (int32_t)i * 0x130;
        uint8_t *node = *(uint8_t **)(physics_tag + 0x78) + (int32_t)i * 0x80;
        real_vector3d *velocity = (real_vector3d *)(contact + 0x54);
        real_vector3d *normal = (real_vector3d *)(contact + 0x60);
        real_point3d *point = (real_point3d *)(contact + 0x04);
        real_point3d position;
        real_vector3d offset;
        real speed, scaled, depth, k;
        uint32_t intensity_bits;

        if ((contact[0] & 2) == 0) {
            continue;
        }
        speed = (real)sqrt((double)(velocity->k * velocity->k + velocity->j * velocity->j +
            velocity->i * velocity->i));
        if (!(speed > 0.03f)) {
            continue;
        }
        scaled = (speed - 0.03f) * 4.5454545f;
        depth = *(real *)(contact + 0x74) - *(real *)(node + 0x68) + 0.003f;
        position.x = depth * normal->i + point->x;
        position.y = depth * normal->j + point->y;
        position.z = depth * normal->k + point->z;
        k = 0.8660254f / speed;
        offset.i = normal->i * 0.5f + k * velocity->i;
        offset.j = normal->j * 0.5f + k * velocity->j;
        offset.k = normal->k * 0.5f + k * velocity->k;
        if (!(scaled >= 0.0f)) {
            scaled = 0.0f;
        } else if (!(scaled <= 1.0f)) {
            scaled = 1.0f;
        }
        intensity_bits = *(uint32_t *)&scaled;
        material_effects_play_at_marker(*(uint32_t *)(tag + 0x3dc), (int16_t)(9 + (*(uint32_t *)(node + 0x24) & 1)),
            *(int16_t *)(contact + 0x70), (uint32_t *)(obj + 0x98), intensity_bits, &position, &offset);
    }
}

#if 0
Original Ghidra decompilation (0x575460):

void FUN_00575460(int param_1)

{
  uint *puVar1;
  int iVar2;
  float fVar3;
  short sVar4;
  uint in_EAX;
  int iVar5;
  byte *pbVar6;
  float local_20;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar5 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)((*(uint *)(iVar5 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(iVar5 + 0x3dc) != -1) {
    iVar5 = 0;
    sVar4 = 0;
    if (0 < *(int *)(iVar2 + 0x74)) {
      do {
        pbVar6 = (byte *)(iVar5 * 0x130 + param_1);
        if (((*pbVar6 & 2) != 0) &&
           (fVar3 = SQRT(*(float *)(pbVar6 + 0x54) * *(float *)(pbVar6 + 0x54) +
                         *(float *)(pbVar6 + 0x5c) * *(float *)(pbVar6 + 0x5c) +
                         *(float *)(pbVar6 + 0x58) * *(float *)(pbVar6 + 0x58)), 0.03 < fVar3)) {
          local_20 = (fVar3 - 0.03) * 4.5454545;
          if (0.0 <= local_20) {
            if (1.0 < local_20) {
              local_20 = 1.0;
            }
          }
          else {
            local_20 = 0.0;
          }
          FUN_00453490(((*(uint *)(iVar5 * 0x80 + *(int *)(iVar2 + 0x78) + 0x24) & 1) != 0) + '\t',
                       (int)*(short *)(pbVar6 + 0x70),puVar1 + 0x26,local_20);
        }
        sVar4 = sVar4 + 1;
        iVar5 = (int)sVar4;
      } while (iVar5 < *(int *)(iVar2 + 0x74));
    }
  }
  return;
}
#endif
