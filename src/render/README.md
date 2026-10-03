# `render` — the scene driver between main and the rasterizer

Retail Halo PC `halo.exe` 1.0.10, `0x50ba80 .. 0x512e80` (the function list also carries a bogus
`.data` entry at `0x6b4c00`), plain C / MSVC 7.1 (cl 13.10.3077, LTCG) / x86. Every `.c` file here
is one function, rewritten against `types/render.h` (plus the camera, frustum, fog and lighting
records of `types/rasterizer.h`) from its Ghidra decompile and, wherever the decompile was wrong or
incomplete, from `objdump -d -M intel` of `bin/halo.exe`. Each file keeps the original Ghidra
output at the bottom inside `#if 0 ... #endif` (for the one function Ghidra never created,
`render_object_get_cull_sphere`, the disassembly it was written from).

Gate: `python tools/build_check.py render` gives **66 ok, 0 failed** (66 files).
`out/phase4/render_smoke.c` (the layout asserts of `types/render.h`) still compiles.

Of the 78 entries in `out/phase4/render_functions.md`, 65 are real module functions and all 65
are written; the 66th file is the object cull-sphere callback at `0x50e8d0`, which Ghidra never
made a function. The other 13 entries are library code or not functions (see
[Misattributed entries](#misattributed-entries)).

## What the module contains

| Family | Range | What it is |
|---|---|---|
| frame and window drivers | `0x50ba80`–`0x50c65f` | `render_frame` walks the split screen `render_view` array; `render_player_frame` resolves fog and clip planes, builds both frustums and draws the mirror pass then the main pass; `render_nonplayer_frame` / `render_pregame_frame` draw letterbox, loading screen and console; `render_window` is the per window pass list (sky, lights, objects, shadows, decals, structure passes, weather, particles, contrails, transparent geometry, lens flares, screen flash, frame statistics) |
| camera and frustum maths | `0x50c660`–`0x50df1f` | `render_camera_mirror` (reflection or portal shift), projection skew / bounds, `chimera__render_camera_build_frustum` (view basis, world planes, far corners, bounds, oblique projection), point / box / sphere culling, the box screen coverage used by the sprite guard, world to screen projection |
| contrails | `0x50df20`–`0x50e92f` | `render_contrails` selects Contrail render types, `render_contrail` builds a vertex pair per point (vertical, horizontal or viewer facing) and queues one transparent draw |
| objects | `0x50e8d0`–`0x50fd8f` | `render_objects` / `render_objects_collect` (visible cluster object list), `render_object` (cached lighting, fog plane test, the fake blob shadow path), `render_object_list` (recursive model draw with `render_model_effect`), the `cached object render states` data array (`object_get_cached_render_state`, `object_render_state_refresh` and three smoothing steppers), shadow begin / end, level of detail pixels |
| particles | `0x50fd90`–`0x510c4f` | `render_particles` collects, sorts (the `std::sort` instantiation at `0x510410..0x510ba0`), groups and builds sprites |
| sky | `0x510c50`–`0x51118f` | `render_sky`: animates and draws the cluster sky model at 1/1024 scale and registers its lens flare lights |
| sprites | `0x511190`–`0x511d7f` | the `build_sprite` family: transform, orientation basis, scale, view fade, per window init, group allocation, quad build, rotational (axis) sprites, end / submit |
| rasterizer side helpers | `0x511d80`–`0x51219f` | device ready, initialise, cinematic effect time hook, index slot lock, lighting workaround, lens flare key / specular, effect slot release, a thunk |
| cinematic screen effect | `0x5121a0`–`0x51252f` | script value getter, the three setters, the per frame update |
| frame statistics | `0x512530`–`0x5132a7` | the 60 frame history, the FPS graph (`fg_init`, `fg_add_sample`, `fg_render`) and the text table |

Global names: C forbids a global named like its typedef, so the render camera, frustum and fog
at `0x007c3114` / `0x007c3168` / `0x007c32f4` are `render_camera_global`,
`render_frustum_global` and `render_fog_state`; the cinematic block pointer at `0x0071cfc4` is
`cinematic_screen_effect_state` and the statistics block at `0x007c30a0` is
`rasterizer_frame_statistics_state`. Constant pointers of the math table (`types/math.h`
`global_math_constant_pointers`) keep the `_pointer` suffix (`global_up3d_pointer` 0x00696720,
`global_forward3d_pointer` 0x00696718, `global_identity4x3_pointer` 0x0069673c,
`global_null_rectangle3d_pointer` 0x00696748, `global_zero_vector3d_pointer` 0x006966f8,
`global_real_rgb_white_pointer` 0x00686b04, `global_real_rgb_black_pointer` 0x00686b0c,
`global_real_rgb_green_pointer` 0x00686b14, `global_real_argb_white_pointer` 0x006851fc).

## Struct layouts

All in `types/render.h`; offsets are bytes from the struct base, `#pragma pack(1)`. Pointer
fields are `uint32_t` with the pointee in the comment, so the 32 bit sizes survive the 64 bit
gate compiler. The camera (`render_camera`, 0x54), frustum (`render_frustum`, 0x18c), fog
(`render_fog`, 0x50), lighting (`render_lighting`, 0x74) and window parameter (0x258) records
belong to `types/rasterizer.h` and are not repeated here.

### `render_view` — size `0xac`, element of `render_views[2]` at `0x00719b70` (main owns it)

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `local_player_index` (-1: no player) |
| `0x02` | `uint8_t` | `nonplayer` (1 forces `render_nonplayer_frame` with type 0) |
| `0x03` | `uint8_t` | `unknown_03` (never written) |
| `0x04` | `render_camera` | `source_camera` (culling camera, copied to `0x007c3114`) |
| `0x58` | `render_camera` | `rasterizer_camera` |

### `render_model_effect` — size `0x28`, stack block of `render_object_list`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `type` (0 none, 1 active camouflage, 2 self occlusion; type 2 is not inherited) |
| `0x02` | `int16_t` | `unknown_02` (copied only) |
| `0x04` | `float` | `unit_37c` (unit +0x37c, type 1) |
| `0x08` | `float` | `unit_380` (unit +0x380, type 1) |
| `0x0c` | `datum_index` | `object_index` |
| `0x10` | `real_point3d` | `centroid` (object bounding centre) |
| `0x1c` | `uint32_t` | `modifier_shader` (Shader*, only for shader types 1 and 5..11) |
| `0x20` | `uint32_t` | `change_colors` (ColorRGB*, object +0x1b8) |
| `0x24` | `uint32_t` | `function_values` (float*, object +0x134) |

### `render_animation` — size `0x08`, EBX block of `widget_list_notify` (new in this review)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `change_colors` (ColorRGB*, object +0x1b8) |
| `0x04` | `uint32_t` | `function_values` (float*, object +0x134) |

### `object_render_data` — size `0x48`, EDI block of `render_object`

| Off | Type | Field |
|---|---|---|
| `0x00` | `datum_index` | `object_index` |
| `0x04` | `uint32_t` | `lighting` (render_lighting*, 0 when unlit) |
| `0x08` | `uint8_t` | `shadow_pass` (1: fake blob shadow path, render_model flags 2) |
| `0x09` | `uint8_t` | `outside_fog_plane` (render_model flag 4) |
| `0x0a` | `uint8_t[2]` | `unknown_0a` |
| `0x0c` | `real_matrix4x3` | `shadow_matrix` (forward = perpendicular, up = lighting.shadow_vector, position = object centre) |
| `0x40` | `float` | `shadow_radius` (written by `rasterizer_object_shadow_begin` 0x530ff0) |
| `0x44` | `int32_t` | `unknown_44` (-1 in the shadow pass block) |

### `cached_object_render_state` — size `0x100`, element of `cached object render states` (`0x007c30ec`, 0x100 entries)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint16_t` | `identifier` |
| `0x02` | `uint16_t` | `unknown_02` |
| `0x04` | `datum_index` | `object_index` |
| `0x08` | `int32_t` | `last_sample_frame` (render_frame_index) |
| `0x0c` | `int32_t` | `last_update_window` (render_window_count; also the eviction age) |
| `0x10` | `int32_t` | `last_update_frame` |
| `0x14` | `render_lighting` | `lighting` (smoothed, handed to render_model) |
| `0x88` | `render_lighting` | `desired_lighting` (latest sample) |
| `0xfc` | `float` | `level_of_detail_pixels` |

### `rendered_particle_datum` — size `0x08`, `render_particles` stack array of 0x400

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint16_t` | `particle_index` |
| `0x02` | `uint16_t` | `definition_index` (sort key 1) |
| `0x04` | `int16_t` | `cluster_index` (sort key 2) |
| `0x06` | `uint8_t` | `first_person` (sort key 3) |
| `0x07` | `uint8_t` | `unknown_07` |

### `build_sprite_group` — size `0x10`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int32_t` | `vertex_slot` (dynamic vertex slot, -1 on failure) |
| `0x04` | `uint32_t` | `vertices` (rasterizer_dynamic_screen_vertex*, NULL makes the group unusable) |
| `0x08` | `int16_t` | `quad_count` |
| `0x0a` | `int16_t` | `unknown_0a` |
| `0x0c` | `uint32_t` | `bitmap` (BitmapData*, the group key) |

### `build_sprite_data` — size `0xa4`

| Off | Type | Field |
|---|---|---|
| `0x00` | `datum_index` | `bitmap_group_index` (Bitmap tag) |
| `0x04` | `int16_t` | `maximum_sprite_count` |
| `0x06` | `int16_t` | `unknown_06` |
| `0x08` | `uint32_t` | `shader` (the particle shader block; +0x28 flags, +0x2a blend function, +0x2c fade mode; +0x98 receives the average radius) |
| `0x0c` | `int16_t` | `sprite_count` |
| `0x0e` | `int16_t` | `unknown_0e` |
| `0x10` | `uint32_t` | `flags` (1 screen space, 2 first person run / draw flag 0x80, 4 particles) |
| `0x14` | `real_point3d` | `centroid` (view space sum, world space mean after `build_sprites_end`) |
| `0x20` | `int16_t` | `group_count` (at most 8) |
| `0x22` | `int16_t` | `unknown_22` |
| `0x24` | `build_sprite_group[8]` | `groups` |

### `billboard_basis` — size `0x28`, `build_sprite` stack block (folded in from a local typedef)

| Off | Type | Field |
|---|---|---|
| `0x00` | `uint32_t` | `unused_00` |
| `0x04` | `real_vector3d` | `tangent` |
| `0x10` | `real_vector3d` | `bitangent` |
| `0x1c` | `real_vector3d` | `normal` (mode 2, or tangent x bitangent for the view fade) |

### `cinematic_screen_effect_globals` — size `0x78`, game state block at `*0x0071cfc4`

| Off | Type | Field |
|---|---|---|
| `0x00` | `int16_t` | `convolution_extra_passes` |
| `0x02` | `int16_t` | `convolution_type` |
| `0x04` | `float` | `convolution_radius` |
| `0x08` | `uint32_t` | `unknown_08` (the weapon block keeps its mask BitmapData* here) |
| `0x0c` | `float` | `filter_light_enhancement_intensity` |
| `0x10` | `float` | `filter_desaturation_intensity` |
| `0x14` | `ColorRGB` | `filter_desaturation_tint` (black means unset; replaced by green) |
| `0x20` | `uint8_t` | `filter_desaturation_is_additive` |
| `0x21` | `uint8_t` | `unknown_21` (weapon block: night vision masked) |
| `0x22` | `uint8_t` | `unknown_22` (weapon block: desaturation masked) |
| `0x23` | `uint8_t` | `video_enabled` |
| `0x24` | `int16_t` | `video_overbright_mode` |
| `0x26` | `int16_t` | `unknown_26` |
| `0x28` | `uint32_t` | `video_scanline_map` (BitmapData*) |
| `0x2c` | `float` | `video_noise_intensity` |
| `0x30` | `float` | `unknown_30` (1.0 from the video setter) |
| `0x34` | `uint32_t` | `video_noise_map` (BitmapData*) |
| `0x38` | `uint8_t` | `active` |
| `0x39` | `uint8_t` | `initialized` |
| `0x3a` | `int16_t` | `unknown_3a` |
| `0x3c` / `0x40` | `float` | `convolution_radius_lower_bound` / `_upper_bound` |
| `0x44` / `0x48` | `float` | `convolution_start_time` / `_end_time` (seconds) |
| `0x4c` / `0x50` | `float` | `filter_light_enhancement_intensity_lower_bound` / `_upper_bound` |
| `0x54` / `0x58` | `float` | `filter_desaturation_intensity_lower_bound` / `_upper_bound` |
| `0x5c` / `0x60` | `float` | `filter_start_time` / `_end_time` |
| `0x64` | `float[4]` | `script_values` |
| `0x74` | `float` | `near_clip_distance` |

The first 0x38 bytes are the layout of `types/interface.h` `weapon_screen_effect_parameters`
(the block `cinematic_screen_effect_update` hands back when no cinematic effect is active).

### `rasterizer_frame_statistics` — size `0x18`, at `0x007c30a0`

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `framerate` |
| `0x04` | `int16_t` | `sample_count` |
| `0x06` | `int16_t` | `unknown_06` |
| `0x08` | `float` | `average_framerate` |
| `0x0c` | `float` | `minimum_framerate` (longest interval) |
| `0x10` | `float` | `maximum_framerate` (shortest interval) |
| `0x14` | `float` | `dropped_percentage` |

### `frame_graph` — size `0x32b0`, `frame_graphs[1]` at `0x006b9260`

| Off | Type | Field |
|---|---|---|
| `0x0000` | `Rectangle2D` | `bounds` (top 30, left 64, bottom 150, right = window right - 64) |
| `0x0008` | `Rectangle2D` | `name_bounds` (scaled to 640 x 480 text space) |
| `0x0010` | `Rectangle2D` | `maximum_bounds` |
| `0x0018` | `Rectangle2D` | `average_bounds` |
| `0x0020` | `rasterizer_dynamic_screen_vertex[0x200]` | `vertices` (line strip; y scrolls left, white) |
| `0x3020` | `rasterizer_dynamic_screen_vertex[5]` | `frame_vertices` (yellow border) |
| `0x3098` | `float` | `maximum` (60.0) |
| `0x309c` | `float` | `average` (of the four recent samples) |
| `0x30a0` | `float[4]` | `recent_samples` |
| `0x30b0` | `char[0x200]` | `name` ("FPS"; extent inferred from the stride) |

## Register conventions worth knowing

All confirmed at the call sites and in the callee prologues; LTCG invented most of them.

| Function | Convention |
|---|---|
| `render_frame` 0x50bea0 | EBX screenshot tile, stack (views, count, page, time since tick, time since frame) |
| `render_player_frame` 0x50ba80 / `render_nonplayer_frame` 0x50bdc0 | EAX tile / nonplayer type, stack view |
| `render_window` 0x50bfb0 | cdecl, seven arguments |
| `chimera__render_camera_build_frustum` 0x50cc40 | EAX bounds or NULL, ECX camera, ESI frustum, stack byte build_projection |
| `render_object` 0x50eba0 / `render_object_shadows` 0x50eb70 | EDI / EAX object_render_data |
| `render_object_shadow_begin` 0x50f830 / `_end` 0x50f980 | EAX data + stack fade, returns AL / ECX data |
| `object_get_cached_render_lighting` 0x50ea00 | ESI object, stack level of detail |
| `render_contrails` 0x50df20 / `render_contrail` 0x50e090 | cdecl |
| `build_sprite` 0x511700 | EBX data, AX sequence, CX sprite, eight stack arguments |
| `build_sprite_rotational` 0x511b40 | EAX data, nine stack arguments |
| `build_sprites_end` 0x511620 / `build_sprite_get_group` 0x511520 | ESI data / EDI data, EAX bitmap |
| `cinematic_screen_effect_update` 0x512360 | EAX the caller block in, EAX out |
| `rasterizer_frame_statistics_sample` 0x512530 | EBX statistics block, stack byte dropped |
| `fg_render` 0x5129a0 / `fg_add_sample` 0x512d90 | BL graph, AL infos / ECX index, stack sample |
| `render_frustum_test_sphere` 0x50d890 / `_bounding_box` 0x50d5b0 | ECX frustum, EDX center / EDI box, stack; return AX |

## Functions and rewrite confidence

| Address | Function | Size | Confidence |
|---|---|---|---|
| 0x50ba80 | `render_player_frame` | 830 | 0.75 |
| 0x50bdc0 | `render_nonplayer_frame` | 211 | 0.8 |
| 0x50bea0 | `render_frame` | 269 | 0.8 |
| 0x50bfb0 | `render_window` | 1498 | 0.7 |
| 0x50c590 | `render_pregame_frame` | 207 | 0.8 |
| 0x50c660 | `render_camera_mirror` | 818 | 0.8 |
| 0x50c9a0 | `render_camera_projection_zrange_push_pop_set` | 235 | 0.65 |
| 0x50ca90 | `render_camera_compute_projection_skew` | 215 | 0.65 |
| 0x50cb70 | `render_camera_compute_frustum_bounds` | 207 | 0.6 |
| 0x50cc40 | `chimera__render_camera_build_frustum` | 2169 | 0.75 |
| 0x50d4c0 | `render_frustum_classify_point_side_planes` | 237 | 0.7 |
| 0x50d5b0 | `render_frustum_test_bounding_box` | 731 | 0.85 |
| 0x50d890 | `render_frustum_test_sphere` | 555 | 0.85 |
| 0x50dac0 | `render_frustum_compute_box_overlap_area` | 763 | 0.85 |
| 0x50ddc0 | `render_frustum_compute_screen_clip_bounds` | 99 | 0.7 |
| 0x50de30 | `render_project_world_point_to_screen` | 234 | 0.6 |
| 0x50df20 | `render_contrails` | 222 | 0.8 |
| 0x50e000 | `contrail_compute_edge_fade_factor` | 133 | 0.75 |
| 0x50e090 | `render_contrail` | 2071 | 0.7 |
| 0x50e8d0 | `render_object_get_cull_sphere` (not a Ghidra function) | 86 | 0.85 |
| 0x50e930 | `render_objects` | 203 | 0.8 |
| 0x50ea00 | `object_get_cached_render_lighting` | 74 | 0.85 |
| 0x50ea50 | `render_object_is_camera_unit` | 112 | 0.8 |
| 0x50eac0 | `render_objects_collect` | 171 | 0.8 |
| 0x50eb70 | `render_object_shadows` | 47 | 0.85 |
| 0x50eba0 | `render_object` | 634 | 0.8 |
| 0x50ee20 | `render_object_list` | 802 | 0.75 |
| 0x50f150 | `object_get_cached_render_state` | 279 | 0.8 |
| 0x50f270 | `object_render_state_refresh` | 683 | 0.8 |
| 0x50f520 | `render_lighting_step_vector3_toward` | 158 | 0.85 |
| 0x50f5c0 | `render_lighting_step_vector4_toward` | 202 | 0.85 |
| 0x50f690 | `render_lighting_step_direction_toward` | 172 | 0.85 |
| 0x50f740 | `object_compute_level_of_detail_pixels` | 233 | 0.8 |
| 0x50f830 | `render_object_shadow_begin` | 327 | 0.8 |
| 0x50f980 | `render_object_shadow_end` | 833 | 0.8 |
| 0x50fcd0 | `render_local_player_gunner_seat_visible` | 184 | 0.55 |
| 0x50fd90 | `render_particles` | 1645 | 0.7 |
| 0x510c50 | `render_sky` | 1325 | 0.7 |
| 0x511190 | `render_sprite_transform_point_and_normal` | 96 | 0.85 |
| 0x5111f0 | `render_billboard_build_orientation_basis` | 318 | 0.75 |
| 0x511330 | `render_billboard_compute_scale` | 113 | 0.8 |
| 0x5113b0 | `render_billboard_compute_view_fade` | 89 | 0.8 |
| 0x511410 | `billboard_system_frame_init` | 262 | 0.85 |
| 0x511520 | `build_sprite_get_group` | 250 | 0.8 |
| 0x511620 | `build_sprites_end` | 222 | 0.8 |
| 0x511700 | `build_sprite` | 1074 | 0.75 |
| 0x511b40 | `build_sprite_rotational` | 564 | 0.75 |
| 0x511d80 | `render_device_is_ready` | 27 | 0.9 |
| 0x511da0 | `render_initialize` | 66 | 0.6 |
| 0x511df0 | `render_cinematic_screen_effect_update` | 129 | 0.85 |
| 0x511e80 | `rasterizer_dynamic_index_slot_lock` | 66 | 0.75 |
| 0x511ef0 | `render_lighting_disable_workaround` | 43 | 0.7 |
| 0x5120f0 | `rasterizer_lens_flare_set_current_key` | 42 | 0.7 |
| 0x512120 | `rasterizer_lens_flare_set_vertex_specular` | 44 | 0.75 |
| 0x512150 | `rasterizer_effect_slot_release_active` | 56 | 0.65 |
| 0x512190 | `render_rasterizer_dispatch_537800` | 15 | 0.9 |
| 0x5121a0 | `cinematic_screen_effect_get_script_value` | 37 | 0.85 |
| 0x5121d0 | `cinematic_screen_effect_set_convolution` | 82 | 0.8 |
| 0x512230 | `cinematic_screen_effect_set_filter` | 97 | 0.8 |
| 0x5122a0 | `cinematic_screen_effect_set_video` | 177 | 0.75 |
| 0x512360 | `cinematic_screen_effect_update` | 464 | 0.8 |
| 0x512530 | `rasterizer_frame_statistics_sample` | 454 | 0.8 |
| 0x512700 | `rasterizer_frame_statistics_graph_init` | 662 | 0.8 |
| 0x5129a0 | `fg_render` (with the tail Ghidra split off at 0x512b80) | 1004 | 0.75 |
| 0x512d90 | `fg_add_sample` | 238 | 0.85 |
| 0x512e80 | `rasterizer_frame_statistics_draw` | 1064 | 0.7 |

Renames against `symbols/functions.txt` are in `symbols/agent_phase4_render.txt` (54 rows, plus
three placeholder rows for the non functions below); `functions.txt` was regenerated with
`tools/merge_symbols.py`.

## Misattributed entries

| Address | Listed as | Finding |
|---|---|---|
| 0x510410..0x510ba0 (10) | `sort_introsort_loop`, `FUN_00510500` ... `FUN_00510ba0` | MSVC STL `std::sort` instantiation over `rendered_particle_datum` (introsort loop, partition, sort_heap, insertion sort, median, make_heap, med3, adjust_heap, push_heap, rotate). Library code, not written. |
| 0x50fd3c | `render_window_call_hook_weather_particle_systems_render` (OpenSauce CE) | Not a function: the `cmp ecx,-1` inside `render_local_player_gunner_seat_visible` 0x50fcd0. Renamed `render_local_player_gunner_seat_visible_tail_0050fd3c`. |
| 0x512b80 | `object_render_state_refresh` (OpenSauce CE) | Not a function: the fall through tail of `fg_render` 0x5129a0, written inside `fg_render.c`. Renamed `fg_render_tail_00512b80` so it no longer duplicates 0x50f270. |
| 0x6b4c00 | `render_window` | Not a function: `.data`, zero bytes, unreferenced (a CE address). Renamed `nonfunction_006b4c00`; the real `render_window` is 0x50bfb0. |
| 0x50fd90 | `contrail_render_all_active` | Misnamed: it is `render_particles`. |
| 0x511b40 | `contrail_draw_segment_blended` | Misnamed: `build_sprite_rotational`, called only by the particle system renderer. |
| 0x50df20 | `contrail_render_by_object_type_mask` | The mask selects Contrail render types, not object types: `render_contrails`. |

Code in the range that is not in the function list and is only declared: the nine structure pass
callbacks `0x511f70`, `0x511f90`, `0x511fe0`, `0x512010`, `0x512020`, `0x512040`, `0x512070`,
`0x512080`, `0x5120c0` that `render_window` hands to `structure_leaf_faces_for_each` (small
trampolines into the rasterizer), and the empty function `0x44ad80` used as a no op callback.

## Known gaps

- The structure pass callbacks above are declared with the `types/structures.h` callback
  shapes; their bodies (and so the exact argument order they forward) are not written.
- `render_local_player_gunner_seat_visible` (0.55) and `render_project_world_point_to_screen`
  (0.6) were not re-traced in this review.
- `unit_data.unknown_37c` / `unknown_380` (0x37c / 0x380) drive the camouflage effect and the
  shadow fade; their meaning (active camouflage amount and fade) is inferred.
- `0x0069c67c` (forced to 1 around the lit structure pass when the BSP has no lightmaps),
  `0x0069c689`, the debug toggles `0x006893e0..0x006893f5` / `0x0069c565` / `0x0069c614` and the
  text tab stops `0x006e474a` keep provisional names.
- Cross module prototype disagreements found while checking the externs (not edited here):
  - `src/structures/structure_bsp_collect_visible_objects.c` types the bounds callback as
    `(handle, float *radius, real_point3d **center)`; the call at 0x554474 is
    `(handle, real_point3d *center_buffer, float *radius)` (see `render_object_get_cull_sphere`).
  - `src/structures/render_camera_update_leaf_and_cluster.c` probes the global camera; the
    function takes EDX = the point (`render_player_frame` passes the view's source camera before
    the global is written).
  - `src/objects/widget_list_notify.c` models only EDI; it also takes a stack lighting pointer
    and EBX = `render_animation*`, both forwarded to the widget render callbacks.
  - `src/rasterizer/chimera__draw_8_bit_text.c` treats its first two stack arguments as opaque;
    `rasterizer_frame_statistics_draw` passes a `Point2DInt` out cursor and the flags -4.
  - `src/structures/structure_bsp_camera_visibility_pass.c` declares
    `chimera__render_camera_build_frustum` with three arguments and
    `src/effects/weather_instance_build_render_geometry.c` declares
    `render_frustum_test_bounding_box` with one; both lose the register arguments.
  - `src/rasterizer` calls 0x512360 `screen_effect_update` and types its block as
    `weapon_screen_effect_parameters`; the two block types share one layout and should be one.
  - `symbols/functions.txt` still names 0x4f5f00 `object_resolve_collideable_reference` and
    0x4f96f0 `object_disconnect_from_map`; they are the collideable cluster iterator and the
    "not yet visited this pass" stamp test used by `render_objects_collect`.
  - Global names differ between modules for 0x007c3108 (`current_local_player_index` elsewhere),
    0x007c30ec and 0x0071cfc4 (`cinematic_globals` in three files).
- `out/phase4/render_types_notes.md` still says the shadow vector step is 0.0015; the binary
  uses 0.012 (0x3c449ba6).

## Review notes (phase-4 review pass)

Written in the review (the rewriters left them out): `render_window`,
`chimera__render_camera_build_frustum`, `render_contrail`, `render_objects_collect`,
`render_object_get_cull_sphere`, `render_object_shadow_begin`, `render_particles`, `render_sky`,
`build_sprite`, `build_sprite_rotational`, `fg_render`, `rasterizer_frame_statistics_draw`.

Fixes against the binary in the rewriters' files:

- `render_objects`: render_object gets the `object_render_data` (object index per entry,
  shadow_pass 0) in EDI; the draft passed nothing.
- `render_object_list`: the first render_model stack argument is the level of detail (times 0.3
  in the shadow pass), not the modifier shader; `widget_list_notify` gets the lighting and the
  `render_animation` block.
- `render_object_shadow_end`: the six planes go with ECX = 6 to 0x552b40 and the render target
  restore takes EAX = 1; planes and box typed.
- `render_player_frame`: 0x553490 takes EDX = the source camera; the z_far clamp tests
  `atmospheric_maximum_density`, not `atmospheric_minimum_distance`; no pre-seeded mirror cluster.
- `render_frame` / `render_pregame_frame`: 0x511df0 gets a 16 byte `rasterizer_frame_time`
  (time, 0, 0), not a double.
- `render_camera_mirror`: the grazing angle fix projects the camera along the mirror normal, not
  along its forward vector.
- `object_render_state_refresh`: the light list is regathered whenever a window has passed, the
  shadow vector step is 0.012, control flow matches the branch targets.
- `object_get_cached_render_state`: the eviction age uses `render_window_count`.
- `rasterizer_frame_statistics_sample`: the EBX statistics pointer (and its NULL test) restored;
  the dropped flag is a byte.
- `rasterizer_frame_statistics_graph_init`: x scale 640 / width, y scale 480 / height, line
  vertex x = i * (right - 128) / 512 + 64, border right = right - 63.
- `cinematic_screen_effect_update`: EAX in / out pass through; 0x00686b0c / 0x00686b14 are
  pointers to black / green; the 0.0001 clears use <=.
- `build_sprites_end`: pointer stride errors on the vertex cache (12 byte entries) and vertex
  buffer slot (0x14) tables; the draw call gets EDX centroid and EDI shader.
- `build_sprite_get_group`: the unusable group test reads `vertices`, not the bitmap key.
- `render_billboard_compute_scale`: the last argument is the BitmapData (its width), not a
  marker count.
- `object_compute_level_of_detail_pixels`: the detail setting is read as a word.
- `render_contrails`: the mask is a stack argument (cdecl), renamed from
  `contrail_render_by_object_type_mask`.
- Return widths: `render_frustum_test_sphere` / `_bounding_box` return AX (`int16_t`),
  `render_project_world_point_to_screen` returns AL.
- Consistency: one extern name per global (`object_data`, `render_camera_global`, the math
  constant pointers), render internal prototypes agree (frustum builder `uint8_t
  build_projection`, `render_window` `int16_t` target and `uint8_t` mirror flag),
  `billboard_basis` folded into `types/render.h`, `render_animation` added.

## C++ layout (converted)

The one-function-per-file `.c` sources were merged into topic files (`camera_frustum.cpp`, `sprites.cpp`,
`objects.cpp`, `screen_effects.cpp`, `particle_sort.cpp`, `window.cpp`, `frame.cpp`). Operations on the module's own
records are member functions of the view classes in `include/halo/render/render.hpp` (`SpriteBuilder`,
`CinematicScreenEffect`, `FrameStatistics`, `ObjectRenderData`); the rest are namespace functions in
`halo::render::<family>`. `render_c_api.cpp` holds the `extern "C"` shims with the original names. The old author
notes and decompile blocks are in `docs/original/render/`.
