// unit_update_marker_skid_effects  (Ghidra: FUN_00575460; renamed from the phase2 proposal)
// address 0x575460, size 475 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary)
// rewrite confidence: 0.25
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

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void material_effects_play_at_marker(int32_t material_hint, int32_t index, int32_t *location_leaf_index,
                          float intensity); // 0x453490, UNSURE signature
extern double sqrt(double x);

// Triggers skid/spark effects at each fast-moving (> 0.03) ground-contact marker of the vehicle,
// scaled by speed, using Vehicle.effect as the gate (only runs when that tag reference is
// valid).
void unit_update_marker_skid_effects(uint32_t unit_index, uint8_t *contact_points)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;

    if (*(int32_t *)((uint8_t *)tag + 0x3dc) == -1) {
        return;
    }

    {
        uint8_t *physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;
        int32_t count = *(int32_t *)(physics_tag + 0x74);
        int32_t i;

        for (i = 0; i < count; i++) {
            uint8_t *marker = contact_points + i * 0x130;

            if ((marker[0] & 2) != 0) {
                float vx = *(float *)(marker + 0x54);
                float vy = *(float *)(marker + 0x58);
                float vz = *(float *)(marker + 0x5c);
                double speed = sqrt((double)(vx * vx + vz * vz + vy * vy));

                if (speed > 0.03) {
                    float scaled = (float)((speed - 0.03) * 4.5454545);
                    float clamped = (scaled < 0.0f) ? 0.0f : (scaled > 1.0f ? 1.0f : scaled);
                    uint8_t *physics_node = *(uint8_t **)(physics_tag + 0x78) + i * 0x80;
                    int32_t material_hint = ((*(uint32_t *)(physics_node + 0x24) & 1) != 0) + 9;

                    material_effects_play_at_marker(material_hint, *(int16_t *)(marker + 0x70),
                                 &obj->location_leaf_index, clamped);
                }
            }
        }
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
