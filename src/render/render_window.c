// render_window  (Ghidra: render_scene_draw, phase-2 name; CEA render_window(local_player_index,
// source_camera, source_frustum, rasterizer_camera, rasterizer_frustum, rasterizer_target,
// has_mirror), hint only; renamed)
// address 0x50bfb0, size 1498 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: objdump -d -M intel 0x50bfb0..0x50c589, traced with the push depth at every stack
//   reference. Ghidra's decompile has the right order of calls but drops every register
//   argument (EDI stage for 0x51a810, EBX / ECX for 0x457000, EAX / ECX for 0x552de0, AL for
//   0x494af0, AX for 0x519200 / 0x4984c0, EAX for 0x50eb70, EBX for 0x512530).
//   - seven cdecl stack arguments; the frame holds an object_render_data at esp+0x10 (the
//     shadow pass block) and the rasterizer_window_parameters at esp+0x58 (0x96 dword rep stosd).
//   - render_window_count (0x007c3104) += 1, render_local_player_index (0x007c3108) = arg 0 as a
//     word; source camera -> 0x007c3114 (0x15 dwords), source frustum -> 0x007c3168 (99 dwords);
//     the parameters get the rasterizer camera and frustum, type = rasterizer_target,
//     window_index = render_window_index (0x007c310a), +0x04 = has_mirror and the fog copied
//     from 0x007c32f4 (0x14 dwords).
//   - player_effect_build_screen_flash 0x457000: ECX = local player index, EBX =
//     &parameters.screen_flash (esp+0x290 = 0x58 + 0x238).
//   - the decal passes: rasterizer_decal_pass_begin 0x51a810 (EDI = stage), then
//     rasterizer_decals_draw_cluster 0x51aa50 (cdecl, zero extended cluster index) for every
//     visible cluster (0x007c3390 stride 0x1a0, count 0x007d0390), then
//     rasterizer_end_decal_pass 0x51b0e0 -- except the stage 1 and stage 4 passes, which end
//     with the fog / depth bias state reset instead (FOGENABLE 0, and DEPTHBIAS 0xc3 /
//     SLOPESCALEDEPTHBIAS 0xaf to 0 when rasterizer_caps.raster_caps has 0x04000000 /
//     0x02000000, and rasterizer_set_shader_stage_config(2) when rasterizer_decal_layer
//     (0x006d98dc) is 3). Stages 2 and 0 / 1 only run when the decal toggles allow
//     (word 0x006893e4 == 0 and byte 0x006893f5 != 0).
//   - the structure passes: structure_leaf_faces_for_each 0x552de0 (EAX = visible_surface_count
//     0x00850394, ECX = visible_surface_indices 0x00850398, stack (picked_surfaces_geometry
//     0x006e3adc, lightmap_begin, material, lightmap_end, transparent)) with the callbacks at
//     0x511f70..0x5120c0 and the empty function 0x44ad80, each gated on picked_surfaces_valid
//     (0x006e3ad8) re-read after every pass. The 0x0069c67c word is forced to 1 around the
//     first lit pass when the BSP has no lightmaps bitmap (+0x0c tag id -1) and restored after.
//   - render_object_shadows 0x50eb70 (EAX = the shadow block: object -1, lighting 0,
//     shadow_pass 1, +0x09 1, +0x40 0, +0x44 -1) when object shadows (0x0069c614) are on.
//   - current_game_engine->post_rasterize (+0x34) when both are non NULL.
//   - render_contrails(0xfffffff3) (cdecl), transparent_geometry_group_draw_all(1) and (0).
//   - at the end, for a window whose window_index is -1 (0x007c1222),
//     rasterizer_frame_statistics_sample (EBX = &rasterizer_frame_statistics_state, stack 0)
//     and rasterizer_frame_statistics_draw.
// register convention: cdecl, seven stack arguments.
//   // blam-cc: cdecl
// UNSURE: the callbacks at 0x511f70..0x5120c0 are small trampolines Ghidra never created as
//   functions (0x511f70 jumps through the pointer at 0x007c0490, 0x512070 to 0x5202f0, the
//   others forward their arguments to 0x521f90 / 0x520910 / 0x520970 / 0x520e50 / 0x522300 /
//   0x51ee60); they are declared, not written. Callback argument types follow
//   types/structures.h. The meaning of 0x0069c67c is open.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "structures.h"
#include "game.h"
#include "render.h"

extern int32_t render_window_count;                   // 0x007c3104, this module
extern int16_t render_local_player_index;             // 0x007c3108, this module
extern int16_t render_window_index;                   // 0x007c310a, this module
extern render_camera render_camera_global;            // 0x007c3114, this module
extern render_frustum render_frustum_global;          // 0x007c3168, this module
extern render_fog render_fog_state;                   // 0x007c32f4, this module
extern rasterizer_frame_statistics rasterizer_frame_statistics_state; // 0x007c30a0, this module
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220, rasterizer module
extern uint8_t console_debug_toggle_69c614;           // 0x0069c614 object shadows enabled
extern int16_t console_debug_toggle_6893e4;           // 0x006893e4 (read as a word here)
extern uint8_t console_debug_toggle_6893f5;           // 0x006893f5 decals enabled
extern int16_t visible_cluster_count;                 // 0x007d0390, structures module
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters]; // 0x007c3390
extern int16_t visible_surface_count;                 // 0x00850394, structures module
extern int32_t visible_surface_indices[0x4000];       // 0x00850398, structures module
extern uint8_t picked_surfaces_valid;                 // 0x006e3ad8, structures module
extern int32_t picked_surfaces_geometry;              // 0x006e3adc, structures module
extern ScenarioStructureBSP *global_structure_bsp;           // 0x00746f9c
extern int16_t renderer_unknown_69c67c;               // 0x0069c67c UNSURE (read as a word)
extern uint32_t rasterizer_active_environment_effect; // 0x0071d1d0 rasterizer_effect_slot*
extern int32_t transparent_geometry_group_last_drawn_key; // 0x006e1d58, rasterizer module
extern uint8_t rasterizer_secondary_groups_drawn;     // 0x0071d274, rasterizer module
extern int16_t rasterizer_decal_layer;                // 0x006d98dc, rasterizer module
extern d3d_caps9 rasterizer_caps;                     // 0x007c10c0, rasterizer module
extern void *rasterizer_device;                       // 0x0071d174
extern game_engine_definition *current_game_engine;   // 0x006f1d20, game module

extern void structure_bsp_cluster_visibility_update(void); // 0x5537c0, structures module
extern void player_effect_build_screen_flash(render_screen_flash *out, int16_t local_player_index);
    // 0x457000, effects module; blam-cc: EBX -> out, CX -> local_player_index
extern void rasterizer_begin_frame(rasterizer_window_parameters *source); // 0x5175c0 (cdecl)
extern void first_person_weapon_update_zoom_static_tint(uint8_t enabled); // 0x494af0; AL
extern void billboard_system_frame_init(void);        // 0x511410, this module
extern void render_sky(void);                         // 0x510c50, this module
extern void first_person_weapon_update_active_state(void); // 0x492430, interface module
extern void object_lights_update_all(void);           // 0x4f0cf0, objects module
extern void render_objects(void);                     // 0x50e930, this module
extern void structure_picked_polygon_refresh(void);   // 0x5527f0, structures module
extern void structure_picked_polygon_draw(void);      // 0x5528f0, structures module
extern void lens_flare_update_samples(void);          // 0x513ba0, rasterizer module
extern void render_object_shadows(object_render_data *data); // 0x50eb70, this module; EAX
extern void lights_apply_spot_falloff(void);          // 0x4f1780, objects module
extern void lights_apply_spot_falloff_specular(void); // 0x4f1950, objects module
extern void rasterizer_decal_pass_begin(int16_t stage); // 0x51a810; blam-cc: DI -> stage
extern void rasterizer_decals_draw_cluster(int16_t cluster_index); // 0x51aa50 (cdecl)
extern void rasterizer_end_decal_pass(void);          // 0x51b0e0
extern void rasterizer_fog_screen_overlay_set_states(void); // 0x51def0
extern void rasterizer_force_bilinear_filtering(void); // 0x51e9f0
extern void rasterizer_dynamic_light_technique_ps2_set_states(void); // 0x521cc0
extern void rasterizer_shader_environment_technique_multipurpose_set_states(void); // 0x520790
extern void rasterizer_shader_environment_technique_self_illumination_set_states(void); // 0x520b90
extern void rasterizer_shader_decal_pass_set_states(void); // 0x520020
extern void rasterizer_water_fade_compute_and_set_states(void); // 0x51eb20
extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200; blam-cc: AX -> mode
extern void structure_leaf_faces_for_each(int32_t render_context,
    structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb,
    structure_lightmap_end_callback lightmap_end,
    structure_transparent_material_callback transparent_material_cb,
    int32_t *surface_indices, int16_t surface_index_count);
    // 0x552de0, structures module; blam-cc: ECX -> surface_indices,
    // EAX -> surface_index_count, stack -> the first five
extern void weather_update_local_player(void);        // 0x458a90, effects module
extern void render_particles(void);                   // 0x50fd90, this module
extern void particle_systems_render(void);            // 0x454b40, effects module
extern void render_contrails(uint32_t render_type_flags); // 0x50df20, this module (cdecl here)
extern void transparent_geometry_group_draw_all(uint8_t resort); // 0x5154a0 (cdecl)
extern void detail_objects_update_render_list(void);  // 0x5522d0, structures module
extern void lens_flare_render_all(void);              // 0x513cf0, rasterizer module
extern void first_person_weapon_update_screen_effects(void); // 0x494730, interface module
extern void rasterizer_screen_flash_render(void);     // 0x52ed00, rasterizer module
extern void widget_draw_fullscreen_region(int16_t controller_index); // 0x4984c0; AX
extern void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics,
                                               uint8_t dropped);
    // 0x512530, this module; blam-cc: EBX -> statistics, stack -> dropped
extern void rasterizer_frame_statistics_draw(void);   // 0x512e80, this module

// structure pass callbacks (code in this range that Ghidra never made functions; see header)
extern void render_window_structure_material_0x511f70(void *shader_data,
    int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count); // jmp [0x007c0490]
extern void render_window_structure_lightmap_begin_0x511f90(void *bitmap_data);
extern void render_window_structure_material_0x511fe0(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count);
extern void render_window_structure_lightmap_begin_0x512010(void *bitmap_data);
extern void render_window_structure_material_0x512020(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count);
extern void render_window_structure_material_0x512040(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count);
extern void render_window_structure_material_0x512070(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count);
extern void render_window_structure_transparent_0x512080(void *shader_data,
    int16_t shader_permutation, int32_t render_context, int32_t first_surface,
    int32_t surface_count);
extern void render_window_structure_material_0x5120c0(void *shader_data, int16_t shader_permutation,
    int32_t render_context, int32_t first_surface, int32_t surface_count);
extern void function_do_nothing(void);                 // 0x44ad80: a bare ret

typedef int32_t (__stdcall *d3d_call2_fn)(void *self, uint32_t a, uint32_t b);

static void set_render_state(uint32_t state, uint32_t value)
{
    ((d3d_call2_fn)(*(void ***)rasterizer_device)[0xe4 / 4])(rasterizer_device, state, value);
}

static void draw_visible_cluster_decals(void)
{
    int16_t i;

    for (i = 0; i < visible_cluster_count; i++) {
        rasterizer_decals_draw_cluster((int16_t)(uint16_t)visible_clusters[i].cluster_index);
    }
}

// The fog and depth bias reset that closes the stage 1 and stage 4 decal passes.
static void reset_decal_fog_and_depth_bias(void)
{
    set_render_state(0x1c, 0); // D3DRS_FOGENABLE
    if ((rasterizer_caps.raster_caps & 0x04000000) != 0) {
        set_render_state(0xc3, 0); // D3DRS_DEPTHBIAS
    }
    if ((rasterizer_caps.raster_caps & 0x02000000) != 0) {
        set_render_state(0xaf, 0); // D3DRS_SLOPESCALEDEPTHBIAS
    }
    if (rasterizer_decal_layer == 3) {
        rasterizer_set_shader_stage_config(2);
    }
}

static void structure_pass(structure_lightmap_begin_callback lightmap_begin,
                           structure_material_callback material_cb,
                           structure_lightmap_end_callback lightmap_end,
                           structure_transparent_material_callback transparent_material_cb)
{
    structure_leaf_faces_for_each(picked_surfaces_geometry, lightmap_begin, material_cb,
                                  lightmap_end, transparent_material_cb, visible_surface_indices,
                                  visible_surface_count);
}

// Draws one window: publishes the cameras and frustum for the rest of the renderer, begins the
// rasterizer frame, then runs every scene pass in order (sky, lights, objects, shadows, decals,
// structure lightmap / environment / decal / water passes, game engine overlay, weather,
// particles, contrails, transparent geometry, detail objects, lens flares, first person screen
// effects, screen flash, fullscreen widgets) and, in the extra window, the frame statistics.
void render_window(int16_t local_player_index, render_camera *source_camera,
                   render_frustum *source_frustum, render_camera *rasterizer_camera,
                   render_frustum *rasterizer_frustum, int16_t rasterizer_target,
                   uint8_t has_mirror)
{
    object_render_data shadow_data;
    rasterizer_window_parameters parameters;
    uint8_t *raw;
    uint32_t i;
    int16_t saved_69c67c;

    render_window_count++;
    render_local_player_index = local_player_index;
    raw = (uint8_t *)&parameters;
    for (i = 0; i < sizeof(parameters); i++) {
        raw[i] = 0;
    }
    render_camera_global = *source_camera;
    render_frustum_global = *source_frustum;
    parameters.camera = *rasterizer_camera;
    parameters.frustum = *rasterizer_frustum;
    parameters.unknown_04 = has_mirror;
    parameters.type = rasterizer_target;
    parameters.window_index = render_window_index;
    parameters.fog = render_fog_state;

    structure_bsp_cluster_visibility_update();
    player_effect_build_screen_flash(&parameters.screen_flash, local_player_index);
    rasterizer_begin_frame(&parameters);
    first_person_weapon_update_zoom_static_tint(1);
    billboard_system_frame_init();
    render_sky();
    first_person_weapon_update_active_state();
    object_lights_update_all();
    render_objects();
    structure_picked_polygon_refresh();
    structure_picked_polygon_draw();
    lens_flare_update_samples();
    if (console_debug_toggle_69c614) {
        shadow_data.object_index = 0xffffffff;
        shadow_data.unknown_44 = -1;
        shadow_data.lighting = 0;
        shadow_data.shadow_pass = 1;
        shadow_data.outside_fog_plane = 1;
        shadow_data.shadow_radius = 0.0f;
        render_object_shadows(&shadow_data);
    }
    lights_apply_spot_falloff();

    if (console_debug_toggle_6893e4 == 0 && console_debug_toggle_6893f5) {
        rasterizer_decal_pass_begin(2);
        draw_visible_cluster_decals();
        rasterizer_end_decal_pass();
    }
    rasterizer_decal_pass_begin(3);
    draw_visible_cluster_decals();
    rasterizer_end_decal_pass();

    if (picked_surfaces_valid) {
        rasterizer_fog_screen_overlay_set_states();
        structure_pass(0, (structure_material_callback)render_window_structure_material_0x511f70,
                       0, 0);
        rasterizer_force_bilinear_filtering();
    }

    if (console_debug_toggle_6893e4 == 0 && console_debug_toggle_6893f5) {
        rasterizer_decal_pass_begin(0);
        draw_visible_cluster_decals();
        rasterizer_end_decal_pass();
        rasterizer_decal_pass_begin(1);
        draw_visible_cluster_decals();
        reset_decal_fog_and_depth_bias();
    }
    lights_apply_spot_falloff_specular();

    if (picked_surfaces_valid) {
        saved_69c67c = renderer_unknown_69c67c;
        // the BSP has no lightmaps bitmap (lightmaps_bitmap.tag_id, +0x0c, is -1)
        if (*(int32_t *)&global_structure_bsp->lightmaps_bitmap.tag_id == -1 && saved_69c67c == 0) {
            renderer_unknown_69c67c = 1;
        }
        rasterizer_dynamic_light_technique_ps2_set_states();
        structure_pass(render_window_structure_lightmap_begin_0x511f90,
                       (structure_material_callback)render_window_structure_material_0x511fe0,
                       (structure_lightmap_end_callback)function_do_nothing, 0);
        renderer_unknown_69c67c = saved_69c67c;
        if (picked_surfaces_valid) {
            rasterizer_shader_environment_technique_multipurpose_set_states();
            structure_pass(render_window_structure_lightmap_begin_0x512010,
                           (structure_material_callback)render_window_structure_material_0x512020,
                           (structure_lightmap_end_callback)function_do_nothing, 0);
            rasterizer_active_environment_effect = 0;
            if (picked_surfaces_valid) {
                rasterizer_shader_environment_technique_self_illumination_set_states();
                structure_pass(0, (structure_material_callback)render_window_structure_material_0x512040,
                               0, 0);
                if (picked_surfaces_valid) {
                    rasterizer_shader_decal_pass_set_states();
                    structure_pass(0,
                        (structure_material_callback)render_window_structure_material_0x512070, 0, 0);
                    if (picked_surfaces_valid) {
                        transparent_geometry_group_last_drawn_key = 0;
                        rasterizer_secondary_groups_drawn = 0;
                        structure_pass(0, 0, 0,
                            (structure_transparent_material_callback)
                                render_window_structure_transparent_0x512080);
                        if (picked_surfaces_valid) {
                            rasterizer_water_fade_compute_and_set_states();
                            structure_pass(0,
                                (structure_material_callback)render_window_structure_material_0x5120c0,
                                0, 0);
                        }
                    }
                }
            }
        }
    }

    if (current_game_engine != 0 && current_game_engine->post_rasterize != 0) {
        ((void (*)(void))current_game_engine->post_rasterize)();
    }
    weather_update_local_player();
    render_particles();
    particle_systems_render();
    render_contrails(0xfffffff3);
    transparent_geometry_group_draw_all(1);

    rasterizer_decal_pass_begin(4);
    draw_visible_cluster_decals();
    reset_decal_fog_and_depth_bias();
    detail_objects_update_render_list();
    transparent_geometry_group_draw_all(0);
    rasterizer_set_shader_stage_config(0);

    if (picked_surfaces_valid) {
        structure_pass(0, (structure_material_callback)function_do_nothing, 0, 0);
        structure_pass(0, (structure_material_callback)function_do_nothing, 0, 0);
    }
    lens_flare_render_all();
    first_person_weapon_update_screen_effects();
    rasterizer_screen_flash_render();
    widget_draw_fullscreen_region(local_player_index);
    if (rasterizer_window.window_index == -1) {
        rasterizer_frame_statistics_sample(&rasterizer_frame_statistics_state, 0);
        rasterizer_frame_statistics_draw();
    }
}

#if 0
Original Ghidra decompilation (0x50bfb0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void render_scene_draw(undefined2 param_1,undefined4 *param_2,undefined4 *param_3,
                      undefined4 *param_4,undefined4 *param_5,short param_6,undefined1 param_7)

{
  int iVar1;
  short sVar2;
  undefined4 *puVar3;
  short *psVar4;
  undefined4 *puVar5;
  short local_258;
  undefined2 local_256;
  undefined1 local_254;
  undefined4 local_250 [21];
  undefined4 local_1fc [99];
  undefined4 local_70 [28];
  
  DAT_007c3104 = DAT_007c3104 + 1;
  DAT_007c3108 = param_1;
  psVar4 = &local_258;
  for (iVar1 = 0x96; iVar1 != 0; iVar1 = iVar1 + -1) {
    psVar4[0] = 0;
    psVar4[1] = 0;
    psVar4 = psVar4 + 2;
  }
  puVar3 = &DAT_007c3114;
  for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *param_2;
    param_2 = param_2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar3 = &DAT_007c3168;
  for (iVar1 = 99; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *param_3;
    param_3 = param_3 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar3 = local_250;
  for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *param_4;
    param_4 = param_4 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar3 = local_1fc;
  for (iVar1 = 99; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *param_5;
    param_5 = param_5 + 1;
    puVar3 = puVar3 + 1;
  }
  local_254 = param_7;
  local_258 = param_6;
  local_256 = _DAT_007c310a;
  puVar3 = &DAT_007c32f4;
  puVar5 = local_70;
  for (iVar1 = 0x14; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar5 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar5 = puVar5 + 1;
  }
  FUN_005537c0();
  FUN_00457000();
  rasterizer_begin_frame(&local_258);
  FUN_00494af0();
  FUN_00511410();
  sky_render_lights_and_lens_flares();
  FUN_00492430();
  object_lights_update_all();
  FUN_0050e930();
  FUN_005527f0();
  FUN_005528f0();
  FUN_00513ba0();
  if (DAT_0069c614 != '\0') {
    FUN_0050eb70();
  }
  lights_apply_spot_falloff();
  if ((DAT_006893e4 == 0) && (DAT_006893f5 != '\0')) {
    FUN_0051a810();
    sVar2 = 0;
    if (0 < DAT_007d0390) {
      do {
        FUN_0051aa50((&DAT_007c3390)[sVar2 * 0xd0]);
        sVar2 = sVar2 + 1;
      } while (sVar2 < DAT_007d0390);
    }
    FUN_0051b0e0();
  }
  FUN_0051a810();
  sVar2 = 0;
  if (0 < DAT_007d0390) {
    do {
      FUN_0051aa50((&DAT_007c3390)[sVar2 * 0xd0]);
      sVar2 = sVar2 + 1;
    } while (sVar2 < DAT_007d0390);
  }
  FUN_0051b0e0();
  if (DAT_006e3ad8 != '\0') {
    FUN_0051def0();
    FUN_00552de0(DAT_006e3adc,0,&DAT_00511f70,0,0);
    FUN_0051e9f0();
  }
  if ((DAT_006893e4 == 0) && (DAT_006893f5 != '\0')) {
    FUN_0051a810();
    sVar2 = 0;
    if (0 < DAT_007d0390) {
      do {
        FUN_0051aa50((&DAT_007c3390)[sVar2 * 0xd0]);
        sVar2 = sVar2 + 1;
      } while (sVar2 < DAT_007d0390);
    }
    FUN_0051b0e0();
    FUN_0051a810();
    sVar2 = 0;
    if (0 < DAT_007d0390) {
      do {
        FUN_0051aa50((&DAT_007c3390)[sVar2 * 0xd0]);
        sVar2 = sVar2 + 1;
      } while (sVar2 < DAT_007d0390);
    }
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
    if ((_DAT_007c10e4 & 0x4000000) != 0) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,0);
    }
    if ((_DAT_007c10e4 & 0x2000000) != 0) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,0);
    }
    if (DAT_006d98dc == 3) {
      rasterizer_set_shader_stage_config();
    }
  }
  lights_apply_spot_falloff_specular();
  if (DAT_006e3ad8 != '\0') {
    sVar2 = (short)_DAT_0069c67c;
    if ((*(int *)(DAT_00746f9c + 0xc) == -1) && (sVar2 == 0)) {
      _DAT_0069c67c = CONCAT22(DAT_0069c67e,1);
    }
    FUN_00521cc0();
    FUN_00552de0(DAT_006e3adc,&LAB_00511f90,&LAB_00511fe0,FUN_0044ad80,0);
    _DAT_0069c67c = CONCAT22(DAT_0069c67e,sVar2);
    if (DAT_006e3ad8 != '\0') {
      FUN_00520790();
      FUN_00552de0(DAT_006e3adc,&LAB_00512010,&LAB_00512020,FUN_0044ad80,0);
      DAT_0071d1d0 = 0;
      if (DAT_006e3ad8 != '\0') {
        FUN_00520b90();
        FUN_00552de0(DAT_006e3adc,0,&LAB_00512040,0,0);
        if (DAT_006e3ad8 != '\0') {
          FUN_00520020();
          FUN_00552de0(DAT_006e3adc,0,&LAB_00512070,0,0);
          if (DAT_006e3ad8 != '\0') {
            DAT_006e1d58 = 0;
            DAT_0071d274 = 0;
            FUN_00552de0(DAT_006e3adc,0,0,0,&LAB_00512080);
            if (DAT_006e3ad8 != '\0') {
              FUN_0051eb20();
              FUN_00552de0(DAT_006e3adc,0,&LAB_005120c0,0,0);
            }
          }
        }
      }
    }
  }
  if ((DAT_006f1d20 != 0) && (*(code **)(DAT_006f1d20 + 0x34) != (code *)0x0)) {
    (**(code **)(DAT_006f1d20 + 0x34))();
  }
  FUN_00458a90();
  contrail_render_all_active();
  FUN_00454b40();
  contrail_render_by_object_type_mask(0xfffffff3);
  transparent_geometry_group_draw_all(1);
  FUN_0051a810();
  sVar2 = 0;
  if (0 < DAT_007d0390) {
    do {
      FUN_0051aa50((&DAT_007c3390)[sVar2 * 0xd0]);
      sVar2 = sVar2 + 1;
    } while (sVar2 < DAT_007d0390);
  }
  (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x1c,0);
  if ((_DAT_007c10e4 & 0x4000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xc3,0);
  }
  if ((_DAT_007c10e4 & 0x2000000) != 0) {
    (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0xaf,0);
  }
  if (DAT_006d98dc == 3) {
    rasterizer_set_shader_stage_config();
  }
  FUN_005522d0();
  transparent_geometry_group_draw_all(0);
  rasterizer_set_shader_stage_config();
  if (DAT_006e3ad8 != '\0') {
    FUN_00552de0(DAT_006e3adc,0,FUN_0044ad80,0,0);
    FUN_00552de0(DAT_006e3adc,0,FUN_0044ad80,0,0);
  }
  decal_render_active_list();
  FUN_00494730();
  rasterizer_screen_flash_render();
  FUN_004984c0();
  if (DAT_007c1220._2_2_ == -1) {
    rasterizer_frame_statistics_sample(0);
    rasterizer_frame_statistics_draw();
  }
  return;
}
#endif
