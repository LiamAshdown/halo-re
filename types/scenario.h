#pragma once
// Blam scenario module (halo.exe 1.0.10 retail, 0x53e660..0x53f150, 17 Ghidra functions).
// This is the thin layer between the cache file and everything that walks the loaded map: it
// resolves the scenario and globals tags, switches the resident structure BSP in and out, and
// answers the small per-location questions the rest of the engine asks of that BSP (which
// cluster, which fog, is it water, is a point in a trigger volume).
//
// Almost everything it touches is tag data whose layout types/tags.h already carries, and that
// layout is reused here, never redefined. The offsets the code uses, all consistent with it:
//
//   Scenario (global_scenario, 0x00746f8c)
//     +0x030 skies.count      +0x034 skies.pointer        stride 0x10  ScenarioSky, tag_id +0x0c
//     +0x204 object_names.count +0x208 .pointer            stride 0x24  ScenarioObjectName
//     +0x364 trigger_volumes.pointer                       stride 0x60  ScenarioTriggerVolume
//     +0x384 netgame_equipment.count +0x388 .pointer        stride 0x90  ScenarioNetgameEquipment,
//            scenario_load stamps +0x10 (unknown_ffffffff) with -1 in every element
//     +0x5a4 structure_bsps.count +0x5a8 .pointer           stride 0x20  ScenarioBSP,
//            structure_bsp.tag_id at +0x1c
//   ScenarioStructureBSP (global_structure_bsp, 0x00746f9c)
//     +0x0b4 collision_bsp.pointer, copied to both 0x00746f90 and 0x00746f98 on every switch
//     +0x0e4 leaves.pointer   stride 0x10, ScenarioStructureBSPLeaf.cluster at +0x08
//     +0x134 clusters.count +0x138 .pointer  stride 0x68: sky +0x00, fog +0x02,
//            background_sound +0x04, weather +0x08
//     +0x14c cluster_data.pointer  the cluster PVS bit matrix, rows of (clusters.count+31)>>5 dwords
//     +0x17c fog_planes.pointer   stride 0x20: front_region +0x00, plane +0x04
//     +0x188 fog_regions.pointer  stride 0x28: fog +0x24, weather_palette +0x26
//     +0x194 fog_palette.pointer  stride 0x88: fog.tag_id +0x2c
//     +0x1fc background_sound_palette.count +0x200 .pointer  stride 0x74: background_sound.tag_id +0x2c
//   Globals (global_game_globals, 0x00746fa0)
//     +0x194 materials.count +0x198 .pointer  stride 0x374  GlobalsMaterial;
//            melee_hit_sound.tag_id at +0x370
//   Sky   +0x58 outdoor fog (color, 8 pad, density +0x6c, start +0x70, opaque +0x74),
//         +0x78 indoor fog (the same shape), indoor_fog_screen.tag_id +0xa4
//   Fog   +0x00 flags bit 0 is_water, +0x74 distance_to_water_plane
//   SoundLooping +0x00 flags bit 0 deafening_to_ais
//
// Records defined elsewhere that this module fills or takes, reused by name only:
//   bsp_leaf_reference (types/objects.h): the 8-byte {leaf_index, cluster_index} pair; it is
//     what 0x53e780 writes through ESI and what 0x53e810 / 0x53ec30 / 0x53ed60 / 0x53ee00 take
//     in EAX (they read cluster_index at +0x04). The Blam name for it is scenario_location.
//   render_fog (types/rasterizer.h): the out-block 0x53e8c0 fills (+0x04..+0x18 and +0x4c).
//   SoundEnvironment (types/tags.h): the tail of scenario_game_globals below.
//
// What the module does own is the one game-state block below, the two structure-BSP
// procedure tables, and the static fallback material. Globals are listed at the bottom.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module constants
// ---------------------------------------------------------------------------
typedef enum scenario_constants {
    k_structure_bsp_activate_procedure_count = 13,    // 0x53e680 loop count, 0x0069e8dc..0x0069e90f
    k_structure_bsp_deactivate_procedure_count = 10,  // 0x53e660 loop count, 0x0069e910..0x0069e937
    k_scenario_location_nudge_attempts = 150,         // 0x53e870 gives up after 0x96 retries
    k_scenario_game_globals_size = 0x7c,              // the game-state carve in 0x45a9c0
    k_scenario_maximum_sky_fog_states = 1,            // one per local player; 0x7c only fits one
    k_structure_bsp_index_none = -1                   // 0x0069e8d8 and scenario_game_globals +0x00
} scenario_constants;

// The cluster fog word (ScenarioStructureBSPCluster.fog, +0x02) as 0x53ec30 and 0x53ee00 decode
// it: -1 is no fog, a clear top bit is a fog region index, a set top bit is a fog plane index
// whose front_region names the region.
typedef enum scenario_cluster_fog_bits {
    k_cluster_fog_is_plane_bit = 0x8000,
    k_cluster_fog_index_mask = 0x7fff
} scenario_cluster_fog_bits;

// The single low flag bit this module tests in the first byte of three different tags.
typedef enum scenario_tag_flag_bits {
    k_fog_flag_is_water = 0x01,                  // Fog.flags bit 0 (0x53ec30, 0x53ed60, 0x53ee00)
    k_sound_looping_flag_deafening_to_ais = 0x01 // SoundLooping.flags bit 0 (0x53e810)
} scenario_tag_flag_bits;

// Tuning literals in .rdata (read with the image, floats cannot be enum members):
//   0x00672c50 15.0f      the sky fog snaps instead of blending once the camera jumps this far
//   0x00672be8 0.05f      the z nudge per retry in 0x53e870, and the blend rate scale in 0x53e8c0
//   0x00672bbc 0.0001f    minimum gap between the fog start and opaque distances
//   0x00672bd4 -FLT_MAX   0x53ee00 result when the location has no fog
//   0x00672be0 +FLT_MAX   0x53ee00 result when the fog is water but has no plane
//   0x00672ac0 0.0f / 0x00672ac4 1.0f  the clamp bounds of the fog screen blend

// ---------------------------------------------------------------------------
// structure bsp procedure tables
// 0x53e680 and 0x53e660 call every slot in order with no arguments and ignore any result, so
// the slot type is a plain void(void). The tables sit back to back in .data: activate at
// 0x0069e8dc (13 slots) then deactivate at 0x0069e910 (10 slots), read from bin/halo.exe:
//   activate:   0x4f7570 objects_recompute_cluster_membership, 0x4f2d50 object_lights_refresh_transforms,
//               0x42ce90 ai_unassigned_actors_attach_to_structure_bsp, 0x450e80 effects_refresh_structure_locations,
//               0x455d20 particles_refresh_structure_locations, 0x454080, 0x44cda0, 0x44e000
//               decal_rehash_object_decals, 0x553060 structure_runtime_decals_mark_dirty, 0x447a60
//               observer_update_location, 0x4762f0 players_structure_bsp_switch_regroup, 0x549a00
//               sounds_refresh_structure_locations, 0x4f4860 scenario_objects_place_for_structure_bsp_on_activate
//   deactivate: 0x4f47c0 objects_delete_unparented_of_type_mask, 0x4f74f0, 0x4f2cb0
//               object_lights_detach_from_structure_bsp, 0x42c940, 0x44ad80 x4 (do nothing), 0x553070
//               structure_runtime_decals_evict, 0x44e140 decals_detach_from_structure_bsp.
// ---------------------------------------------------------------------------
typedef void (*structure_bsp_procedure)(void);

// ---------------------------------------------------------------------------
// scenario_sky_fog_state  (scenario_game_globals +0x04, stride 0x2c)
// 0x53e8c0 keeps one per local player and moves it toward the fog of the current sky: the
// outdoor fog block of sky[cluster.sky] (Sky +0x58), or, when the camera cluster has no sky
// (-1), the indoor fog block of sky[0] (Sky +0x78). Each field is established by the snap path
// at 0x53ea72, which copies the sky block field by field, and by the blend path, which hands
// the address of each field to the move-toward helper 0x470d40 (0x50f520 for the color):
//   start/opaque distances blend at a rate equal to the distance moved, density, color and the
//   screen blend at 0.05 times that distance.
// The snap path also runs when the slot is not valid, the camera moved 15 units or more, or
// either opaque distance is 0. The game-state reset 0x45b050 clears exactly these 11 dwords
// (a rep stosd of 0xb dwords from +0x04).
// ---------------------------------------------------------------------------
typedef struct scenario_sky_fog_state {
    uint8_t valid;                 // 0x00 set to 1 by the snap path, tested before blending
    uint8_t unknown_01[3];         // 0x01 never read or written
    Point3D camera_position;       // 0x04 last camera position, rewritten every call; the
                                   //      distance from it gates the snap and scales the blend
    float start_distance;          // 0x10 Sky fog start_distance   (+0x70 / +0x90)
    float opaque_distance;         // 0x14 Sky fog opaque_distance  (+0x74 / +0x94)
    float maximum_density;         // 0x18 Sky fog maximum_density  (+0x6c / +0x8c)
    ColorRGB color;                // 0x1c Sky fog color            (+0x58 / +0x78)
    float fog_screen_blend;        // 0x28 target 1.0 on the indoor path when sky[0] has an
                                   //      indoor_fog_screen (tag_id at Sky +0xa4 not -1), else 0.0;
                                   //      handed out clamped to [0, 1] as render_fog +0x4c
} scenario_sky_fog_state;          // size 0x2c

// ---------------------------------------------------------------------------
// sky_fog_block  (an overlay for Sky +0x58 outdoor fog and Sky +0x78 indoor fog, 0x20 each)
// types/tags.h spells the two groups out as separate outdoor_fog_* / indoor_fog_* fields with the
// same shape; 0x53e8c0 picks one base pointer and then reads both through the same offsets, so the
// rewrite needs one type for it. This is an overlay, not a new tag struct.
// ---------------------------------------------------------------------------
typedef struct sky_fog_block {
    ColorRGB color;                // 0x00 Sky.outdoor_fog_color / indoor_fog_color
    uint8_t unknown_0c[8];         // 0x0c Sky _pad_64 / _pad_84, never read
    float maximum_density;         // 0x14 snap copies it to scenario_sky_fog_state +0x18
    float start_distance;          // 0x18 to +0x10
    float opaque_distance;         // 0x1c to +0x14; 0 forces the snap path
} sky_fog_block;                   // size 0x20

// ---------------------------------------------------------------------------
// scenario_game_globals  (the 0x7c-byte game-state block 0x00746f94 points at)
// Carved from the game-state arena by 0x45a9c0 (crc32 of the size 0x7c, pointer stored straight
// into 0x00746f94), so it is saved and restored with the game state. The layout is pinned by
// three writers that between them touch every byte:
//   - scenario_structure_bsp_switch 0x53eeb0 stores the new bsp index as a WORD at +0x00 (and
//     -1 on unload); 0x53efc0 compares it against 0x0069e8d8 after a game-state load and
//     re-switches when they differ, which is why the index is kept in the game state at all.
//     0x45afb0 and engine_shutdown_subsystems 0x541010 also store 0xffff there.
//   - the game-state reset 0x45b050 zeroes 0xb dwords from +0x04, clears the byte at +0x30, and copies the 0x12 dwords of
//     k_default_sound_environment 0x0065e508 to +0x34.
//   - 0x53e8c0 indexes +0x04 with stride 0x2c; the sound module 0x53f150 compares the byte at
//     +0x30 and interpolates the SoundEnvironment at +0x34 (out/phase4/structures_types_notes.md).
// 4 + 0x2c + 4 + 0x48 == 0x7c, so the block has room for exactly one sky fog slot: a local
// player index above 0 would run into the sound fields. PC has one local player.
// ---------------------------------------------------------------------------
typedef struct scenario_game_globals {
    int16_t structure_bsp_index;                // 0x00 the bsp the saved game expects; -1 unloaded
    uint16_t unknown_02;                        // 0x02 never read or written
    scenario_sky_fog_state sky_fog[1];          // 0x04 k_scenario_maximum_sky_fog_states
    uint8_t sound_environment_is_water;         // 0x30 owned by the sound module (0x53f150):
                                                //      the is_water flag of the fog the cached
                                                //      environment came from
    uint8_t unknown_31[3];                      // 0x31 never read or written
    SoundEnvironment sound_environment;         // 0x34 owned by the sound module (0x53f150):
                                                //      the interpolated listener environment,
                                                //      reset from k_default_sound_environment
} scenario_game_globals;                        // size 0x7c

// ---------------------------------------------------------------------------
// scenario_trigger_volume_box  (the last 0x18 bytes of ScenarioTriggerVolume, +0x48..+0x5f)
// types/tags.h names these starting_corner / ending_corner_offset, which is how
// scenario_trigger_volume_contains_point 0x53f020 reads them for the rotational type (1): it
// builds a matrix4x3 from rotation_vector_forward (+0x30) and rotation_vector_up (+0x3c) with
// 0x4cb970, puts starting_corner in the matrix position, transforms the point into that frame
// with 0x4cbf80 and requires 0 < local < ending_corner_offset on each axis.
// For the fixed type (0) the same 24 bytes are instead read as axis-aligned bounds, x then y
// then z, each as a (lower, upper) pair: x against +0x48/+0x4c, y against +0x50/+0x54, z against
// +0x58/+0x5c, strict on both sides. Any other type value returns false.
// This is an overlay for that tail, not a replacement for the tag block.
// ---------------------------------------------------------------------------
typedef struct scenario_trigger_volume_rotational_box {
    Point3D origin;                // 0x00 ScenarioTriggerVolume.starting_corner
    Vector3D extents;              // 0x0c ScenarioTriggerVolume.ending_corner_offset
} scenario_trigger_volume_rotational_box;  // size 0x18

typedef struct scenario_trigger_volume_fixed_box {
    float x_bounds[2];             // 0x00 lower, upper
    float y_bounds[2];             // 0x08
    float z_bounds[2];             // 0x10
} scenario_trigger_volume_fixed_box;       // size 0x18

typedef union scenario_trigger_volume_box {
    scenario_trigger_volume_fixed_box fixed;            // type 0 scenariotriggervolumetype_fixed
    scenario_trigger_volume_rotational_box rotational;  // type 1 scenariotriggervolumetype_rotational
} scenario_trigger_volume_box;             // size 0x18

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// global 0x0069e8d4: datum_index global_scenario_index    cache_file_load result, -1 when no map
//                    (the name types/hs.h already uses)
// global 0x0069e8d8: int16_t global_structure_bsp_index   the resident bsp, -1 while none is
//                    (types/physics.h name); only scenario_structure_bsp_switch sets it valid
// global 0x00746f8c: Scenario *global_scenario             tag data of global_scenario_index
// global 0x00746f90: ModelCollisionGeometryBSP *global_collision_bsp
//                    == global_structure_bsp->collision_bsp.pointer; the root 0x53e780 and
//                    0x53e870 hand to bsp3d_node_find_leaf 0x5013a0 in ECX. Other headers call
//                    this address global_globals (a misnomer carried over from earlier passes)
// global 0x00746f94: scenario_game_globals *global_scenario_game_globals  0x7c bytes of game state
// global 0x00746f98: ModelCollisionGeometryBSP *global_structure_collision_bsp  the same value
//                    as 0x00746f90, written by the same two stores in 0x53eeb0
// global 0x00746f9c: ScenarioStructureBSP *global_structure_bsp   tag data of the resident bsp
// global 0x00746fa0: Globals *global_game_globals          tag data of "globals\globals" ('matg')
// global 0x0069e8dc: structure_bsp_procedure structure_bsp_activate_procedures[13]
// global 0x0069e910: structure_bsp_procedure structure_bsp_deactivate_procedures[10]
// global 0x006e3208: GlobalsMaterial k_default_global_material   0x374 bytes of .bss returned
//                    for an out-of-range material index by 0x53e7c0, whose melee_hit_sound.tag_id
//                    (0x006e3578 == 0x006e3208 + 0x370) is set to -1 on first use
//
// Globals this module reads or writes but does not own:
//   0x0087bc14  tag_instance *tag_instances              (types/cache.h) stride 0x20, data +0x14
//   0x006a8958  structure_bsp_data                       (types/cache.h) cleared by 0x53efc0
//   0x00721e4c  uint8_t material_table_warning_issued    (types/physics.h) the first-use latch of
//               k_default_global_material; 0x507cc0 inlines the same lookup
//   0x00719769 / 0x0071976a  main-loop frame latches     0x53eeb0 clears both before the switch
//               and sets 0x0071976a on the way out; every other user is the main loop 0x4c7610,
//               its tail 0x4c7f10 or 0x5381c0, so they belong to main
//   0x0065512c  k_empty_string                           (types/hs.h) the buffer the inlined
//               error-print loop of scenario_load scans on a failed cache_file_load
//   0x0065e508  k_default_sound_environment              (types/sound.h)
//   0x007c32f4  render_fog render_fog                    (types/render.h) the 0x53e8c0 output
#pragma pack(pop)
