#pragma once
// Blam units module (halo.exe 1.0.10 retail, 0x5579e0..0x575e30, 249 functions).
// The unit layer of the object hierarchy: the unit_data extension that every biped and
// vehicle carries on top of the common object record, the control-input record the player
// and the AI both push into a unit, the seating / weapon-inventory / grenade bookkeeping,
// the animation-state machine, the dialogue ("speech") queue, and the two concrete
// extensions biped_data and vehicle_data.
//
// Offsets in comments are byte offsets from the object base (the same numbering the
// decompiled module uses) unless a struct says otherwise. Where the binary itself carries
// the layout it is used in preference to the decompiler and the fact is called out:
//   - The object_type_definition table at 0x0069bfdc chains three rows for a biped
//     (object -> unit -> biped) and three for a vehicle (object -> unit -> vehicle).
//     Reading those rows out of .data fixes the three sizes this whole header hangs on:
//       0x0069b360 "object"  object_size 0x1f4
//       0x0069b428 "unit"    object_size 0x4cc   -> unit_data is 0x2d8 bytes at 0x1f4
//       0x0069b4f0 "biped"   object_size 0x550   -> biped_data is 0x84 bytes at 0x4cc
//       0x0069b5b8 "vehicle" object_size 0x5c0   -> vehicle_data is 0xf4 bytes at 0x4cc
//     The same rows fix several function attributions: the update column of the unit row
//     (+0x34) is 0x5625b0, the biped row uses 0x5590a0 and the vehicle row uses 0x570ee0,
//     so the function Ghidra currently calls unit_update at 0x5590a0 is biped_update and
//     the real unit_update is 0x5625b0 (see out/phase4/units_types_notes.md).
//   - The pointer table at 0x0069fde4 is six string pointers -- "asleep", "alert",
//     "stand", "crouch", "flee", "flaming". unit_set_or_test_seat_and_weapon_label
//     (0x5651e0) matches a seat label against that table and stores the index at 0x2a7,
//     and 0x56c2f0 indexes it to name the state, so the table is the enum.
//   - the 0x40-byte layout of unit_control_data is fixed twice over: 0x5639f0 copies the whole
//     record into the unit at 0x478 with a 0x10-dword block move and then unpacks every
//     field individually, and 0x55b110 reads 0x480 and 0x494..0x49c back out of that copy.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   datum_index, data_array, data_iterator
//   types/math.h     real_point3d, real_vector3d, real_matrix4x3, real_plane3d
//   types/cache.h    tag_instance (0x0087bc14, tag data at +0x14)
//   types/objects.h  object (0x1f4; unit_data starts immediately after it), object_header,
//                    object_type_definition, object_placement_data, damage_data,
//                    object_marker, object_type
//   types/tags.h     Unit (0x2f0, seats TagReflexive at 0x2e4 with UnitSeat stride 0x11c),
//                    UnitSeat (0x11c), UnitWeapon (0x24), UnitPoweredSeat (0x44),
//                    UnitDialogueVariant (0x18),
//                    Biped (0x4f4; biped_flags 0x2f4, jump_velocity 0x3b4, footsteps
//                    material_effects at 0x38c, standing / crouching *collision* height 0x424
//                    and 0x428 and collision_radius 0x42c -- the camera heights are 0x400 /
//                    0x404 and crouch_transition_time is 0x408, crouch_camera_velocity 0x4cc,
//                    pelvis / head model node index 0x4e4 and 0x4e6, contact_point 0x4e8),
//                    Vehicle (vehicle_flags 0x2f0, maximum_forward_speed 0x2f8,
//                    maximum_left_turn 0x308, wheel_circumference 0x310, turn_rate 0x314,
//                    blur_speed 0x318, maximum_left_slide 0x330, suspension_sound 0x3b0,
//                    crash_sound 0x3c0, material_effects 0x3d0, effect 0x3e0),
//                    ModelAnimationsAnimationGraphUnitSeat (0x64, the block the byte at
//                    unit 0x2a0 indexes; its weapons at +0x5c have stride 0xbc and their
//                    weapon types at +0xb4 have stride 0x3c), Weapon, DamageEffect, Physics
//
// A note on the two extensions: biped_data and vehicle_data both start at 0x4cc, so the
// same byte offset means two different things depending on the object type. Every field
// below was assigned to one extension or the other by which function touches it, and the
// handful that stayed ambiguous are called out in the notes file.

#include <stddef.h> // offsetof
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum unit_constants {
    k_unit_data_offset = 0x1f4,            // object_type_definition "object" object_size
    k_unit_object_size = 0x4cc,            // object_type_definition "unit" object_size
    k_biped_object_size = 0x550,           // object_type_definition "biped" object_size
    k_vehicle_object_size = 0x5c0,         // object_type_definition "vehicle" object_size
    k_unit_data_size = 0x2d8,              // 0x4cc - 0x1f4
    k_biped_data_size = 0x84,              // 0x550 - 0x4cc
    k_vehicle_data_size = 0xf4,            // 0x5c0 - 0x4cc
    k_maximum_weapons_per_unit = 4,        // the 0x2f8 handle array and the 0x308 tick array
                                           // are both walked with a 4-iteration loop
                                           // (unit_drop_inventory_weapons, 0x56d660)
    k_maximum_grenade_types = 2,           // 0x5699a0 wraps the 0x31e counts at index 1, and
                                           // 0x55b110 moves both counts as one int16
    k_unit_recent_damage_count = 4,        // 0x568230 scans 0x430 four times, stride 0x10
    k_unit_base_animation_state_count = 6, // the 0x0069fde4 name table
    k_unit_animation_overlay_count = 3,    // the three index / frame pairs at 0x2aa, 0x2ae, 0x2b2
    k_unit_aiming_bound_count = 4,         // 0x563b50 writes four floats at 0x2b8 and at 0x2c8
    k_unit_animation_control_count = 3,    // 0x563b50 loops three floats at 0x364
    k_unit_control_data_size = 0x40,       // 0x5639f0 block move, 0x10 dwords
    k_unit_speech_size = 0x30,             // 0x560f20 block move, 0xc dwords
    k_unit_ground_adjust_iterations = 0x14 // 0x55ad00 seeds the 0x525 limit with 0x14
} unit_constants;

// ---------------------------------------------------------------------------
// unit_base_animation_state  (the byte at unit 0x2a7)
// Straight out of the six-pointer string table at 0x0069fde4, which
// unit_set_or_test_seat_and_weapon_label matches the seat label against and 0x56c2f0
// indexes to produce the name of the state. unit_exit_vehicle_seat and every detach path force
// the value back to _unit_base_animation_state_stand.
// ---------------------------------------------------------------------------
typedef enum unit_base_animation_state {
    _unit_base_animation_state_none = -1,  // 0x5651e0 leaves -1 when no label matched
    _unit_base_animation_state_asleep = 0,
    _unit_base_animation_state_alert = 1,
    _unit_base_animation_state_stand = 2,
    _unit_base_animation_state_crouch = 3,
    _unit_base_animation_state_flee = 4,
    _unit_base_animation_state_flaming = 5
} unit_base_animation_state;

// ---------------------------------------------------------------------------
// unit_animation_state  (the byte at unit 0x2a3, written by unit_try_set_animation_state)
// Only the values this module sets or tests for a nameable reason are named; the rest of
// the range is reached through the priority tables in 0x560d00 and 0x5692b0 and stays
// numeric here. 0x5651e0 stores 0xff (-1) to mean "no state".
// ---------------------------------------------------------------------------
typedef enum unit_animation_state {
    _unit_animation_state_none = -1,           // 0x5651e0 writes 0xff
    _unit_animation_state_idle = 0,
    _unit_animation_state_unknown_02 = 2,      // unit_update_facing treats 2 and 3 as the
    _unit_animation_state_unknown_03 = 3,      //   two turning-in-place states
    _unit_animation_state_unknown_17 = 0x17,   // 0x566de0 land / stand transition source
    _unit_animation_state_unknown_18 = 0x18,   // 0x55e840 idle-basis refresh trigger
    _unit_animation_state_ready_weapon = 0x19, // 0x569a20 sets it when readying a weapon
    _unit_animation_state_seat_enter = 0x1a,   // unit_enter_vehicle_seat (0x566970)
    _unit_animation_state_seat_exit = 0x1b,    // 0x56b5f0, 0x56c470, 0x56ab50, 0x5674a0
    _unit_animation_state_custom_animation = 0x1c, // unit_start_user_animation (0x5702a0),
                                               //   unit_get_custom_animation_time_remaining
    _unit_animation_state_scripted_action = 0x1d,  // 0x569530 starts the action animation
    _unit_animation_state_unknown_1f = 0x1f,
    _unit_animation_state_throwing_grenade = 0x21, // unit_begin_throw_grenade (0x56e080)
    _unit_animation_state_unknown_25 = 0x25,   // forced on a seat occupant that is being
                                               //   ejected (0x5590a0, 0x5674a0, 0x568610)
    _unit_animation_state_unknown_29 = 0x29
} unit_animation_state;

// ---------------------------------------------------------------------------
// unit_flags  (the dword at unit 0x204)
// Every bit below is one this module reads, sets or clears; the function that does it is
// named. Bits nothing in the module touches are left out rather than invented.
// ---------------------------------------------------------------------------
typedef enum unit_flags {
    _unit_flag_unattended = 0x00000001,        // 0x569bf0 sets it when the unit has neither
                                               //   an actor nor a swarm reference; unit_update
                                               //   then drives the aiming vectors off the
                                               //   object basis instead of off control input
    _unit_flag_unknown_10 = 0x00000010,        // unit_update, paired with 0x80000
    _unit_flag_unknown_20 = 0x00000020,
    _unit_flag_disoriented = 0x00000080,       // 0x5705a0 sets it with the stun timer
    _unit_flag_permutation_dirty = 0x00000100, // 0x561620 runs 0x561990 then clears it
    _unit_flag_unknown_200 = 0x00000200,       // 0x562030
    _unit_flag_unknown_1000 = 0x00001000,      // gates evade (0x55e190) and fall damage
    _unit_flag_unknown_2000 = 0x00002000,      // set by both seat-teardown paths
    _unit_flag_unknown_4000 = 0x00004000,      // suppresses turning in unit_update_facing
    _unit_flag_detached = 0x00008000,          // unit_detach_from_parent (0x570140), 0x56ff40
    _unit_flag_permutation_chosen = 0x00020000,// 0x568540 caches a random variant once
    _unit_flag_unknown_80000 = 0x00080000,     // toggled every other tick by unit_update;
                                               //   0x55b110 forces it off on a network create
    _unit_flag_delete_when_dropped = 0x00100000, // unit_drop_object_from_hand deletes the
                                               //   dropped object when this is set
    _unit_flag_unknown_800000 = 0x00800000,    // 0x561d50 sets or clears it over a chain
    _unit_flag_unknown_1000000 = 0x01000000,   // 0x56a290
    _unit_flag_idle_turn_seeded = 0x02000000,  // 0x570650 seeds the idle turn angle once
    _unit_flag_unknown_4000000 = 0x04000000,   // unit_update flips it, gated on 0x2a3
    _unit_flag_unknown_8000000 = 0x08000000,
    _unit_flag_unknown_80000000 = 0x80000000   // tested as (char)flags < 0 by 0x5590a0 and
                                               //   by 0x566de0
} unit_flags;

// ---------------------------------------------------------------------------
// unit_control_flags  (unit_control_data 0x02, widened into the dword at unit 0x208)
// 0x5639f0 stores the 16-bit control word zero-extended, so only the low 16 bits are ever
// set from input. unit_update ORs in the low six bits of the driver unit (mask 0x3f) and its
// and its bits 10..14 (mask 0x7c00) when this unit is a vehicle being driven, and shifts the word
// right by 13 to pick a grenade action.
// ---------------------------------------------------------------------------
typedef enum unit_control_flags {
    _unit_control_flag_crouch = 0x0001,        // vehicle_update gates the brake on it
    _unit_control_flag_jump = 0x0002,          // 0x55ec90 counts ticks of it
    _unit_control_flag_unknown_4 = 0x0004,
    _unit_control_flag_unknown_8 = 0x0008,
    _unit_control_flag_unknown_10 = 0x0010,    // unit_update, only when the seat allows it
    _unit_control_flag_exact_facing = 0x0020,  // unit_update_facing skips the rate limit
    _unit_control_flag_action = 0x0040,        // biped_update uses it to enter a seat
    _unit_control_flag_unknown_80 = 0x0080,
    _unit_control_flag_look_dont_turn = 0x0100,// unit_update_facing refuses to turn the body
    _unit_control_flag_force_alert = 0x0200,   // 0x565420
    _unit_control_flag_reload = 0x0400,        // unit_update
    _unit_control_flag_primary_trigger = 0x0800,   // unit_update, 0x56d400
    _unit_control_flag_secondary_trigger = 0x1000, // unit_update
    _unit_control_flag_grenade = 0x2000,       // the bit the >> 13 in unit_update lands on
    _unit_control_flag_exchange_weapon = 0x4000
} unit_control_flags;

// ---------------------------------------------------------------------------
// unit_animation_state_flags  (the uint16 at unit 0x298)
// ---------------------------------------------------------------------------
typedef enum unit_animation_state_flags {
    _unit_animation_flag_action_active = 0x0001,  // 0x566de0, 0x569530, 0x5702a0 set it
    _unit_animation_flag_aiming_enabled = 0x0002, // 0x5651e0 sets it when the matched weapon
                                               //   has aiming animations; 0x563b50 gates the
                                               //   overlay blend on it
    _unit_animation_flag_unknown_4 = 0x0004,   // 0x565420 sets it, unit_update clears it
    _unit_animation_flag_unknown_8 = 0x0008    // 0x566de0
} unit_animation_state_flags;

// ---------------------------------------------------------------------------
// unit_throwing_grenade_state  (the byte at unit 0x28d)
// ---------------------------------------------------------------------------
typedef enum unit_throwing_grenade_state {
    _unit_throwing_grenade_state_none = 0,
    _unit_throwing_grenade_state_begin = 1,    // unit_begin_throw_grenade
    _unit_throwing_grenade_state_in_hand = 2,  // unit_throw_grenade_move_to_hand
    _unit_throwing_grenade_state_released = 3  // unit_release_thrown_grenade
} unit_throwing_grenade_state;

// ---------------------------------------------------------------------------
// unit_melee_state  (the byte at unit 0x289)
// ---------------------------------------------------------------------------
typedef enum unit_melee_state {
    _unit_melee_state_none = 0,                // every melee routine clears it when done
    _unit_melee_state_ready = 1,               // 0x569a20
    _unit_melee_state_unknown_3 = 3,           // 0x55cfd0 pairs it with the 0x4f4 target
    _unit_melee_state_lunge = 4                // 0x569a20 sets it, 0x56fc80 ticks damage in it
} unit_melee_state;

// ---------------------------------------------------------------------------
// unit_control_data  (0x40 bytes)
// The record the player, the AI and the network layer all hand to 0x5639f0. That function
// block-moves the whole 0x40 bytes into unit 0x478 when running as a server and then
// unpacks each field into the live unit, which is what pins every offset below.
// ---------------------------------------------------------------------------
typedef struct unit_control_data {
    int8_t animation_state;         // 0x00 -> unit 0x2a6, the seat / overlay command
                                    //      0x565420 switches on
    int8_t aiming_speed;            // 0x01 -> unit 0x288
    uint16_t control_flags;         // 0x02 -> unit 0x208, unit_control_flags
    int16_t weapon_index;           // 0x04 -> unit 0x2f4 (desired weapon) when not -1
    int16_t grenade_index;          // 0x06 -> unit 0x31d (desired grenade) when not -1
    int16_t zoom_level;             // 0x08 -> unit 0x321; 0x55b110 reads it back at 0x480
    int16_t unknown_0a;             // 0x0a never read by this module
    real_vector3d throttle;         // 0x0c -> unit 0x278
    float primary_trigger;          // 0x18 -> unit 0x284
    real_vector3d facing_vector;    // 0x1c -> unit 0x224; 0x55b110 copies 0x494 to 0x4ac
    real_vector3d aiming_vector;    // 0x28 -> unit 0x230
    real_vector3d looking_vector;   // 0x34 -> unit 0x254
} unit_control_data;                // size 0x40

// ---------------------------------------------------------------------------
// unit_speech  (0x30 bytes)
// 0x560f20 block-moves 0xc dwords of this into unit 0x388 (the playing line) or into
// unit 0x3b8 (the queued line), and 0x561030 builds one on the stack field by field, which
// is where the named members come from. The unnamed tail is zero in every construction the
// module performs.
// ---------------------------------------------------------------------------
typedef struct unit_speech {
    int16_t priority;               // 0x00 compared against the priority table at 0x0065e94c
    int16_t scream_type;            // 0x02 -1 in the 0x561030 construction
    datum_index sound_tag;          // 0x04 handed to 0x00543ce0 to start the line
    int16_t delay_ticks;            // 0x08 copied to the 0x3f8 countdown
    int16_t lipsync_ticks;          // 0x0a copied to the 0x3fc countdown
    int16_t tail_ticks;             // 0x0c copied to the 0x3fe countdown
    int16_t unknown_0e;             // 0x0e
    int32_t unknown_10;             // 0x10 -1 in the 0x561030 construction
    int16_t unknown_14;             // 0x14 -1
    int16_t ai_line_index;          // 0x16 passed to ai_communication_record_line_played
    int16_t unknown_18;             // 0x18 -1
    int8_t suppress_line_record;    // 0x1a 0x561030 skips the line bookkeeping when set
    int8_t unknown_1b;              // 0x1b
    uint8_t unknown_1c[0x14];       // 0x1c zeroed by every construction, never read back
} unit_speech;                      // size 0x30

// ---------------------------------------------------------------------------
// unit_recent_damage  (0x10 bytes, four of them at unit 0x430)
// 0x568230 keeps the four most recent damage sources so the AI does not re-broadcast the
// same hit-reaction line. A slot whose tick is -1 is free; otherwise the oldest tick wins.
// ---------------------------------------------------------------------------
typedef struct unit_recent_damage {
    int32_t tick;                   // 0x00 game tick the slot was last touched, -1 = empty
    float damage;                   // 0x04 accumulated damage from this source
    datum_index responsible_unit;   // 0x08 matched first when merging into an existing slot
    datum_index responsible_player; // 0x0c indexes the player data_array at 0x0087a480
} unit_recent_damage;               // size 0x10

// ---------------------------------------------------------------------------
// unit_animation_overlay  (0x04 bytes, three of them at unit 0x2aa)
// 0x565e00 owns slot 0, 0x566410 slot 1 and 0x566de0 slot 2; 0x563b50 blends whichever
// slots hold an index other than -1, and every seat-teardown path frees all three.
// ---------------------------------------------------------------------------
typedef struct unit_animation_overlay {
    int16_t animation_index;        // 0x00 -1 when the slot is free
    int16_t frame;                  // 0x02 the frame 0x004d4dd0 / 0x004d4f90 blends
} unit_animation_overlay;           // size 0x04

// ---------------------------------------------------------------------------
// biped_movement_solver_data  (0xc8 bytes)
// The argument record the two biped movement integrators, biped_integrate_movement (0x55bea0)
// and biped_integrate_movement_with_collision (0x55cfd0), build on the stack and hand to the
// generic object movement / collision slide solver at 0x55efd0 (a physics-module function
// listed as skipped in out/phase4/units_types_notes.md).
//
// The layout is pinned by the two integrators writing exactly the same offsets in the same
// order from two independently compiled stack frames: the frame of 0x55bea0 starts the record
// at local_1f0 and the frame of 0x55cfd0 at local_64c, and every field below lands on the same
// relative offset in both. Everything from result_surface_index on is written by the solver
// and read back by the caller on the next line, which is what marks the input / output split.
// ---------------------------------------------------------------------------
typedef enum biped_movement_solver_flags {
    _biped_movement_solver_airborne = 0x0001,      // biped_data.flags bit 0; the solver's air-control branch (was misnamed grounded)
    _biped_movement_solver_jumping = 0x0002,       // biped_data.flags bit 1
    _biped_movement_solver_crouching = 0x0004,     // crouch_fraction != 0
    _biped_movement_solver_crouch_began = 0x0008,  // and the caller had not latched a landing
    _biped_movement_solver_flying = 0x0010,        // Biped.biped_flags "flying" and not dead
    _biped_movement_solver_absolute_movement = 0x0020,    // biped_data.flags bit 2
    _biped_movement_solver_no_collision = 0x0040,    // biped_data.flags bit 3
    _biped_movement_solver_dead = 0x0080,          // object.vitality_flags health-frozen bit
    _biped_movement_solver_passes_through_bipeds = 0x0100, // Biped.biped_flags bit 0x20
    _biped_movement_solver_climbs_any_surface = 0x0200     // Biped.biped_flags bit 0x40, alive
} biped_movement_solver_flags;

// the byte the solver writes back at offset 0xa0
typedef enum biped_movement_solver_result_flags {
    _biped_movement_result_airborne = 0x01,   // no ground plane found (0x560044); copied into biped_data.flags bit 0
    _biped_movement_result_jumping = 0x02,    // copied into biped_data.flags bit 1
    _biped_movement_result_landed = 0x04,     // latches the landing byte of the caller; 0x55cfd0
                                              //   also runs unit_track_target_lock_timeout
                                              //   only when it is clear
    _biped_movement_result_moving = 0x10      // keeps the object out of the "at rest" state
} biped_movement_solver_result_flags;

typedef struct biped_movement_solver_data {
    datum_index object_index;           // 0x00 the object being moved
    uint32_t flags;                     // 0x04 biped_movement_solver_flags; both integrators
                                        //      clear only the low 16 bits before filling it
    real_point3d start_position;        // 0x08 out: the position the solve started from;
                                        //      0x55cfd0 diffs result_position against it to
                                        //      build the melee lunge ray
    real_vector3d facing;               // 0x14 object.forward, or unit_data.desired_facing_vector
                                        //      on the player-physics path
    real_vector3d aiming;               // 0x20 the unit_data.aiming_vector of the *live* object,
                                        //      or object.forward for a simple_creature Unit
    real_vector3d velocity;             // 0x2c object.velocity on entry
    float height_change;                // 0x38 how far the collision pill grew or shrank this
                                        //      tick; 0x55bea0 always passes 0
    real_vector3d movement_delta;       // 0x3c the per-tick displacement, from the frame_info of
                                        //      the animation or from the player physics block
    float frozen_fraction;              // 0x48 biped_integrate_movement: 1.0 on the landing_type==1 frozen path else
                                        //    0; biped_movement_solve scales every desired velocity by (1 - it)
    float maximum_acceleration;         // 0x4c 0.0053333333 by default, FLT_MAX when the
                                        //      animation drives the velocity directly
    float airborne_acceleration;        // 0x50 GlobalsPlayerInformation.airborne_acceleration / 30
    float pill_height;                  // 0x54 first output of unit_get_crouch_height_offset
    float pill_radius;                  // 0x58 second output of it (Biped.collision_radius)
    float steep_landing_maximum_slide;  // 0x5c solve: airborne + too-steep ground contact rejected when projected
                                        //    slide > it (FLT_MAX off; 0.1 for freshly grounded AI)
    float steep_landing_minimum_penetration; // 0x60 solve: same test also needs penetration/|delta| < it (0 off; 0.5
                                             //    for freshly grounded AI)
    float cosine_maximum_slope_angle;   // 0x64 Biped tag 0x4d0
    float negative_sine_downhill_falloff_angle; // 0x68 Biped tag 0x4d4
    float negative_sine_downhill_cutoff_angle;  // 0x6c Biped tag 0x4d8
    float downhill_velocity_scale;      // 0x70 Biped tag 0x364
    float sine_uphill_falloff_angle;    // 0x74 Biped tag 0x4dc
    float sine_uphill_cutoff_angle;     // 0x78 Biped tag 0x4e0
    float uphill_velocity_scale;        // 0x7c Biped tag 0x370
    real_vector3d ground_normal;        // 0x80 biped_data.ground_normal on entry, rewritten on
                                        //      exit (the callers copy all four dwords back)
    uint32_t ground_plane;              // 0x8c biped_data.unknown_520
    datum_index ground_surface_index;   // 0x90 biped_data.ground_surface_index on entry
    uint32_t unknown_94;                // 0x94 neither integrator touches it
    uint32_t fastest_contact_object;    // 0x98 biped_movement_solve 0x55efd0 stores the contacted object with highest
                                        //    relative speed (vehicles preferred); 0x55dfcd passes it to
                                        //    biped_update_target_lock_timer
    datum_index result_surface_index;   // 0x9c out: -1 when the solve ended airborne; otherwise
                                        //      stored in biped_data.last_ground_object_index and the 0x4d3
                                        //      countdown is reloaded with 60
    uint8_t result_flags;               // 0xa0 out: biped_movement_solver_result_flags
    uint8_t pad_a1[3];                  // 0xa1 alignment
    datum_index result_ground_surface_index; // 0xa4 out: the new biped_data.ground_surface_index
    uint32_t snapped_ground_surface_index; // 0xa8 biped_movement_solve: -1, or the BSP surface synthesized as a
                                           //    ground contact when the sweep found none; that contact is preferred
                                           //    and gives 0 impact speed
    real_point3d result_position;       // 0xac out: the solved position
    real_vector3d result_velocity;      // 0xb8 out: the solved velocity
    float result_impact_speed;          // 0xc4 out: the landing speed the fall-damage and
                                        //      footstep-effect paths consume
    float result_blocked_distance;      // 0xc8 out: 0x55efd0 stores (0x560296) the distance
                                        //      between the swept result velocity and the
                                        //      requested one; the struct was documented as
                                        //      0xc8 bytes, so C callers' locals were 4 short
} biped_movement_solver_data;           // size 0xcc


// ---------------------------------------------------------------------------
// unit_data  (0x2d8 bytes, at object + 0x1f4; a unit object is 0x4cc bytes total)
// Offsets in the comments are absolute object offsets, the numbering the decompiled module
// uses, so that reading a field comment against the packs needs no arithmetic.
// ---------------------------------------------------------------------------
typedef struct unit_data {
    datum_index actor_index;            // 0x1f4 -1 when the unit has no AI; 0x569bf0 and
                                        //       0x55e2d0 test it, 0x568610 clears it
    datum_index swarm_actor_index;      // 0x1f8 0x568610 turns it into (index & 0xffff) *
                                        //       0x724 against the actor data_array at
                                        //       0x00880360, which is what fixes it as an
                                        //       actor handle rather than an object one
    datum_index swarm_next_unit_index;  // 0x1fc never read in this module; the only dword
                                        //       left between the two actor handles and the
                                        //       flags word
    uint32_t swarm_previous_unit_index; // 0x200 back-link of the swarm unit list whose forward link is 0x1fc
                                        //    (actor_link_to_unit_cluster 0x4279f0 / actor_remove_from_unit_cluster
                                        //    0x427c90); -1 at unit_new
                                        //       melee and drop paths are on the *item* being
                                        //       dropped, not on the unit)
    uint32_t flags;                     // 0x204 unit_flags
    uint32_t control_flags;             // 0x208 unit_control_flags, zero-extended from
                                        //       unit_control_data.control_flags
    int16_t update_tick_counter;        // 0x20c unit_update increments it every tick and
                                        //       resets it when this unit wins the staggered
                                        //       expensive-update slot
    int8_t shield_sapping;              // 0x20e function input 7 (shield sapping) = 1 - x/90 in
                                        //    unit_update_scale_function_inputs (0x563860)
    int8_t scripted_base_animation_state; // 0x20f hs unit_set_seat (0x47c260) stores it; used instead of the computed
                                          //    base state unless -1
                                        //       a seat / animation index
    int32_t persistent_control_ticks;   // 0x210 unit_set_control_countdown (0x563b20): while > 0 unit_update ORs
                                        //    persistent_control_flags into the controls, and clears them at 0
    uint32_t persistent_control_flags;  // 0x214 the control bits held (the actor death "fire wildly" path uses 0x800,
                                        //    primary trigger)
                                        //       tested for bit 0x800 by unit_update
    datum_index controlling_player;     // 0x218 indexes the player data_array at 0x0087a480
                                        //       (stride 0x200, unit handle at +0x34)
    int16_t ai_stimulus_type;           // 0x21c ai_refresh_unit_stimulus_and_alert (0x42c2a0): re-alerts nearby
                                        //    actors only for a higher stimulus (weapon fire 1, hurt scream 2) or
                                        //    after 30 ticks
    int16_t emotion_animation_index;    // 0x21e unit_scripting_set_emotion_animation writes
                                        //       the model region index it looked up here;
                                        //       0x563b50 prefers it over the graph default
    uint32_t ai_stimulus_tick;          // 0x220 game time stamped with ai_stimulus_type
    real_vector3d desired_facing_vector;// 0x224 unit_control_data.facing_vector
    real_vector3d desired_aiming_vector;// 0x230 unit_control_data.aiming_vector
    real_vector3d aiming_vector;        // 0x23c the current aim; 0x5696f0 returns it and
                                        //       unit_release_thrown_grenade launches along it
    real_vector3d aiming_velocity;      // 0x248 seeded from global_origin3d by unit_update
    real_vector3d desired_looking_vector;// 0x254 unit_control_data.looking_vector
    real_vector3d looking_vector;       // 0x260 0x56bc80 and 0x56c100 cone-test against it
    real_vector3d looking_velocity;     // 0x26c seeded from global_origin3d by unit_update
    real_vector3d throttle;             // 0x278 unit_control_data.throttle
    float primary_trigger;              // 0x284 unit_control_data.primary_trigger
    int8_t aiming_speed;                // 0x288 unit_control_data.aiming_speed
    int8_t melee_state;                 // 0x289 unit_melee_state
    int8_t melee_damage_countdown;      // 0x28a 0x56fc80 and 0x56fd40 reload it with 10 and
                                        //       tick it down between melee damage pulses
    int8_t flaming_ticks;               // 0x28b 60 + rand(90) when an AI is set on fire (0x5705a0); unit_update
                                        //    counts it down and applies the flaming-death damage at 0 (0x570720);
                                        //    forces base state 5 (flaming)
                                        //       stun duration, unit_update decrements it
    int8_t delayed_weapon_drop_ticks;   // 0x28c counted down by unit_update, which drops the current weapon at 0 (set
                                        //    by the actor death path 0x428ab0)
                                        //       seat-teardown paths require it to be 0
    int8_t throwing_grenade_state;      // 0x28d unit_throwing_grenade_state
    int16_t throwing_grenade_counter;   // 0x28e unit_begin_throw_grenade zeroes it,
                                        //       unit_update increments it
    int16_t throwing_grenade_duration;  // 0x290 unit_release_thrown_grenade divides the
                                        //       counter by it to get the throw fraction
    int16_t pad_292;                    // 0x292
    datum_index throwing_grenade_projectile; // 0x294 the grenade object attached to the hand,
                                        //       -1 once released
    uint16_t animation_state_flags;     // 0x298 unit_animation_state_flags
    int16_t aiming_animation_index;     // 0x29a the aiming animation, blended with the aiming screen by 0x563b50
                                        //    (paired with looking_animation_index)
                                        //       unit_try_set_animation_state allocated, -1
                                        //       when none; 0x563b50 needs it before it will
                                        //       blend the aiming overlay
    int16_t looking_animation_index;    // 0x29c the "look" unit animation permutation (unit_try_set_animation_state),
                                        //    blended by the looking vector (0x563b50); -1 none
                                        //       seat / turret overlay
    int16_t overlay_animation_index;       // 0x29e third animation overlay index next to aiming/looking (seat / turret overlay); -1 at spawn, nothing else touches it
    int8_t animation_definition_index;  // 0x2a0 index into the unit block of the animation graph
                                        //       (tag data + 0x0c count, + 0x10 address,
                                        //       stride 100); -1 when the unit has none
    int8_t animation_weapon_index;      // 0x2a1 index into the weapons of that block at +0x5c,
                                        //       stride 0xbc
    int8_t animation_weapon_type_index; // 0x2a2 index into the types of that weapon at +0xb4,
                                        //       stride 0x3c
    int8_t animation_state;             // 0x2a3 unit_animation_state
    int8_t replacement_animation_state; // 0x2a4 overlay slot 0 command (0x565e00): 1 disarm, 2 drop, 3 ready, 4 put
                                        //    away, 5/6 reload, 7 melee, 8 throw grenade, 9 overheat
                                        //       this is 0; 0x565420 clears it
    int8_t overlay_animation_state;     // 0x2a5 overlay slot 1 command (0x566410): fire / charged / chamber
    int8_t seat_command;                // 0x2a6 unit_control_data.animation_state
    int8_t base_animation_state;        // 0x2a7 unit_base_animation_state
    int8_t emotion_animation_frame;     // 0x2a8 -1 when idle; 0x563b50 plays the emotion
                                        //       animation of the graph at this frame
    int8_t pad_2a9;                     // 0x2a9 alignment; never read
    unit_animation_overlay overlays[3]; // 0x2aa 0x2ae 0x2b2, index then frame in each pair
    int8_t aiming_bounds_valid;         // 0x2b6 0x563b50 sets it after filling aiming_bounds
    int8_t looking_bounds_valid;        // 0x2b7 0x563b50 sets it after filling looking_bounds
    float aiming_bounds[4];             // 0x2b8 -yaw, +yaw, -pitch, +pitch, each an int16
                                        //       frame count from the graph times its float scale;
                                        //       0x5697a0 clamps a direction into this box
    float looking_bounds[4];            // 0x2c8 the same four for looking, taken from the
                                        //       graph unit block at +0x20..+0x36
    uint8_t unknown_2d8[8];             // 0x2d8 untouched by this module
    float illumination;                 // 0x2e0 unit_calculate_luminosity: the 0.299 / 0.587
                                        //       / 0.114 luma of the sampled lighting, or the
                                        //       value of the parent when attached
    float attached_light_luminosity;    // 0x2e4 object_sum_attached_light_luminance result
    float mouth_aperture;               // 0x2e8 function input 4 (mouth aperture); blends the "talk" animation
                                        //    (0x563b50); decays 0.1 per tick
                                        //       unit block by it; unit_update decays it
                                        //       toward 0 each tick
    uint32_t last_entrance_attempt;     // 0x2ec
    int16_t vehicle_seat_index;         // 0x2f0 index into the Unit tag seats block of the parent
                                        //       (stride 0x11c), -1 when not seated
    int16_t current_weapon_index;       // 0x2f2 slot in weapons[], -1 when unarmed
    int16_t desired_weapon_index;       // 0x2f4 unit_control_data.weapon_index;
                                        //       unit_ready_desired_weapon consumes it
    int16_t pad_2f6;                    // 0x2f6
    datum_index weapons[4];             // 0x2f8 the inventory; 0x56d660 returns the first -1
    int32_t weapon_ready_ticks[4];      // 0x308 zeroed when a weapon is picked up; 0x56dba0
                                        //       picks the lowest when choosing a replacement
    datum_index equipment_object_index; // 0x318 the object currently held in the hand
                                        //       (0x56d1a0 attaches it, 0x56d2c0 and 0x56d300
                                        //       release it), -1 when empty
    int8_t current_grenade_index;       // 0x31c unit_get_current_grenade_index returns it
    int8_t desired_grenade_index;       // 0x31d unit_control_data.grenade_index
    int8_t grenade_counts[2];           // 0x31e unit_get_grenade_count indexes it; 0x55b110
                                        //       restores both bytes at once from 0x52c
    int8_t zoom_level;                  // 0x320 -1 when not zoomed
    int8_t desired_zoom_level;          // 0x321 unit_control_data.zoom_level; 0x5659c0 and
                                        //       0x565a70 force it back to -1
    int8_t weapon_control_idle_ticks;   // 0x322 0 while any weapon control (0x7c00) is held, else counts up to 0x7f
                                        //    (unit_evaluate_flee_reaction wants > 120)
                                        //       unit_update; 0x55e2d0 flees above 120
    int8_t aiming_change;               // 0x323 clamp(aim angle change / (aiming_velocity_maximum / 30)) * 255;
                                        //    function input 3 (aiming change)
    datum_index driver_unit_index;      // 0x324 the child object in the first tracked seat;
                                        //       0x56ce30 recomputes it and unit_update copies
                                        //       the control input of this occupant into itself
    datum_index gunner_unit_index;      // 0x328 the child object in the second tracked seat
    datum_index last_parent_object_index; // 0x32c the object this unit was last seated in,
                                        //       recorded by every detach path
    int32_t last_seat_change_tick;      // 0x330 game time at that detach
    int16_t encounter_index;            // 0x334 the actor's encounter, kept on the unit (copied at death,
                                        //    ai_unit_set_squad_reference 0x435750); -1 none
    int16_t squad_index;                // 0x336 the squad within encounter_index
                                        //       the unit leaves the seat (0x568610, 0x568cb0)
    float driver_seat_power;            // 0x338 powered seat 0: ramps up by 1/(driver_powerup_time*30) while the seat
                                        //    has a driver, down by the powerdown time; function input 1
                                        //       ground-effect routines all multiply by
    float gunner_seat_power;            // 0x33c powered seat 1, the same for the gunner; function input 2
    float integrated_light_power;       // 0x340 +1/6 per tick while the flashlight flag (0x80000) is set, else -1/24;
                                        //    function input 5
                                        //       and 1/6 up
    float integrated_light_energy;      // 0x344 1.0 at unit_new; drains 1/3600 per tick while the light is on,
                                        //    recharges 1/900; the light goes off at 0
                                        //       1/3600 down; packed into the network update
    float integrated_night_vision_power; // 0x348 +1/12 / -1/24 on unit flag 0x4000000 (night vision)
                                        //       0x5659c0 and 0x565a70
    real_point3d seat_acceleration_last_position; // 0x34c 0x56e820 second-differences the (parent) position against
                                                  //    this and the last velocity for the seat acceleration animation
                                                  //    controls
                                        //       frame to frame and 0x570cb0 shifts it by the
                                        //       movement delta of the parent
    real_vector3d seat_acceleration_last_velocity; // 0x358 the previous tick's position delta (0x56e820)
    float animation_controls_smoothed[3]; // 0x364 unit_update runs 0.7 * old + 0.3 * new;
                                        //       0x563b50 drives three graph animations by them
    float animation_controls[3];        // 0x370 the raw 0..1 values 0x56e820 computes
    float active_camouflage_power;      // 0x37c ramps by 1/120 (or the weapon's regrowth rate) while unit flag 0x10
                                        //    is set; damage and firing drain it; drawn as the camouflage effect while
                                        //    > 0
                                        //       reduced by damage in 0x5674a0
    float super_active_camouflage_power; // 0x380 ramps 1/90 on unit flag 0x20 (actor variant super active camouflage)
    datum_index dialogue_tag_index;     // 0x384 the unit_dialogue tag 0x560d00 walks
                                        //       (records of stride 0x10 at tag data + 0x1c)
    unit_speech current_speech;         // 0x388 the line being played
    unit_speech pending_speech;         // 0x3b8 the line queued behind it; 0x561620 promotes
                                        //       it through 0x560f20 when the current one ends
    int16_t minor_hurt_speech_decay_ticks; // 0x3e8 22 after a low-damage hurt line (0x561140); 0x561620 counts it
                                           //    down and takes one off minor_hurt_speech_count
    int16_t minor_hurt_speech_count;    // 0x3ea low-damage hurt lines recently spoken; another is refused above 2
    int16_t minor_hurt_speech_delay_ticks; // 0x3ec 30 after a low-damage line; must be 0 for the next
    int16_t major_hurt_speech_delay_ticks; // 0x3ee 60 after a high-damage line; blocks non-scripted lines while set
                                           //    (nothing in the rewrite counts it down: 0x561620 decrements 0x3ec
                                           //    twice)
    uint32_t communication_hold_tick;      // 0x3f0 ai_communication_record_line_played stamps it with game tick + max(speech_duration_ticks - 45, 0); -1 at spawn; unit_animation_change_priority_check hands it back to its callers
    int8_t speech_started;              // 0x3f4 0x561620 sets it once the sound was started
    int8_t speech_lipsync_stopped;      // 0x3f5 set once the lipsync countdown hit 0
    int8_t speech_finished;             // 0x3f6 set once the duration countdown hit 0
    int8_t pad_3f7;                     // 0x3f7 alignment
    int16_t speech_delay_ticks;         // 0x3f8 loaded from unit_speech.delay_ticks
    int16_t speech_duration_ticks;      // 0x3fa 0x560f20 computes it from the length of the
                                        //       sound tag at +0x84 (times 30, divided by 1000),
                                        //       or 0x2d when there is no sound
    int16_t speech_lipsync_ticks;       // 0x3fc loaded from unit_speech.lipsync_ticks
    int16_t speech_tail_ticks;          // 0x3fe loaded from unit_speech.tail_ticks
    datum_index speech_sound_handle;    // 0x400 the handle 0x00543ce0 returned, -1 when idle
    int16_t delayed_damage_category;    // 0x404 DamageEffect category; unit_update hands it to
                                        //    actor_react_to_threat_event when delayed_damage_ticks expires
    int16_t delayed_damage_ticks;       // 0x406 45 on each local damage
    float delayed_damage_amount;        // 0x408 the peak recent body + shield damage in the window
                                        //       consumed by unit_update
    datum_index delayed_damage_responsible_object; // 0x40c the damage's responsible object, forgotten by
                                                   //    unit_forget_object_reference
    int32_t flaming_responsible_object; // 0x410 the killer when the flaming state starts (0x5705a0); source of the
                                        //    flaming-death damage
    float idle_turn_angle;              // 0x414 0x570650 seeds it from the current heading
                                        //       plus a random offset, 0x570840 wanders it
    float idle_turn_offset;             // 0x418 the second, tighter angle of the wander
    int32_t death_time;                 // 0x41c game time of death (unit_release_transient_state paths, 0x562030); -1
                                        //    alive
                                        //       seat-teardown paths
    int16_t feign_death_ticks;          // 0x420 (rand + Unit feign_death_time) * 30 when a feign-capable unit takes
                                        //    enough damage; the unit stands up at 0; the AI treats a dead unit with
                                        //    it set as not dead
    int16_t active_camouflage_regrowth; // 0x422 1 after firing drains the camouflage: the weapon's regrowth rate
                                        //    applies until the power is back to 1
                                        //       predicted-state flag
    float stun;                         // 0x424 raised by DamageEffect stun up to its maximum; movement, jump and
                                        //    input scale by 1 - penalty * stun; zeroed when stun_ticks expires
                                        //       unit_update and the movement solvers scale
                                        //       velocity by 1 - stun_movement_penalty * this.
                                        //       Confirmed: both integrators multiply it by
                                        //       GlobalsPlayerInformation.stun_movement_penalty
                                        //       (tag +0x80 of the block at globals + 0x174)
    int16_t stun_ticks;                 // 0x428 DamageEffect stun time * 30, clamped
    int16_t ai_communication_count;     // 0x42a 0x568230 counts hits and broadcasts once the
                                        //       count reaches 3 (5 for a player)
    int32_t ai_communication_tick;      // 0x42c tick of the last hit; the count resets after
                                        //       0x78 ticks
    unit_recent_damage recent_damage[4];// 0x430 the four-slot damage cache
    uint32_t user_animation_indices;    // 0x470
    int8_t network_update_forced;       // 0x474 set on the server for trigger / grenade controls and by
                                        //    unit_detach_reposition_and_nudge; cleared by both network update
                                        //    encoders
                                        //       bits 0x2800, cleared once the delta is sent
    int8_t network_update_applied;      // 0x475 write-only flag: unit_new clears it; the network create/update apply paths
                                        //       and the scripted spawn set it
    int8_t pad_476[2];                  // 0x476 alignment
    unit_control_data saved_control;    // 0x478 the server-side copy 0x5639f0 block-moves
    int8_t control_update_id_valid;     // 0x4b8 unit_apply_control_block: 1 with a source update id;
                                        //    game_engine_server_update_player_positions consumes it
    int8_t pad_4b9[3];                  // 0x4b9 alignment
    int32_t control_update_id;          // 0x4bc the network update id of the control record; the queued position with
                                        //    this tick is applied
                                        //       unit_update reads it back
    uint8_t position_after_completing_last_client_update[12]; // 0x4c0 untouched by this module
} unit_data;                            // size 0x2d8 (object 0x1f4 .. 0x4cc)

#ifdef HALO_TYPES_OBJECTS_H
// A unit object as one struct: the common object header (types/objects.h) followed by unit_data,
// so a field is one fixed offset from the object like the original code uses (bipeds and vehicles
// append their own data after 0x4cc).
typedef struct unit_object {
    object base;                        // 0x000
    unit_data unit;                     // 0x1f4
} unit_object;
typedef char unit_object_unit_at_1f4[offsetof(unit_object, unit) == 0x1f4 ? 1 : -1];
#endif

// ---------------------------------------------------------------------------
// unit_state_change_record  (0x20 bytes, passed BY VALUE to 0x566c00, which replaces the unit with
// its network id and sends the whole record as message type 0xc)
// Built by unit_apply_damage_effects (0x567ec2..0x567f0b, ebp-0x64) and unit_exit_vehicle_seat
// (0x5681b4..0x5681d8).
// ---------------------------------------------------------------------------
typedef struct unit_state_change_record {
    datum_index unit;               // 0x00
    uint8_t valid;                  // 0x04
    uint8_t killed;                 // 0x05
    uint8_t knocked_down;           // 0x06 the stun roll of unit_apply_damage_effects
    uint8_t violent;                // 0x07 damage effect +0x30 at least 0x672be4 on a kill
    uint8_t stunned;                // 0x08 unit tag flag 0x80 without effect flag 4, or a melee in progress
    uint8_t special;                // 0x09 notify flags 0x8a
    uint8_t no_direction;           // 0x0a
    uint8_t pad_0b;                 // 0x0b
    int16_t region_index;           // 0x0c
    int16_t pad_0e;                 // 0x0e
    float angle;                    // 0x10 between the unit's forward and the damage direction (xy)
    real_vector2d direction;        // 0x14 damage direction (xy), when no_direction is clear
    uint32_t player_value;          // 0x1c the unit's player +0x2c
} unit_state_change_record;         // size 0x20

// ---------------------------------------------------------------------------
// biped_data  (0x84 bytes, at object + 0x4cc; a biped object is 0x550 bytes total)
// Assigned from the functions the biped row of the object_type_definition table reaches:
// biped_update (0x5590a0) and everything below it, the two movement solvers 0x55bea0 and
// 0x55cfd0, the ground-adjustment solver 0x557a90 / 0x558000 / 0x558a20, and the four
// network columns 0x55aed0, 0x55b3d0, 0x55b440 and 0x55b5f0.
// ---------------------------------------------------------------------------
typedef struct biped_data {
    uint32_t flags;                     // 0x4cc bit 0 = AIRBORNE (the solver's result bit 0; 0x560800 levels when set) (0x55ecf0 sets it, 0x560800
                                        //       and 0x569b30 test it), bit 1 = jumping
                                        //       (0x559fa0 sets 0 and 1 together; the recorded-
                                        //       animation update also sets it, or BYTE
                                        //       [edi+0x4cc],0x2 at 0x44ac86), bit 4 =
                                        //       the 0x55bea0 landing latch, bit 5 (0x20) =
                                        //       the ground-adjust dirty bit 0x55ad00 sets
                                        //       and 0x55ad70 clears
    int8_t landing_ticks;               // 0x4d0 0 on landing (0x55eaa0), +1 per landing tick (0x55eb90) until
                                        //    landing_duration_ticks
    int8_t landing_duration_ticks;      // 0x4d1 impact speed against Biped soft / hard landing velocities, scaled
                                        //    into the landing times * 30
                                        //       loaded by 0x55eaa0
    int8_t movement_state;              // 0x4d2 biped_update maps the animation state onto
                                        //       0 (standing), 1 (moving) or 2 (other);
                                        //       unit_update_facing and 0x560410 branch on it
    int8_t last_ground_object_ticks;    // 0x4d3 60 while the solver reports a supporting object, counts down
                                        //    otherwise; last_ground_object_index clears at 0
                                        //       movement solvers every tick that
                                        //       last_ground_object_index is refreshed
    datum_index last_ground_object_index; // 0x4d4 the object the biped last stood on, as the
                                        //       movement solver reported it; both integrators
                                        //       (0x55bea0 and 0x55cfd0) store it and reload the
                                        //       0x4d3 countdown with 60, or clear it to -1 once
                                        //       that countdown runs out while airborne. It is an
                                        //       object datum: the elevator rider sweep in
                                        //       device_machine_update compares it with the
                                        //       machine's own object index (0x44b4e4)
    datum_index ground_surface_index;   // 0x4d8 the supporting surface 0x560630 found, -1
                                        //       when airborne; 0x560800 refuses to level the
                                        //       up-vector without it
    datum_index cached_ground_surface_index; // 0x4dc bsp surface under the biped (0x55ab30, not a datum); reset to -1
                                             //    every tick by the integrators
                                        //       look-at result
    real_point3d cached_ground_point;   // 0x4e0 the point on that surface under the biped; AI target code uses it as
                                        //    the ground position
    int32_t cached_ground_point_tick;   // 0x4ec 0x55ab30 refreshes the cache at most once per game tick
    datum_index last_ground_surface_index; // 0x4f0 last valid cached_ground_surface_index, retried while the point
                                           //    still projects inside it
    datum_index melee_target_index;     // 0x4f4 the object 0x55cfd0 hands to 0x56ff40 when
                                        //       melee_state is 3
    int32_t last_falling_reaction_tick; // 0x4f8 the evade / airborne-vehicle flee reactions stamp it and wait 15
                                        //    ticks
                                        //       their reactions to once every 15 ticks
    datum_index bump_object_index;      // 0x4fc the object the biped ran into (0x55e0a0); with the bump possession
                                        //    cheat the local player takes it over after 3 ticks
    int8_t bump_ticks;                  // 0x500 ticks the same bump object has been touched; -15 as the cooldown
                                        //    after a possession
                                        //       0x55e0a0 saturates it at 0xf1
    int8_t airborne_ticks;              // 0x501 +1 per airborne tick up to 0x7f, 0 on the ground
                                        //    (unit_predict_movement_delta)
                                        //       at 0x7f by biped_update
    int8_t slipping_ticks;              // 0x502 the same counter for movement flag bit 1 (slipping: the change
                                        //    exceeded maximum_acceleration)
    int8_t stop_moving_ticks;           // 0x503 0x560410: 1 when movement starts to stop, counts up while standing
                                        //    and fires footstep trigger 3 at 4
    int8_t jump_ticks;                  // 0x504 ticks on the ground since the last jump (0x55ec90); a jump (0x55ecf0)
                                        //    needs > 5
    int8_t melee_ticks;                 // 0x505 3/4 of the weapon's first-person melee animation on a melee, counts
                                        //    down; weapon control 0x10 while > 0
    int8_t melee_inflict_tick;          // 0x506 melee_ticks value at which unit_melee_attack_scan runs
    int8_t pad_507;                     // 0x507 alignment
    int16_t landing_type;               // 0x508 0 soft, 1 hard, -1 none (0x55eaa0); a hard landing blocks jumping
                                        //       and 0x55eb90 turns it into a trigger id
    int16_t pad_50a;                    // 0x50a
    float crouch_fraction;              // 0x50c 0..1; the movement solvers step it by the
                                        //       crouch_camera_velocity of the Biped tag (0x4cc),
                                        //       unit_get_camera_position and 0x55a2e0 blend
                                        //       the standing and crouching heights with it
    float bank_angle;                   // 0x510 steps toward throttle * Biped bank_angle; unit_update_up_vector
                                        //    rotates the up vector by it
    real_vector3d ground_normal;        // 0x514 the supporting plane normal 0x560630 caches
    uint32_t ground_plane_distance;     // 0x520 the d of the ground plane whose normal is ground_normal (the solver's
                                        //    plane.d)
    uint8_t ground_adjust_iteration;    // 0x524 0x557a90 increments it up to 0x7f
    uint8_t ground_adjust_iteration_limit; // 0x525 0x55ad00 seeds it with 0x14; the solver
                                        //       stops once the iteration reaches it
    uint8_t baseline_valid;             // 0x526 set by the network create and scripted spawn
    uint8_t network_update_sequence;    // 0x527 0x55b5f0 rejects an update whose sequence is
                                        //       behind this one
    uint8_t network_delta_sequence;     // 0x528 0x55b440 increments it per delta sent and
                                        //       wraps it at 0xff
    uint8_t pad_529[3];                 // 0x529 alignment
    int16_t network_grenade_counts;     // 0x52c both grenade counts as one int16; 0x55b110
                                        //       copies it into unit_data.grenade_counts
    int16_t pad_52e;                    // 0x52e
    float network_body_vitality;        // 0x530 0x55b110 copies it into object 0xe0
    float network_shield_vitality;      // 0x534 0x55b110 writes object 0xe4 as this times 3
    int8_t network_shield_stunned;      // 0x538 0x55b110 turns it into object 0x104
    int8_t pad_539[3];                  // 0x539 alignment
    int8_t network_baseline_valid;      // 0x53c 0x55b5f0 sets it when it snapshots the block
    int8_t pad_53d[3];                  // 0x53d alignment
    int16_t baseline_grenade_counts;    // 0x540 the snapshot 0x55b5f0 keeps of 0x52c
    int16_t pad_542;                    // 0x542
    float baseline_body_vitality;       // 0x544 snapshot of 0x530
    float baseline_shield_vitality;     // 0x548 snapshot of 0x534
    int8_t baseline_shield_stunned;     // 0x54c snapshot of 0x538
    int8_t pad_54d[3];                  // 0x54d alignment
} biped_data;                           // size 0x84 (object 0x4cc .. 0x550)

// ---------------------------------------------------------------------------
// vehicle_data  (0xf4 bytes, at object + 0x4cc; a vehicle object is 0x5c0 bytes total)
// Assigned from vehicle_update (0x570ee0), unit_calculate_animation_controls (0x5756f0,
// the +0x38 column of the vehicle row), the reset column 0x570b00, which zeroes 0x4cc..0x520
// and so bounds the live part of the record -- and the lean, thruster and skid helpers
// 0x572b60..0x575640.
// ---------------------------------------------------------------------------
typedef struct vehicle_data {
    uint16_t flags;                     // 0x4cc bit 0 = over the blur_speed of the Vehicle tag
                                        //       (0x318), bit 2 = has ground contact, bit 3 =
                                        //       hovering, bit 4 = controls were active this
                                        //       tick; vehicle_update writes it a byte and a
                                        //       word at a time, never as a dword
    int16_t decay_ticks_remaining;      // 0x4ce 0x4ce reloaded with 15 while controls are active;
                                        //    unit_update_recoil_decay damps velocity by 0.835 per tick and stops on
                                        //    the 0 edge
                                        //       controls move; unit_update_recoil_decay
                                        //       counts it down and fires on the 0 edge
    uint8_t airborne_ticks;             // 0x4d0 0x575640 increments it while off the ground
                                        //       and 0x5756f0 folds it into the blend weight
    uint8_t push_direction;             // 0x4d1 direction code a player push interaction (action type 11) stores:
                                        //       1 / 2 push sideways, 3 / 4 push along the vehicle's forward axis;
                                        //       vehicle_update applies the velocity impulse while it is non-zero
    uint8_t push_ticks;                 // 0x4d2 ticks the impulse has been applied; vehicle_update clears it
                                        //       and push_direction once it reaches 0x1e
    uint8_t landing_ticks;              // 0x4d3 0x575640 bumps it when ground contact resumes
    float forward_velocity;             // 0x4d4 divided by the Vehicle tag field
                                        //       maximum_forward_speed (0x2f8) or
                                        //       maximum_reverse_speed (0x2fc)
    float sideways_velocity;            // 0x4d8 divided by maximum_left_slide (0x330) or
                                        //       maximum_right_slide (0x334)
    float turning_velocity;             // 0x4dc divided by maximum_left_turn (0x308) or
                                        //       maximum_right_turn (0x30c)
    float wheel_rotation;               // 0x4e0 0x572cd0 accumulates forward_velocity into it
                                        //       and wraps it at wheel_circumference (0x310)
    float left_wheel_rotation;          // 0x4e4 0x572b60 accumulates forward minus turning
    float right_wheel_rotation;         // 0x4e8 0x572b60 accumulates forward plus turning
    float ground_lean;                  // 0x4ec 0..1, rate-limited to 0.1 per tick by the
                                        //       hover routines 0x5738b0 and 0x5739a0
    float ground_contact_fraction;      // 0x4f0 0..1; 0x573100 and 0x573f60 ease it toward
                                        //       the speed fraction and the thruster effects
                                        //       scale by it
    uint8_t contact_point_traction[20]; // 0x4f4 one wear byte per physics mass point;
                                        //       0x575170 reads and rewrites entry i, 0xff
                                        //       meaning full traction. UNRESOLVED: the array
                                        //       bound is the mass point count, not a
                                        //       constant, and 0x570b00 only zeroes the first
                                        //       two dwords of it -- see the notes file
    real_vector3d accumulated_force;    // 0x508 object_physics_tick (0x507840) adds the three
                                        //       floats into its force sum (fadd 0x507942..
                                        //       0x507963) and zeroes them (0x507993..); the
                                        //       mass-point overlap solver accumulates into it;
                                        //       0x570b00 zeroes it
    real_vector3d accumulated_torque;   // 0x514 the same for the torque sum (fadd 0x507971..
                                        //       0x50798d, zeroed 0x5079a5..0x5079b5)
    uint32_t active_marker_mask;        // 0x520 one bit per hover / contact marker; 0x575e30
                                        //       averages the positions of the set ones
    uint8_t collision_update_pending;   // 0x524 0x524 set when mass-point overlap applies force to the vehicle;
                                        //    cleared by vehicle_encode_network_update and player_update_history_play
    uint8_t network_position_pending;   // 0x525 write-only flag (biped_data reuses the byte as ground-adjust limit): the scripted spawn and vehicle reset set 1, the reset clears it in network games
    uint8_t network_epoch;              // 0x526 incremented by the vehicle reset at 0x572410 (which also sets 0x525 and 0x528 to 1); sent in every
                                        //    vehicle network update header; the receiver compares it with its own epoch (0x572742) and takes a different path on a mismatch
    uint8_t network_update_sequence;    // 0x527 0x5724d0 increments it and wraps it at 0xff
    uint8_t network_delta_sequence;     // 0x528 base of the delta record 0x5724d0 encodes
    uint8_t unknown_529[3];             // 0x529 untouched by this module
    real_point3d network_baseline_position;         // 0x52c vehicle_network_baseline_take (0x572410) copies position here
    real_vector3d network_baseline_velocity;        // 0x538 the same for velocity
    real_vector3d network_baseline_angular_velocity; // 0x544 the same for angular velocity
    real_vector3d network_baseline_forward;         // 0x550 the same for the forward vector
    real_vector3d network_baseline_up;              // 0x55c the same for the up vector
    uint8_t unknown_568[0x44];          // 0x568 untouched by this module
    int32_t network_update_tick;        // 0x5ac game tick of the last seat change or network
                                        //       update; vehicle_update rate-limits on it
    int16_t cinematic_facing_index;     // 0x5b0 0x570de0 indexes the cinematic direction table
                                        //       of the scenario with it
    uint8_t pad_5b2[2];                 // 0x5b2 padding
    real_point3d network_update_position; // 0x5b4 the vehicle position at the last network_update_tick refresh (CEA spawn_position)
} vehicle_data;                         // size 0xf4 (object 0x4cc .. 0x5c0)

#ifdef HALO_TYPES_OBJECTS_H
// Biped and vehicle objects as one struct each: object header, unit_data, then the type's own data.
typedef struct biped_object {
    object base;                        // 0x000
    unit_data unit;                     // 0x1f4
    biped_data biped;                   // 0x4cc
} biped_object;
typedef struct vehicle_object {
    object base;                        // 0x000
    unit_data unit;                     // 0x1f4
    vehicle_data vehicle;               // 0x4cc
} vehicle_object;
typedef char biped_object_biped_at_4cc[offsetof(biped_object, biped) == 0x4cc ? 1 : -1];
typedef char vehicle_object_vehicle_at_4cc[offsetof(vehicle_object, vehicle) == 0x4cc ? 1 : -1];
#endif

// ---------------------------------------------------------------------------
// unit-side records that are not part of an object extension
// ---------------------------------------------------------------------------

// The scale/flags request unit_apply_scale_change (0x562030) receives as its second
// argument. UNSURE: the shape is inferred from the two slots the function reads (a float at
// +0x00 tested > 0, a flags dword at +0x04 tested for bit 0); nothing in this module
// constructs one, so the caller that does may show it to be longer than 8 bytes.
typedef struct unit_scale_request {
    float scale;                        // 0x00 written to object.body_vitality when > 0
    uint32_t flags;                     // 0x04 bit 0 = run the seat/grenade teardown
} unit_scale_request;                   // size 0x08, UNSURE

// The wire record the biped health/grenade/shield network column exchanges. Confirmed from
// *both* ends, which is what makes the leading offsets solid rather than inferred:
//   - the sender, unit_submit_periodic_network_update (0x55b440), builds it on the stack as
//     nine locals at -0x1c..-0x03 that are contiguous and naturally aligned, and hands
//     message_delta_encode_message pointers to exactly base+0x00 and base+0x0c;
//   - the receiver, unit_apply_network_health_update (0x55b5f0), reads +0x04 against
//     biped_data.network_update_sequence (0x527), +0x05 against network_delta_sequence (0x528),
//     +0x06 as the "this is a full update, take the baseline too" flag and +0x07 == 1 as the
//     gate on applying the shield value -- the same four bytes, in the same order, that the
//     sender writes there.
// UNSURE: everything from +0x08 on. The sender fills it and the receiver never reads it
//   directly (the field descriptors of message_delta do), so the names below are how the sender uses them.
// UNSURE: the record is reached as param_2[0x11] (+0x44) of the message wrapper the network
//   layer passes in; the wrapper itself is not typed here.
typedef struct unit_network_update_record {
    int32_t hash_key;                   // 0x00 hash_table_get result, 0 when absent
    uint8_t update_sequence;            // 0x04 biped_data.network_update_sequence
    uint8_t delta_sequence;             // 0x05 biped_data.network_delta_sequence
    uint8_t is_full_update;             // 0x06 sender writes (mode == 0); receiver takes the
                                        //      baseline block only when this is non-zero
    int8_t shield_recharging;           // 0x07 sender writes object.unknown_122; receiver applies
                                        //      object.shield_vitality only when this is 1
    int32_t timestamp_milliseconds;     // 0x08 QueryPerformanceCounter * 1000 / frequency
    int16_t grenade_counts;             // 0x0c unit_data.grenade_counts as one int16, UNSURE
    int16_t unknown_0e;                 // 0x0e never written by the sender, UNSURE
    float body_vitality;                // 0x10 object.body_vitality, UNSURE
    float shield_vitality;              // 0x14 object.shield_vitality/3 or the cached network
                                        //      value, UNSURE
    uint8_t shield_stunned;             // 0x18 object.shield_stun_ticks > 0, UNSURE
} unit_network_update_record;           // size 0x19, UNSURE past 0x08

// The network packet unit_apply_network_control_update (0x566c90) receives in EAX. Its first
// field is itself a pointer -- Ghidra emits *(int *)*in_EAX for the kind test -- and the rest
// of the record is a bit-packed payload whose message_delta field definition is not in this
// module, so it is carried as an opaque byte window rather than invented field by field.
typedef struct unit_network_control_packet {
    int32_t *kind_ptr;                  // 0x00 *kind_ptr == 0 selects the control-update path
    uint8_t payload[32];                // 0x04 UNSURE: bit-packed, see src/units/unit_apply_network_control_update.c
} unit_network_control_packet;          // size 0x24, UNSURE

// The units-side view of the head of the object record. types/objects.h declares 0x019..0x021 as
// unknown_019[7] + player_visibility_mask and 0x022.. as unknown_022[0x3a] because the
// objects module never reads them; unit_refresh_anchor_position (0x558eb0) reads and writes a
// 12-byte point at 0x01c, which straddles that boundary. Rather than re-cut `object` from the
// units side (the evidence the objects module has for 0x020 is good), the field is declared here
// as an overlay and objects.h is left alone. Only cached_anchor_point is meaningful; the
// leading bytes exist to place it.
// UNSURE: 0x558eb0 is the only reader/writer of 0x01c in the packs for this module, so the
// name anchor point describes what the function does with it, not a proven engine name.
typedef struct unit_object_anchor {
    uint8_t before_01c[0x1c];           // 0x000 object.definition_tag .. unknown_019 tail
    real_point3d cached_anchor_point;   // 0x01c compared against object.position (0x05c)
} unit_object_anchor;                   // overlay of the first 0x28 bytes of `object`

#pragma pack(pop)

// The jump-table body at 0x561604 that unit_dispatch_reaction_animation (0x5614a0) indexes
// with a reaction code. Ghidra shows it as PTR_LAB_00561604, i.e. a table of code labels, and
// every entry is entered with no stack arguments -- whatever the handlers read, they read out
// of registers or globals.
// UNSURE: element count, and whether the handlers really take no arguments.
typedef void (*unit_reaction_animation_handler)(void);

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// global 0x006e4a08: real_point3d unit_ground_adjust_node_positions[64]
//     0x557a90 copies the position of each skeleton node (object nodes array, stride 0x34,
//     position at +0x28) into this array before running the ground-adjustment solve, then
//     hands the array to 0x558a20 so the node bases can be rotated onto the solved
//     positions. It is the only global this module writes, and nothing else in the image
//     references it. UNRESOLVED: the element count is the node count of the animation graph, so
//     the bound of 64 is the engine node maximum rather than a value this module proves.
//
// Globals this module reads but does not own, listed so the ownership stays honest:
//   0x008603b0 data_array *object_data              (types/objects.h)
//   0x0087bc14 tag_instance *tag_instances          (types/cache.h)
//   0x0087a480 data_array *player_data              (players module, stride 0x200)
//   0x0087a478 the local player globals; count at +0x0c, handles from +0x04
//   0x0087a464 data_array *object_list_header_data  (types/hs.h; 0x56bbd0 allocates one)
//   0x00880360 data_array *actor_data               (ai module, stride 0x724)
//   0x006f1d6c game.h game_time_globals: current tick at +0x0c, +0x1c leftover_time (the
//              fractional-tick accumulator, R32; units scales it by 29.999998)
//   0x006f1d20 game_engine_definition *current_game_engine (game.h, R04): non-NULL when a
//              multiplayer engine is loaded; every damage and seat path branches on it
//   0x006f1cf0 the vehicle network update period
//   0x00719720 the game connection role: 1 = client, 2 = server
//   0x0071c2d8 the player-control globals whose +0xf48 is the prediction history
//   0x0071c419 the "unit updates are suppressed" flag unit_update checks between phases
//   0x00746fa0 the globals tag data; +0x174 the player information block, +0x18c and +0x190
//              the grenade tables, +0x194 / +0x198 the material table (stride 0x374)
//   0x006b0b80 and 0x006b0b84 the game engine globals the friendly-fire test reads
//   0x006ef910 a pointer to the AI update-stagger record {int16 threshold, int16 highest,
//              uint8 claimed}; unit_update writes two of its three fields but 0x45b780
//              (ai) owns and resets it
//   0x0087abc1, 0x0087abc2, 0x0087abc3, 0x0087abc4 the cheat / debug toggles
//   0x00719cd0 random_seed_global                   (types/math.h)
//   0x0065e7a8, 0x0065e94c, 0x0065e964 the speech fallback chain, priority table and
//              minimum repeat interval table (.rdata, read-only)
//   0x0069fde4 the six unit_base_animation_state names (.data, read-only)
//   0x00696714, 0x00696718, 0x00696720, 0x006966f8 the shared constant vectors
//   0x00696664 the matrix4x3_multiply thunk pointer (types/math.h)
