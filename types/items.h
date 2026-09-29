#pragma once
// Blam items module (halo.exe 1.0.10 retail, 0x4bbb50..0x4c6340, 101 functions).
// The item branch of the object hierarchy: the item_data extension every weapon / equipment /
// garbage object carries on top of the common object record, the resting-on-ground and
// ground-alignment bookkeeping the item physics uses, and the whole weapon layer -- the
// per-trigger firing/charging state machine, the per-magazine reload state machine, the
// heat/age meters, the first-person animation state, and the two network delta records
// weapons and equipment replicate.
//
// Offsets in comments are byte offsets from the OBJECT base (the same numbering the
// decompiled module uses) unless a struct says otherwise. Where the binary itself carries
// the layout it is used in preference to the decompiler and the fact is called out:
//   - The object_type_definition rows at 0x0069bfdc fix every size in this header. Reading
//     .data (PE image base 0x400000, .data VA 0x676000 -> file offset 0x276000) gives, at
//     +0x08 of each row, the runtime object_size:
//       0x0069b360 "object"     obje  0x1f4
//       0x0069b680 "item"       item  0x22c  -> item_data      is 0x38  at object+0x1f4
//       0x0069b748 "weapon"     weap  0x340  -> weapon_data    is 0x114 at object+0x22c
//       0x0069b810 "equipment"  eqip  0x294  -> equipment_data is 0x68  at object+0x22c
//       0x0069b8d8 "garbage"    garb  0x244  -> garbage_data   is 0x18  at object+0x22c
//     Each row subdefinitions[] at +0x80 gives the chain: item is {object, item}, weapon is
//     {object, item, weapon}, equipment is {object, item, equipment}, garbage is
//     {object, item, garbage}. So all three concrete types start their own extension at 0x22c
//     and the same byte offset means three different things depending on object_header.type.
//   - The same rows correct three fields that types/objects.h currently leaves unknown, and
//     the correction is what makes FUN_004c1350 readable. object_type_definition is
//       +0x0a int16  scenario_placement_offset   byte offset of a TagReflexive in the Scenario tag
//       +0x0c int16  scenario_palette_offset     byte offset of the matching palette TagReflexive
//       +0x0e int16  scenario_placement_size     stride of one placement record
//       +0x10 int32  network_delta_message_type  index into the network message group, -1 = none
//     The weapon row holds 0x270 / 0x27c / 0x5c / 3 and Scenario.weapons is at 0x270,
//     Scenario.weapon_palette at 0x27c and sizeof(ScenarioWeapon) is 0x5c; the equipment row
//     holds 0x258 / 0x264 / 0x28 / 2 against Scenario.equipment 0x258, equipment_palette
//     0x264, sizeof(ScenarioEquipment) 0x28; the biped row holds 0x228 / 0x234 / 0x78 against
//     Scenario.bipeds / biped_palette / sizeof(ScenarioUnit). The item and garbage rows hold
//     -1 in all four, which is why neither type has a network delta record here.
//     The scenario placement sweep at 0x4f3ba0 (Ghidra calls it
//     scenario_objects_place, which does not match its body) walks +0x0a/+0x0c/+0x0e
//     against the scenario tag data, and FUN_004bc0f0 / FUN_004c5f10 pass +0x10 straight to
//     message_delta_encode_message as the message type.
//   - The 2-entry string table at 0x006961b8 is read out of .data as
//     {"~primary-blur", "~secondary-blur"}; weapon_update indexes it by trigger index, which
//     is what identifies weapon_trigger_state.firing_rate as the blur driver.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   datum_index, data_array, data_iterator, object_iterator style handles
//   types/math.h     real_point3d, real_vector3d
//   types/cache.h    tag_instance (0x0087bc14, tag data at +0x14)
//   types/objects.h  object (0x1f4; item_data starts immediately after it), object_header,
//                    object_type_definition, object_placement_data, damage_data, object_type,
//                    object_type_mask (_object_mask_item 0x1c, _object_mask_weapon 0x004,
//                    _object_mask_equipment 0x008, _object_mask_garbage 0x010 -- every
//                    object_try_and_get in this module passes one of those, which is how each
//                    function below was attributed to a concrete type)
//   types/units.h    unit_data (unit_update at 0x5625b0 is what pushes weapon_data.control_flags
//                    and primary_trigger into the held weapon through weapon_set_control_flags)
//   types/tags.h     Object (0x17c), Item (0x308; item_flags 0x17c, pickup_text_index 0x180,
//                    scale 0x184, item_a_in..d_in 0x19c..0x1a2, material_effects 0x248,
//                    collision_sound 0x258, detonation_delay 0x2e0, detonating_effect 0x2e8,
//                    detonation_effect 0x2f8), Equipment (0x3b0; powerup_type 0x308,
//                    grenade_type 0x30a, powerup_time 0x30c, pickup_sound 0x310),
//                    Garbage (0x3b0), Weapon (0x508; weapon_flags 0x308, label 0x30c,
//                    secondary_trigger_mode 0x32c, maximum_alternate_shots_loaded 0x32e,
//                    weapon_a_in..d_in 0x330..0x336, ready_time 0x338, ready_effect 0x33c,
//                    heat_recovery_threshold 0x34c, overheated_threshold 0x350,
//                    heat_detonation_threshold 0x354, heat_detonation_fraction 0x358,
//                    heat_loss_rate 0x35c, heat_illumination 0x360,
//                    first_person_animations 0x46c, zoom_levels 0x3da,
//                    zoom_magnification_range 0x3dc, age_heat_recovery_penalty 0x440,
//                    age_rate_of_fire_penalty 0x444, age_misfire_start 0x448,
//                    age_misfire_chance 0x44c, weapon_type 0x4e2, magazines 0x4f0,
//                    triggers 0x4fc), WeaponMagazine (0x70; flags 0x00,
//                    rounds_recharged 0x04, rounds_total_initial 0x06,
//                    rounds_reserved_maximum 0x08, rounds_loaded_maximum 0x0a,
//                    reload_time 0x14, rounds_reloaded 0x18, chamber_time 0x1c),
//                    WeaponTrigger (0x114; flags 0x00, maximum_rate_of_fire 0x04,
//                    blurred_rate_of_fire 0x14, magazine 0x20, rounds_per_shot 0x22,
//                    minimum_rounds_loaded 0x24, charging_time 0x48, charged_time 0x4c,
//                    overcharged_action 0x50, spew_time 0x58, projectile 0x94,
//                    ejection_port_recovery_time 0xa4, illumination_recovery_time 0xa8,
//                    heat_generated_per_round 0xb8, age_generated_per_round 0xbc,
//                    overload_time 0xc4, illumination_recovery_rate 0xf0,
//                    ejection_port_recovery_rate 0xf4, firing_acceleration_rate 0xf8,
//                    firing_deceleration_rate 0xfc, error_acceleration_rate 0x100,
//                    error_deceleration_rate 0x104, firing_effects 0x108),
//                    ScenarioWeapon (0x5c; rounds_reserved 0x48, rounds_loaded 0x4a,
//                    flags 0x4c), ScenarioEquipment (0x28), ModelAnimations, DamageEffect
//
// Not defined here on purpose: 22 of the 101 functions Ghidra placed in this address range
// operate on projectiles, not items. Their layout is types/projectiles.h projectile_data
// (0xbc bytes at object+0x1f4; the "projectile" row object_size is 0x2b0), which overlaps
// item_data, so mixing the two in one struct would be wrong. R49: the projectile appendix in
// out/phase4/items_types_notes.md is SUPERSEDED by projectiles.h (established from
// projectile_new 0x4bd7c0 and 0x4be1b0's absolute offsets): object+0x230 is an int16 state,
// +0x232 is material_response_index, +0x278 is a genuine field (thrown_grenade), and +0x248/+0x24c are the arming
// timer. Use projectiles.h, not that appendix. Three more "functions" (0x4c62d0, 0x4c62f0, 0x4c6340) are one shell/main routine
// that Ghidra split into three; they touch nothing in this header.
//
// Note on sizes: like types/memory.h, structs holding datum handles measure to the documented
// size under a 32-bit data organization.

#include <stddef.h> // offsetof
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum item_constants {
    k_item_data_offset = 0x1f4,          // object_type_definition "object" object_size
    k_item_object_size = 0x22c,          // object_type_definition "item" object_size
    k_weapon_object_size = 0x340,        // object_type_definition "weapon" object_size
    k_equipment_object_size = 0x294,     // object_type_definition "equipment" object_size
    k_garbage_object_size = 0x244,       // object_type_definition "garbage" object_size
    k_item_extension_offset = 0x22c,     // where weapon/equipment/garbage_data all begin

    // weapon_update, weapon_reset_triggers, weapon_set_ammo_counts and
    // item_transfer_ammunition all bound their loops by the Weapon tag own triggers
    // (0x4fc) and magazines (0x4f0) counts; the runtime arrays sized below are what those
    // counts may not exceed, fixed by weapon_data being exactly 0x114 bytes.
    k_maximum_weapon_triggers = 2,       // 0x260 + 2*0x28 lands exactly on the magazines
    k_maximum_weapon_magazines = 2,      // 0x2b0 + 2*0x0c, and item_has_active_state reads
                                         //   magazine state at 0x2b0 and 0x2bc only

    // weapon_trigger_effect_set_state counter sentinels
    k_weapon_trigger_effect_ticks_infinite = -1,  // states tracking/out-of-ammo park at -1

    // weapon_update automatic-reload nag: the trigger empty_ticks counter has to pass 10
    // before an empty client-side weapon asks the host to reload.
    k_weapon_empty_reload_delay_ticks = 10,

    // network delta message types this module encodes (object_type_definition +0x10 for the
    // two creation messages, literal constants for the rest)
    k_message_equipment_creation = 0x1f,
    k_message_weapon_creation = 0x20,
    k_message_weapon_ammo_pickup = 0x2c,
    k_message_weapon_reload_begin = 0x2b,
    k_message_weapon_reload_end = 0x2d,
    k_message_weapon_reload_cancel = 0x2e
} item_constants;

// ---------------------------------------------------------------------------
// item_flags  (the uint32 at object 0x1f4, i.e. item_data.flags)
// Bit 0x01/0x02 are set as a pair by item_set_holder (0x4bcfc0); 0x04 is owned by
// item_compute_rotation (0x4bd500); 0x08 and 0x10 are the two resting modes item_update
// (0x4bc5c0) sets when the "ground point" marker test lands on a structure surface versus on
// another object; 0x20 comes straight out of the scenario placement record.
// ---------------------------------------------------------------------------
typedef enum item_flags {
    _item_in_inventory_bit = 0x01,        // held by some unit; item_get_effective_position
                                          //   follows object 0xc0 instead of reading 0xa0
    _item_held_by_player_bit = 0x02,      // the holder unit_data.controlling_player != -1.
                                          //   weapon_update and weapon_fire_trigger gate the
                                          //   first-person/prediction paths on it
    _item_rotation_valid_bit = 0x04,      // rotation_axis / rotation_sine / rotation_cosine
                                          //   hold a real rotation; cleared when the angular
                                          //   velocity is exactly zero
    _item_at_rest_on_structure_bit = 0x08,// resting_surface_index names the surface. item_update
                                          //   clears it and re-accelerates once that surface
                                          //   reports flag 0x08 (a destroyed breakable)
    _item_at_rest_on_object_bit = 0x10,   // resting_object_index / contact_point are live;
                                          //   cleared when that object dies
    _item_does_not_accelerate_bit = 0x20, // item_accelerate returns immediately. weapon_new_from
                                          //   _placement (0x4c1350) sets it when the scenario
                                          //   record does NOT have does_accelerate (bit 2)
    _item_unknown_40_bit = 0x40           // cleared by item_set_holder and by
                                          //   equipment_pickup_play_sound (0x4bbb50); nothing
                                          //   in this module sets or tests it
} item_flags;

// ---------------------------------------------------------------------------
// item_data  (object 0x1f4 .. 0x22c, the extension shared by weapon, equipment and garbage)
// Everything below is established by item_update (0x4bc5c0), item_accelerate (0x4bd080),
// item_compute_rotation (0x4bd500), item_align_to_normal_and_point (0x4bd5d0),
// item_get_effective_position (0x4bd740), item_set_holder (0x4bcfc0),
// item_detonation_timer_start (0x4bd450) and item_any_detonating (0x4bcf50).
// ---------------------------------------------------------------------------
typedef struct item_data {
    uint32_t flags;                  // 0x1f4 item_flags
    int16_t detonation_countdown;    // 0x1f8 ticks. item_detonation_timer_start seeds it once
                                     //       from the Item tag detonation_delay; item_update
                                     //       counts it down and, at zero, plays the detonation
                                     //       effect and deletes the object. item_any_detonating
                                     //       iterates _object_mask_item looking for > 0.
    int16_t resting_surface_index;   // 0x1fa -1 when unset. Index into the collideable-surface
                                     //       table reached through 0x00746f98 + 0x40 (stride
                                     //       0x0c); item_update re-wakes the item when that
                                     //       entry byte at +0x08 has bit 0x08 set.
    int16_t resting_bsp_index;       // 0x1fc stamped from the int16 at 0x0069e8d8 when the item
                                     //       came to rest; a mismatch invalidates the rest
    int16_t pad_1fe;                 // 0x1fe never read or written by this module
    datum_index ignore_object_index; // 0x200 the object the in-flight collision test skips
                                     //       (item_update passes it to the sweep at 0x401a20,
                                     //       item_accelerate tests it against -1). item_update
                                     //       forces it to -1 the moment the item comes to rest.
    int32_t held_game_time;          // 0x204 while _item_in_inventory_bit is set, item_update
                                     //       stamps the game tick from *(int *)(0x006f1d6c+0xc)
    datum_index resting_object_index; // 0x208 the object the item is lying on
    real_point3d contact_point;      // 0x20c contact point in the resting object node space;
                                     //       item_update writes it with
                                     //       matrix4x3_inverse_transform_point against the node
                                     //       marker and reads it back with the forward transform
    real_vector3d rotation_axis;     // 0x218 DUAL USE, and the two writers are mutually
                                     //       exclusive on object flag 0x20 (at rest):
                                     //       item_compute_rotation writes the normalized object
                                     //       angular velocity here only while the object is NOT
                                     //       at rest, and item_update writes the ground normal
                                     //       here in the branch that has just set object flag
                                     //       0x20. Readers take it as the axis for
                                     //       vector3d_rotate_about_axis and as the normal for
                                     //       item_align_to_normal_and_point.
    float rotation_sine;             // 0x224 sin(|angular velocity|), 0.0 when the axis is void
    float rotation_cosine;           // 0x228 cos(|angular velocity|), 1.0 when the axis is void
} item_data;                         // size 0x38 (object 0x1f4 .. 0x22c)

// An item object as one struct: the common object header (types/objects.h) followed by item_data,
// so a field is one fixed offset from the object like the original code uses.
typedef struct item_object {
    object base;                        // 0x000
    item_data item;                     // 0x1f4
} item_object;
typedef char item_object_item_at_1f4[offsetof(item_object, item) == 0x1f4 ? 1 : -1];

// ---------------------------------------------------------------------------
// garbage_data  (object 0x22c .. 0x244)
// The garbage row vtable only fills +0x28 (0x4bc490) and +0x34 (0x4bc510), and Ghidra has
// not recovered either as a function, so no code in the exported image reads or writes this
// block. The size is the only thing the binary states.
// ---------------------------------------------------------------------------
typedef struct garbage_data {
    uint8_t unknown_22c[0x18];       // 0x22c UNRESOLVED in full; see the notes file
} garbage_data;                      // size 0x18 (object 0x22c .. 0x244)

// ---------------------------------------------------------------------------
// equipment_network_state  (one replicated snapshot, 0x24 bytes)
// equipment_build_creation_message (0x4bbc90) reads it field by field and
// equipment_create_from_creation_message (0x4bbe20) writes it and then copies velocity and
// angular_velocity straight into object 0x68 and object 0x8c.
// ---------------------------------------------------------------------------
typedef struct equipment_network_state {
    real_point3d position;           // 0x00
    real_vector3d velocity;          // 0x0c
    real_vector3d angular_velocity;  // 0x18
} equipment_network_state;           // size 0x24

// ---------------------------------------------------------------------------
// equipment_data  (object 0x22c .. 0x294)
// Only the network delta block is reachable from this module: the equipment vtable
// query/create column (0x4bba90), notify column (0x4bbae0), baseline builder (0x4bbc30),
// creation decoder (0x4bc070) and update applier (0x4bc250) were not recovered as functions
// by Ghidra, so 0x22c..0x244 has no reader anywhere in the export.
// ---------------------------------------------------------------------------
typedef struct equipment_data {
    uint8_t unknown_22c[0x18];       // 0x22c UNRESOLVED in full; see the notes file
    uint8_t network_state_valid;     // 0x244 equipment_create_from_creation_message sets 1
    uint8_t network_baseline_index;  // 0x245 carried in both the creation and the update
                                     //       message and compared before an update is taken
    uint8_t network_sequence;         // 0x246 incremented by
                                     //       equipment_build_network_update (0x4bc0f0) on every
                                     //       successful encode and wrapped from 0xff to 0
    uint8_t pad_247;                  // 0x247
    equipment_network_state network_state;      // 0x248 the state the host is replicating
    uint8_t last_update_valid;                  // 0x26c UNSURE
    uint8_t pad_26d[3];                         // 0x26d UNSURE
    equipment_network_state last_update_state;  // 0x270 UNSURE: the 0x26c..0x294 tail is laid
                                     //       out by analogy with weapon_data 0x310..0x340 and
                                     //       projectile 0x294..0x2b0, both of which store the
                                     //       last applied update as {uint8 valid, pad, state}
                                     //       and both of which end exactly on the type
                                     //       object_size the way this does. No recovered
                                     //       function touches it, because the equipment update
                                     //       applier (0x4bc250) is one of the Ghidra misses.
} equipment_data;                    // size 0x68 (object 0x22c .. 0x294)

// ---------------------------------------------------------------------------
// weapon runtime flags  (the uint32 at object 0x22c, i.e. weapon_data.flags)
// ---------------------------------------------------------------------------
typedef enum weapon_flags {
    _weapon_overheated_bit = 0x01,        // weapon_update sets it when heat reaches the tag
                                          //   overheated_threshold and clears bits 0x01|0x02
                                          //   together once heat falls under
                                          //   heat_recovery_threshold
    _weapon_overheat_cue_played_bit = 0x02,// set one tick after overheating, so the cue at
                                          //   0x450b20 fires once
    _weapon_alternate_shot_armed_bit = 0x04,// weapon_fire_trigger sets it when the secondary
                                          //   trigger of a weapon_type 3 weapon fires;
                                          //   weapon_update consumes and clears it on overheat
    _weapon_ammo_prediction_pending_bit = 0x08 // weapon_predict_ammo (0x4c3530) sets it after
                                          //   writing predicted_rounds_*; the server
                                          //   corrections at 0x4c3870 / 0x4c4ac0 and
                                          //   weapon_trigger_begin_reload clear it
} weapon_flags;

// ---------------------------------------------------------------------------
// weapon_control_flags  (the uint16 at object 0x230, i.e. weapon_data.control_flags)
// unit_update (0x5625b0) rebuilds this word every tick from its own unit_control_flags and
// hands it, together with the analog trigger, to weapon_set_control_flags (0x4c2990 -- the
// function Ghidra currently calls item_set_permutation; see the notes file). A weapon that is
// not the unit current weapon gets exactly _weapon_control_not_current_bit and nothing else.
// ---------------------------------------------------------------------------
typedef enum weapon_control_flags {
    _weapon_control_unknown_01_bit = 0x01,  // from unit control flag 0x10 plus a caller predicate
    _weapon_control_primary_trigger_bit = 0x02,  // unit control flag 0x800
    _weapon_control_secondary_trigger_bit = 0x04,// unit control flag 0x1000; only reaches the
                                          //   trigger when the Weapon tag has
                                          //   secondary_trigger_overrides_grenades (0x1000)
    _weapon_control_unknown_08_bit = 0x08,  // unit control flag 0x400
    _weapon_control_inhibited_bit = 0x10,   // weapon_update ignores every trigger while it is
                                          //   set, and trigger effect state 0 will not
                                          //   auto-reload
    _weapon_control_not_current_bit = 0x20, // the only bit set when the unit is holding a
                                          //   different weapon
    _weapon_control_unknown_40_bit = 0x40
} weapon_control_flags;

// ---------------------------------------------------------------------------
// weapon_state  (the int8 at object 0x238, i.e. weapon_data.state)
// weapon_set_state (0x4c5670) is the only writer. Its switch maps the state onto the index of
// an animation in the weapon own animation graph, and that mapping is what names the states:
//   0 -> 0, 1 -> 9, 2 -> 10, 3 -> 5, 4 -> 6, 5 and 6 -> 3, 7 and 8 -> 8, 9 -> 1, 10 -> 2.
// weapon_force_settled_state (0x4c5630) treats 7, 8 and 10 as the states that may persist and
// forces anything else back to idle; weapon_prevents_grenade_throwing (0x4c2f30) treats 5..10
// as "busy".
// ---------------------------------------------------------------------------
typedef enum weapon_state {
    _weapon_state_idle = 0,
    _weapon_state_fire_primary = 1,       // weapon_fire_trigger, trigger 0
    _weapon_state_fire_secondary = 2,     // weapon_fire_trigger, trigger 1
    _weapon_state_chamber_primary = 3,    // weapon_magazine_begin_chamber (0x4c3b00)
    _weapon_state_chamber_secondary = 4,
    _weapon_state_reload_primary = 5,     // weapon_trigger_begin_reload
    _weapon_state_reload_secondary = 6,
    _weapon_state_charged_primary = 7,    // weapon_trigger_become_charged (0x4c3bc0)
    _weapon_state_charged_secondary = 8,
    _weapon_state_ready = 9,              // weapon_ready (0x4c2840)
    _weapon_state_put_away = 10           // weapon_put_away (0x4c28f0)
} weapon_state;

// ---------------------------------------------------------------------------
// weapon_trigger_effect_state  (the int8 at weapon_trigger_state 0x01)
// The state byte and its tick counter are always written as a pair, by
// weapon_trigger_effect_set_state (0x4c49c0) or by one of the small setters below.
// the weapon_update per-trigger switch is the whole state machine.
// ---------------------------------------------------------------------------
typedef enum weapon_trigger_effect_state {
    _weapon_trigger_effect_idle = 0,        // weapon_trigger_effect_clear (0x4c3e40 / 0x4c48f0)
    _weapon_trigger_effect_overloading = 1, // entered when the tag trigger has overload_time
                                            //   (0xc4) but no charging_time; the burst
                                            //   continuation at 0x4c3c60 re-enters it
    _weapon_trigger_effect_charging = 2,    // entered when the tag trigger has charging_time
                                            //   (0x48); the counter runs that time down
    _weapon_trigger_effect_charged = 3,     // 0x4c3bc0. weapon_update divides the remaining
                                            //   counter by the tag charged_time (0x4c) into
                                            //   weapon_data.charged_fraction
    _weapon_trigger_effect_locked = 4,      // UNSURE: only weapon_update reads it, and only to
                                            //   drop to out-of-ammo or idle
    _weapon_trigger_effect_tracking = 5,    // weapon_fire_trigger parks here (counter -1) when
                                            //   the tag trigger has flag 0x01; held while
                                            //   weapon_data.tracked_object_index is live and
                                            //   released by 0x4c3eb0
    _weapon_trigger_effect_spewing = 6,     // 0x4c3d00 when the tag trigger has spew_time (0x58)
    _weapon_trigger_effect_out_of_ammo = 7, // 0x4c3e70, counter -1
    _weapon_trigger_effect_reset = 8        // weapon_reset_triggers (0x4c4b50), counter 0, so
                                            //   the next weapon_update drops it to idle
} weapon_trigger_effect_state;

// ---------------------------------------------------------------------------
// weapon_trigger_state_flags  (the uint32 at weapon_trigger_state 0x04)
// ---------------------------------------------------------------------------
typedef enum weapon_trigger_state_flags {
    _weapon_trigger_not_pulled_bit = 0x01,  // weapon_update sets it on any tick the trigger is
                                            //   not down; weapon_fire_trigger clears it
    _weapon_trigger_was_pulled_bit = 0x02,  // previous tick pull, for the edge detector the
                                            //   tag trigger flag 0x10 (latching) needs
    _weapon_trigger_latched_bit = 0x04,     // the latch itself, toggled on each rising edge
    _weapon_trigger_blur_applied_bit = 0x10,// the "~primary-blur"/"~secondary-blur" permutation
                                            //   at 0x006961b8 is currently applied
    _weapon_trigger_charge_effect_bit = 0x20// a charging effect is playing for this trigger
} weapon_trigger_state_flags;

// ---------------------------------------------------------------------------
// weapon_trigger_state  (weapon_data.triggers[i], object 0x260 + i*0x28)
// The stride is fixed twice over: every accessor spells the base as
// (uint32 *)object + trigger_index*10 + 0x98 (weapon_update, weapon_fire_trigger,
// trigger_create_projectiles, weapon_trigger_ready_to_fire) or as
// (uint8 *)object + trigger_index*0x28 + 0x260 (all the small state setters), and
// item_has_active_state reads effect_state for both triggers at 0x261 and 0x289.
// ---------------------------------------------------------------------------
typedef struct weapon_trigger_state {
    int8_t idle_ticks;               // 0x00 ticks since the last shot, saturating at 0x7f.
                                     //      weapon_trigger_ready_to_fire (0x4c3190) compares
                                     //      idle_ticks + 1 against 30 / rate_of_fire
    int8_t effect_state;             // 0x01 weapon_trigger_effect_state
    int16_t effect_state_ticks;      // 0x02 decremented once per tick while nonzero; -1 parks
                                     //      the state forever
    uint32_t flags;                  // 0x04 weapon_trigger_state_flags
    uint16_t firing_effect_used_mask;// 0x08 one bit per tag firing effect already used; reset to
                                     //      0 once every effect has fired
    uint16_t firing_effect_index;    // 0x0a the firing effect (barrel) the next round uses;
                                     //      randomized when the tag trigger has flag 0x02
    int16_t firing_effect_rounds;    // 0x0c rounds left before the firing effect is re-picked,
                                     //      seeded from the effect record int16 at +0x10 and
                                     //      decremented by weapon_fire_trigger
    int16_t pad_0e;                  // 0x0e never read or written by this module
    float firing_rate;               // 0x10 0..1 spin-up. Climbs by the tag trigger
                                     //      firing_acceleration_rate (0xf8) while pulled and
                                     //      falls by firing_deceleration_rate (0xfc);
                                     //      weapon_trigger_ready_to_fire interpolates
                                     //      maximum_rate_of_fire[0..1] with it, and crossing the
                                     //      tag blurred_rate_of_fire (0x14) swaps the blur
                                     //      permutation named at 0x006961b8
    float ejection_port_recovery;    // 0x14 set to 1.0 by weapon_fire_trigger when the tag
                                     //      trigger has ejection_port_recovery_time (0xa4), and
                                     //      decayed by ejection_port_recovery_rate (0xf4)
    float illumination_recovery;     // 0x18 set to 1.0 when the tag trigger has
                                     //      illumination_recovery_time (0xa8), decayed by
                                     //      illumination_recovery_rate (0xf0)
    float error;                     // 0x1c 0..1 accuracy bloom. Climbs by
                                     //      error_acceleration_rate (0x100) while the trigger is
                                     //      pulled or the effect state is spewing/locked, and
                                     //      falls by error_deceleration_rate (0x104);
                                     //      trigger_create_projectiles reads it for the spread
    datum_index effect_handle;       // 0x20 the looping sound or particle system
                                     //      weapon_play_trigger_tag_effect started for this
                                     //      trigger, -1 when none
    int8_t empty_ticks;              // 0x24 counts the ticks an empty client-side weapon has
                                     //      waited; at 11 it asks the host to reload and resets
    uint8_t pad_25[3];           // 0x25 never read or written by this module
} weapon_trigger_state;              // size 0x28

// ---------------------------------------------------------------------------
// weapon_magazine_state_enum  (the int16 at weapon_magazine_state 0x00)
// The cycle is idle -> reloading (reload_time) -> chamber_pending -> chambering
// (chamber_time) -> idle, driven by the weapon_update per-magazine loop.
// ---------------------------------------------------------------------------
typedef enum weapon_magazine_state_enum {
    _weapon_magazine_idle = 0,
    _weapon_magazine_reloading = 1,        // weapon_trigger_begin_reload (0x4c35b0)
    _weapon_magazine_chamber_pending = 2,  // the reload step finished (0x4c3900 / 0x4c3a20)
    _weapon_magazine_chambering = 3        // weapon_magazine_begin_chamber (0x4c3b00)
} weapon_magazine_state_enum;

// ---------------------------------------------------------------------------
// weapon_magazine_state  (weapon_data.magazines[i], object 0x2b0 + i*0x0c)
// The stride is fixed by item_add_ammunition (0x4c25a0), which indexes
// object + 0x2b6 + magazine_index*0x0c and returns object + 0x2b0 + magazine_index*0x0c, and
// by weapon_set_ammo_counts, which walks 0x2b6 and 0x2b8 with the same stride.
// ---------------------------------------------------------------------------
typedef struct weapon_magazine_state {
    int16_t state;                   // 0x00 weapon_magazine_state_enum
    int16_t state_ticks;             // 0x02 ticks remaining in the current state
    int16_t state_ticks_total;       // 0x04 what state_ticks started at, so the HUD can show a
                                     //      fraction; weapon_reset_triggers compares
                                     //      state_ticks*2 against a fresh animation length
    int16_t rounds_unloaded;         // 0x06 reserve, clamped to the tag magazine
                                     //      rounds_reserved_maximum (0x08). item_add_ammunition
                                     //      and item_transfer_ammunition move this number.
    int16_t rounds_loaded;           // 0x08 in the magazine, clamped to the tag magazine
                                     //      rounds_loaded_maximum (0x0a)
    int16_t rounds_recharged_accumulator; // 0x0a the fractional part of the per-tick battery
                                     //      recharge. weapon_update (0x4c1530) adds the tag
                                     //      magazine rounds_recharged % 30 here every tick and
                                     //      moves one whole round into rounds_loaded whenever it
                                     //      passes 29, so a magazine whose rounds_recharged is
                                     //      below 30 still refills, one round every
                                     //      30/rounds_recharged ticks. Nothing else reads it.
} weapon_magazine_state;             // size 0x0c

// ---------------------------------------------------------------------------
// weapon_network_state  (one replicated snapshot, 0x2c bytes)
// weapon_apply_network_update (0x4c6070) block-copies exactly 11 dwords out of 0x2e4 and back,
// which is what fixes the size; weapon_build_creation_message (0x4c5a50) and
// weapon_create_from_creation_message (0x4c5c10) name the fields that are actually carried.
// ---------------------------------------------------------------------------
typedef struct weapon_network_state {
    real_point3d position;           // 0x00
    real_vector3d velocity;          // 0x0c
    uint8_t unknown_18[0x0c];        // 0x18 inside the block copy but never read or written by
                                     //      name; the equipment record has angular_velocity in
                                     //      the same slot, so this is very likely a vestigial
                                     //      angular_velocity. UNRESOLVED.
    int16_t rounds_unloaded[2];      // 0x24 per magazine; applied to
                                     //      weapon_magazine_state.rounds_unloaded unless that
                                     //      magazine is mid-reload
    float age;                       // 0x28 battery level complement, applied to weapon_data.age
} weapon_network_state;              // size 0x2c

// ---------------------------------------------------------------------------
// weapon_data  (object 0x22c .. 0x340)
// Established by weapon_update (0x4c1530 -- the item row and weapon row of
// object_type_definition prove this is the weapon type update column, not an item one; see
// the notes file), weapon_fire_trigger, trigger_create_projectiles,
// weapon_trigger_begin_reload, weapon_set_state, weapon_set_ammo_counts,
// weapon_set_loaded_ammo_fraction, weapon_new_from_placement (0x4c1350),
// item_has_active_state, and the network trio 0x4c5a50 / 0x4c5c10 / 0x4c5f10 / 0x4c6070.
// ---------------------------------------------------------------------------
typedef struct weapon_data {
    uint32_t flags;                  // 0x22c weapon_flags
    uint16_t control_flags;          // 0x230 weapon_control_flags, refreshed every tick by
                                     //       unit_update through weapon_set_control_flags
    uint16_t control_flags_high;            // 0x232 the high half of the dword weapon_set_control_flags
                                     //       writes; never read
    float primary_trigger;           // 0x234 0..1 analog pull, also from
                                     //       weapon_set_control_flags. A tag trigger with flag
                                     //       0x200 uses this instead of its own firing_rate, and
                                     //       treats > 0.05 as "pulled"
    int8_t state;                    // 0x238 weapon_state
    int8_t pad_239;              // 0x239 never read or written by this module
    int16_t action_ticks;            // 0x23a ticks left in the ready/put-away animation, seeded
                                     //       by weapon_ready from
                                     //       weapon_get_first_person_animation_time; while it is
                                     //       positive weapon_update ignores the triggers
    float heat;                      // 0x23c 0..1. Gains the tag trigger
                                     //       heat_generated_per_round per shot and loses
                                     //       heat_loss_rate/30 per tick, scaled by
                                     //       age_heat_recovery_penalty
    float age;                       // 0x240 0..1. Gains age_generated_per_round per shot;
                                     //       weapon_set_loaded_ammo_fraction writes 1 - fraction
                                     //       here for battery weapons, and it is the one weapon
                                     //       field carried in the network state
    float charged_fraction;          // 0x244 while a trigger is charged, weapon_update stores
                                     //       1 - ticks/(30*charged_time) here; heat decay is
                                     //       suppressed on any tick this is nonzero
    float ready_timer;               // 0x248 counts down by 1/24 per tick. UNSURE: the decrement
                                     //       is skipped, and unit_update writes this field
                                     //       directly through 0x4c2b20, when the holder Object
                                     //       tag has flag 0x800000
    uint32_t unknown_24c;            // 0x24c never read or written by this module
    datum_index tracked_object_index;// 0x250 scrubbed to -1 by weapon_update when the object it
                                     //       names has died, and required by trigger effect
                                     //       state 5; 0x4c3eb0 clears it
    uint32_t unknown_254;            // 0x254 never read or written by this module
    uint32_t unknown_258;            // 0x258 never read or written by this module
    int16_t alternate_shots_loaded;  // 0x25c incremented by weapon_fire_trigger for a
                                     //       secondary_trigger_mode of 3 or 4, and bounded by
                                     //       the tag maximum_alternate_shots_loaded (0x32e)
    int16_t pad_25e;             // 0x25e never read or written by this module
    weapon_trigger_state triggers[2];    // 0x260 .. 0x2b0
    weapon_magazine_state magazines[2];  // 0x2b0 .. 0x2c8
    uint32_t unknown_2c8;            // 0x2c8 never read or written by this module
    datum_index overheat_effect_handle;  // 0x2cc the looping effect or particle system started
                                     //       when the weapon overheated; weapon_put_away deletes
                                     //       the particle system it names and resets it to -1
    int32_t last_fire_game_time;     // 0x2d0 weapon_fire_trigger stamps the game tick from
                                     //       *(int *)(0x006f1d6c + 0x0c)
    int16_t predicted_rounds_unloaded[2];// 0x2d4 client-side prediction written by
                                     //       weapon_predict_ammo (0x4c3530);
                                     //       weapon_trigger_begin_reload copies both arrays into
                                     //       the magazine states when it runs on a client
    int16_t predicted_rounds_loaded[2];  // 0x2d8
    uint32_t unknown_2dc;            // 0x2dc never read or written by this module
    uint8_t network_state_valid;     // 0x2e0 weapon_create_from_creation_message sets 1
    uint8_t network_baseline_index;  // 0x2e1 carried in the creation and update messages and
                                     //       compared before an update is taken
    uint8_t network_sequence;        // 0x2e2 incremented by weapon_build_network_update
                                     //       (0x4c5f10) per successful encode, wrapping 0xff->0
    uint8_t pad_2e3;                 // 0x2e3
    weapon_network_state network_state;     // 0x2e4 .. 0x310 the state being replicated
    uint8_t last_update_valid;              // 0x310 set by weapon_apply_network_update
    uint8_t pad_311[3];                     // 0x311
    weapon_network_state last_update_state; // 0x314 .. 0x340 a verbatim copy of the last update
                                     //       that was accepted, kept for interpolation
} weapon_data;                       // size 0x114 (object 0x22c .. 0x340)

// Weapon and equipment objects as one struct each: object header, item_data, then the type's own data.
typedef struct weapon_object {
    object base;                        // 0x000
    item_data item;                     // 0x1f4
    weapon_data weapon;                 // 0x22c
} weapon_object;
typedef struct equipment_object {
    object base;                        // 0x000
    item_data item;                     // 0x1f4
    equipment_data equipment;           // 0x22c
} equipment_object;
typedef char weapon_object_weapon_at_22c[offsetof(weapon_object, weapon) == 0x22c ? 1 : -1];
typedef char equipment_object_equipment_at_22c[offsetof(equipment_object, equipment) == 0x22c ? 1 : -1];

// ---------------------------------------------------------------------------
// weapon_hud_ammo_state  (weapon_build_hud_ammo_state, 0x4c29d0)
// The caller-supplied output record. The per-magazine rows are spelled with an explicit
// stride of 10 bytes off a base of 0x0c -- the code writes
// out + magazine_index*10 + 0x0c .. + 0x14 -- and the six fields below add up to exactly 10,
// so the nested struct reproduces the stride without any padding.
// ---------------------------------------------------------------------------
typedef struct weapon_hud_magazine_state {
    uint8_t reloading;               // 0x00 magazine state is reloading (1) or chambering (3)
    uint8_t idle;                    // 0x01 magazine state is idle (0)
    int16_t rounds_loaded;           // 0x02 weapon_magazine_state.rounds_loaded
    int16_t rounds_loaded_maximum;   // 0x04 the tag magazine field at 0x0a
    int16_t rounds_unloaded;         // 0x06 weapon_magazine_state.rounds_unloaded
    int16_t rounds_reserved_maximum; // 0x08 the tag magazine field at 0x08
} weapon_hud_magazine_state;         // size 0x0a

typedef struct weapon_hud_ammo_state {
    float heat;                      // 0x00 weapon_data.heat
    float age;                       // 0x04 weapon_data.age
    uint8_t overheated;              // 0x08 weapon_flags & _weapon_overheated_bit
    uint8_t pad_09;                  // 0x09
    int16_t magazine_count;          // 0x0a low half of the Weapon tag magazines count
    weapon_hud_magazine_state magazines[2]; // 0x0c .. 0x20
} weapon_hud_ammo_state;             // size 0x20

// ---------------------------------------------------------------------------
// equipment_creation_message  (network delta message type 0x1f)
// Built on the stack by equipment_build_creation_message (0x4bbc90) and consumed by
// equipment_create_from_creation_message (0x4bbe20). The three object handles travel as
// hash_table_get results, not as raw datum indices.
// ---------------------------------------------------------------------------
typedef struct equipment_creation_message {
    datum_index definition_tag;      // 0x00 object 0x000
    int32_t object_hash;             // 0x04 hash_table_get of the item itself
    int16_t name_index;              // 0x08 object 0x0b8
    int16_t pad_0a;                  // 0x0a
    int32_t owner_hash;              // 0x0c hash_table_get of object 0x0c0
    int32_t parent_hash;             // 0x10 hash_table_get of object 0x0c4
    uint32_t object_flags;           // 0x14 OR-ed into object 0x010 by the receiver
    real_point3d position;           // 0x18 equipment_data.network_state.position
    real_vector3d forward;           // 0x24 object 0x074
    real_vector3d up;                // 0x30 object 0x080
    real_vector3d velocity;          // 0x3c equipment_data.network_state.velocity
    real_vector3d angular_velocity;  // 0x48 equipment_data.network_state.angular_velocity
    uint8_t baseline_index;          // 0x54 equipment_data.network_baseline_index
    uint8_t pad_55[3];               // 0x55
} equipment_creation_message;        // size 0x58

// ---------------------------------------------------------------------------
// weapon_creation_message  (network delta message type 0x20)
// Built by weapon_build_creation_message (0x4c5a50) and consumed by
// weapon_create_from_creation_message (0x4c5c10).
// ---------------------------------------------------------------------------
typedef struct weapon_creation_message {
    datum_index definition_tag;      // 0x00 object 0x000
    int32_t object_hash;             // 0x04
    int16_t name_index;              // 0x08 object 0x0b8
    int16_t pad_0a;                  // 0x0a
    int32_t owner_hash;              // 0x0c hash_table_get of object 0x0c0
    int32_t parent_hash;             // 0x10 hash_table_get of object 0x0c4
    uint32_t object_flags;           // 0x14 OR-ed into object 0x010 by the receiver
    real_point3d position;           // 0x18 weapon_data.network_state.position
    real_vector3d forward;           // 0x24 object 0x074
    real_vector3d up;                // 0x30 object 0x080
    real_vector3d velocity;          // 0x3c weapon_data.network_state.velocity
    uint8_t baseline_index;          // 0x48 weapon_data.network_baseline_index
    uint8_t pad_49;                  // 0x49
    int16_t rounds_unloaded[2];      // 0x4a copied into weapon_data.network_state and from
                                     //      there into the magazine states
    uint8_t pad_4e[2];               // 0x4e alignment before the float
    float age;                       // 0x50 copied into weapon_data.age
    int16_t rounds_loaded[2];        // 0x54 written straight into the magazine states
} weapon_creation_message;           // size 0x58

// ---------------------------------------------------------------------------
// weapon_magazine_ammo_message  (network delta message types 0x2b, 0x2d and 0x2e)
// One record shape, three messages: 0x2b when a reload starts (0x4c3470), 0x2d when a reload
// step completes (0x4c37b0) and 0x2e when the host finishes or cancels one (0x4c4a00). The
// receivers are weapon_predict_ammo (0x4c3530), weapon_apply_ammo_correction (0x4c3870) and
// weapon_apply_ammo_correction_and_resync (0x4c4ac0).
// ---------------------------------------------------------------------------
typedef struct weapon_magazine_ammo_message {
    int32_t object_hash;             // 0x00 hash_table_get of the weapon
    int16_t magazine_index;          // 0x04
    int16_t rounds_unloaded;         // 0x06 weapon_magazine_state.rounds_unloaded
    int16_t rounds_loaded;           // 0x08 weapon_magazine_state.rounds_loaded
} weapon_magazine_ammo_message;      // size 0x0a

// ---------------------------------------------------------------------------
// weapon_ammo_pickup_message  (network delta message type 0x2c, 0x4c2510)
// Posted by item_transfer_ammunition when rounds move out of one item magazine.
// ---------------------------------------------------------------------------
typedef struct weapon_ammo_pickup_message {
    int32_t object_hash;             // 0x00 hash_table_get of the weapon
    int16_t magazine_index;          // 0x04
    int16_t rounds;                  // 0x06
} weapon_ammo_pickup_message;        // size 0x08

// ---------------------------------------------------------------------------
// weapon_network_update_header  (network delta message type 0x21, the per-tick weapon update)
// Not owned by this module: it is the networking module's per-update header sub-record, reached
// through update_record[0x11]. Only the four bytes weapon_apply_network_update (0x4c6070) and
// weapon_build_network_update (0x4c5f10) touch are named; the record is longer than this.
// Folded here out of src/items/weapon_apply_network_update.c so the whole module shares one
// spelling -- move it to a types/network.h when that module is written.
// ---------------------------------------------------------------------------
typedef struct weapon_network_update_header {
    uint8_t unknown_00[4];           // 0x00
    uint8_t baseline_index;          // 0x04 must match weapon_data.network_baseline_index or the
                                     //      update is for a baseline this weapon never saw
    uint8_t sequence;                // 0x05 wrapping 0..0xff; an update whose sequence is not
                                     //      newer by less than 0x1e is dropped
    uint8_t force_baseline;          // 0x06 nonzero also refreshes network_baseline_index /
                                     //      network_state and forces the world relink
} weapon_network_update_header;      // at least 0x07

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// global 0x006961b8: char *weapon_blur_permutation_names[2]
//     Read out of .data as {"~primary-blur", "~secondary-blur"} (the pointers are 0x0066b1e0
//     and 0x0066b1d0). weapon_update indexes it by trigger index and passes the name to
//     object_set_permutation_by_name when weapon_trigger_state.firing_rate crosses the tag
//     trigger blurred_rate_of_fire.
// global 0x00696550: float weapon_network_update_position_tolerance   // 0.5 world units;
//     weapon_apply_network_update only relinks the object when the incoming position moved
//     further than this
// global 0x0087abc9: uint8_t weapon_infinite_ammo    // read only by weapon_fire_trigger, which
//     skips both the round deduction and the heat gain while it is set
// global 0x006894c0: uint8_t weapon_client_side_projectiles  // read only by weapon_fire_trigger,
//     selecting network role 3 for a tag trigger with flag 0x2000. UNSURE of the name.
// global 0x00672ea0: float k_weapon_zoom_fov_maximum  // 3.1101768, the upper clamp in 0x4c2e50
// global 0x00672ea4: float k_weapon_zoom_fov_minimum  // 0.031415928, the lower clamp
//
// Globals the module reads but does not own, listed because every accessor in this header
// depends on them:
//   0x008603b0  data_array *object_data          // owned by types/objects.h
//   0x0087bc14  tag_instance *tag_instances      // owned by types/cache.h
//   0x0069bfdc  object_type_definition *object_type_definitions[12]
//   0x0069e8d8  int16_t structure_bsp_index      // stamped into item_data.resting_bsp_index
//   0x00746f98  void *collision_bsp_globals      // +0x40 is the surface table
//                                                //   item_data.resting_surface_index indexes
//   0x006f1d6c  void *game_time_globals          // +0x0c is the game tick
//   0x006f1d20  game_engine_definition *current_game_engine  // game.h (R04); non-NULL = a
//                                                // multiplayer engine is loaded
//   0x00719720  int32_t network_game_mode        // 0 local, 1 client, 2 host
//   0x0071c419  uint8_t weapons_frozen           // weapon_update returns immediately when set
//   0x0087abc2  uint8_t weapon_bottomless_clip   // shared with the unit grenade code
//   0x00719cd0  uint32_t object_random_seed      // owned by types/objects.h
//   0x0065512c  char k_empty_string[1]           // weapon_get_label (0x4c24d0) returns it for
//                                                //   an invalid weapon

#pragma pack(pop)
