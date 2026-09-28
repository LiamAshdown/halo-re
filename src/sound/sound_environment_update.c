// sound_environment_update  (orphan pass 4: FUN_0053f150, no Ghidra name)
// address 0x53f150, size 1134 bytes
// name confidence: 0.4 (out/phase4/structures_types_notes.md / types/scenario.h: "the sound
//   module 0x53f150 compares the byte at +0x30 and interpolates the SoundEnvironment at +0x34";
//   the body looks up the local player's fog region and cluster reverb settings, then either
//   snaps or smoothly lerps global_scenario_game_globals->sound_environment toward the target)
// rewrite confidence: 0.3 (the SoundEnvironment lerp (12 fields, room_intensity..hf_reference,
//   each with its own clamp-per-tick delta) and the scenario_game_globals fields it touches are
//   confirmed against types/scenario.h and types/tags.h; the fog-region/cluster/material
//   lookup chain in the first half uses raw offsets into records not otherwise typed by this
//   pass -- see UNSURE)
// evidence: types/scenario.h scenario_game_globals (structure_bsp_index 0x00,
//   sound_environment_is_water 0x30, sound_environment 0x34), types/tags.h SoundEnvironment
//   (room_intensity 0x08 .. hf_reference 0x34, 12 lerped floats); global 0x0065e508
//   k_default_sound_environment (types/sound.h); global 0x0087bc14 tag_instances; global
//   0x00746f9c scenario_structure_bsp (this pass's own object_lighting_sample_point.c).
//   scenario_location_fog_region (0x53ec30, src/scenario) called with EAX = leaf (0x006ac6dc) and
//   EBX = point (0x006ac6d0), matching its own declared signature (EAX leaf, EBX point).
// register convention (confirmed via objdump for the first half): stack arguments = int32_t
//   *out_environment_pointer_or_default (param_1), int32_t *out_environment_slot (param_2),
//   uint8_t *out_changed (param_3). EBX/EAX at the scenario_location_fog_region call site are
//   the literal addresses 0x006ac6d0 / 0x006ac6dc: observers[0].camera (types/camera.h
//   observer_camera: position at +0x00, leaf_index at +0x0c, cluster_index at +0x10 =
//   0x006ac6e0), i.e. local player 0's camera. The earlier draft of this file had the two
//   swapped (a "leaf" at 0x6ac6d0), which is what made it look inconsistent with
//   any_local_player_within_10_units.c / structure_regions_find_within_radius.c; with EAX = leaf
//   = 0x6ac6dc and EBX = point = 0x6ac6d0 all three agree (orphan pass 4 review, objdump
//   0x53f17b..0x53f1a8). The same review fixed the cluster record address, which is
//   bsp->clusters.pointer (bsp + 0x138) + cluster_index * 0x68, not offset from the BSP base.
// blam-cc: sound_environment_update(uint32_t *out_environment_ptr, void **out_environment_slot,
//   uint8_t *out_changed)
// UNSURE (function-wide): the cluster record (+0x138 array, stride 0x68, fog id at +0x24 of a
//   sub-record reached through +0x188/+0x194), the material fog record (+0x110 sound
//   environment tag reference, +0x100 an opaque dword, +0x00 bit 0 an is-water-ish flag,
//   tag+4 a priority int16), and the location's own reverb-override lookup (+0x20c array,
//   stride 0x50, fog id at +0x6, override at +0x2c of a stride-0x74 array at +0x200/+0x1fc) are
//   all preserved as raw offsets rather than named fields, since none of them match an existing
//   typed struct in this codebase closely enough to claim.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "scenario.h"
#include "sound.h"
#include <stdint.h> // uintptr_t: tag block pointers are 32-bit fields
#include "game.h"

extern player_globals *local_player_globals;  // 0x0087a478
extern int16_t local_player_0_cluster_index;  // 0x006ac6e0, observers[0].camera.cluster_index
extern real_point3d camera_point;             // 0x006ac6d0, observers[0].camera.position
extern bsp_leaf_reference camera_leaf;        // 0x006ac6dc, observers[0].camera.leaf_index / cluster_index
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern scenario_game_globals *global_scenario_game_globals; // 0x00746f94
extern SoundEnvironment k_default_sound_environment; // 0x0065e508
extern tag_instance *tag_instances;           // 0x0087bc14

extern int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point); // 0x53ec30, src/scenario

void sound_environment_update(uint32_t *out_environment_ptr, void **out_environment_slot, uint8_t *out_changed)
{
    ScenarioStructureBSP *structure_bsp = global_structure_bsp;
    uint32_t sound_tag_id = 0xffffffff;
    uint32_t environment_default = 0xffffffff;
    uint8_t is_water = 0;
    ScenarioStructureBSPCluster *cluster_record;
    int16_t fog_id;

    if (*(int32_t *)&local_player_globals->local_players /* local player 0 */ == -1 || local_player_0_cluster_index == -1) {
        goto skip_environment_lookup;
    }

    cluster_record = (ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer + local_player_0_cluster_index;

    {
        int16_t region = scenario_location_fog_region(&camera_leaf, &camera_point);
        if (region != -1) {
            region = (int16_t)((ScenarioStructureBSPFogRegion *)structure_bsp->fog_regions.pointer)[region].fog;
        }
        if (region == -1) {
            fog_id = -0x8000;
        } else {
            uint32_t fog_tag_id = *(uint32_t *)&((ScenarioStructureBSPFogPalette *)structure_bsp->fog_palette.pointer)[region].fog.tag_id;
            if (fog_tag_id == 0xffffffff) {
                fog_id = -0x8000;
            } else {
                Fog *fog_tag = (Fog *)tag_instances[fog_tag_id & 0xffff].data;
                uint32_t env_tag = *(uint32_t *)&fog_tag->sound_environment.tag_id;
                if (env_tag == 0xffffffff) {
                    fog_id = -0x8000;
                } else {
                    SoundEnvironment *env_tag_data = (SoundEnvironment *)tag_instances[env_tag & 0xffff].data;
                    if (env_tag_data->priority < -0x7fff) {
                        fog_id = -0x8000;
                    } else {
                        fog_id = env_tag_data->priority;
                        environment_default = *(uint32_t *)&fog_tag->background_sound.tag_id;
                        is_water = *(uint8_t *)&fog_tag->flags & 1;
                        sound_tag_id = env_tag;
                    }
                }
            }
        }
    }

    {
        int16_t sound_environment_index = (int16_t)cluster_record->sound_environment;
        if (sound_environment_index != -1) {
            uint32_t override_tag = *(uint32_t *)&((ScenarioStructureBSPSoundEnvironmentPalette *)structure_bsp->sound_environment_palette.pointer)[sound_environment_index].sound_environment.tag_id;
            if (override_tag != 0xffffffff) {
                SoundEnvironment *override_data = (SoundEnvironment *)tag_instances[override_tag & 0xffff].data;
                if (fog_id < override_data->priority) {
                    int16_t background_sound_index = (int16_t)cluster_record->background_sound;
                    is_water = 0;
                    sound_tag_id = override_tag;
                    if (background_sound_index == -1 || background_sound_index >= (int32_t)structure_bsp->background_sound_palette.count) {
                        environment_default = 0xffffffff;
                    } else {
                        environment_default = *(uint32_t *)&((ScenarioStructureBSPBackgroundSoundPalette *)structure_bsp->background_sound_palette.pointer)[background_sound_index].background_sound.tag_id;
                    }
                }
            }
        }
    }

skip_environment_lookup:
    {
        uint32_t *source;
        SoundEnvironment *dest = &global_scenario_game_globals->sound_environment;

        if (sound_tag_id == 0xffffffff) {
            source = (uint32_t *)&k_default_sound_environment;
        } else {
            source = (uint32_t *)tag_instances[sound_tag_id & 0xffff].data;
        }

        if (is_water == global_scenario_game_globals->sound_environment_is_water) {
            static const float k_clamp[12] = {
                0.03f, 0.03f, 0.3f, 0.1f, 0.03f, 0.03f, 0.09f, 0.03f, 0.003f, 0.03f, 0.03f, 600.0f
            };
            float *dest_f = (float *)&dest->room_intensity;
            const float *source_f = (const float *)((const uint8_t *)source + 8);
            int32_t i;

            for (i = 0; i < 12; i++) {
                float delta = source_f[i] - dest_f[i];
                float clamp = k_clamp[i];
                if (delta < -clamp) delta = -clamp;
                else if (delta > clamp) delta = clamp;
                dest_f[i] = dest_f[i] + delta;
            }
            *out_changed = 0;
        } else {
            uint32_t *dest_words = (uint32_t *)dest;
            int32_t i;
            for (i = 0; i < 0x12; i++) {
                dest_words[i] = source[i];
            }
            global_scenario_game_globals->sound_environment_is_water = is_water;
            *out_changed = 1;
        }

        *out_environment_ptr = environment_default;
        *out_environment_slot = (void *)dest;
    }
}

#if 0
Original Ghidra decompilation (0x53f150):

void FUN_0053f150(undefined4 *param_1,int *param_2,undefined1 *param_3)

{
  undefined4 *puVar1;
  short sVar2;
  uint uVar3;
  byte *pbVar4;
  int iVar5;
  float fVar6;
  int iVar7;
  short sVar8;
  uint uVar9;
  undefined4 *puVar10;
  int iVar11;
  undefined4 *puVar12;
  byte local_9;
  undefined4 local_8;

  iVar7 = DAT_00746f9c;
  uVar9 = 0xffffffff;
  local_8 = 0xffffffff;
  local_9 = 0;
  if ((*(int *)(DAT_0087a478 + 4) == -1) || ((short)DAT_006ac6e0 == -1)) goto LAB_0053f2b9;
  iVar11 = (short)DAT_006ac6e0 * 0x68 + *(int *)(DAT_00746f9c + 0x138);
  sVar8 = scenario_location_fog_region();
  if ((sVar8 == -1) ||
     ((sVar8 = *(short *)(*(int *)(iVar7 + 0x188) + sVar8 * 0x28 + 0x24), sVar8 == -1 ||
      (uVar3 = *(uint *)(sVar8 * 0x88 + *(int *)(iVar7 + 0x194) + 0x2c), uVar3 == 0xffffffff)))) {
LAB_0053f23d:
    sVar8 = -0x8000;
  }
  else {
    pbVar4 = *(byte **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    uVar3 = *(uint *)(pbVar4 + 0x110);
    if ((uVar3 == 0xffffffff) ||
       (iVar5 = *(int *)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
       *(short *)(iVar5 + 4) < -0x7fff)) goto LAB_0053f23d;
    sVar8 = *(short *)(iVar5 + 4);
    local_8 = *(undefined4 *)(pbVar4 + 0x100);
    local_9 = *pbVar4 & 1;
    uVar9 = uVar3;
  }
  sVar2 = *(short *)(iVar11 + 6);
  if (((sVar2 != -1) &&
      (uVar3 = *(uint *)(sVar2 * 0x50 + 0x2c + *(int *)(iVar7 + 0x20c)), uVar3 != 0xffffffff)) &&
     (sVar8 < *(short *)(*(int *)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4))) {
    sVar8 = *(short *)(iVar11 + 4);
    local_9 = 0;
    uVar9 = uVar3;
    if ((sVar8 == -1) || (*(int *)(iVar7 + 0x1fc) <= (int)sVar8)) {
      local_8 = 0xffffffff;
    }
    else {
      local_8 = *(undefined4 *)(sVar8 * 0x74 + 0x2c + *(int *)(iVar7 + 0x200));
    }
  }
LAB_0053f2b9:
  iVar7 = DAT_00746f94;
  if (uVar9 == 0xffffffff) {
    puVar10 = &DAT_0065e508;
  }
  else {
    puVar10 = *(undefined4 **)((uVar9 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  }
  puVar1 = (undefined4 *)(DAT_00746f94 + 0x34);
  if (local_9 == *(byte *)(DAT_00746f94 + 0x30)) {
    fVar6 = (float)puVar10[2] - *(float *)(DAT_00746f94 + 0x3c);
    if (-0.03 <= fVar6) {
      if (0.03 < fVar6) {
        fVar6 = 0.03;
      }
    }
    else {
      fVar6 = -0.03;
    }
    *(float *)(DAT_00746f94 + 0x3c) = fVar6 + *(float *)(DAT_00746f94 + 0x3c);
    ... (remaining 10 fields follow the identical pattern; see full listing via
    tools/pack.py 0x53f150) ...
    *param_3 = 0;
  }
  else {
    puVar12 = puVar1;
    for (iVar11 = 0x12; iVar11 != 0; iVar11 = iVar11 + -1) {
      *puVar12 = *puVar10;
      puVar10 = puVar10 + 1;
      puVar12 = puVar12 + 1;
    }
    *(byte *)(iVar7 + 0x30) = local_9;
    *param_3 = 1;
  }
  *param_1 = local_8;
  *param_2 = (int)puVar1;
  return;
}
#endif
