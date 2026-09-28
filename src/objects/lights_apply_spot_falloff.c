// lights_apply_spot_falloff
// address 0x4f1780, size 453 bytes
// name confidence: 0.6 (out/phase4/objects_types_notes.md names this function directly, in the
// `light` struct comment: "the lights_apply_spot_falloff input" and "Light tag ... cutoff angle
// 0x14, intensity 0x24 region")
// rewrite confidence: 0.35
// evidence: types/objects.h light (flags 0x02 with light_flags, unknown_08 "queue slot",
// definition_tag 0x04, radius 0x54, position 0x30, direction 0x3c); types/tags.h Light
// (cutoff_angle 0x14, cos_cutoff_angle 0x20, sin_cutoff_angle 0x28), LightFlags bit3 ==
// supersize_in_first_person.
// UNSURE: rasterizer_light_cone_set_texture_stage_states and structure_debug_draw_surfaces_in_box_alt are opaque externals outside this module (the render
// dispatch this function feeds); DAT_0087aa00/DAT_0068944c are not owned by this module either.
// The final call's mask argument (`bVar4 - 1 & (uint)local_404`) reads a 1028-byte uninitialized
// stack buffer (`local_404`) whose true source this decompile never shows being filled.
// register convention: none (no parameters).
// reconciled: R04 0x006f1d20 uint8_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"

extern uint8_t *lights_enabled;         // 0x0071cfb8
extern game_engine_definition *current_game_engine;              // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t game_engine_unknown_aa00;              // 0x0087aa00, UNSURE: not owned by this module
extern int16_t light_count_enabled;              // 0x0068944c, word (cmp WORD PTR at 0x4f17b3)
extern int16_t light_active_list_count; // 0x008607c8, word (cmp di,WORD PTR at 0x4f1929)
extern datum_index light_active_list[];  // 0x008607cc, the array itself ([ecx*4+0x8607cc] at 0x4f17df)
extern data_array *light_data;          // 0x00860b14
extern tag_instance *tag_instances;     // 0x0087bc14

extern void rasterizer_light_cone_set_texture_stage_states(void); // UNSURE: zero visible args; out of range, 0x51d6a0
extern int16_t light_collect_object_references(uint32_t light_handle, int16_t max_count, int16_t *out_buffer);
    // 0x4f1700, blam-cc: ECX, SI, EDI (0x4f183a: EDI = the local buffer, ESI = 0x200, ECX = the light)
extern void structure_debug_draw_surfaces_in_box_alt(void *render_point, real_point3d *query_point, float radius,
    int16_t cluster_count, int16_t *cluster_indices); // 0x552a60

void lights_apply_spot_falloff(void)
{
    int16_t references[0x200]; // [esp+0x28]: filled by light_collect_object_references

    rasterizer_light_cone_set_texture_stage_states();

    if (*lights_enabled != 0 &&
        (current_game_engine == 0 || ((game_engine_unknown_aa00 & 1) == 0 && 1 < light_count_enabled))) {
        int16_t i;

        for (i = 0; i < light_active_list_count; i++) {
            light *l = &((light *)light_data->data)[light_active_list[i] & 0xffff];

            if ((l->flags & _light_always_visible_bit) != 0) {
                int32_t queue_slot = *(int32_t *)&((struct light *)l)->unknown_08;

                if (queue_slot != -1) {
                    Light *tag = (Light *)tag_instances[l->definition_tag & 0xffff].data;
                    int8_t is_cone = (l->flags & _light_needs_cone_update_bit) != 0 &&
                        (tag->flags & 8) != 0; // supersize_in_first_person
                    int16_t marker_count = 0;
                    float radius = l->radius;
                    real_point3d position;

                    if (!is_cone) {
                        marker_count = light_collect_object_references(light_active_list[i], 0x200, references);
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

                    structure_debug_draw_surfaces_in_box_alt(queue_slot, &position, radius, marker_count,
                        is_cone ? (int16_t *)0 : references);
                        // FIXED (0x4f1905..0x4f1913): the buffer's address, or NULL for a cone light
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f1780):

void FUN_004f1780(void)

{
  int iVar1;
  int iVar2;
  float fVar3;
  bool bVar4;
  undefined4 uVar5;
  short sVar6;
  float local_418;
  float local_414;
  float local_410;
  float local_40c;
  int local_408;
  undefined1 local_404 [1028];

  FUN_0051d6a0();
  if ((*DAT_0071cfb8 != '\0') &&
     ((DAT_006f1d20 == 0 || (((DAT_0087aa00 & 1) == 0 && (1 < DAT_0068944c)))))) {
    sVar6 = 0;
    if (0 < DAT_008607c8) {
      do {
        iVar1 = ((&DAT_008607cc)[sVar6] & 0xffff) * 0x7c + *(int *)(DAT_00860b14 + 0x34);
        if ((*(ushort *)(iVar1 + 2) & 1) != 0) {
          local_408 = *(int *)(iVar1 + 8);
          if (local_408 != -1) {
            if (((*(ushort *)(iVar1 + 2) & 8) == 0) ||
               ((**(byte **)((*(uint *)(iVar1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 8) == 0
               )) {
              bVar4 = false;
            }
            else {
              bVar4 = true;
            }
            uVar5 = 0;
            if (!bVar4) {
              uVar5 = FUN_004f1700();
            }
            local_418 = *(float *)(iVar1 + 0x54);
            iVar2 = *(int *)((*(uint *)(iVar1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (1.5707964 <= *(float *)(iVar2 + 0x14)) {
              local_414 = *(float *)(iVar1 + 0x30);
              local_410 = *(float *)(iVar1 + 0x34);
              local_40c = *(float *)(iVar1 + 0x38);
            }
            else if (0.7853982 <= *(float *)(iVar2 + 0x14)) {
              fVar3 = local_418 * *(float *)(iVar2 + 0x20);
              local_414 = fVar3 * *(float *)(iVar1 + 0x3c) + *(float *)(iVar1 + 0x30);
              local_410 = fVar3 * *(float *)(iVar1 + 0x40) + *(float *)(iVar1 + 0x34);
              local_40c = fVar3 * *(float *)(iVar1 + 0x44) + *(float *)(iVar1 + 0x38);
              local_418 = local_418 * *(float *)(iVar2 + 0x28);
            }
            else {
              local_418 = local_418 / *(float *)(iVar2 + 0x20);
              local_414 = local_418 * *(float *)(iVar1 + 0x3c) + *(float *)(iVar1 + 0x30);
              local_410 = local_418 * *(float *)(iVar1 + 0x40) + *(float *)(iVar1 + 0x34);
              local_40c = local_418 * *(float *)(iVar1 + 0x44) + *(float *)(iVar1 + 0x38);
            }
            FUN_00552a60(local_408,&local_414,local_418,uVar5,bVar4 - 1 & (uint)local_404);
          }
        }
        sVar6 = sVar6 + 1;
      } while (sVar6 < DAT_008607c8);
    }
  }
  return;
}
#endif
