// unit_update_marker_traction_effects  (Ghidra: FUN_00575170; renamed from the phase2 proposal)
// address 0x575170, size 750 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary)
// rewrite confidence: 0.15 -- the animation-graph node-array traversal (iVar6/local_18, its own
//   nested count+pointer sub-block at +0x68/+0x6c) and the per-physics-node record it indexes
//   into (physics_tag+0x78, stride 0x80) are not documented in any header available to this
//   module; reproduced with Ghidra's own locals rather than invented field names.
// evidence: types/units.h vehicle_data.contact_point_traction (0x4f4, "0x575170 reads and
//   rewrites entry i, 0xff meaning full traction"); types/objects.h object.position (0x05c);
//   types/tags.h Vehicle.suspension_sound (tag_id at absolute 0x3bc, per the module's
//   TagDependency-at-relative-+0xc idiom); the physics.tag_id-at-0x8c idiom (contact-point
//   count at Physics+0x74).
// UNSURE: essentially the whole per-node transform/hit-test block (matrix4x3_from_forward_up,
//   the two matrix4x3_transform_point/normal calls and collision_test_movement_segment) is register-resident with
//   no visible arguments, and is reproduced as literally as Ghidra's own locals allow.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern uint8_t lerp_find_threshold_byte(real lo, real hi, real threshold); // 0x4cf7a0, UNSURE signature
extern datum_index sound_start_at_object_marker(datum_index effect_index, void *position, float intensity,
                                 uint32_t flag); // 0x543ce0, UNSURE signature

// Updates a per-marker traction/wear value for each of the unit's contact points via surface
// material tests, triggering a friction-spark light effect (Vehicle.suspension_sound, reused
// here as the effect tag) once the aggregate change exceeds 0.3.
// UNSURE: see file header -- the node-array traversal and per-node transform are not fully
// resolved.
// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; object_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> object_index
uint32_t unit_update_marker_traction_effects(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;

    if (*(int32_t *)((uint8_t *)tag + 0x44) == -1) { // tag->animation_graph.tag_id
        return 0;
    }
    {
        uint8_t *graph_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x44) & 0xffff].data;
        if (*(int32_t *)(graph_tag + 0x24) == 0) {
            return 0;
        }
        {
            uint8_t *node_array = *(uint8_t **)(graph_tag + 0x28);
            uint8_t *physics_tag;
            real_matrix4x3 basis;
            float max_delta = 0.0f;
            int32_t node_count;
            int32_t i;

            if (node_array == 0) {
                return 0;
            }
            physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;

            matrix4x3_from_forward_up(&obj->up, &obj->forward, &basis);
            basis.position = obj->position;
            node_count = *(int32_t *)(node_array + 0x68);

            for (i = 0; i < node_count; i++) {
                uint8_t *entry = *(uint8_t **)(node_array + 0x6c) + i * 0x14;
                int16_t marker_index = *(int16_t *)entry;

                if (marker_index >= 0 && marker_index < *(int32_t *)(physics_tag + 0x74) &&
                    *(int16_t *)(entry + 2) != -1) {
                    uint8_t traction = ((uint8_t *)obj)[0x4f4 + i]; // vehicle_data.contact_point_traction[i]
                    uint8_t *physics_node = *(uint8_t **)(physics_tag + 0x78) + marker_index * 0x80;
                    float weight = (traction == 0xff) ? 1.0f : (float)traction * 0.003921569f;
                    real_point3d world_point;
                    real_vector3d world_normal;
                    real_vector3d delta;
                    float k;
                    uint8_t hit_scratch[20];
                    float hit_fraction = 0.0f; // UNSURE: collision_test_movement_segment's out-param, not modeled
                    uint8_t new_traction;

                    matrix4x3_transform_point(&world_point, (real_point3d *)physics_node, &basis);
                    matrix4x3_transform_normal(&world_normal, (real_vector3d *)(physics_node + 4), &basis);

                    k = (*(float *)(entry + 4) - *(float *)(entry + 8));
                    {
                        float mid = (*(float *)(entry + 8) - 0.0f) - k; // UNSURE: local_14+0x14 unresolved
                        world_point.x += world_normal.i * mid; // UNSURE
                        world_point.y += world_normal.j * mid;
                        world_point.z += world_normal.k * mid;
                    }
                    k = k + k;
                    delta.i = world_normal.i * k;
                    delta.j = world_normal.j * k;
                    delta.k = world_normal.k * k;

                    collision_test_movement_segment(0xc0a0, &world_point, &delta, object_index, hit_scratch);

                    {
                        float fVar4 = (1.0f - hit_fraction) + (1.0f - hit_fraction);
                        float clamped_fVar4 = (fVar4 < 0.0f) ? 0.0f : (fVar4 > 1.0f ? 1.0f : fVar4);
                        if (max_delta < clamped_fVar4 - weight) {
                            max_delta = clamped_fVar4 - weight;
                        }
                        new_traction = lerp_find_threshold_byte(0.0f, 1.0f, (clamped_fVar4 + weight) * 0.5f);
                        ((uint8_t *)obj)[0x4f4 + i] = new_traction;
                    }
                }
            }

            if (*(int32_t *)((uint8_t *)tag + 0x3bc) != -1 && max_delta > 0.3f) {
                float scaled = (float)((max_delta - 0.3f) * 1.6666667f);
                float clamped = (scaled < 0.0f) ? 0.0f : (scaled > 1.0f ? 1.0f : scaled);
                sound_start_at_object_marker(*(int32_t *)((uint8_t *)tag + 0x3bc), (void *)0xffffffff, clamped, 0);
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x575170):

undefined4 FUN_00575170(uint param_1)

{
  byte bVar1;
  short sVar2;
  uint *puVar3;
  float fVar4;
  undefined1 uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  undefined1 local_e0 [20];
  float local_cc;
  undefined1 local_90 [40];
  uint local_68;
  uint local_64;
  uint local_60;
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  int local_30;
  int local_2c;
  float local_28;
  float local_24;
  float local_20;
  int local_1c;
  int local_18;
  int local_14;
  float local_10;
  float local_c;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar8 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(uint *)(iVar8 + 0x44) == 0xffffffff) ||
      (iVar6 = *(int *)((*(uint *)(iVar8 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
      *(int *)(iVar6 + 0x24) == 0)) || (iVar6 = *(int *)(iVar6 + 0x28), iVar6 == 0)) {
    return 0;
  }
  local_14 = *(int *)((*(uint *)(iVar8 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_10 = 0.0;
  local_2c = iVar8;
  local_18 = iVar6;
  matrix4x3_from_forward_up(local_90);
  local_68 = puVar3[0x17];
  local_64 = puVar3[0x18];
  local_60 = puVar3[0x19];
  local_1c = 0;
  if (0 < *(int *)(iVar6 + 0x68)) {
    iVar7 = 0;
    do {
      sVar2 = *(short *)(*(int *)(iVar6 + 0x6c) + iVar7 * 0x14);
      iVar8 = *(int *)(iVar6 + 0x6c) + iVar7 * 0x14;
      if (((-1 < sVar2) && ((int)sVar2 < *(int *)(local_14 + 0x74))) &&
         (*(short *)(iVar8 + 2) != -1)) {
        bVar1 = *(byte *)(iVar7 + 0x4f4 + (int)puVar3);
        local_30 = sVar2 * 0x80 + *(int *)(local_14 + 0x78);
        if (bVar1 == 0xff) {
          local_c = 1.0;
        }
        else {
          local_c = (float)bVar1 * 0.003921569;
        }
        matrix4x3_transform_point(local_90);
        matrix4x3_transform_normal(local_90);
        local_34 = *(float *)(iVar8 + 4) - *(float *)(iVar8 + 8);
        fVar4 = (*(float *)(iVar8 + 8) - *(float *)(local_14 + 0x14)) - local_34;
        local_48 = local_28 * fVar4 + local_54;
        local_44 = local_24 * fVar4 + local_50;
        local_40 = local_20 * fVar4 + local_4c;
        local_34 = local_34 + local_34;
        local_3c = local_28 * local_34;
        local_38 = local_24 * local_34;
        local_34 = local_20 * local_34;
        FUN_00505880(0xc0a0,&local_48,&local_3c,param_1,local_e0);
        fVar4 = (1.0 - local_cc) + (1.0 - local_cc);
        if (0.0 <= fVar4) {
          if (1.0 < fVar4) {
            fVar4 = 1.0;
          }
        }
        else {
          fVar4 = 0.0;
        }
        if (local_10 < fVar4 - local_c) {
          local_10 = fVar4 - local_c;
        }
        uVar5 = lerp_find_threshold_byte(0,0x3f800000,(fVar4 + local_c) * 0.5);
        *(undefined1 *)(iVar7 + 0x4f4 + (int)puVar3) = uVar5;
      }
      local_1c = local_1c + 1;
      iVar7 = (int)(short)local_1c;
      iVar6 = local_18;
      iVar8 = local_2c;
    } while (iVar7 < *(int *)(local_18 + 0x68));
  }
  if ((*(int *)(iVar8 + 0x3bc) != -1) && (0.3 < local_10)) {
    local_c = (local_10 - 0.3) * 1.6666667;
    if (0.0 <= local_c) {
      if (1.0 < local_c) {
        local_c = 1.0;
      }
    }
    else {
      local_c = 0.0;
    }
    FUN_00543ce0(*(int *)(iVar8 + 0x3bc),0xffffffff,local_c,0);
    return 1;
  }
  return 0;
}
#endif
