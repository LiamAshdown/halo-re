// Blam objects module (halo.exe 1.0.10 retail, 0x4088e0..0x4ffda0, 252 functions).
// The object system: the object data_array and the variable-length object record that every
// biped/vehicle/weapon/.../sound_scenery extends, the creation and damage paths, the object
// type definition table with its vtable columns, the per-object lighting and attachment
// bookkeeping, and the widget subsystems that hang off an object (antenna, flag, glow, plus
// the light_volume and lightning render hooks).
//
// Offsets in comments are byte offsets from the struct base and were recovered from the
// decompiled module. Where the binary itself carries a layout it is used in preference to the
// decompiler and the fact is called out on the struct:
//   - widget_type_definition is read straight out of .data at 0x0069c010 (5 rows of 0x28
//     bytes, PE VA 0x676000 maps to file offset 0x276000). The group tags and function
//     pointers there fixed the widget type order and corrected several function attributions
//     (see out/phase4/objects_types_notes.md).
//   - every tag-side layout this module touches already exists in types/tags.h; each one was
//     verified against the arithmetic here rather than redefined.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h  data_array, datum_index, data_iterator, memory_pool, datum_header
//   types/math.h    real_matrix4x3 (the object node array element, and both halves of
//                   object_marker), real_point3d, real_vector3d
//   types/cache.h   tag_instance (the 0x20-byte table at 0x0087bc14 that every tag-data
//                   lookup in this module goes through: path at +0x10, data at +0x14)
//   types/tags.h    Object (0x17c, the object definition tag: object_type 0x00, flags 0x02,
//                   scales_change_colors 0x24, model TagID 0x34, animation_graph TagID 0x44,
//                   collision_model TagID 0x7c, creation_effect TagID 0xac,
//                   forced_shader_permutation_index 0x13e, attachments 0x140, widgets 0x14c,
//                   functions 0x158, change_colors 0x164), ObjectAttachment (0x48),
//                   ObjectWidget (0x20), ObjectFunction (0x168), ObjectChangeColors (0x2c),
//                   GBXModel (nodes 0xb8, regions 0xc4), ModelRegion (0x4c, permutations
//                   0x40), ModelRegionPermutation (0x58, flags 0x20, permutation_number 0x24),
//                   ModelCollisionGeometry (0x298, maximum body vitality 0x08, maximum shield
//                   vitality 0xcc, regions 0x240), ModelCollisionGeometryRegion (0x54, flags
//                   0x20), DamageEffect, Antenna (vertices 0xc4) and AntennaVertex (0x80),
//                   Flag (width 0x0c, height 0x0e, attachment_points 0x54), Glow (0x154,
//                   number_of_particles 0x20, glow_flags 0x28), Light (duration 0xf4),
//                   LightVolume, Lightning
//
// Note on sizes: like types/memory.h, structs holding pointers only measure to the documented
// size under a 32-bit data organization.

#include <stddef.h> // offsetof
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum object_constants {
    k_maximum_objects = 0x800,             // objects_initialize: game_state_new("object", 0x800)
    k_maximum_object_types = 12,           // the pointer table at 0x0069bfdc has 12 entries
    k_maximum_object_subdefinitions = 16,  // object_type_definition_chain_build scans 16 slots
    k_maximum_object_regions = 8,          // the region byte arrays at object 0x178 and 0x180
    k_maximum_object_attachments = 8,      // object 0x144 type bytes, object 0x14c handles
    k_maximum_object_functions = 4,        // object 0x134 out values, bitmask byte at 0x123
    k_maximum_object_change_colors = 4,    // object 0x1b8, four ColorRGB
    k_maximum_object_names = 0x200,        // 0x006b8cb8 is 0x800 bytes of int32
    k_maximum_clusters = 0x200,            // every per-cluster head table is 0x200 int32
    k_maximum_widget_types = 5,            // 0x0069c010, five rows of 0x28 bytes
    k_maximum_widgets = 0x40,              // widgets_initialize: data_new("widget", 0x40)
    k_maximum_antennas = 12,               // antennas_initialize: data_new("antenna", 0xc)
    k_maximum_flags = 2,                   // flags_initialize: data_new("flag", 2)
    k_maximum_lights = 0x380,              // lights_initialize: game_state_new("lights", 0x380)
    k_maximum_transient_lights = 8,        // light_transient_add refuses at 8
    k_maximum_glow_markers = 5,            // the object_marker array inside a glow instance
    k_maximum_flag_cloth_vertices = 0xe1,  // flag_new refuses width*height of 0xe1 or more
    k_maximum_flag_cloth_cells = 196,      // (0x16bc - 0x1534) / 2
    k_maximum_damage_candidates = 0x40,    // damage_apply_area_effect result buffer
    k_object_gather_maximum = 0x800        // object_find_in_sphere intermediate buffer
} object_constants;

// object_type: the byte at object_header 0x03 and the int16 at object 0xb4. Same values as
// the ObjectType enum in types/tags.h; declared again here in the engine spelling because
// this module builds type masks out of them.
typedef enum object_type {
    _object_type_biped = 0,
    _object_type_vehicle = 1,
    _object_type_weapon = 2,
    _object_type_equipment = 3,
    _object_type_garbage = 4,
    _object_type_projectile = 5,
    _object_type_scenery = 6,
    _object_type_device_machine = 7,
    _object_type_device_control = 8,
    _object_type_device_light_fixture = 9,
    _object_type_placeholder = 10,
    _object_type_sound_scenery = 11
} object_type;

// Masks built from 1 << object_type. object_try_and_get and object_iterator_next take one.
typedef enum object_type_mask {
    _object_mask_none = 0,
    _object_mask_biped = 0x001,
    _object_mask_vehicle = 0x002,
    _object_mask_unit = 0x003,            // object_apply_damage, object_get_controlling_player_index
    _object_mask_weapon = 0x004,
    _object_mask_equipment = 0x008,
    _object_mask_garbage = 0x010,
    _object_mask_item = 0x01c,
    _object_mask_projectile = 0x020,
    _object_mask_scenery = 0x040,
    _object_mask_device_machine = 0x080,
    _object_mask_device_control = 0x100,
    _object_mask_device_light_fixture = 0x200,
    _object_mask_device = 0x380,
    _object_mask_placeholder = 0x400,
    _object_mask_sound_scenery = 0x800,
    // objects_delete_unparented_of_type_mask and scenario_objects_place_for_structure_bsp both
    // iterate with 0x240, that is scenery plus device_light_fixture.
    _object_mask_scenery_and_light_fixture = 0x240,
    // object_new_with_datum_role_control skips the per-node function blocks for these types.
    _object_mask_no_node_functions = 0xfe0,
    _object_mask_all = 0xffffffff
} object_type_mask;

// object_header flags, the byte at object_header 0x02. object_new_with_datum_role_control
// stamps 0x44 onto a fresh slot; objects_update consumes 0x04, 0x08 and 0x10.
typedef enum object_header_flags {
    _object_header_active_bit = 0x01,         // object_mark_pending_delete sets it,
                                              // object_clear_pending_delete_flag clears it
    _object_header_unknown_02_bit = 0x02,
    _object_header_needs_update_bit = 0x04,   // objects_update runs object_update then clears it
    _object_header_delete_pending_bit = 0x08, // object_delete_recursive sets it,
                                              // object_is_delete_pending reads it
    _object_header_just_created_bit = 0x10,   // cleared at the top of the objects_update sweep
    _object_header_connected_bit = 0x20,      // set at create together with 0x40
    _object_header_in_pvs_pass_bit = 0x40,    // objects_update only reconciles slots with 0x60
    _object_header_unknown_80_bit = 0x80
} object_header_flags;

// object flags, the uint32 at object 0x10.
typedef enum object_flags {
    _object_no_collision_bit = 0x00000001,       // object_damage_apply_line_of_sight inverts bit 0
    _object_in_water_bit = 0x00000010,           // the object is in a water medium. projectile_new
                                                 //   (0x4bd7c0) probes the medium through the
                                                 //   structure BSP globals and sets or clears it;
                                                 //   projectile_compute_deceleration (0x4c0310)
                                                 //   and projectile_update's gravity pick branch
                                                 //   on it, and the water overpenetrate response
                                                 //   in projectile_response (0x4bf390) toggles it
    _object_at_rest_bit = 0x00000020,            // at rest / physics suspended: the at-rest
                                                 //   column of the memory dump; both attach
                                                 //   paths set it and 0x4bef80 (impulse and
                                                 //   spin) clears it (0x4bf0af)
    _object_needs_cluster_update_bit = 0x00000800,
    _object_mirrored_geometry_bit = 0x00001000,  // object_get_node_local_transform negates the
                                                 // marker axis when it is set
    _object_unknown_2000_bit = 0x00002000,       // set by projectile_new (0x4bd7f2)
    _object_in_tracked_list_bit = 0x00010000,    // object_list_membership_set; the garbage
                                                 // column of the memory dump
    _object_unknown_20000_bit = 0x00020000,
    _object_definition_flag0_bit = 0x00040000,   // copied from Object tag flags bit 0 at create;
                                                 //   projectile_new sets 0xc0000 (0x4bda1c)
    _object_connected_to_map_bit = 0x00080000,   // the post-setup gate in the constructor
    _object_do_not_delete_bit = 0x00100000,      // object_mark_pending_delete refuses when set
    _object_outside_map_bit = 0x00200000,        // the outside column of the memory dump
    _object_has_collision_model_bit = 0x02000000,// set when the Object tag collision_model
                                                 // TagID is valid
    _object_changed_bit = 0x04000000,            // object_datum_consume_pending_flag tests and
                                                 // clears it; also set after a successful attach
                                                 // broadcast (0x4c28d5); also OR-ed in at 0x4c3a10, 0x4ed981
    _object_took_network_update_bit = 0x08000000 // "has taken a network update": the gate and the
                                                 //   set in projectile_apply_network_update
                                                 //   (0x4c1070) and its weapon sibling
} object_flags;

// object vitality flags, the uint16 at object 0x106. The module reaches the high half through
// the byte at 0x107, so bit 0x0200 is written as 0x02 on that byte, and so on.
typedef enum object_vitality_flags {
    _object_health_below_low_bit = 0x0001,     // object_apply_body_damage crossed the low threshold
    _object_shield_below_low_bit = 0x0002,     // object_apply_shield_damage crossed the low threshold
    _object_health_frozen_bit = 0x0004,        // the dead column of the memory dump; guards the
                                               // health restore and stun clear paths
    _object_shield_depleted_bit = 0x0008,      // set when shield damage breaks through
    _object_shield_recharging_bit = 0x0010,    // object_shield_recharge_start
    _object_region_response_80_bit = 0x0080,   // object_destroy_region, region flag 0x20
    _object_region_response_100_bit = 0x0100,  // object_destroy_region, region flag 0x40
    _object_region_response_200_bit = 0x0200,  // object_destroy_region, region flag 0x80
    _object_region_response_400_bit = 0x0400,  // object_destroy_region, region flag 0x100
    _object_hash_flag_bit = 0x0800,            // object_hash_set_flag_bit3 and its clear twin
    _object_stunned_bit = 0x1000,              // object_update_vitality_and_regeneration
    _object_shield_stationary_bit = 0x2000     // object_clear_stationary_flag
} object_vitality_flags;

// object_attachment_type: the signed byte array at object 0x144. Written by
// object_create_attachments from the group tag of the ObjectAttachment reference, read back
// by object_delete_attachments and object_for_each_light_attachment.
typedef enum object_attachment_type {
    _object_attachment_type_none = -1,
    _object_attachment_type_light = 0,          // group tag 0x6c696768
    _object_attachment_type_looping_sound = 1,  // group tag 0x6c736e64
    _object_attachment_type_effect = 2,         // group tag 0x65666665
    _object_attachment_type_contrail = 3,       // group tag 0x636f6e74
    _object_attachment_type_particle_system = 4 // group tag 0x7063746c
} object_attachment_type;

// object_widget_type: the int16 at widget 0x02. The order is the row order of
// widget_type_definition in .data at 0x0069c010, read directly out of bin/halo.exe.
typedef enum object_widget_type {
    _object_widget_type_flag = 0,         // group tag 0x666c6167
    _object_widget_type_antenna = 1,      // group tag 0x616e7421
    _object_widget_type_glow = 2,         // group tag 0x676c7721
    _object_widget_type_light_volume = 3, // group tag 0x6d677332
    _object_widget_type_lightning = 4     // group tag 0x656c6563
} object_widget_type;

// The selector object_function_evaluate_input switches on, and the value stored in the
// ObjectFunction scale_period_by / scale_function_by / add / scale_result_by fields. Values
// 1..4 index object.function_in_values, values 5..8 index object.function_out_values; both
// branches compile to object + 0x120 + 4*selector, which is why Ghidra folds them together.
typedef enum object_function_input {
    _object_function_input_none = 0,
    _object_function_input_a_in = 1,
    _object_function_input_b_in = 2,
    _object_function_input_c_in = 3,
    _object_function_input_d_in = 4,
    _object_function_input_a_out = 5,
    _object_function_input_b_out = 6,
    _object_function_input_c_out = 7,
    _object_function_input_d_out = 8
} object_function_input;

// object_globals.ambient_cluster_mode, set by objects_set_ambient_cluster_override and read
// by objects_get_ambient_cluster.
typedef enum object_ambient_cluster_mode {
    _object_ambient_cluster_none = 0,
    _object_ambient_cluster_from_tracked_object = 1,
    _object_ambient_cluster_override = 2
} object_ambient_cluster_mode;

// ---------------------------------------------------------------------------
// object_type_definition  (the 12-entry pointer table at 0x0069bfdc, chained into one list by
// object_type_definition_chain_build)
// A definition carries an array of up to 16 sub-definitions; the chain threads every one of
// them through the link at 0xc0, and the 0x14..0x7c columns are the engine vtable. The
// broadcast helpers at 0x4f3e30..0x4f4480 call one column across every sub-definition of one
// type; the override helpers at 0x4f44f0..0x4f4760 scan the sub-definition array from index
// 15 downwards and use the first non-null entry.
// ---------------------------------------------------------------------------
typedef struct object_type_definition {
    char *name;                            // 0x00 object_dump_write prints it
    uint32_t group_tag;                    // 0x04 tag_group (cache.h) fourcc of the definition
                                           //      tag: 'bipd' 'vehi' 'weap' 'eqip' 'garb' 'proj'
                                           //      'scen' 'mach' 'ctrl' 'lifi' 'plac' 'ssce',
                                           //      read out of the 12 rows via 0x0069bfdc. (The
                                           //      old "0 = delete immediately, 3 = recursively"
                                           //      reading was the network role, not this field.)
    int16_t object_size;                   // 0x08 runtime block size passed to object_block_data_new
    int16_t scenario_placement_offset;     // 0x0a byte offset of this type's placement tag_block
                                           //      inside the Scenario tag data; -1 for garb, proj,
                                           //      plac. 0x4f4880 does cmp WORD [eax+0xa],-1 and
                                           //      adds it to the scenario pointer 0x00746f8c
    int16_t scenario_palette_offset;       // 0x0c byte offset of the matching palette tag_block
                                           //      (entries 0x30 bytes, tag id at +0x0c); -1 when
                                           //      there is none
    int16_t scenario_placement_size;       // 0x0e stride of one placement entry (0x4f4880
                                           //      movsx [eax+0xe]): bipd 0x78, vehi 0x78, weap
                                           //      0x5c, eqip 0x28, scen 0x48, mach 0x40, ctrl
                                           //      0x40, lifi 0x58, ssce 0x28, else -1
    int32_t network_delta_message_type;    // 0x10 -1 = the type sends no delta updates (garb,
                                           //      scen, mach, ctrl, lifi, plac, ssce); proj 1,
                                           //      eqip 2, weap 3, bipd 4, vehi 5. The multiplayer
                                           //      spawn paths choose network role 0 vs 3 on != -1
    void *initialize;                      // 0x14 object_type_definition_chain_build
    void *dispose;                         // 0x18 objects_dispose
    void *reset;                           // 0x1c objects_reset
    void *flush;                           // 0x20 objects_flush_dirty_state
    void *notify_created;                  // 0x24 object_type_definitions_notify_0x24
    void *query_create;                    // 0x28 AND style, object_type_definitions_query_0x28
    void *notify_two_args_2c;              // 0x2c
    void *notify_delete;                   // 0x30
    void *query_34;                        // 0x34 OR style
    void *notify_38;                       // 0x38
    void *notify_3c;                       // 0x3c
    void *notify_region_damage;            // 0x40 object_destroy_region forwards the region here
    void *query_44;                        // 0x44 OR style, used by object_children_recurse_prune
    void *notify_two_args_48;              // 0x48
    void *notify_4c;                       // 0x4c
    void *notify_reset_scale;              // 0x50 object_reset_default_scale_and_color
    void *notify_54;                       // 0x54
    void *notify_58;                       // 0x58
    void *notify_5c;                       // 0x5c
    void *unknown_60;                      // 0x60 never reached from this module
    void *override_get_64;                 // 0x64 object_type_override_get_0x64
    void *override_call_68;                // 0x68
    void *override_call_6c;                // 0x6c
    void *override_call_70;                // 0x70
    void *override_call_74;                // 0x74 defaults to true when no override exists
    void *unknown_78;                      // 0x78 never reached from this module
    void *override_call_7c;                // 0x7c
    struct object_type_definition *subdefinitions[16]; // 0x80 the first null entry ends the array
    struct object_type_definition *next;   // 0xc0 chain link, head at 0x008603dc
} object_type_definition;                  // size 0xc4 as far as this module reaches. The
                                           // pointer table gives no stride, so whether the
                                           // record continues past 0xc4 is unresolved.

// ---------------------------------------------------------------------------
// object_header  (element of the object data_array, stride 0x0c, fixed by every
// *(data + 8 + index*0xc) object lookup in the module)
// ---------------------------------------------------------------------------
typedef struct object_header {
    int16_t identifier;         // 0x00 datum salt, 0 marks the slot empty
    uint8_t flags;              // 0x02 object_header_flags
    uint8_t type;               // 0x03 object_type, tested as 1 << type against a type mask
    int16_t cluster_index;      // 0x04 cached BSP cluster, -1 at create; objects_update tests
                                //      it against the per-frame PVS bitset
    int16_t block_size;         // 0x06 bytes of pool storage behind data, grown by
                                //      object_block_data_grow, reported by the memory dump
    struct object *data;        // 0x08
} object_header;                // size 0x0c

// ---------------------------------------------------------------------------
// object_block_reference  (object 0x1e8, 0x1ec and 0x1f0)
// object_block_data_grow(field_offset, byte_count) appends byte_count zeroed bytes to the
// pool block, then writes {size, offset} at object + field_offset. Consumers recompute the
// array address as (uint8_t *)object + offset.
// ---------------------------------------------------------------------------
typedef struct object_block_reference {
    int16_t size;               // 0x00 bytes
    int16_t offset;             // 0x02 byte offset from the object base
} object_block_reference;       // size 0x04

// ---------------------------------------------------------------------------
// object  (the common header every object type extends; the whole record lives in the
// "objects" memory_pool block that object_block_data_new allocates, so one real record is
// object_type_definition.object_size bytes plus the three trailing arrays)
// Fields past 0x1f4 that show up in this module (0x218 on bipeds and vehicles, 0x328 on
// units, 0x1f8 and 0x1fc) belong to the unit extension and are not part of this struct.
// ---------------------------------------------------------------------------
typedef struct object {
    datum_index definition_tag;     // 0x000 the Object tag; every tag-data lookup starts here
    int32_t network_role;           // 0x004 the role/control value object_new was called with;
                                    //       object_delete dispatches on 0 versus 3
    uint8_t unknown_008;            // 0x008
    uint8_t network_state_009;      // 0x009 network state: projectile_new (0x4bda48),
                                    //       weapon_new and equipment_new zero it together with
                                    //       their three per-type network bytes when the game is
                                    //       a network client or server (0x00719720 == 1 or 2)
    uint8_t unknown_00a[2];         // 0x00a
    int32_t network_update_tick;    // 0x00c game tick stamp, -1 = never (-1 at create).
                                    //       projectile_is_old_enough 0x4c1270: -1 counts as
                                    //       old, else game_time >= stamp + [0x006894c8]
    uint32_t flags;                 // 0x010 object_flags
    int32_t cluster_stamp;          // 0x014 compared against 0x008603cc so that
                                    //       object_collect_in_clusters reports each object
                                    //       once per gather
    // 0x018..0x05b: the network interpolation block. projectile_apply_network_update
    // (0x4c1070) stores, for one accepted update: BYTE [ebp+0x18] = 1, the position at
    // [ebp+0x1c] (3 dwords), BYTE [ebp+0x44] = 1 and the velocity at [ebp+0x48] (3 dwords);
    // the weapon and equipment siblings do the same. The old player_visibility_mask at 0x020
    // was a misreading: 0x4f4880 sets and clears bit (structure BSP index) of WORD +0x20 of
    // the SCENARIO placement entries it walks (placements.address + i * stride), never of an
    // object, so no object field carries that mask.
    uint8_t network_position_valid; // 0x018 zeroed at create, 1 once an update is accepted
    uint8_t unknown_019[3];         // 0x019
    real_point3d network_position;  // 0x01c
    uint8_t unknown_028[0x1c];      // 0x028
    uint8_t network_velocity_valid; // 0x044
    uint8_t unknown_045[3];         // 0x045
    real_vector3d network_velocity; // 0x048
    uint8_t network_timestamp_valid;// 0x054 object_nudge_position_by_velocity (0x4f7c40)
                                    //       requires 0x18, 0x44 and this byte all == 1
    uint8_t unknown_055[3];         // 0x055
    uint32_t network_timestamp;     // 0x058 millisecond stamp; 0x4f7c40 subtracts it from
                                    //       time_query_performance_counter_ms (0x449210) and
                                    //       extrapolates network_position along velocity
    real_point3d position;          // 0x05c object_get_position, object_get_world_matrix
    real_vector3d velocity;         // 0x068 object_get_root_object_velocities, first output
    real_vector3d forward;          // 0x074 object_get_orientation, first output
    real_vector3d up;               // 0x080 object_get_orientation, second output
    real_vector3d angular_velocity; // 0x08c object_get_root_object_velocities, second output
    int32_t location_leaf_index;    // 0x098 object_set_cluster_and_parent
    int16_t location_cluster_index; // 0x09c -1 at create; mirrored into object_header 0x04
    int16_t unknown_09e;            // 0x09e
    real_point3d bounding_center;   // 0x0a0 the object_find_in_sphere sphere centre and the
                                    //       damage line-of-sight target point
    float bounding_radius;          // 0x0ac written by every bounding-radius variant
    float scale;                    // 0x0b0 multiplies the radius when non-zero
    int16_t type;                   // 0x0b4 object_type, copied from Object tag offset 0
    int16_t unknown_0b6;            // 0x0b6
    int16_t owner_team;             // 0x0b8 the owning team (formerly name_index). Copied from
                                    //       object_placement_data.owner_team; 0x42b8f4/0x42b8fb
                                    //       load it from two objects into CX/DX for
                                    //       teams_are_enemies 0x45bd50; 0x44b298 bounds it to
                                    //       0..9; object_placement_data_initialize (0x4f5411)
                                    //       copies the creating object's team (inheritance)
    int16_t render_cache_slot;      // 0x0ba -1 at create; object_reserve_render_cache_slot
    uint32_t unknown_0bc;           // 0x0bc
    uint32_t owner_linkage;         // 0x0c0 seeded from the creating object at the same offset
    datum_index creator_object;     // 0x0c4 the creating object (formerly unknown_0c4), from
                                    //       object_placement_data 0x0c (0x4f5705);
                                    //       projectile_send_creation resolves it through the
                                    //       object network hash table and projectile_new walks
                                    //       it up parent_object to find the firing unit
    uint32_t unknown_0c8;           // 0x0c8
    datum_index animation_graph;    // 0x0cc from Object tag animation_graph TagID
    int16_t animation_index;        // 0x0d0 -1 at create; object_start_animation
    int16_t animation_frame;        // 0x0d2 object_animation_get_frames_remaining
    int16_t unknown_0d4;            // 0x0d4
    int16_t node_function_count;    // 0x0d6 grown by object_copy_default_node_transforms
    float maximum_body_vitality;    // 0x0d8 ModelCollisionGeometry offset 0x08
    float maximum_shield_vitality;  // 0x0dc ModelCollisionGeometry offset 0xcc
    float body_vitality;            // 0x0e0 fraction of maximum_body_vitality, 1.0 when whole
    float shield_vitality;          // 0x0e4 fraction of maximum_shield_vitality
    float current_shield_damage;    // 0x0e8 flicker meter, clamped to 1.0
    float current_body_damage;      // 0x0ec
    datum_index damage_owner;       // 0x0f0 -1 at create; object_clear_references_to_object
                                    //       scrubs every object that points at a dying one
    float recent_shield_damage;     // 0x0f4 clamped to 1.0 by object_apply_shield_damage
    float recent_body_damage;       // 0x0f8
    int32_t shield_damage_ticks;    // 0x0fc zeroed whenever the shield takes damage
    int32_t body_damage_ticks;      // 0x100 -1 at create
    int16_t shield_stun_ticks;      // 0x104 game tick stamp taken from the tick counter
    uint16_t vitality_flags;        // 0x106 object_vitality_flags
    uint32_t unknown_108;           // 0x108
    datum_index placement_id;       // 0x10c -1 at create; object_get_root_parent_placement
                                    //       returns it for the root of an attachment chain
    datum_index next_tracked_object;// 0x110 singly linked list rooted at object_globals 0x08
    datum_index next_object;        // 0x114 next sibling in the parent child list
    datum_index first_child_object; // 0x118
    datum_index parent_object;      // 0x11c -1 when unattached; the chain every root walk follows
    uint8_t parent_marker_index;    // 0x120 0xff when unattached
    uint8_t unknown_121;            // 0x121
    uint8_t unknown_122;            // 0x122 set by object_shield_recharge_start
    uint8_t function_valid_flags;   // 0x123 bit i is set when function_out_values[i] is live
    float function_in_values[4];    // 0x124 a_in..d_in, written by the owning object type
    float function_out_values[4];   // 0x134 written by object_update_functions, read by
                                    //       object_function_get_value
    int8_t attachment_types[8];     // 0x144 object_attachment_type, -1 for an empty slot
    datum_index attachment_handles[8]; // 0x14c the light, looping sound, effect, contrail or
                                    //       particle system instance for each attachment
    datum_index first_widget;       // 0x16c -1 at create; head of the widget list
    datum_index cached_render_state_index; // 0x170 -1 at create; element of the 0x100-byte
                                    //       table at *0x007c30ec (0x50f150 loads it at 0x50f171
                                    //       and stores it back at 0x50f23d)
    uint16_t destroyed_region_flags;// 0x174 one bit per region; object_destroy_region refuses
                                    //       a region whose bit is already set
    uint16_t forced_shader_permutation; // 0x176 copied from Object tag offset 0x13e
    uint8_t region_vitality[8];     // 0x178 per-region damage accumulator in 0..255;
                                    //       object_apply_body_damage compares it, scaled by
                                    //       1/255, against the region damage_threshold at
                                    //       ModelCollisionGeometryRegion 0x28
    uint8_t region_permutations[8]; // 0x180 active permutation index per region; the array
                                    //       object_set_permutation_by_name and
                                    //       object_regions_initialize_permutations write,
                                    //       object_regions_reset_permutation_lock forces to
                                    //       0 or 1, and model_markers_get_by_name is handed
    uint8_t unknown_188[0x30];      // 0x188 untouched by this module
    ColorRGB change_colors[4];      // 0x1b8 object_update_change_colors, clamped to [0,1]
    object_block_reference node_function_values;   // 0x1e8 node_count * 0x20 bytes
    object_block_reference node_function_defaults; // 0x1ec node_count * 0x20 bytes, the source
                                    //       object_copy_default_node_transforms copies from
    object_block_reference nodes;   // 0x1f0 node_count * 0x34, an array of real_matrix4x3;
                                    //       object_get_node_marker_address indexes it
} object;                           // size 0x1f4 for the common header

// ---------------------------------------------------------------------------
// object_placement_data  (object_placement_data_initialize zeroes 0x22 dwords, then
// object_new_with_datum_role_control copies the fields below into the new object)
// ---------------------------------------------------------------------------
typedef struct object_placement_data {
    datum_index definition_tag;     // 0x00 the Object tag to create
    uint32_t flags;                 // 0x04 bit 0 sets the object mirrored-geometry flag,
                                    //      bit 1 gates the connect-to-map step
    uint32_t owner_linkage;         // 0x08 goes to object 0xc0, taken from the current object
    uint32_t role;                  // 0x0c goes to object 0xc4 (object.creator_object);
                                    //      object_placement_data_initialize sets it to the
                                    //      source object handle (0x4f5405)
    uint32_t unknown_10;            // 0x10
    int16_t owner_team;             // 0x14 goes to object 0xb8 (object.owner_team); 0xffff
                                    //      when there is no current object, else the current
                                    //      object's team (0x4f5411). Formerly name_index.
    int16_t permutation_group;      // 0x16 goes to object 0xbe
    real_point3d position;          // 0x18 goes to object position, then offset along up
    float height_above_origin;      // 0x24 position += height_above_origin * up
    real_vector3d velocity;         // 0x28
    real_vector3d forward;          // 0x34 defaults to the constant vector at 0x00696718
    real_vector3d up;               // 0x40 defaults to the constant vector at 0x00696720
    real_vector3d angular_velocity; // 0x4c
    real_vector3d network_vectors[4]; // 0x58 four vectors seeded from the constant at
                                    //      0x00686b04 and handed to object_set_position_network
} object_placement_data;            // size 0x88

// ---------------------------------------------------------------------------
// damage_data  (damage_data_initialize zeroes 0x15 dwords and seeds the sentinels)
// ---------------------------------------------------------------------------
typedef struct damage_data {
    datum_index damage_effect_tag;  // 0x00 the DamageEffect definition
    uint32_t flags;                 // 0x04 bits 0 and 2 suppress the parent-chain walk,
                                    //      bit 5 marks the child currently being recursed into
    datum_index responsible_player; // 0x08 -1; validated against the player data_array and
                                    //      reset to -1 when that player is gone
    datum_index responsible_object; // 0x0c -1; resolved with the unit type mask
    int16_t team_index;             // 0x10 0xffff; indexes the ten-team bitfield
    int16_t unknown_12;             // 0x12
    int32_t location_leaf_index;    // 0x14 damage_apply_area_effect hands 0x14 to
                                    //      object_find_in_sphere as the search origin location
    int16_t location_cluster_index; // 0x18 0xffff
    int16_t unknown_1a;             // 0x1a
    real_point3d epicentre;         // 0x1c sphere centre of the area search
    real_point3d origin;            // 0x28 ray origin of the line-of-sight test
    real_vector3d direction;        // 0x34 UNSURE: written by callers outside this module and
                                    //      consumed by the knockback impulse
    float random_blend;             // 0x40 1.0; blends the fixed and random damage amounts
    float multiplier;               // 0x44 1.0; divided by the child count when a vehicle
                                    //      spreads damage across its seated bipeds
    uint32_t unknown_48;            // 0x48
    int16_t material_type;          // 0x4c 0xffff; the collision material of the damaged
                                    //      surface. 0x4ffde0 hands it to 0x53e7c0 (the matg
                                    //      materials block, stride 0x374) and scales by
                                    //      DamageEffect +0x200 + material_type * 4
    int16_t unknown_4e;             // 0x4e
    uint32_t unknown_50;            // 0x50
} damage_data;                      // size 0x54

// ---------------------------------------------------------------------------
// damage_effect_vector_block  (the flat 15-float argument block damage_effect_new_at_location
// 0x4f0010 builds on its stack and hands to the effects-module spawner at 0x450870, paired
// positionally with the five name strings "normal", "incident", "negative incident",
// "reflection" and "gravity")
// UNSURE: Ghidra shows this as five separate 3-float locals (local_98 .. local_60) that happen
// to be contiguous and in stack order. The pairing with the five names is by position only;
// vector4 turns out to be the constant triple copied from PTR_DAT_0069672c, and vector0 is
// reused as reflection scratch part way through, so the field names below are the slot order,
// not confirmed semantics.
// ---------------------------------------------------------------------------
typedef struct damage_effect_vector_block {
    real_vector3d vector0;          // 0x00 incident, then overwritten with 2*(n.i)*i scratch
    real_vector3d vector1;          // 0x0c negated normal
    real_vector3d vector2;          // 0x18 normal
    real_vector3d vector3;          // 0x24 reflection, normal - vector0
    real_vector3d vector4;          // 0x30 the 0x0069672c constant ("gravity")
} damage_effect_vector_block;       // size 0x3c

// ---------------------------------------------------------------------------
// object_iterator  (object_iterator_next)
// ---------------------------------------------------------------------------
typedef struct object_iterator {
    uint32_t type_mask;             // 0x00 tested as 1 << object_header.type
    uint8_t flags_mask;             // 0x04 every bit of it must be set in object_header.flags
    uint8_t unknown_05;             // 0x05
    int16_t index;                  // 0x06 the next slot to look at
    datum_index handle;             // 0x08 handle of the object just returned
} object_iterator;                  // size 0x0c

// ---------------------------------------------------------------------------
// object_shield_impulse_result  (the caller-owned out-block object_apply_shield_damage
// @0x4ef820 writes through its tenth parameter). The parameter reaches that function in an
// EBX Ghidra never sources, so only the two fields it writes are established.
// ---------------------------------------------------------------------------
typedef struct object_shield_impulse_result {
    uint8_t unknown_00[4];          // 0x00
    float shield_damage_dealt;      // 0x04
    uint8_t depleted_this_call;     // 0x08
} object_shield_impulse_result;     // UNSURE: size and layout inferred only from those two writes


// ---------------------------------------------------------------------------
// object_globals  (0x006b8cbc, the 0x98-byte game-state block objects_initialize reserves)
// ---------------------------------------------------------------------------
typedef struct object_globals {
    uint8_t unknown_00;             // 0x00
    uint8_t collecting_in_clusters; // 0x01 raised around the object_collect_in_clusters walk
    uint8_t unknown_02[2];          // 0x02
    int16_t unknown_04;             // 0x04 zeroed at the top of objects_update
    int16_t unknown_06;             // 0x06
    datum_index first_tracked_object; // 0x08 head of the object_list_membership_set list
    uint32_t cluster_pvs_previous[16]; // 0x0c last frame bitset, one bit per cluster
    uint32_t cluster_pvs_current[16];  // 0x4c this frame bitset, copied in from the BSP
                                    //      globals; a difference drives the create and
                                    //      delete sweep in objects_update
    uint32_t unknown_8c;            // 0x8c
    int16_t ambient_cluster_mode;   // 0x90 object_ambient_cluster_mode
    int16_t unknown_92;             // 0x92
    int16_t ambient_cluster_index;  // 0x94
    int16_t unknown_96;             // 0x96
} object_globals;                   // size 0x98

// ---------------------------------------------------------------------------
// object_cluster_reference  (element of the collideable and noncollideable reference
// data_arrays at 0x008603d4 and 0x008603c4, stride 0x0c)
// Each BSP cluster owns a chain of these; object_collect_in_clusters walks it.
// ---------------------------------------------------------------------------
typedef struct object_cluster_reference {
    int16_t identifier;             // 0x00 datum salt
    int16_t unknown_02;             // 0x02
    datum_index object_index;       // 0x04
    datum_index next_reference;     // 0x08
} object_cluster_reference;         // size 0x0c

// ---------------------------------------------------------------------------
// object_placement_cursor  (the two-dword out-block object_get_root_parent_placement 0x4f5f70
// writes through ESI, and that object_test_in_atmosphere_zone 0x4f76e0 then walks)
// The first slot is the ADDRESS of whichever of the two adjacent three-global groups applies to
// the root object -- {0x008603c0, 0x008603c4, 0x008603c8} when the root has no collision model
// and {0x008603d0, 0x008603d4, 0x008603d8} when it has one -- so the caller reads
// cluster_globals[2] to reach the data_array of object_cluster_reference records. The original
// walks that adjacency by address arithmetic instead of naming each global, and both functions
// preserve the trick.
// The second slot starts as the root object placement_id and is then advanced to each
// next_reference of each reference as the chain is walked.
// ---------------------------------------------------------------------------
typedef struct object_placement_cursor {
    int32_t *cluster_globals;       // 0x00 &cluster_first of the applicable three-global group
    datum_index next_reference;     // 0x04 placement_id, then object_cluster_reference.next_reference
} object_placement_cursor;          // size 0x08

// ---------------------------------------------------------------------------
// bsp_leaf_reference  (the 8-byte {leaf, cluster} pair every caller of bsp3d_node_find_leaf
// 0x5013a0 fills in and then passes on)
// bsp3d_node_find_leaf returns the structure BSP leaf index for a point, or -1 when the point
// is outside the BSP; the caller then reads the cluster of that leaf out of the leaf array at
// structure_bsp_globals (0x00746f9c) +0xe4, which is a ScenarioStructureBSPLeaf array of
// stride 0x10 whose `cluster` field is at +0x08 (types/tags.h). Both halves are set to -1
// together when the probe fails.
// Proved by: object_set_cluster_and_parent 0x4f5c30 (writes the pair straight into
// object +0x98 / +0x9c, which is why `object.location_leaf_index` and
// `object.location_cluster_index` have exactly this shape), object_light_recompute_transform
// 0x4f2a00 (hands it to the light placement helper 0x551f00 as `leaf_and_cluster`),
// antenna_apply_marker_delta 0x4fb1c0 and flag_pole_get_marker_positions 0x4fc020 (both build
// one per node and forward it to point_physics_update), objects_recompute_cluster_membership
// 0x4f7570 and object_set_position_and_recalculate.
// Ghidra sizes every caller stack buffer at 8 bytes, so the two trailing bytes are padding.
// ---------------------------------------------------------------------------
typedef struct bsp_leaf_reference {
    int32_t leaf_index;             // 0x00 bsp3d_node_find_leaf result, -1 when outside
    int16_t cluster_index;          // 0x04 ScenarioStructureBSPLeaf.cluster, -1 alongside -1
    int16_t unknown_06;             // 0x06 padding; never read
} bsp_leaf_reference;               // size 0x08

// ---------------------------------------------------------------------------
// object_marker  (the record object_get_node_local_transform fills, one per matching marker)
// Also the element type of the marker array inside a glow instance and of the static scratch
// buffer at 0x006b8cc0 that object_attachment_get_blended_marker writes.
// ---------------------------------------------------------------------------
typedef struct object_marker {
    int16_t node_index;             // 0x00 zeroed on the identity fallback path
    int16_t unknown_02;             // 0x02
    real_matrix4x3 transform;       // 0x04 models path: the node-relative marker matrix.
                                    //      Objects fallback only: identity
    real_matrix4x3 node_transform;  // 0x38 models path: the world marker matrix. Objects
                                    //      fallback only: 13 dwords copied from the object
                                    //      node array. In mirrored mode node_transform.left
                                    //      (+0x48, +0x4c, +0x50) is negated: fchs at
                                    //      0x4d7937..0x4d794c (0x4d7850) and 0x4f6152..0x4f6162
} object_marker;                    // size 0x6c

// ---------------------------------------------------------------------------
// object_memory_dump_record  (objects_dump_memory builds one array of these per object type
// and one per object definition, then qsorts both with stride 0x18)
// ---------------------------------------------------------------------------
typedef struct object_memory_dump_record {
    datum_index definition_tag;     // 0x00 -1 in the by-type array
    int16_t type;                   // 0x04 -1 in the by-definition array
    int16_t maximum_size;           // 0x06 largest object_header.block_size seen
    int32_t total_size;             // 0x08 the key object_dump_compare_by_total_size sorts on
    int16_t count;                  // 0x0c
    int16_t active_count;           // 0x0e object_header flag bit 0
    int16_t garbage_count;          // 0x10 object flag 0x10000
    int16_t dead_count;             // 0x12 object vitality flag 0x0004
    int16_t outside_map_count;      // 0x14 root object flag 0x200000, or cluster -1
    int16_t at_rest_count;          // 0x16 object flag 0x20
} object_memory_dump_record;        // size 0x18

// ---------------------------------------------------------------------------
// object_statistics  (the four-short out-block objects_get_statistics 0x4f7950 fills through
// EDX; the last two shorts are overwritten as one float before the function returns, so the
// record is really two counters and a fraction)
// ---------------------------------------------------------------------------
typedef struct object_statistics {
    int16_t count;                  // 0x00 slots whose object_header.identifier is nonzero
    int16_t active_count;           // 0x02 of those, the ones with object_header flag bit 0
    float pool_fullness_fraction;   // 0x04 1 - free_bytes / 0x200000; the constant divisor
                                    //      means the objects memory_pool is assumed to be a
                                    //      fixed 2 MiB rather than read from memory_pool.size
} object_statistics;                // size 0x08

// ---------------------------------------------------------------------------
// hash_table  (hash_table_set_or_remove, hash_table_get, hash_table_grow_freelist)
// A generic open hash table with chained buckets and a GlobalAlloc-backed node freelist. It
// is not object specific, but this module owns the code, so it is declared here.
// ---------------------------------------------------------------------------
typedef struct hash_node {
    int32_t key;                    // 0x00 -1 while free
    int32_t value;                  // 0x04 -1 while free
    struct hash_node *next;         // 0x08 bucket chain, or the freelist link while free
} hash_node;                        // size 0x0c

typedef struct hash_bucket {
    int32_t count;                  // 0x00
    hash_node *first;               // 0x04
} hash_bucket;                      // size 0x08

typedef struct hash_node_block {
    hash_node *nodes;               // 0x00 a GlobalAlloc of 600 bytes, that is 50 nodes
    struct hash_node_block *next;   // 0x04
} hash_node_block;                  // size 0x08

typedef struct hash_table {
    uint8_t initialized;            // 0x00 both entry points refuse unless this is 1
    uint8_t unknown_01[3];          // 0x01
    int32_t bucket_count;           // 0x04 the modulus; the key is made positive first
    hash_bucket *buckets;           // 0x08
    int32_t entry_count;            // 0x0c
    hash_node *freelist;            // 0x10
    hash_node_block *blocks;        // 0x14
} hash_table;                       // size 0x18

// network_id_table: the networked-object and remote-player id tables (0x00687130 and 0x00687558).
// id_to_index maps a network id to an index (hash_table_get), handles[] turns a network id
// into the local datum handle (types/projectiles.h, types/networking.h describe the users).
typedef struct network_id_table {
    uint8_t unknown_00[0x0c];           // 0x00
    hash_table id_to_index;             // 0x0c
    uint32_t unknown_24;                // 0x24
    datum_index *handles;               // 0x28 indexed by network id
} network_id_table;
typedef char network_id_table_handles_at_28[offsetof(network_id_table, handles) == 0x28 ? 1 : -1];

// ---------------------------------------------------------------------------
// light  (element of the lights data_array at 0x00860b14, stride 0x7c, fixed by the
// (index & 0xffff) * 0x7c arithmetic in light_new_attached and object_light_clear_dirty_flag)
// The two constructors write an overlapping set of fields from 0x5c on: light_new_attached
// stores three marker indices there, light_new_positioned stores a marker index followed by
// a position and a direction. The union is left flattened below.
// ---------------------------------------------------------------------------
typedef enum light_flags {
    _light_always_visible_bit = 0x0001,  // copied from Light tag flags bit 0
    _light_attached_bit = 0x0002,        // set when the owner is parented or always visible
    _light_transform_dirty_bit = 0x0004, // object_lights_refresh_transforms recomputes the
                                         // transform and clears it. UNSURE: the companion
                                         // object_light_clear_dirty_flag tests 0x0002 but
                                         // clears 0x0004, so one of the two is a bug or the
                                         // two bits carry one meaning between them.
    _light_needs_cone_update_bit = 0x0008
} light_flags;

typedef struct light {
    int16_t identifier;             // 0x00 datum salt
    uint16_t flags;                 // 0x02 light_flags
    datum_index definition_tag;     // 0x04 the Light tag
    uint32_t unknown_08;            // 0x08
    int32_t creation_tick;          // 0x0c seeded from the light frame counter minus 1
    datum_index next_light;         // 0x10 -1 at create
    uint8_t unknown_14[0x18];       // 0x14
    datum_index owner_object;       // 0x2c
    real_point3d position;          // 0x30 world-space placement. object_light_recompute_transform
                                    //      writes it from object_marker.node_transform.position
                                    //      on the marker path and from matrix4x3_transform_point
                                    //      on the node path.
    real_vector3d direction;        // 0x3c world-space direction, taken from the same marker
                                    //      node_transform.forward / matrix4x3_transform_normal
    real_vector3d up;               // 0x48 world-space up. From node_transform.up on the marker
                                    //      path; on the node path vector3d_build_perpendicular
                                    //      derives it from direction and it is then normalized.
                                    //      (0x4f2a7f..0x4f2a9f and 0x4f2af6..0x4f2b05)
    float radius;                   // 0x54 the object_lights_gather_nearest falloff denominator
    int32_t marker_link;            // 0x58 -1 when the light has no marker; the positioned
                                    //      form stores the current frame counter instead
    int16_t marker_index;           // 0x5c doubles as the ATTACHMENT index on the marker path:
                                    //      object_light_recompute_transform loads it into DX for
                                    //      object_get_attachment_marker_name (0x4f2a2f)
    int16_t marker_index_secondary; // 0x5e attached form only
    real_point3d local_position;    // 0x60 node-space placement of the positioned form; the node
                                    //      path transforms it into position above. The attached
                                    //      form uses the int16 here as a change_color_index.
    real_vector3d local_direction;  // 0x6c node-space direction, transformed into direction above
    uint32_t unknown_78;            // 0x78
} light;                            // size 0x7c

// ---------------------------------------------------------------------------
// light_transient  (light_transient_add fills one of eight slots of a parallel-array table
// based at 0x008609cc with stride 0x28; the live count is the int16 at 0x00860b0c)
// ---------------------------------------------------------------------------
typedef struct light_transient {
    void *definition;               // 0x00 tag data of the Light definition
    real_point3d position;          // 0x04
    uint32_t unknown_10;            // 0x10
    uint32_t unknown_14;            // 0x14
    uint32_t color;                 // 0x18 packed ARGB
    int16_t unknown_1c;             // 0x1c 0xffff
    int16_t unknown_1e;             // 0x1e 0xffff
    int16_t slot_index;             // 0x20 the index of this entry
    uint8_t unknown_22;             // 0x22
    uint8_t intensity;              // 0x23 the scalar argument rounded into 0..255
    uint32_t unknown_24;            // 0x24
} light_transient;                  // size 0x28

// ---------------------------------------------------------------------------
// widget  (element of the widget data_array at 0x00860398, stride 0x0c, from widget_new and
// widget_delete_all)
// ---------------------------------------------------------------------------
typedef struct widget {
    int16_t identifier;             // 0x00 datum salt
    int16_t type;                   // 0x02 object_widget_type
    datum_index instance;           // 0x04 the antenna, flag or glow handle; -1 when the type
                                    //      has no new hook
    datum_index next_widget;        // 0x08 -1 ends the list rooted at object 0x16c
} widget;                           // size 0x0c

// ---------------------------------------------------------------------------
// widget_type_definition  (five rows of 0x28 bytes at 0x0069c010)
// Read directly out of .data in bin/halo.exe rather than inferred, which is what fixed the
// widget type order and the identity of the render hooks:
//   row 0  0x666c6167  flag        flags_initialize 0x4fb4d0    update 0x4fba00  render 0x4fb980
//   row 1  0x616e7421  antenna     antennas_initialize 0x4faa20 update 0x4fad20  render 0x4fac90
//   row 2  0x676c7721  glow        0x4fcbb0                     update null      render 0x4fcdb0
//   row 3  0x6d677332  lightvolume 0x4fe680                     update null      render 0x4fe900
//   row 4  0x656c6563  lightning   0x4fee80                     update null      render 0x4ff010
// ---------------------------------------------------------------------------
typedef struct widget_type_definition {
    uint32_t group_tag;             // 0x00 matched against the ObjectWidget reference group tag
    int32_t flag;                   // 0x04 the per-type value widget_list_has_flag reports;
                                    //      1 for flag, 0 for the other four
    void *initialize;               // 0x08 widgets_initialize
    void *dispose;                  // 0x0c widgets_dispose
    void *dispose_clear_flag;       // 0x10 widgets_dispose_clear_flag
    void *reset;                    // 0x14 the five callbacks objects_dispose runs
    void *new_instance;             // 0x18 widget_new; a null here means the widget carries
                                    //      no instance handle
    void *delete_instance;          // 0x1c widget_delete_all
    void *update;                   // 0x20 widgets_update_all; null for glow, light volume
                                    //      and lightning
    void *render;                   // 0x24 widget_list_notify
} widget_type_definition;           // size 0x28

// ---------------------------------------------------------------------------
// antenna  (element of the antenna data_array at 0x008603ac, stride 700 = 0x2bc, from the
// (index & 0xffff) * 700 arithmetic in antenna_new)
// ---------------------------------------------------------------------------
typedef struct antenna_vertex {
    real_point3d position;          // 0x00 laid out along the tag vertex offsets at create
    real_vector3d velocity;         // 0x0c zeroed at create
    float texture_scale;            // 0x18 AntennaVertex length divided by the bitmap span
    int16_t unknown_1c;             // 0x1c zeroed at create
    int16_t unknown_1e;             // 0x1e
} antenna_vertex;                   // size 0x20

typedef struct antenna {
    int16_t identifier;             // 0x00 datum salt
    int16_t unknown_02;             // 0x02
    uint8_t unknown_04;             // 0x04 zeroed at create
    uint8_t degenerate;             // 0x05 set when the tag has fewer than two vertices
    int16_t unknown_06;             // 0x06
    datum_index definition_tag;     // 0x08 the Antenna tag
    datum_index object_index;       // 0x0c -1 at create
    real_point3d previous_marker_position; // 0x10 widget_apply_marker_delta shifts every
                                    //      vertex by the movement of the anchoring marker
    antenna_vertex vertices[21];    // 0x1c 20 tag vertices plus the trailing tip vertex
} antenna;                          // size 0x2bc

// ---------------------------------------------------------------------------
// flag  (element of the flag data_array at 0x008603a8, stride 0x16bc, from the
// (index & 0xffff) * 0x16bc arithmetic in flag_new)
// ---------------------------------------------------------------------------
typedef struct flag_vertex {
    real_point3d position;          // 0x00 seeded from the constant at 0x006966f8
    real_point3d previous_position; // 0x0c seeded from the constant at 0x00696714
} flag_vertex;                      // size 0x18

typedef struct flag {
    int16_t identifier;             // 0x00 datum salt
    uint8_t invalid;                // 0x02 raised when the tag grid is out of range
    uint8_t unknown_03;             // 0x03 zeroed at create
    uint32_t unknown_04;            // 0x04
    datum_index object_index;       // 0x08 -1 at create
    datum_index definition_tag;     // 0x0c the Flag tag
    real_point3d previous_marker_position; // 0x10
    flag_vertex vertices[225];      // 0x1c row-major width by height cloth grid; flag_new
                                    //      refuses a tag whose product reaches 0xe1
    int16_t cell_split_codes[196];  // 0x1534 one quad-diagonal code per cell, written by
                                    //        flag_cloth_set_region_split_flags and consumed
                                    //        by the triangulation switch in flag_render
} flag;                             // size 0x16bc

// ---------------------------------------------------------------------------
// glow  (element of the data_array at 0x008603a0; the particles live in the separate array
// at 0x008603a4). Widget type 2, group tag 0x676c7721.
// The functions at 0x4fcdb0..0x4fe570 that earlier passes named lightning_* operate on this
// record: the widget type table proves the ownership, and every tag offset they read (flags
// 0x28, distance bounds 0x84..0x90, particle size bounds 0xa0, colour bounds 0xb8..0xd0)
// lands inside the Glow struct in types/tags.h.
// ---------------------------------------------------------------------------
typedef struct glow_particle {
    int16_t identifier;             // 0x00 datum salt
    int16_t unknown_02;             // 0x02
    datum_index handle;             // 0x04 the allocator writes the handle back into the record
    uint8_t unknown_08[0x20];       // 0x08
    float t;                        // 0x28 position along the marker chain, advanced by dt and
                                    //      clamped or ping-ponged per the tag loop mode
    uint8_t unknown_2c[0x0c];       // 0x2c
    float base_color[3];            // 0x38 randomized between the two tag colour bounds
    float render_color[3];          // 0x44 base_color faded by the remaining lifetime
    int16_t age;                    // 0x50 ticks elapsed
    int16_t lifetime;               // 0x52 ticks total
    uint32_t flags;                 // 0x54 bit 0 is the alternating trailing-particle flag
    float fade;                     // 0x58 1 - age/lifetime, clamped at 0
    struct glow_particle *next;     // 0x5c
    struct glow_particle *previous; // 0x60
} glow_particle;                    // size at least 0x64. UNRESOLVED: the data_array for
                                    // these is created outside this module, so the stride is
                                    // not proven here and anything past 0x64 is unknown.

typedef struct glow {
    int16_t identifier;             // 0x00 datum salt
    uint8_t disabled;               // 0x02 the update is skipped while set
    uint8_t unknown_03;             // 0x03
    int16_t marker_count;           // 0x04 markers actually resolved, at most 5
    int16_t unknown_06;             // 0x06
    object_marker markers[5];       // 0x08 filled by object_get_node_local_transform from the
                                    //      TagString marker name at Glow tag offset 0
    datum_index definition_tag;     // 0x224 the Glow tag
    int16_t particle_count;         // 0x228 Glow number_of_particles, the spacing denominator
    int16_t marker_order[5];        // 0x22a nearest-neighbour ordering of markers
    float total_length;             // 0x234 sum of the distances along the ordered chain
    float cumulative_length[5];     // 0x238 running distance at each ordered marker
    int16_t spawn_count;            // 0x24c particles the initial chain build allocates
    int16_t unknown_24e;            // 0x24e
    glow_particle *first_particle;  // 0x250
    glow_particle *last_particle;   // 0x254
} glow;                             // size at least 0x258. UNRESOLVED: the data_array is
                                    // created outside this module, so the stride is not
                                    // proven here.

// (object_zone_light_table, formerly declared here for 0x006b8d78, was dropped: the block is
// types/physics.h breakable_surface_globals, same 0x4204 layout. The first index is the
// structure BSP index 0x0069e8d8, not a local player (0x43d79a: movsx WORD ds:0x69e8d8, then
// shl 5 + 1 into ds:0x6b8d78); the bits are the intact breakable surfaces and the floats their
// health (0x4ffde0 decrements them and fires the break effect 0x500090). The pointer is set
// once at 0x45ab31. The two functions at 0x4ffd40 / 0x4ffda0 that were named
// breakable_surfaces_reset (0x4ffd40, was zone_light_table_initialize) / breakable_surface_is_intact (0x4ffda0) are the breakable-surface reset and
// the "is surface intact" test.)

#pragma pack(pop)

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// global 0x008603a0: data_array *glow_data                  // widget type 2 instances
// global 0x008603a4: data_array *glow_particle_data         // glow_particle datums
// global 0x008603a8: data_array *flag_data                  // flags_initialize, 2 slots of flag
// global 0x008603ac: data_array *antenna_data               // antennas_initialize, 12 slots of antenna
// global 0x00860398: data_array *widget_data                // widgets_initialize, 0x40 slots of widget
// global 0x008603b0: data_array *object_data                // objects_initialize, 0x800 object_header
// global 0x008603c0: datum_index *noncollideable_cluster_first    // 0x200 entries, head per cluster
// global 0x008603c4: data_array *noncollideable_object_references // object_cluster_reference
// global 0x008603c8: void *noncollideable_cluster_partition // cluster_partition_new result
// global 0x008603cc: int32_t object_cluster_stamp           // bumped once per object_collect_in_clusters pass
// global 0x008603d0: datum_index *collideable_cluster_first      // 0x200 entries, head per cluster
// global 0x008603d4: data_array *collideable_object_references   // object_cluster_reference
// global 0x008603d8: void *collideable_cluster_partition
// global 0x008603dc: object_type_definition *object_type_definition_list // chain head
// global 0x008603e0: int32_t object_control_binding_active
// global 0x008603e4: void *object_control_binding_slot_config    // 6 slots by 8 entries
// global 0x008603e8: void *object_control_binding_slot_config_1
// global 0x008603ec: int32_t object_control_binding_identifiers  // machine or player id per slot
// global 0x008603f0: datum_index object_control_binding_objects  // up to 3 per slot
// global 0x008603f4: uint8_t object_control_binding_flags        // the byte the query returns
// global 0x008603f6: int16_t object_control_binding_flags_1
// global 0x008603f8: int32_t object_control_binding_unknown_f8
// global 0x008603fc: int32_t object_control_binding_unknown_fc
// global 0x00860430: int32_t object_control_binding_unknown_430
// global 0x00860434: int32_t object_control_binding_unknown_434
// global 0x00860480: int32_t object_control_binding_unknown_480
// global 0x00860484: int32_t object_control_binding_unknown_484
// global 0x008607a0: uint8_t object_control_binding_unknown_7a0
// global 0x008607a1: uint8_t object_control_binding_unknown_7a1
// global 0x008607c0: int32_t light_render_unknown_7c0
// global 0x008607c4: int32_t light_frame_counter            // light lifetimes are seeded from it
// global 0x008607c8: int32_t light_render_unknown_7c8
// global 0x008607cc: int32_t *light_active_list             // lights_apply_spot_falloff input
// global 0x008609cc: light_transient light_transient_table  // 8 slots of stride 0x28
// global 0x00860b0c: int16_t light_transient_count
// global 0x00860b10: int32_t light_update_unknown_b10
// global 0x00860b14: data_array *light_data                 // lights_initialize, 0x380 slots of light
// global 0x00860b20: datum_index *light_cluster_first       // 0x200 entries, reset to -1 on dispose
// global 0x00860b24: data_array *light_cluster_references
// global 0x00860b28: data_array *light_object_references
// global 0x006b8cb4: memory_pool *object_memory_pool        // game_state_new_pool("objects")
// global 0x006b8cb8: datum_index *object_name_list          // 0x200 entries, object_lookup_table_get
// global 0x006b8cbc: object_globals *object_globals_pointer // 0x98 bytes of game state
// global 0x006b8cc0: object_marker object_marker_scratch    // object_attachment_get_blended_marker result
// global 0x006b8d70: void *light_volume_instances           // widget type 3 datum table
// global 0x006b8d74: void *lightning_instances              // widget type 4 datum table
// (0x006b8d78 is types/physics.h breakable_surface_globals *breakable_surface_globals; physics owns it)
// global 0x0071cfb8: uint8_t *lights_enabled                // one byte of game state
// global 0x0069bfdc: object_type_definition *object_type_definitions   // 12 read-only pointers
// global 0x0069c010: widget_type_definition widget_type_definitions    // 5 read-only rows
// global 0x0069b354: void *object_delete_callbacks          // 3 entries, run by object_delete
// global 0x00719cd0: uint32_t object_random_seed            // region permutation selection LCG
// global 0x00719cd4: uint32_t widget_random_seed            // cloth wind and glow randomization
//
// Globals this module reads but does not own, listed so the ownership stays honest:
//   0x0087bc14 tag_instance *tag_instances        (types/cache.h)
//   0x0087a480 data_array *player_data            (players module)
//   0x0087a478 the BSP cluster PVS source copied into object_globals.cluster_pvs_current
//   0x0087a464 and 0x0087a468 the hash chain object_hash_set_flag_bit3 walks
//   0x006f1d6c the game time globals, where +0x0c is the current tick
//   0x006f1d20 game_engine_definition *current_game_engine (game.h, R04): non-NULL when a
//              multiplayer engine is loaded; every damage and creation path branches on it
//   0x00746f8c Scenario *global_scenario (the placement blocks object_type_definition +0x0a/+0x0c
//              index); 0x00746f9c ScenarioStructureBSP *global_structure_bsp (scenario.h, the
//              resident structure BSP); 0x00746fa0 the matg game globals
//   0x0069e8d8 int16_t global_structure_bsp_index (the structure BSP index, types/physics.h)
//   0x006b8d78 breakable_surface_globals *breakable_surface_globals (types/physics.h)
//   0x00696714, 0x00696718, 0x00696720, 0x006966f8, 0x00686b04 the shared constant vectors
