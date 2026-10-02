// scenario_sky_fog_state_update  (Ghidra: FUN_0053e8c0, still unnamed there; renamed here --
// out/phase2/results/scenario_00.json's guess "sound_environment_interpolate_for_position" is
// contradicted by out/phase4/scenario_types_notes.md, whose disassembly-level pass over this
// function establishes it as the sky-fog blend and gives it the header's own struct name)
// address 0x53e8c0, size 665 bytes
// name confidence: 0.4   rewrite confidence: 0.75 (raised in the phase-4 review after a line-by-line
//   pass against objdump 0x53e8c0..0x53eb58; the only caller is render_player_frame 0x50babe)
// evidence: out/phase4/scenario_types_notes.md scenario_sky_fog_state / scenario_game_globals /
// render_fog sections, and the raw disassembly quoted below (0x53e8c0-0x53eb58), which pins down
// every field write, the exact register/stack argument mapping and the shared value_step_toward_target
// (0x470d40) / render_lighting_step_vector3_toward (0x50f520) blend calls -- both already
// established elsewhere in this codebase and reused verbatim here.
// register convention: AX -> sky_index, stack -> local_player_index, camera_position, out.
//   // blam-cc: AX -> sky_index, stack -> local_player_index, camera_position, out
//   UNSURE: the outer fields of scenario_sky_fog_state ("valid", "fog_screen_blend") and the
//   indoor/outdoor sky selection logic are re-derived a second time inside the indoor
//   (sky_index == -1) branch, exactly duplicating the computation already done above it -- kept
//   as literal repeated code rather than a shared helper, matching the disassembly, not because
//   it does anything different the second time.
//   UNSURE: when local_player_index == -1 *and* the resolved sky has no tag data (sky_data ==
//   NULL), the function reads its output straight out of the never-initialized `local_scratch`
//   stack scratch (the local_player_index == -1 path always uses a fresh on-stack
//   scenario_sky_fog_state instead of a slot in global_scenario_game_globals, and nothing
//   initializes it when the sky lookup fails). Preserved as-is; not a rewrite bug.
// reconciled: R41 render_fog.unknown_4c (uint32 raw bits) -> float sky_fog_screen_blend; same stores

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "rasterizer.h"
#include "scenario.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern float sqrtf(float x);

extern void value_step_toward_target(float *value, float target, float max_step); // 0x470d40, game module, blam-cc: ECX->value, stack->target,max_step
extern void render_lighting_step_vector3_toward(float *current, float *target, float max_delta); // 0x50f520, render module, blam-cc: ECX->current, EDX->target, stack->max_delta

extern tag_instance *tag_instances;                              // 0x0087bc14
extern Scenario *global_scenario;                                 // 0x00746f8c
extern scenario_game_globals *global_scenario_game_globals;       // 0x00746f94

// blam-cc: AX -> sky_index, stack -> local_player_index, camera_position, out
// Updates the per-local-player sky fog blend state and writes the render module's render_fog
// from it. `sky_index` is the camera cluster's sky (-1 if the cluster has no sky, in which case
// the indoor fog of sky 0 is used instead, gated by whether that sky has an indoor_fog_screen
// dependency). The state is kept in global_scenario_game_globals->sky_fog[local_player_index]
// (or a throwaway stack scratch when local_player_index == -1). It snaps straight to the target
// sky fog when the state was not valid yet, the camera jumped 15 or more world units, or either
// the new or the previous opaque distance is 0; otherwise it blends every field toward the
// target at a rate scaled by the distance moved.
void scenario_sky_fog_state_update(int16_t sky_index, int16_t local_player_index,
                                    real_point3d *camera_position, render_fog *out)
{
    uint32_t sky_tag;
    Sky *sky_data;
    scenario_sky_fog_state *state;
    scenario_sky_fog_state local_scratch;
    sky_fog_block *fog;
    float distance;
    float fog_screen_blend_target;

    sky_tag = 0xffffffff;
    if (sky_index == -1) {
        if (0 < (int32_t)global_scenario->skies.count) {
            sky_tag = *(uint32_t *)&((ScenarioSky *)global_scenario->skies.pointer)[0].sky.tag_id;
        }
    } else if (0 <= sky_index && (int32_t)sky_index < (int32_t)global_scenario->skies.count) {
        sky_tag = *(uint32_t *)&((ScenarioSky *)global_scenario->skies.pointer)[sky_index].sky.tag_id;
    }

    sky_data = (Sky *)0;
    if (sky_tag != 0xffffffff) {
        sky_data = (Sky *)tag_instances[sky_tag & 0xffff].data;
    }

    if (local_player_index == -1) {
        state = &local_scratch;
    } else {
        state = &global_scenario_game_globals->sky_fog[local_player_index];
    }

    if (sky_data != (Sky *)0) {
        if (sky_index == -1) {
            fog = (sky_fog_block *)&sky_data->indoor_fog_color;

            sky_tag = 0xffffffff;
            if (0 < (int32_t)global_scenario->skies.count) {
                sky_tag = *(uint32_t *)&((ScenarioSky *)global_scenario->skies.pointer)[0].sky.tag_id;
            }
            sky_data = (Sky *)0;
            if (sky_tag != 0xffffffff) {
                sky_data = (Sky *)tag_instances[sky_tag & 0xffff].data;
            }

            fog_screen_blend_target = 1.0f;
            if (*(uint32_t *)&sky_data->indoor_fog_screen.tag_id != 0xffffffff) {
                goto sky_fog_resolved;
            }
        } else {
            fog = (sky_fog_block *)&sky_data->outdoor_fog_color;
        }
        fog_screen_blend_target = 0.0f;
sky_fog_resolved:

        // summed x, z, y as at 0x53e9a5 (fsqrt inline); the 15-unit test is `test ah,5 / jp`,
        // which also snaps on an unordered (NaN) distance
        distance = sqrtf(
            (camera_position->x - state->camera_position.x) * (camera_position->x - state->camera_position.x) +
            (camera_position->z - state->camera_position.z) * (camera_position->z - state->camera_position.z) +
            (camera_position->y - state->camera_position.y) * (camera_position->y - state->camera_position.y));

        if (local_player_index == -1 || !(distance < 15.0f) || state->valid == 0 ||
            fog->opaque_distance == 0.0f || state->opaque_distance == 0.0f) {
            state->start_distance   = fog->start_distance;
            state->opaque_distance  = fog->opaque_distance;
            state->maximum_density  = fog->maximum_density;
            state->color            = fog->color;
            state->fog_screen_blend = fog_screen_blend_target;
            state->valid = 1;
        } else {
            value_step_toward_target(&state->start_distance, fog->start_distance, distance);
            value_step_toward_target(&state->opaque_distance, fog->opaque_distance, distance);
            distance = distance * 0.05f;
            value_step_toward_target(&state->maximum_density, fog->maximum_density, distance);
            render_lighting_step_vector3_toward((float *)&state->color, (float *)&fog->color, distance);
            value_step_toward_target(&state->fog_screen_blend, fog_screen_blend_target, distance);
        }

        state->camera_position.x = camera_position->x;
        state->camera_position.y = camera_position->y;
        state->camera_position.z = camera_position->z;
    }

    out->atmospheric_color = state->color;
    out->atmospheric_maximum_density = state->maximum_density;
    out->atmospheric_minimum_distance = state->start_distance;

    if (state->opaque_distance == 0.0f) {
        out->atmospheric_maximum_distance = 0.0f;
    } else {
        out->atmospheric_maximum_distance = state->start_distance + 0.0001f;
        if (out->atmospheric_maximum_distance < state->opaque_distance) {
            out->atmospheric_maximum_distance = state->opaque_distance;
        }
    }

    if (state->fog_screen_blend < 0.0f) {
        out->sky_fog_screen_blend = 0.0f; // mov DWORD [ecx+0x4c],0
    } else if (1.0f < state->fog_screen_blend) {
        out->sky_fog_screen_blend = 1.0f; // mov DWORD [ecx+0x4c],0x3f800000
    } else {
        out->sky_fog_screen_blend = state->fog_screen_blend; // clamped [0,1]
    }
}

#if 0
Original Ghidra decompilation (0x53e8c0):

void FUN_0053e8c0(short param_1,float *param_2,int param_3)

{
  float fVar1;
  short in_AX;
  uint uVar2;
  int iVar3;
  char *pcVar4;
  undefined4 *puVar5;
  undefined4 local_30;
  char local_2c [44];

  uVar2 = 0xffffffff;
  if (in_AX == -1) {
    if (0 < *(int *)(global_scenario + 0x30)) {
      uVar2 = *(uint *)(*(int *)(global_scenario + 0x34) + 0xc);
    }
  }
  else if ((-1 < in_AX) && ((int)in_AX < *(int *)(global_scenario + 0x30))) {
    uVar2 = *(uint *)(in_AX * 0x10 + 0xc + *(int *)(global_scenario + 0x34));
  }
  iVar3 = 0;
  if (uVar2 != 0xffffffff) {
    iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  }
  if (param_1 == -1) {
    pcVar4 = local_2c;
  }
  else {
    pcVar4 = (char *)(param_1 * 0x2c + 4 + DAT_00746f94);
  }
  if (iVar3 == 0) goto LAB_0053eab2;
  if (in_AX == -1) {
    puVar5 = (undefined4 *)(iVar3 + 0x78);
    uVar2 = 0xffffffff;
    if (0 < *(int *)(global_scenario + 0x30)) {
      uVar2 = *(uint *)(*(int *)(global_scenario + 0x34) + 0xc);
    }
    iVar3 = 0;
    if (uVar2 != 0xffffffff) {
      iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    }
    local_30 = 0x3f800000;
    if (*(int *)(iVar3 + 0xa4) == -1) goto LAB_0053e984;
  }
  else {
    puVar5 = (undefined4 *)(iVar3 + 0x58);
LAB_0053e984:
    local_30 = 0;
  }
  fVar1 = SQRT((param_2[1] - *(float *)(pcVar4 + 8)) * (param_2[1] - *(float *)(pcVar4 + 8)) +
               (param_2[2] - *(float *)(pcVar4 + 0xc)) * (param_2[2] - *(float *)(pcVar4 + 0xc)) +
               (*param_2 - *(float *)(pcVar4 + 4)) * (*param_2 - *(float *)(pcVar4 + 4)));
  if ((((param_1 == -1) || (15.0 <= fVar1)) || (*pcVar4 == '\0')) ||
     (((float)puVar5[7] == 0.0 || (*(float *)(pcVar4 + 0x14) == 0.0)))) {
    *(undefined4 *)(pcVar4 + 0x10) = puVar5[6];
    *(undefined4 *)(pcVar4 + 0x14) = puVar5[7];
    *(undefined4 *)(pcVar4 + 0x18) = puVar5[5];
    *(undefined4 *)(pcVar4 + 0x1c) = *puVar5;
    *(undefined4 *)(pcVar4 + 0x20) = puVar5[1];
    *(undefined4 *)(pcVar4 + 0x24) = puVar5[2];
    *(undefined4 *)(pcVar4 + 0x28) = local_30;
    *pcVar4 = '\x01';
  }
  else {
    FUN_00470d40(puVar5[6],fVar1);
    FUN_00470d40(puVar5[7],fVar1);
    fVar1 = fVar1 * 0.05;
    FUN_00470d40(puVar5[5],fVar1);
    FUN_0050f520(fVar1);
    FUN_00470d40(local_30,fVar1);
  }
  *(float *)(pcVar4 + 4) = *param_2;
  *(float *)(pcVar4 + 8) = param_2[1];
  *(float *)(pcVar4 + 0xc) = param_2[2];
LAB_0053eab2:
  *(undefined4 *)(param_3 + 4) = *(undefined4 *)(pcVar4 + 0x1c);
  *(undefined4 *)(param_3 + 8) = *(undefined4 *)(pcVar4 + 0x20);
  *(undefined4 *)(param_3 + 0xc) = *(undefined4 *)(pcVar4 + 0x24);
  *(undefined4 *)(param_3 + 0x10) = *(undefined4 *)(pcVar4 + 0x18);
  *(undefined4 *)(param_3 + 0x14) = *(undefined4 *)(pcVar4 + 0x10);
  if (*(float *)(pcVar4 + 0x14) == 0.0) {
    fVar1 = 0.0;
  }
  else {
    fVar1 = *(float *)(pcVar4 + 0x10) + 0.0001;
    if (fVar1 < *(float *)(pcVar4 + 0x14)) {
      fVar1 = *(float *)(pcVar4 + 0x14);
    }
  }
  *(float *)(param_3 + 0x18) = fVar1;
  if (*(float *)(pcVar4 + 0x28) < 0.0) {
    *(undefined4 *)(param_3 + 0x4c) = 0;
    return;
  }
  if (1.0 < *(float *)(pcVar4 + 0x28)) {
    *(undefined4 *)(param_3 + 0x4c) = 0x3f800000;
    return;
  }
  *(undefined4 *)(param_3 + 0x4c) = *(undefined4 *)(pcVar4 + 0x28);
  return;
}

Raw disassembly (0x53e8c0-0x53eb58) confirming the register/stack argument layout: entry loads
EDX = global_scenario, BP = [esp+0x3c] (first stack arg, after `sub esp,0x30` + 2 pushes ==
local_player_index), DI = AX (sky_index), and later `mov ecx,[esp+0x48]` /
`mov ecx,[esp+0x4c]` read the second and third stack args (camera_position, out) at the points
those are first used; the move_toward call sites (0x53ea0c-0x53ea5c) show ECX = destination
field address, EDX = target color pointer only for the 0x50f520 (color) call, and the pushed
stack value(s) as target/rate for 0x470d40, matching value_step_toward_target's and
render_lighting_step_vector3_toward's own established signatures.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
