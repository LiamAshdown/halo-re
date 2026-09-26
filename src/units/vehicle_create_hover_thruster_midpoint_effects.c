// vehicle_create_hover_thruster_midpoint_effects  (Ghidra: already named)
// address 0x574bc0, size 860 bytes
// name confidence: 0.5 (cea-pdb hints via the "hover thrusters"/"midpoint" strings)
// rewrite confidence: 0.15 -- mirrors vehicle_create_hover_thruster_effects.c's treatment for
//   the same reasons (register-only marker/raycast buffers); this variant additionally
//   computes a marker-to-hit "midpoint" position and passes a 4-string label set (kind == 4)
//   to effect_new_with_color.
// evidence: types/tags.h Vehicle.effect (tag_id at absolute 0x3ec); types/units.h
//   unit_data.unknown_338 ("a 0..1 scalar the vehicle lean, thruster and ground-effect routines
//   all multiply by", gating this function on being > 0, matching the functions.md summary's
//   "gated on the unit's speed (offset 0xce > 0)"); callees as in the sibling function.
// UNSURE: see vehicle_create_hover_thruster_effects.c's header; the same caveats apply to the
//   raycast result field offsets and to effect_new_with_color's exact argument shape for kind == 4.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                void *marker, uint32_t flags); // 0x4f6080
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out,
    void *seed, real lo, real hi); // 0x4cd1b0, UNSURE args
extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern void effect_new_with_color(uint32_t effect, uint32_t param_2, void *param_3, int32_t kind,
                          char **labels, void *midpoint_block, void *reflection_block,
                          float param_8, float param_9, int32_t param_10, int32_t param_11,
                          int32_t param_12); // 0x450980, this call site's variant (kind == 4)

// Spawns hover-thruster ground-effect visuals positioned at the midpoint between each "hover
// thrusters" marker and the surface below it, scaled by vehicle speed (unit_data.unknown_338).
// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; unit_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> unit_index
void vehicle_create_hover_thruster_midpoint_effects(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (*(int32_t *)((uint8_t *)tag + 0x3ec) == -1 || unit->unknown_338 <= 0.0f) {
        return;
    }

    {
        uint8_t markers[68];
        real_point3d marker_positions[16]; // local_660, UNSURE bound (0x1b floats/marker)
        int16_t marker_count = (int16_t)object_get_node_local_transform(unit_index, "hover thrusters", markers, 0xf);
        int32_t i;

        for (i = 0; i < marker_count; i++) {
            real_point3d direction; // local_78c
            uint8_t hit_scratch[20];

            vector3d_randomize_direction(0, (real_vector3d *)&direction, 0, 0.0f, 15.0f); // UNSURE args/angle
            if (collision_test_movement_segment(0x61, &marker_positions[i], (real_vector3d *)&direction, unit_index, hit_scratch) != 0) {
                float fraction = *(float *)(hit_scratch + 0);
                float intensity = (1.0f - fraction) * -direction.x * unit->unknown_338;

                if (intensity >= 0.0f) {
                    if (intensity > 1.0f) {
                        intensity = 1.0f;
                    }
                    if (intensity > 0.0f) {
                        real_point3d normal;
                        real_vector3d incident, reflected;
                        char *labels[4] = { "incident", "normal", "reflected", "midpoint" };
                        real_point3d midpoint;
                        float dot;

                        normal.x = *(float *)(hit_scratch + 0x38); // UNSURE
                        normal.y = *(float *)(hit_scratch + 0x34);
                        normal.z = *(float *)(hit_scratch + 0x30);
                        midpoint.x = (normal.x + marker_positions[i].x) * 0.5f;
                        midpoint.y = (normal.y + marker_positions[i].y) * 0.5f;
                        midpoint.z = (normal.z + marker_positions[i].z) * 0.5f;

                        incident.i = -direction.x;
                        incident.j = -direction.y;
                        incident.k = -direction.z;
                        dot = *(float *)(hit_scratch + 0x2c) * direction.x +
                              *(float *)(hit_scratch + 0x28) * direction.y +
                              *(float *)(hit_scratch + 0x24) * direction.z;
                        dot += dot;
                        reflected.i = direction.x - *(float *)(hit_scratch + 0x2c) * dot;
                        reflected.j = direction.y - *(float *)(hit_scratch + 0x28) * dot;
                        reflected.k = direction.z - *(float *)(hit_scratch + 0x24) * dot;

                        {
                            real_point3d midpoint_block[3] = { normal, midpoint, normal };
                            real_vector3d reflection_block[3] = { incident, incident, reflected };
                            effect_new_with_color(*(uint32_t *)((uint8_t *)tag + 0x3ec), 0xffffffff, 0, 4, labels,
                                         midpoint_block, reflection_block, intensity, intensity, 0, 0, 1);
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x574bc0):

void vehicle_create_hover_thruster_midpoint_effects(uint param_1)

{
  uint *puVar1;
  int iVar2;
  float fVar3;
  char cVar4;
  short sVar5;
  short sVar6;
  int iVar7;
  float local_7b4;
  float local_7b0;
  float local_7ac;
  float local_7a4;
  undefined1 local_78c [12];
  char *local_780;
  char *local_77c;
  char *local_778;
  char *local_774;
  float local_770;
  float local_76c;
  float local_768;
  float local_764;
  float local_760;
  float local_75c;
  float local_758;
  float local_754;
  float local_750;
  float local_74c;
  float local_748;
  float local_744;
  float local_740;
  float local_73c;
  float local_738;
  float local_734;
  float local_730;
  float local_72c;
  float local_728;
  float local_724;
  float local_720;
  float local_71c;
  float local_718;
  float local_714;
  undefined1 local_710 [20];
  float local_6fc;
  float local_6f8;
  float local_6f4;
  float local_6f0;
  float local_6ec;
  float local_6e8;
  float local_6e4;
  undefined1 local_6c0 [68];
  float local_67c [7];
  float local_660 [408];

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(int *)(iVar2 + 0x3ec) != -1) && (0.0 < (float)puVar1[0xce])) {
    sVar5 = object_get_node_local_transform(param_1,"hover thrusters",local_6c0,0xf);
    sVar6 = 0;
    if (0 < sVar5) {
      iVar7 = 0;
      do {
        vector3d_randomize_direction(0,0x41700000);
        cVar4 = FUN_00505880(0x61,local_660 + iVar7 * 0x1b,local_78c,param_1,local_710);
        if ((cVar4 != '\0') &&
           (local_7a4 = (1.0 - local_6fc) * -local_67c[iVar7 * 0x1b] * (float)puVar1[0xce],
           0.0 <= local_7a4)) {
          if (local_7a4 <= 1.0) {
            if (local_7a4 <= 0.0) goto LAB_00574f01;
          }
          else {
            local_7a4 = 1.0;
          }
          local_770 = local_6f8;
          local_74c = (local_6f8 + local_660[iVar7 * 0x1b]) * 0.5;
          local_76c = local_6f4;
          local_768 = local_6f0;
          local_764 = local_6f8;
          local_760 = local_6f4;
          local_75c = local_6f0;
          local_758 = local_6f8;
          local_754 = local_6f4;
          local_748 = (local_6f4 + local_660[iVar7 * 0x1b + 1]) * 0.5;
          local_750 = local_6f0;
          local_780 = "incident";
          local_734 = local_6ec;
          local_744 = (local_6f0 + local_660[iVar7 * 0x1b + 2]) * 0.5;
          local_730 = local_6e8;
          local_77c = "normal";
          local_740 = -local_7b4;
          local_72c = local_6e4;
          local_778 = "reflected";
          local_774 = "midpoint";
          local_73c = -local_7b0;
          local_738 = -local_7ac;
          fVar3 = local_6ec * local_7b4 + local_6e8 * local_7b0 + local_6e4 * local_7ac;
          fVar3 = fVar3 + fVar3;
          local_728 = local_7b4 - local_6ec * fVar3;
          local_724 = local_7b0 - local_6e8 * fVar3;
          local_720 = local_7ac - local_6e4 * fVar3;
          local_71c = local_7b4 - local_6ec * fVar3;
          local_718 = local_7b0 - local_6e8 * fVar3;
          local_714 = local_7ac - local_6e4 * fVar3;
          FUN_00450980(*(undefined4 *)(iVar2 + 0x3ec),0xffffffff,0,4,&local_780,&local_770,
                       &local_740,local_7a4,local_7a4,0,0,1);
        }
LAB_00574f01:
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < sVar5);
    }
  }
  return;
}
#endif
