// lights_apply_spot_falloff_specular
// address 0x4f1950, size 474 bytes
// name confidence: 0.35 (still FUN_004f1950 in Ghidra; out/phase4/objects_functions.md: "Variant
// of lights_apply_spot_falloff that also applies a per-type brightness scale before forwarding
// to a different render sink")
// rewrite confidence: 0.3
// evidence: same as lights_apply_spot_falloff.c, plus types/tags.h LightFlags bit1 ==
// no_specular and Light.specular_radius_multiplier (0x24).
// UNSURE: same caveats as lights_apply_spot_falloff.c — rasterizer_shader_environment_technique_ps2_set_states and structure_debug_draw_surfaces_in_box are
// opaque externals, and the final mask argument reads an apparently-unwritten 1024-byte stack
// buffer.
// register convention: none (no parameters).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern uint8_t *lights_enabled;         // 0x0071cfb8
extern uint8_t network_predicted_state_flag;              // 0x006f1d20, predicted/network flag
extern uint8_t g_0087aa00;              // 0x0087aa00, UNSURE: not owned by this module
extern int32_t g_0068944c;              // 0x0068944c, UNSURE: not owned by this module
extern int32_t light_active_list_count; // 0x008607c8
extern datum_index *light_active_list;  // 0x008607cc
extern data_array *light_data;          // 0x00860b14
extern tag_instance *tag_instances;     // 0x0087bc14

extern void rasterizer_shader_environment_technique_ps2_set_states(void); // UNSURE: zero visible args; out of range, 0x5212d0
extern int16_t FUN_004f1700(void); // UNSURE: zero visible args at this call site
extern void structure_debug_draw_surfaces_in_box(int32_t queue_slot, real_point3d *position, float radius,
    int16_t marker_count, uint32_t mask); // UNSURE: out of range, 0x552980

void lights_apply_spot_falloff_specular(void)
{
    uint8_t scratch[1024]; // UNSURE: `local_400`, never visibly written in the original either

    rasterizer_shader_environment_technique_ps2_set_states();

    if (*lights_enabled != 0 &&
        (network_predicted_state_flag == 0 || ((g_0087aa00 & 1) == 0 && 1 < g_0068944c))) {
        int16_t i;

        for (i = 0; i < light_active_list_count; i++) {
            light *l = &((light *)light_data->data)[light_active_list[i] & 0xffff];

            if ((l->flags & _light_always_visible_bit) != 0) {
                int32_t queue_slot = *(int32_t *)((uint8_t *)l + 8);

                if (queue_slot != -1) {
                    Light *tag = (Light *)tag_instances[l->definition_tag & 0xffff].data;

                    if (((uint32_t)tag->flags & 2) == 0) { // no_specular clear
                        int8_t is_cone = (l->flags & _light_needs_cone_update_bit) != 0 &&
                            (tag->flags & 8) != 0; // supersize_in_first_person
                        int16_t marker_count = 0;
                        float radius = l->radius;
                        real_point3d position;

                        if (!is_cone) {
                            marker_count = FUN_004f1700(); // UNSURE: zero visible args here
                        }

                        if (((uint32_t)tag->flags & 2) == 0) {
                            radius = radius * tag->specular_radius_multiplier;
                        }

                        if (1.5707964f <= tag->cutoff_angle) {
                            position = l->position;
                        } else if (0.7853982f <= tag->cutoff_angle) {
                            float scale = radius * tag->cos_cutoff_angle;

                            position.x = scale * l->direction.i + l->position.x;
                            position.y = scale * l->direction.j + l->position.y;
                            position.z = scale * l->direction.k + l->position.z;
                            radius = radius * tag->sin_cutoff_angle;
                        } else {
                            radius = radius / tag->cos_cutoff_angle;
                            position.x = radius * l->direction.i + l->position.x;
                            position.y = radius * l->direction.j + l->position.y;
                            position.z = radius * l->direction.k + l->position.z;
                        }

                        structure_debug_draw_surfaces_in_box(queue_slot, &position, radius, marker_count,
                            (is_cone - 1) & *(uint32_t *)scratch); // UNSURE: mask semantics
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f1950):

void FUN_004f1950(void)

{
  int iVar1;
  byte bVar2;
  byte *pbVar3;
  float fVar4;
  bool bVar5;
  undefined4 uVar6;
  short sVar7;
  float local_414;
  float local_410;
  float local_40c;
  float local_408;
  int local_404;
  undefined1 local_400 [1024];

  FUN_005212d0();
  if ((*DAT_0071cfb8 != '\0') &&
     (((DAT_006f1d20 == 0 || (((DAT_0087aa00 & 1) == 0 && (1 < DAT_0068944c)))) &&
      (sVar7 = 0, 0 < DAT_008607c8)))) {
    do {
      iVar1 = ((&DAT_008607cc)[sVar7] & 0xffff) * 0x7c + *(int *)(DAT_00860b14 + 0x34);
      if ((((*(ushort *)(iVar1 + 2) & 1) != 0) && (local_404 = *(int *)(iVar1 + 8), local_404 != -1)
          ) && (bVar2 = **(byte **)((*(uint *)(iVar1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
               (bVar2 & 2) == 0)) {
        if (((*(ushort *)(iVar1 + 2) & 8) == 0) || ((bVar2 & 8) == 0)) {
          bVar5 = false;
        }
        else {
          bVar5 = true;
        }
        uVar6 = 0;
        if (!bVar5) {
          uVar6 = FUN_004f1700();
        }
        local_414 = *(float *)(iVar1 + 0x54);
        pbVar3 = *(byte **)((*(uint *)(iVar1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if ((*pbVar3 & 2) == 0) {
          local_414 = local_414 * *(float *)(pbVar3 + 0x24);
        }
        if (1.5707964 <= *(float *)(pbVar3 + 0x14)) {
          local_410 = *(float *)(iVar1 + 0x30);
          local_40c = *(float *)(iVar1 + 0x34);
          local_408 = *(float *)(iVar1 + 0x38);
        }
        else if (0.7853982 <= *(float *)(pbVar3 + 0x14)) {
          fVar4 = local_414 * *(float *)(pbVar3 + 0x20);
          local_410 = fVar4 * *(float *)(iVar1 + 0x3c) + *(float *)(iVar1 + 0x30);
          local_40c = fVar4 * *(float *)(iVar1 + 0x40) + *(float *)(iVar1 + 0x34);
          local_408 = fVar4 * *(float *)(iVar1 + 0x44) + *(float *)(iVar1 + 0x38);
          local_414 = local_414 * *(float *)(pbVar3 + 0x28);
        }
        else {
          local_414 = local_414 / *(float *)(pbVar3 + 0x20);
          local_410 = local_414 * *(float *)(iVar1 + 0x3c) + *(float *)(iVar1 + 0x30);
          local_40c = local_414 * *(float *)(iVar1 + 0x40) + *(float *)(iVar1 + 0x34);
          local_408 = local_414 * *(float *)(iVar1 + 0x44) + *(float *)(iVar1 + 0x38);
        }
        FUN_00552980(local_404,&local_410,local_414,uVar6,bVar5 - 1 & (uint)local_400);
      }
      sVar7 = sVar7 + 1;
    } while (sVar7 < DAT_008607c8);
  }
  return;
}
#endif
