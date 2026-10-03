#pragma once
// Blam render module (halo.exe 1.0.10 retail, 0x50ba80..0x512e80 plus the stray 0x6b4c00 entry,
// 78 Ghidra functions).
// This is the scene driver that sits between main and the rasterizer: the per window loop
// (render_frame 0x50bea0, the player and non player window paths 0x50ba80 / 0x50bdc0, and the
// scene pass 0x50bfb0), the camera and frustum maths (0x50c660..0x50de30), contrail and particle
// geometry (0x50df20..0x50e090, 0x50fd90 and its std::sort instantiation 0x510410..0x510ba0),
// the object pass with its cached lighting and fake shadows (0x50e930..0x50f980), sky lights
// (0x510c50), the shared sprite builder (0x511190..0x511b40), the cinematic screen effect state
// (0x5121a0..0x512360) and the frame rate statistics and graph (0x512530..0x512e80).
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - The camera, frustum, fog and lighting records this module fills are already defined in
//     types/rasterizer.h and are NOT redefined here: render_camera (0x54; every copy in this
//     module is a 0x15 dword rep movsd), render_frustum (0x18c; 99 dword copies in
//     render_window 0x50bfb0), render_fog (0x50; the 0x14 dword copy of 0x007c32f4),
//     render_lighting (0x74; the 0x1d dword copy in object_render_state_refresh 0x50f270) and
//     rasterizer_window_parameters (0x258; built on the stack of 0x50bfb0 and zeroed with a 0x96
//     dword rep stosd). The arithmetic of this module re-derives and agrees with every one of
//     them; the render_lighting fields rasterizer.h marks "hint only" (reflection_tint +0x4c,
//     shadow_vector +0x5c, shadow_color +0x68) are pinned by 0x50f270, which steps exactly those
//     members toward the fresh sample (see out/phase4/render_types_notes.md).
//
//   - render_view is pinned by its producer render_frame_all_views 0x4c9260 (main module), which
//     fills the last element field by field (+0x58 position, +0x64 forward, +0x70 up, +0x7c
//     mirrored, +0x80 field of view, +0x84 viewport rectangle, +0x94/+0x98 clip distances) and
//     copies 0x15 dwords from +0x58 to +0x04, and by the stride 0xac every consumer walks.
//
//   - cached_object_render_state is the element of the data array named
//     "cached object render states" (0x0066ebf0), created with 0x100 entries of 0x100 bytes
//     at 0x45aa9c.
//
//   - cinematic_screen_effect_globals is the 0x78 byte game state block (0x1e dword rep stosd
//     at 0x449f12 and 0x51579e) that the hs evaluators cinematic_screen_effect_* 0x481150..
//     0x481360 and script_screen_effect_set_value 0x4810f0 write; the hs function table at
//     0x0065a8a8..0x0065a9a0 names every writer.
//
//   - frame_graph is one 0x32b0 byte element (fg_add_sample 0x512d90 scales its index by
//     0x32b0); the only instance at 0x006b9260 ends exactly at lens_flare_object_visibility
//     0x006bc510 (types/rasterizer.h).
//
//   - Every pointer field is held as uint32_t with the pointee type written first in its
//     comment (the effects.h / rasterizer.h convention), so the 32 bit sizes survive a 64 bit
//     host compiler.
//
// Struct names follow the CEA symbol hints where one exists (hint only): object_render_data,
// render_model_effect, rendered_particle_datum, build_sprite_data,
// cinematic_screen_effect_globals, rasterizer_frame_statistics_s and the fg_* frame graph.
// CEA calls the 0xac window record "struct render_window"; it is named render_view here so that
// render_window stays free for the function at 0x50bfb0.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h      datum_index, data_array
//   types/math.h        real_point3d, real_vector3d, real_matrix4x3, real_rectangle3d
//   types/rasterizer.h  render_camera, render_frustum, render_fog, render_lighting,
//                       rasterizer_window_parameters, rasterizer_frame_time,
//                       rasterizer_dynamic_screen_vertex (the 0x18 position / colour / uv
//                       vertex the contrail, sprite and frame graph code writes),
//                       lens_flare_batch_key, rasterizer_dynamic_index_slot
//   types/structures.h  structure_bsp_mirror_result (CEA render_mirror; render_camera_mirror
//                       0x50c660 reads its plane and the two shader floats)
//   types/effects.h     contrail, contrail_point, particle
//   types/tags.h        Contrail, Particle, Sky, SkyLight, BitmapGroup, BitmapData,
//                       GlobalsRasterizerData, ColorRGB, ColorARGB, Rectangle2D, Point2DInt
//
// Functions in this address range that are misnamed, misattributed, library code or not
// functions at all are listed at the end of out/phase4/render_types_notes.md.

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module wide constants
// ---------------------------------------------------------------------------
typedef enum render_constants {
    k_maximum_render_views = 2,                  // 0x00719b70 array; main fills at most one
                                                 // player window plus the trailing non player
                                                 // one (0x719b70 + 2 * 0xac = 0x719cc8, and the
                                                 // next referenced global is 0x00719ccc)
    k_maximum_rendered_objects = 0x100,          // 0x50eac0 hands 0x100 to the cluster object
                                                 // collector and latches 0x0071cfbe when full
    k_cached_object_render_state_count = 0x100,  // "cached object render states", 0x45aa9c
    k_maximum_rendered_particles = 0x400,        // render_particles 0x50fd90: 0x2000 byte stack
                                                 // array of 8 byte rendered_particle_datum
    k_maximum_rendered_particle_groups = 0x200,  // 0x50fd90 stops grouping past 0x1ff
    k_maximum_build_sprite_groups = 8,           // build_sprite_get_group 0x511520 fails at 8
    k_build_sprite_large_quad_limit = 10,        // build_sprite 0x511700 drops quads that cover
                                                 // more than half the view once 10 were drawn
    k_frame_statistics_history = 60,             // 0x3c clamp in 0x512530
    k_frame_graph_count = 1,                     // one 0x32b0 element at 0x006b9260
    k_frame_graph_vertex_count = 0x200,          // fg_init 0x512700 loop bound
    k_frame_graph_frame_vertex_count = 5,        // the 0x1e dword border strip
    k_render_virtual_screen_width = 640,         // render_camera_view_to_screen 0x50de30
    k_render_virtual_screen_height = 480,
    k_cinematic_script_value_count = 4           // script_screen_effect_set_value 0x4810f0
                                                 // and 0x5121a0 accept 0..3
} render_constants;

// Result of the frustum culling tests render_frustum_cube_visible 0x50d5b0 and
// render_frustum_sphere_visible 0x50d890.
typedef enum render_frustum_visibility {
    _render_frustum_outside = 0,
    _render_frustum_partial = 1,
    _render_frustum_inside = 2
} render_frustum_visibility;

// render_frustum_build_point_flags 0x50d4c0: one bit per side plane of render_frustum.
typedef enum render_frustum_point_flags {
    _render_frustum_point_plane0_bit = 0x01,     // world_planes[0], frustum +0x78
    _render_frustum_point_plane1_bit = 0x02,     // world_planes[1], frustum +0x88
    _render_frustum_point_plane3_bit = 0x04,     // world_planes[3], frustum +0xa8
    _render_frustum_point_plane2_bit = 0x08      // world_planes[2], frustum +0x98
} render_frustum_point_flags;

// The rasterizer_target argument of render_window 0x50bfb0, stored as
// rasterizer_window_parameters.type.
typedef enum render_target_type {
    _render_target_main = 1,                     // the player pass and the non player paths
    _render_target_mirror = 2                    // the reflected pass 0x50ba80 draws first
} render_target_type;

// ---------------------------------------------------------------------------
// render_view  (CEA "struct render_window"; element of the main owned array 0x00719b70)
// render_frame 0x50bea0 walks it with stride 0xac: byte +0x02 clear and a player index other
// than -1 selects render_player_frame 0x50ba80, anything else render_nonplayer_frame 0x50bdc0.
// Both copy source_camera (0x15 dwords from +0x04) into 0x007c3114 and rasterizer_camera
// (0x15 dwords from +0x58) into the rasterizer window; 0x50ba80 clamps source_camera.z_far
// (+0x44) against the fog and against z_near (+0x40).
// ---------------------------------------------------------------------------
typedef struct render_view {
    int16_t local_player_index;     // 0x00 -1 for no player; 0x4c9260 writes -1 into the
                                    //      trailing element
    uint8_t nonplayer;              // 0x02 1 in the trailing element; forces 0x50bdc0 with
                                    //      EAX 0 (letterbox, loading screen, console)
    uint8_t unknown_03;             // 0x03 never written
    render_camera source_camera;    // 0x04 culling camera, copied to 0x007c3114
    render_camera rasterizer_camera; // 0x58 the camera the rasterizer draws with
} render_view;                      // size 0xac

// ---------------------------------------------------------------------------
// render_model_effect  (CEA name; the 10 dword block render_object_list 0x50ee20 builds on its
// stack and hands to render_model 0x4d6fc0)
// 0x50ee20 copies its parent block with a 0xa dword rep movsd, resets type 2 to 0 (clearing
// +0x1c..+0x24) for children, then sets type 1 for a unit whose +0x37c is positive and type 2
// for an Object tag with flags bit 1. types/interface.h describes the first 0x20 bytes of the
// same record as first_person_light_parameters (the first person weapon path leaves +0x1c 0).
// ---------------------------------------------------------------------------
typedef enum render_model_effect_type {
    _render_model_effect_none = 0,
    _render_model_effect_active_camouflage = 1,  // unit +0x37c > 0
    _render_model_effect_self_occlusion = 2      // Object.flags bit 1 (transparent self
                                                 // occlusion); not inherited by children
} render_model_effect_type;

typedef struct render_model_effect {
    int16_t type;                   // 0x00 render_model_effect_type
    int16_t unknown_02;             // 0x02 copied, never set on its own
    float unit_37c;                 // 0x04 unit +0x37c (type 1)
    float unit_380;                 // 0x08 unit +0x380 (type 1)
    datum_index object_index;       // 0x0c the object that started the effect
    real_point3d centroid;          // 0x10 object.bounding_center (+0xa0)
    uint32_t modifier_shader;       // 0x1c void* Shader tag data of Object.modifier_shader
                                    //      (+0x9c); kept only for shader_type 1 or 5..11
    uint32_t change_colors;         // 0x20 ColorRGB* object.change_colors (+0x1b8)
    uint32_t function_values;       // 0x24 float* object.function_out_values (+0x134)
} render_model_effect;              // size 0x28

// ---------------------------------------------------------------------------
// render_animation  (CEA name, hint only; the 8 byte EBX block render_object_list 0x50ee20 builds
// on its stack (esp+0x18 / +0x1c) for widget_list_notify 0x4ffca0, which forwards it unchanged as
// the fourth argument of every widget type render callback, CEA antenna_render(object_index,
// antenna_index, lighting, animation))
// ---------------------------------------------------------------------------
// render_animation is defined in types/rasterizer.h (rasterizer_model_draw_context aliases it).

// ---------------------------------------------------------------------------
// object_render_data  (CEA name; the EDI block of render_object 0x50eba0)
// render_objects 0x50e930 keeps one on its stack (0x50 byte frame, struct at +0x10) and
// render_window 0x50bfb0 builds another for the shadow pass (object -1, lighting 0, both flags
// 1, +0x40 0, +0x44 -1) at esp+0x10, below the window parameters at esp+0x58, so the record
// cannot exceed 0x48 bytes. render_object_shadow_begin 0x50f830 builds the shadow basis at
// +0x0c through 0x4cb970 and has rasterizer_environment_shadow_begin 0x530ff0 write the shadow
// volume radius at +0x40; render_object_shadow_end 0x50f980 (ECX) reads +0x10..+0x40.
// ---------------------------------------------------------------------------
typedef struct object_render_data {
    datum_index object_index;       // 0x00 copied from render_objects[] per iteration
    uint32_t lighting;              // 0x04 render_lighting*, object_get_cached_render_lighting
                                    //      0x50ea00; 0 when the object is not lit
    uint8_t shadow_pass;            // 0x08 1 selects the fake shadow path in 0x50eba0 and
                                    //      render_model flags 2 in 0x50ee20
    uint8_t outside_fog_plane;      // 0x09 object centre in front of the planar fog plane (or
                                    //      fog planar_mode != 1); render_model flag 4
    uint8_t unknown_0a[2];          // 0x0a never written
    real_matrix4x3 shadow_matrix;   // 0x0c scale, then the basis around lighting.shadow_vector
                                    //      and the object centre at +0x34
    float shadow_radius;            // 0x40 written through the out pointer of 0x530ff0, 0 when the
                                    //      shadow is not drawn
    int32_t unknown_44;             // 0x44 -1 in the shadow pass block; no reader in this module
} object_render_data;               // size 0x48

// ---------------------------------------------------------------------------
// cached_object_render_state  (element of the "cached object render states" data array,
// 0x007c30ec; object +0x170 holds the datum of the entry for that object)
// object_get_cached_render_lighting 0x50ea00 returns &lighting; object_render_state_refresh
// 0x50f270 resamples desired_lighting, steps lighting toward it at 0.03 per call (0.012 for
// the shadow vector) or copies it whole (0x1d dwords), and stamps the counters. The eviction in
// 0x50f150 picks the entry with the oldest last_update_window.
// ---------------------------------------------------------------------------
typedef struct cached_object_render_state {
    uint16_t identifier;            // 0x00 datum header
    uint16_t unknown_02;            // 0x02 never read or written here
    datum_index object_index;       // 0x04 compared by 0x50f150 before reuse
    int32_t last_sample_frame;      // 0x08 render_frame_index (0x007c3100) at the last full
                                    //      object_sample_ambient_lighting
    int32_t last_update_window;     // 0x0c render_window_count (0x007c3104) at the last refresh
    int32_t last_update_frame;      // 0x10 render_frame_index at the last refresh
    render_lighting lighting;       // 0x14 the smoothed lighting handed to render_model
    render_lighting desired_lighting; // 0x88 the latest sample
    float level_of_detail_pixels;   // 0xfc 0x50f740 projected size; above 400 or 100 pixels the
                                    //      refresh interval drops from 10 to 3 to 0 frames
} cached_object_render_state;       // size 0x100

// ---------------------------------------------------------------------------
// rendered_particle_datum  (CEA name; element of the 0x400 entry stack array of
// render_particles 0x50fd90, sorted by the std::sort instantiation 0x510410..0x510ba0)
// Every comparison in 0x5109d0 / 0x510830 / 0x510a90 / 0x510b20 orders by the signed words at
// +0x02 and +0x04, then the unsigned byte at +0x06; 0x50fd90 then groups equal runs.
// ---------------------------------------------------------------------------
typedef struct rendered_particle_datum {
    uint16_t particle_index;        // 0x00 low half of the particle datum (effects.h particle)
    uint16_t definition_index;      // 0x02 low half of particle.definition_index (+0x04)
    int16_t cluster_index;          // 0x04 particle.location.cluster_index (+0x2c)
    uint8_t first_person;           // 0x06 particle flag 0x20 and the viewer owns it (+0x0f)
    uint8_t unknown_07;             // 0x07 never written
} rendered_particle_datum;          // size 0x08

// ---------------------------------------------------------------------------
// build_sprite_data  (CEA name; the EBX block of build_sprite 0x511700 and the ESI block of
// build_sprites_end 0x511620; built on the stack by render_particles 0x50fd90 at esp+0x50,
// by the particle system and weather renderers 0x455xxx / 0x4592xx and by the object widgets
// 0x4fb4a2 / 0x4fe651)
// A group is one Bitmap texture page: build_sprite_get_group 0x511520 (EDI data, EAX bitmap)
// finds or appends one, locking maximum_sprite_count quads of dynamic vertices for it.
// ---------------------------------------------------------------------------
typedef enum build_sprite_data_flags {
    _build_sprite_data_screen_space_bit = 0x01,  // no world_to_view transform and 2D quads only
                                                 // (0x511190, 0x5111f0, 0x511330, 0x511700);
                                                 // build_sprites_end skips the draw
    _build_sprite_data_flag_1_bit = 0x02,        // forwarded to the draw as 0x80; render_particles
                                                 // sets it for a first person run (the
                                                 // rendered_particle_datum.first_person key)
    _build_sprite_data_flag_2_bit = 0x04         // set by render_particles, cleared by
                                                 // build_sprites_end
} build_sprite_data_flags;

// The per sprite flags argument of build_sprite (its eighth stack argument).
typedef enum build_sprite_flags {
    _build_sprite_already_transformed_bit = 0x01, // 0x511190 copies origin and direction as is
    _build_sprite_mirror_u_bit = 0x02,
    _build_sprite_mirror_v_bit = 0x04
} build_sprite_flags;

typedef struct build_sprite_group {
    int32_t vertex_slot;            // 0x00 rasterizer dynamic vertex slot (0x51bdd0), -1 when
                                    //      the allocation failed
    uint32_t vertices;              // 0x04 rasterizer_dynamic_screen_vertex*, locked by
                                    //      0x51be40; 0 on failure
    int16_t quad_count;             // 0x08 four vertices each
    int16_t unknown_0a;             // 0x0a never written
    struct BitmapData *bitmap;      // 0x0c texture page; the group key
} build_sprite_group;               // size 0x10

typedef struct build_sprite_data {
    datum_index bitmap_group_index; // 0x00 Bitmap tag; sequences at +0x54/+0x58 (stride 0x40),
                                    //      sprites at sequence +0x34/+0x38 (stride 0x20),
                                    //      bitmap data at +0x64 (stride 0x30)
    int16_t maximum_sprite_count;   // 0x04 quads locked per group and the total cap
    int16_t unknown_06;             // 0x06 never written
    uint32_t shader;                // 0x08 shader_effect*: the particle shader block of the
                                    //      tag (Particle +0xb0); +0x2c framebuffer_fade_mode
                                    //      feeds the vertex fade, render_particles stores the
                                    //      average radius at +0x98
    int16_t sprite_count;           // 0x0c quads built so far
    int16_t unknown_0e;             // 0x0e never written
    uint32_t flags;                 // 0x10 build_sprite_data_flags
    real_point3d centroid;          // 0x14 sum of the view space origins; build_sprites_end
                                    //      averages it and returns it to world space
    int16_t group_count;            // 0x20 at most k_maximum_build_sprite_groups
    int16_t unknown_22;             // 0x22 never written
    build_sprite_group groups[8];   // 0x24
} build_sprite_data;                // size 0xa4

// ---------------------------------------------------------------------------
// billboard_basis  (the 0x28 byte stack block build_sprite 0x511700 keeps at ebp-0x90 and hands
// to render_billboard_build_orientation_basis 0x5111f0, which fills tangent / bitangent (and
// normal for mode 2); build_sprite then crosses tangent x bitangent into normal for the view
// fade 0x5113b0 and lays each corner out as tangent * a + bitangent * b)
// Offsets pinned by 0x5111f0 (stores at [edi+0x04..0x18], [edi+0x1c]) and by the build_sprite
// loop (ebp-0x8c / -0x80 / -0x74).
// ---------------------------------------------------------------------------
typedef struct billboard_basis {
    uint32_t unused_00;             // 0x00 never read or written
    real_vector3d tangent;          // 0x04 mode 0 (1,0,0); mode 1 the normalized direction;
                                    //      mode 2 cross(view up or view left, direction)
    real_vector3d bitangent;        // 0x10
    real_vector3d normal;           // 0x1c mode 2, or the build_sprite tangent x bitangent
} billboard_basis;                  // size 0x28

// ---------------------------------------------------------------------------
// cinematic_screen_effect_globals  (CEA name; the 0x78 byte game state block at *0x0071cfc4)
// Field names follow the CEA argument names of the hs setters (hint only); the offsets are
// pinned by the setters 0x5121d0 (convolution), 0x512230 (filter), 0x481280 (desaturation
// tint), 0x5122a0 (video), 0x481150 / 0x481340 (start / stop), 0x4810f0 (script values),
// 0x481360 (near clip) and by cinematic_screen_effect_update 0x512360, which returns this block
// to the rasterizer screen effect passes 0x52d8a0 / 0x52e2d0.
// ---------------------------------------------------------------------------
typedef struct cinematic_screen_effect_globals {
    int16_t convolution_extra_passes; // 0x00 cleared with +0x02 once the radius drops below
                                    //      0.0001
    int16_t convolution_type;       // 0x02 the rasterizer runs the effect when it is non zero
    float convolution_radius;       // 0x04 lerp of the two bounds below by the convolution time
    uint32_t mask_bitmap_data;      // 0x08 0x08 shares its layout with weapon_screen_effect_parameters, whose 0x08 is
                                    //    the mask BitmapData pointer the rasterizer tests for non zero (see 0x512360)
                                    //      block clears write it. The weapon block that shares
                                    //      this layout (types/interface.h
                                    //      weapon_screen_effect_parameters) keeps its mask
                                    //      BitmapData* here; see 0x512360
    float filter_light_enhancement_intensity; // 0x0c lerp of +0x4c/+0x50 by the filter time
    float filter_desaturation_intensity;      // 0x10 lerp of +0x54/+0x58
    ColorRGB filter_desaturation_tint; // 0x14 replaced by *0x00686b14 while it equals
                                    //      *0x00686b0c (black)
    uint8_t filter_desaturation_is_additive; // 0x20
    uint8_t night_vision_masked;    // 0x21 0x21 shared layout with
                                    //    weapon_screen_effect_parameters.night_vision_masked; cleared by
                                    //    cinematic_screen_effect_set_filter
                                    //      keeps night_vision_masked here
    uint8_t desaturation_masked;    // 0x22 0x22 shared layout with
                                    //    weapon_screen_effect_parameters.desaturation_masked; cleared by
                                    //    cinematic_screen_effect_set_filter
                                    //      keeps desaturation_masked here
    uint8_t video_enabled;          // 0x23 1 from the video setter, 0 from the other two
    int16_t video_overbright_mode;  // 0x24
    int16_t unknown_26;             // 0x26 never written
    uint32_t video_scanline_map;    // 0x28 BitmapData* of GlobalsRasterizerData
                                    //      video_scanline_map (+0x128 tag id, bitmap data +0x64)
    float video_noise_intensity;    // 0x2c
    float unknown_30;               // 0x30 1.0 from the video setter
    uint32_t video_noise_map;       // 0x34 BitmapData* of video_noise_map (+0x138)
    uint8_t active;                 // 0x38 start sets it, stop clears it; update needs it
    uint8_t initialized;            // 0x39 start with clear 0 wipes +0x00..+0x37 only once
    int16_t unknown_3a;             // 0x3a never written
    float convolution_radius_lower_bound; // 0x3c
    float convolution_radius_upper_bound; // 0x40
    float convolution_start_time;   // 0x44 game time in seconds (ticks / 30)
    float convolution_end_time;     // 0x48 start plus convolution_time
    float filter_light_enhancement_intensity_lower_bound; // 0x4c
    float filter_light_enhancement_intensity_upper_bound; // 0x50
    float filter_desaturation_intensity_lower_bound;      // 0x54
    float filter_desaturation_intensity_upper_bound;      // 0x58
    float filter_start_time;        // 0x5c
    float filter_end_time;          // 0x60 start plus filter_time
    float script_values[4];         // 0x64 1.0 after a reset; 0x5121a0 reads them
    float near_clip_distance;       // 0x74 cinematic_set_near_clip_distance; when positive
                                    //      0x517470 uses it for 0x0069c65c
} cinematic_screen_effect_globals;  // size 0x78

// ---------------------------------------------------------------------------
// rasterizer_frame_statistics  (CEA rasterizer_frame_statistics_s; the EBX block of
// rasterizer_frame_statistics_get_fps 0x512530, which every caller points at 0x007c30a0)
// All rates are frames per second from millisecond intervals of the 60 frame history.
// ---------------------------------------------------------------------------
typedef struct rasterizer_frame_statistics {
    float framerate;                // 0x00 from the latest interval
    int16_t sample_count;           // 0x04 history entries used
    int16_t unknown_06;             // 0x06 never written
    float average_framerate;        // 0x08 over the whole history
    float minimum_framerate;        // 0x0c from the longest interval
    float maximum_framerate;        // 0x10 from the shortest interval
    float dropped_percentage;       // 0x14 dropped flags in the history * 100 / count
} rasterizer_frame_statistics;      // size 0x18

// ---------------------------------------------------------------------------
// frame_graph  (the fg_* graph; one element at 0x006b9260, stride 0x32b0)
// fg_init 0x512700 rebuilds it whenever the game window size changes, fg_add_sample 0x512d90
// (ECX index) scrolls the vertex heights and folds the sample into the running average, and
// fg_render 0x5129a0 draws the line strip, the border and the three labels.
// ---------------------------------------------------------------------------
typedef struct frame_graph {
    Rectangle2D bounds;             // 0x0000 top 30, left 64, bottom 150 (the baseline), right
                                    //        = window right edge (0x0069c63a) - 64
    Rectangle2D name_bounds;        // 0x0008 text rectangles; bottom 480, right 640
    Rectangle2D maximum_bounds;     // 0x0010
    Rectangle2D average_bounds;     // 0x0018
    rasterizer_dynamic_screen_vertex vertices[0x200]; // 0x0020 line strip; y 150.0 at init
                                    //        and colour 0xffffffff
    rasterizer_dynamic_screen_vertex frame_vertices[5]; // 0x3020 yellow border strip
    float maximum;                  // 0x3098 60.0; samples are clamped to it
    float average;                  // 0x309c mean of the four recent samples
    float recent_samples[4];        // 0x30a0
    char name[0x200];               // 0x30b0 "FPS" stored as the dword 0x00535046; its extent
                                    //        is inferred from the element stride
} frame_graph;                      // size 0x32b0

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// the render window state (render_frame 0x50bea0 and render_window 0x50bfb0)
// global 0x007c3100: int32_t render_frame_index            bumped once per render_frame and
//                    per pregame frame 0x50c590; the particle and cache staleness clock
// global 0x007c3104: int32_t render_window_count           bumped once per render_window call
// global 0x007c3108: int16_t render_local_player_index     render_window argument; -1 for the
//                    mirror pass (every store and most reads are WORD sized)
// global 0x007c310a: int16_t render_window_index           loop index of render_frame
// global 0x007c310c: float render_time_since_tick          game_time_globals.leftover_time;
//                    the frame time handed to 0x511df0 is game_time / 30 + this
// global 0x007c3110: float render_time_since_frame         seconds since the previous frame;
//                    sky animation and weather accumulate it
// global 0x007c3114: render_camera render_camera           the source (culling) camera
// global 0x007c3168: render_frustum render_frustum         built from it by 0x50cc40
// global 0x007c32f4: render_fog render_fog                 filled by 0x53e8c0 in 0x50ba80
//                    (0x007c3344.. continue with the structures module globals)
//
// objects and lighting
// global 0x006b8d84: float render_saved_projection_z[4]    render_camera_hack_frustum_z
//                    0x50c9a0 save / restore of projection[0..3][2]
// global 0x006b8dc0: int16_t rendered_object_count         (0x006b8dc2 is not part of it)
// global 0x006b8dc4: datum_index rendered_objects[0x100]   0x50eac0
// global 0x006b91c8: render_lighting render_uncached_object_lighting  0x50ea00 fallback when
//                    no cache entry can be had
// global 0x007c30e8: uint8_t render_debug_objects          UNSURE name; makes 0x50ee20 call
//                    0x4f4410 per object; no writer in the binary (console global)
// global 0x007c30ec: data_array *cached_object_render_states  0x100 x 0x100, 0x45aa9c
// global 0x0071cfbe: uint8_t rendered_objects_full_warning  latched by 0x50eac0
//
// sky
// global 0x006b923c: float sky_animation_times[9]          0x510c50 adds the frame time, fmod
//                    by the animation period (bounded by next global)
//
// sprites
// global 0x007c30c4: float build_sprite_screen_coverage    0x511410 resets, 0x511700 adds
// global 0x007c30c8: int16_t build_sprite_large_quad_count  quads covering more than half
// global 0x007c30d0: real_vector3d build_sprite_view_up    world k axis in view space
// global 0x007c30dc: real_vector3d build_sprite_view_left  world j axis in view space
// global 0x0071cfbf: uint8_t build_sprite_group_warning    latched when a vertex slot fails
//
// camera
// global 0x0071cfbd: uint8_t render_clip_warning           latched when z_far <= z_near
//
// cinematic screen effect and model tint
// global 0x0071cfc0: ColorARGB *rasterizer_model_ambient_reflection_tint  0x10 byte game state
//                    block carved by 0x511da0; hs rasterizer_model_ambient_reflection_tint
//                    0x481050 writes it, 0x52ad03 uploads it
// global 0x0071cfc4: cinematic_screen_effect_globals *cinematic_screen_effect_globals  0x78
//                    byte game state block (carved in 0x5169c0, see types/rasterizer.h)
//
// frame statistics
// global 0x007c30a0: rasterizer_frame_statistics rasterizer_frame_statistics
// global 0x0071cfe8: uint32_t frame_statistics_times[60]   system milliseconds, newest first
// global 0x0071d0d8: uint8_t frame_statistics_dropped[60]
// global 0x0071d114: int16_t frame_statistics_count        (0x0071d116 is not part of it)
// global 0x0071cfd0: int64_t frame_statistics_unknown_d0   read by 0x512e80, never written
// global 0x0071cfd8: int64_t frame_statistics_unknown_d8   read by 0x512e80, never written
// global 0x0071d118: int32_t frame_graph_window_width      fg_init cache; holds bottom - top of
//                    the game window rectangle 0x0069c634 (the names are swapped)
// global 0x0071d11c: int32_t frame_graph_window_height     holds right - left
// global 0x0071d120: int32_t frame_statistics_key_a_latch  edge detect for the infos toggle
// global 0x0071d124: int32_t frame_statistics_key_b_latch  edge detect for the graph toggle
// global 0x0071d128: int32_t frame_graph_render_graph      fg_render BL; gates the strips
//                    and the three labels
// global 0x0071d12c: int32_t frame_graph_render_infos      fg_render AL; only enables the
//                    shared device state setup
// global 0x0071d130: int32_t frame_statistics_last_time    QueryPerformanceCounter ms
// global 0x006b9260: frame_graph frame_graphs[1]
//
// Globals this module reads but does not own:
//   0x00719b70  render_view render_views[2]      main, render_frame_all_views 0x4c9260
//   0x007c1220  rasterizer_window_parameters     types/rasterizer.h
//   0x007c3344..0x007d0390  render leaf / cluster / visibility state (types/structures.h)
//   0x0087abd0 / 0x0087abe8 / 0x0087abec  particle, contrail point and contrail data
//               arrays (types/effects.h)
//   0x008603b0  object data array; 0x0087bc14 tag_instances (types/cache.h)
//   0x0069c65c..0x0069c668  the default near / far clip pairs 0x511df0 seeds (0.0625, 1024,
//               0.01171875, 1024); types/rasterizer.h names 0x0069c65c letterbox height
//   0x006e09e8 / 0x006d98f0 / 0x006d99d8 / 0x006dd9e0 / 0x007bf04c  rasterizer dynamic
//               vertex and index caches (types/rasterizer.h)
//   0x00746fb0  lens_flare_batch_key lens_flare_current_key, written by 0x5120f0
//   0x0069e708  lens_flare_vertex_specular, written by 0x512120
//   0x0071d278  the active rasterizer effect slot pointer 0x512150 releases (&effects[48])
//   0x006e4738..0x006e4752  interface text draw state
//   0x006893e0..0x006893f5, 0x00689450, 0x00689480, 0x0069c565, 0x0069c614  debug toggles
// ---------------------------------------------------------------------------

#pragma pack(pop)

#include <stddef.h>
static_assert(sizeof(render_view) == 0xac, "render_view size");
static_assert(offsetof(render_view, local_player_index) == 0x00, "render_view::local_player_index");
static_assert(offsetof(render_view, nonplayer) == 0x02, "render_view::nonplayer");
static_assert(offsetof(render_view, unknown_03) == 0x03, "render_view::unknown_03");
static_assert(offsetof(render_view, source_camera) == 0x04, "render_view::source_camera");
static_assert(offsetof(render_view, rasterizer_camera) == 0x58, "render_view::rasterizer_camera");
static_assert(sizeof(render_model_effect) == 0x28, "render_model_effect size");
static_assert(offsetof(render_model_effect, type) == 0x00, "render_model_effect::type");
static_assert(offsetof(render_model_effect, unknown_02) == 0x02, "render_model_effect::unknown_02");
static_assert(offsetof(render_model_effect, unit_37c) == 0x04, "render_model_effect::unit_37c");
static_assert(offsetof(render_model_effect, unit_380) == 0x08, "render_model_effect::unit_380");
static_assert(offsetof(render_model_effect, object_index) == 0x0c, "render_model_effect::object_index");
static_assert(offsetof(render_model_effect, centroid) == 0x10, "render_model_effect::centroid");
static_assert(offsetof(render_model_effect, modifier_shader) == 0x1c, "render_model_effect::modifier_shader");
static_assert(offsetof(render_model_effect, change_colors) == 0x20, "render_model_effect::change_colors");
static_assert(offsetof(render_model_effect, function_values) == 0x24, "render_model_effect::function_values");
static_assert(sizeof(render_animation) == 0x08, "render_animation size");
static_assert(offsetof(render_animation, change_colors) == 0x00, "render_animation::change_colors");
static_assert(offsetof(render_animation, function_values) == 0x04, "render_animation::function_values");
static_assert(sizeof(object_render_data) == 0x48, "object_render_data size");
static_assert(offsetof(object_render_data, object_index) == 0x00, "object_render_data::object_index");
static_assert(offsetof(object_render_data, lighting) == 0x04, "object_render_data::lighting");
static_assert(offsetof(object_render_data, shadow_pass) == 0x08, "object_render_data::shadow_pass");
static_assert(offsetof(object_render_data, outside_fog_plane) == 0x09, "object_render_data::outside_fog_plane");
static_assert(offsetof(object_render_data, unknown_0a) == 0x0a, "object_render_data::unknown_0a");
static_assert(offsetof(object_render_data, shadow_matrix) == 0x0c, "object_render_data::shadow_matrix");
static_assert(offsetof(object_render_data, shadow_radius) == 0x40, "object_render_data::shadow_radius");
static_assert(offsetof(object_render_data, unknown_44) == 0x44, "object_render_data::unknown_44");
static_assert(sizeof(cached_object_render_state) == 0x100, "cached_object_render_state size");
static_assert(offsetof(cached_object_render_state, identifier) == 0x00, "cached_object_render_state::identifier");
static_assert(offsetof(cached_object_render_state, unknown_02) == 0x02, "cached_object_render_state::unknown_02");
static_assert(offsetof(cached_object_render_state, object_index) == 0x04, "cached_object_render_state::object_index");
static_assert(offsetof(cached_object_render_state, last_sample_frame) == 0x08, "cached_object_render_state::last_sample_frame");
static_assert(offsetof(cached_object_render_state, last_update_window) == 0x0c, "cached_object_render_state::last_update_window");
static_assert(offsetof(cached_object_render_state, last_update_frame) == 0x10, "cached_object_render_state::last_update_frame");
static_assert(offsetof(cached_object_render_state, lighting) == 0x14, "cached_object_render_state::lighting");
static_assert(offsetof(cached_object_render_state, desired_lighting) == 0x88, "cached_object_render_state::desired_lighting");
static_assert(offsetof(cached_object_render_state, level_of_detail_pixels) == 0xfc, "cached_object_render_state::level_of_detail_pixels");
static_assert(sizeof(rendered_particle_datum) == 0x08, "rendered_particle_datum size");
static_assert(offsetof(rendered_particle_datum, particle_index) == 0x00, "rendered_particle_datum::particle_index");
static_assert(offsetof(rendered_particle_datum, definition_index) == 0x02, "rendered_particle_datum::definition_index");
static_assert(offsetof(rendered_particle_datum, cluster_index) == 0x04, "rendered_particle_datum::cluster_index");
static_assert(offsetof(rendered_particle_datum, first_person) == 0x06, "rendered_particle_datum::first_person");
static_assert(offsetof(rendered_particle_datum, unknown_07) == 0x07, "rendered_particle_datum::unknown_07");
static_assert(sizeof(build_sprite_group) == 0x10, "build_sprite_group size");
static_assert(offsetof(build_sprite_group, vertex_slot) == 0x00, "build_sprite_group::vertex_slot");
static_assert(offsetof(build_sprite_group, vertices) == 0x04, "build_sprite_group::vertices");
static_assert(offsetof(build_sprite_group, quad_count) == 0x08, "build_sprite_group::quad_count");
static_assert(offsetof(build_sprite_group, unknown_0a) == 0x0a, "build_sprite_group::unknown_0a");
static_assert(offsetof(build_sprite_group, bitmap) == 0x0c, "build_sprite_group::bitmap");
static_assert(sizeof(build_sprite_data) == 0xa4, "build_sprite_data size");
static_assert(offsetof(build_sprite_data, bitmap_group_index) == 0x00, "build_sprite_data::bitmap_group_index");
static_assert(offsetof(build_sprite_data, maximum_sprite_count) == 0x04, "build_sprite_data::maximum_sprite_count");
static_assert(offsetof(build_sprite_data, unknown_06) == 0x06, "build_sprite_data::unknown_06");
static_assert(offsetof(build_sprite_data, shader) == 0x08, "build_sprite_data::shader");
static_assert(offsetof(build_sprite_data, sprite_count) == 0x0c, "build_sprite_data::sprite_count");
static_assert(offsetof(build_sprite_data, unknown_0e) == 0x0e, "build_sprite_data::unknown_0e");
static_assert(offsetof(build_sprite_data, flags) == 0x10, "build_sprite_data::flags");
static_assert(offsetof(build_sprite_data, centroid) == 0x14, "build_sprite_data::centroid");
static_assert(offsetof(build_sprite_data, group_count) == 0x20, "build_sprite_data::group_count");
static_assert(offsetof(build_sprite_data, unknown_22) == 0x22, "build_sprite_data::unknown_22");
static_assert(offsetof(build_sprite_data, groups) == 0x24, "build_sprite_data::groups");
static_assert(sizeof(billboard_basis) == 0x28, "billboard_basis size");
static_assert(offsetof(billboard_basis, unused_00) == 0x00, "billboard_basis::unused_00");
static_assert(offsetof(billboard_basis, tangent) == 0x04, "billboard_basis::tangent");
static_assert(offsetof(billboard_basis, bitangent) == 0x10, "billboard_basis::bitangent");
static_assert(offsetof(billboard_basis, normal) == 0x1c, "billboard_basis::normal");
static_assert(sizeof(cinematic_screen_effect_globals) == 0x78, "cinematic_screen_effect_globals size");
static_assert(offsetof(cinematic_screen_effect_globals, convolution_extra_passes) == 0x00, "cinematic_screen_effect_globals::convolution_extra_passes");
static_assert(offsetof(cinematic_screen_effect_globals, convolution_type) == 0x02, "cinematic_screen_effect_globals::convolution_type");
static_assert(offsetof(cinematic_screen_effect_globals, convolution_radius) == 0x04, "cinematic_screen_effect_globals::convolution_radius");
static_assert(offsetof(cinematic_screen_effect_globals, mask_bitmap_data) == 0x08, "cinematic_screen_effect_globals::mask_bitmap_data");
static_assert(offsetof(cinematic_screen_effect_globals, filter_light_enhancement_intensity) == 0x0c, "cinematic_screen_effect_globals::filter_light_enhancement_intensity");
static_assert(offsetof(cinematic_screen_effect_globals, filter_desaturation_intensity) == 0x10, "cinematic_screen_effect_globals::filter_desaturation_intensity");
static_assert(offsetof(cinematic_screen_effect_globals, filter_desaturation_tint) == 0x14, "cinematic_screen_effect_globals::filter_desaturation_tint");
static_assert(offsetof(cinematic_screen_effect_globals, filter_desaturation_is_additive) == 0x20, "cinematic_screen_effect_globals::filter_desaturation_is_additive");
static_assert(offsetof(cinematic_screen_effect_globals, night_vision_masked) == 0x21, "cinematic_screen_effect_globals::night_vision_masked");
static_assert(offsetof(cinematic_screen_effect_globals, desaturation_masked) == 0x22, "cinematic_screen_effect_globals::desaturation_masked");
static_assert(offsetof(cinematic_screen_effect_globals, video_enabled) == 0x23, "cinematic_screen_effect_globals::video_enabled");
static_assert(offsetof(cinematic_screen_effect_globals, video_overbright_mode) == 0x24, "cinematic_screen_effect_globals::video_overbright_mode");
static_assert(offsetof(cinematic_screen_effect_globals, unknown_26) == 0x26, "cinematic_screen_effect_globals::unknown_26");
static_assert(offsetof(cinematic_screen_effect_globals, video_scanline_map) == 0x28, "cinematic_screen_effect_globals::video_scanline_map");
static_assert(offsetof(cinematic_screen_effect_globals, video_noise_intensity) == 0x2c, "cinematic_screen_effect_globals::video_noise_intensity");
static_assert(offsetof(cinematic_screen_effect_globals, unknown_30) == 0x30, "cinematic_screen_effect_globals::unknown_30");
static_assert(offsetof(cinematic_screen_effect_globals, video_noise_map) == 0x34, "cinematic_screen_effect_globals::video_noise_map");
static_assert(offsetof(cinematic_screen_effect_globals, active) == 0x38, "cinematic_screen_effect_globals::active");
static_assert(offsetof(cinematic_screen_effect_globals, initialized) == 0x39, "cinematic_screen_effect_globals::initialized");
static_assert(offsetof(cinematic_screen_effect_globals, unknown_3a) == 0x3a, "cinematic_screen_effect_globals::unknown_3a");
static_assert(offsetof(cinematic_screen_effect_globals, convolution_radius_lower_bound) == 0x3c, "cinematic_screen_effect_globals::convolution_radius_lower_bound");
static_assert(offsetof(cinematic_screen_effect_globals, convolution_radius_upper_bound) == 0x40, "cinematic_screen_effect_globals::convolution_radius_upper_bound");
static_assert(offsetof(cinematic_screen_effect_globals, convolution_start_time) == 0x44, "cinematic_screen_effect_globals::convolution_start_time");
static_assert(offsetof(cinematic_screen_effect_globals, convolution_end_time) == 0x48, "cinematic_screen_effect_globals::convolution_end_time");
static_assert(offsetof(cinematic_screen_effect_globals, filter_light_enhancement_intensity_lower_bound) == 0x4c, "cinematic_screen_effect_globals::filter_light_enhancement_intensity_lower_bound");
static_assert(offsetof(cinematic_screen_effect_globals, filter_light_enhancement_intensity_upper_bound) == 0x50, "cinematic_screen_effect_globals::filter_light_enhancement_intensity_upper_bound");
static_assert(offsetof(cinematic_screen_effect_globals, filter_desaturation_intensity_lower_bound) == 0x54, "cinematic_screen_effect_globals::filter_desaturation_intensity_lower_bound");
static_assert(offsetof(cinematic_screen_effect_globals, filter_desaturation_intensity_upper_bound) == 0x58, "cinematic_screen_effect_globals::filter_desaturation_intensity_upper_bound");
static_assert(offsetof(cinematic_screen_effect_globals, filter_start_time) == 0x5c, "cinematic_screen_effect_globals::filter_start_time");
static_assert(offsetof(cinematic_screen_effect_globals, filter_end_time) == 0x60, "cinematic_screen_effect_globals::filter_end_time");
static_assert(offsetof(cinematic_screen_effect_globals, script_values) == 0x64, "cinematic_screen_effect_globals::script_values");
static_assert(offsetof(cinematic_screen_effect_globals, near_clip_distance) == 0x74, "cinematic_screen_effect_globals::near_clip_distance");
static_assert(sizeof(rasterizer_frame_statistics) == 0x18, "rasterizer_frame_statistics size");
static_assert(offsetof(rasterizer_frame_statistics, framerate) == 0x00, "rasterizer_frame_statistics::framerate");
static_assert(offsetof(rasterizer_frame_statistics, sample_count) == 0x04, "rasterizer_frame_statistics::sample_count");
static_assert(offsetof(rasterizer_frame_statistics, unknown_06) == 0x06, "rasterizer_frame_statistics::unknown_06");
static_assert(offsetof(rasterizer_frame_statistics, average_framerate) == 0x08, "rasterizer_frame_statistics::average_framerate");
static_assert(offsetof(rasterizer_frame_statistics, minimum_framerate) == 0x0c, "rasterizer_frame_statistics::minimum_framerate");
static_assert(offsetof(rasterizer_frame_statistics, maximum_framerate) == 0x10, "rasterizer_frame_statistics::maximum_framerate");
static_assert(offsetof(rasterizer_frame_statistics, dropped_percentage) == 0x14, "rasterizer_frame_statistics::dropped_percentage");
static_assert(sizeof(frame_graph) == 0x32b0, "frame_graph size");
static_assert(offsetof(frame_graph, bounds) == 0x0000, "frame_graph::bounds");
static_assert(offsetof(frame_graph, name_bounds) == 0x0008, "frame_graph::name_bounds");
static_assert(offsetof(frame_graph, maximum_bounds) == 0x0010, "frame_graph::maximum_bounds");
static_assert(offsetof(frame_graph, average_bounds) == 0x0018, "frame_graph::average_bounds");
static_assert(offsetof(frame_graph, vertices) == 0x0020, "frame_graph::vertices");
static_assert(offsetof(frame_graph, frame_vertices) == 0x3020, "frame_graph::frame_vertices");
static_assert(offsetof(frame_graph, maximum) == 0x3098, "frame_graph::maximum");
static_assert(offsetof(frame_graph, average) == 0x309c, "frame_graph::average");
static_assert(offsetof(frame_graph, recent_samples) == 0x30a0, "frame_graph::recent_samples");
static_assert(offsetof(frame_graph, name) == 0x30b0, "frame_graph::name");
