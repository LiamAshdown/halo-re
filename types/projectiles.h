#pragma once
// Blam projectiles module (halo.exe 1.0.10 retail, 0x4bda60..0x4c1070, 22 Ghidra functions).
// The projectile branch of the object hierarchy: the projectile_data extension every
// projectile object carries on top of the common object record, the per-tick ballistic
// integrator and its collision-response state machine, the detonation / arming timers, the
// velocity-decay ("damage range") bookkeeping, and the four network records projectiles
// replicate.
//
// Offsets in comments are byte offsets from the OBJECT base (the same numbering the
// decompiled module uses) unless a struct says otherwise. Where the binary itself carries the
// layout it is used in preference to the decompiler, and the fact is called out:
//
//   - The object_type_definition row at 0x0069b9a0 fixes the size of projectile_data. Read out
//     of .data (PE image base 0x400000, .data VA 0x676000 -> file offset 0x276000) the row is
//       name              "projectile"
//       group             'proj'   (0x70726f6a, the field types/objects.h calls "category")
//       object_size       0x2b0
//       0x0a / 0x0c / 0x0e  -1 / -1 / -1     no scenario placement block for projectiles
//       0x10              1                  the network delta message index
//       subdefinitions[]  { object, projectile }
//     The chain is two deep, so projectile_data starts at object+0x1f4 (the "object" row
//     object_size) and is exactly 0x2b0 - 0x1f4 = 0xbc bytes. It therefore OVERLAPS item_data,
//     which is why these fields are not in types/items.h.
//
//   - The static ProjectileMaterialResponse at 0x00695e20 (the fallback record every function
//     below substitutes for an out-of-range material index) independently confirms the whole
//     tag block: it is 0xa0 zero bytes except for three TagDependency triples
//     {'effe', &k_empty_string, 0, -1} at +0x04, +0x3c and +0x68, which is exactly
//     default_effect / potential_effect / detonation_effect in types/tags.h. The consumers
//     read the tag_id of each at +0x10, +0x48 and +0x74, and index the block with stride 0xa0.
//
//   - Every tag offset the module touches lands on a named Projectile field, which is what
//     makes the "projectile, not item" attribution airtight (Object base is 0x17c):
//       0x17c projectile_flags   bit 0x04 detonation_max_time_if_attached, 0x08
//                                has_super_combining_explosion, 0x20
//                                random_attached_detonation_time, 0x40
//                                minimum_unattached_detonation_time, 0x01
//                                oriented_along_velocity, 0x02 ai_must_use_ballistic_aiming
//       0x180 detonation_timer_starts     0x182 impact_noise
//       0x184..0x18a projectile_a_in..d_in  0x198 super_detonation.tag_id
//       0x1a0 collision_radius   0x1a4 arming_time   0x1b8 effect.tag_id
//       0x1bc/0x1c0 timer[2]     0x1c4 minimum_velocity   0x1c8 maximum_range
//       0x1cc air_gravity_scale  0x1d0/0x1d4 air_damage_range[2]
//       0x1d8 water_gravity_scale 0x1dc/0x1e0 water_damage_range[2]
//       0x1e4 initial_velocity   0x1e8 final_velocity   0x1ec guided_angular_velocity
//       0x200 detonation_started.tag_id   0x210 flyby_sound.tag_id
//       0x220 attached_detonation_damage.tag_id   0x230 impact_damage.tag_id
//       0x240/0x244 projectile_material_response reflexive (count / pointer)
//     None of 0x1a0, 0x1bc, 0x1c8, 0x1cc, 0x1e4, 0x1e8, 0x1ec, 0x200, 0x210 exists in the Item
//     tag, and the Item offsets item_update really reads (0x248 material_effects, 0x258
//     collision_sound, 0x2e0 detonation_delay) are past the end of the Projectile tag (0x24c).
//
//   - The function table on the projectile row recovered five entries Ghidra never created, and
//     they are where most of this header comes from. Column -> address:
//       +0x28 projectile_new                 0x4bd7c0   (the whole initial state)
//       +0x34 projectile_update              0x4bdc00
//       +0x38 projectile_update_function_values 0x4c0250
//       +0x3c projectile_notify_object_deleted 0x4bf0c0
//       +0x44 projectile_force_detonate      0x4c0ac0
//       +0x64 projectile_send_creation       0x4c0b10
//       +0x68 projectile_network_baseline_take 0x4c0ed0
//       +0x6c projectile_build_network_update 0x4c0f30
//       +0x70 projectile_apply_network_update 0x4c1070
//       +0x74 projectile_is_old_enough       0x4c1270
//     (+0x14..0x20 and +0x30 are the shared no-op 0x0044ad80, +0x60 the shared
//      "return false" 0x00571dd0 and +0x78 the shared "return true" 0x00572a80.)
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   datum_index, data_array
//   types/math.h     real_point3d, real_vector3d, and the shared constant vectors
//                    (0x00696714 origin, 0x0069671c left, 0x00696720 up, 0x0069672c "down",
//                    0x006b7af4 sphere_point_table, 0x00719cd0 random_seed_global)
//   types/cache.h    tag_instance (0x0087bc14, tag data at +0x14)
//   types/objects.h  object (0x1f4; projectile_data starts immediately after it), object_header,
//                    object_type_definition, object_type, object_type_mask
//                    (_object_mask_projectile 0x020 -- the mask every object_try_and_get in this
//                    module passes, which is how each function was attributed),
//                    bsp_leaf_reference (0x08, reused inside collision_result), damage_data
//   types/items.h    item_data (the extension projectile_data overlaps; do not mix them)
//   types/tags.h     Object (0x17c), Projectile (0x24c), ProjectileMaterialResponse (0xa0),
//                    ProjectileFlags, ProjectileResponse (disappear 0, detonate 1, reflect 2,
//                    overpenetrate 3, attach 4), ProjectileDetonationTimerStarts,
//                    ProjectileFunctionIn, ProjectileScaleEffectsBy, ObjectNoise, DamageEffect
//
// Misattributed in this range, and therefore not typed here:
//   0x4be1b0 "resolution_list_add_resolution" is NOT a function. It is the mid-body loop of
//            projectile_update (0x4bdc00); both decompilations contain the same LAB_004be5ad /
//            LAB_004be5c1 labels and 0x4be1b0 has zero callers. Same failure mode as the
//            0x4c62d0 trio in types/items.h.
//   0x4beb30, 0x4bee20, 0x4beec0 are the AI ballistic-aiming helpers. They take a *Projectile
//            tag pointer* and plain vectors, never an object, and belong with the AI aiming
//            code rather than with projectile_data. 0x4beec0 picks the swept/gravity solver
//            0x4beb30 when the tag has ai_must_use_ballistic_aiming and a positive
//            air_gravity_scale, otherwise the straight-line solver 0x4bee20.
//   0x4bef80 touches only object fields (velocity 0x68, angular_velocity 0x8c, object flag
//            0x20) plus 0x4c0180. It owns nothing in this header.
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
typedef enum projectile_constants {
    k_projectile_data_offset = 0x1f4,     // object_type_definition "object" object_size
    k_projectile_object_size = 0x2b0,     // object_type_definition "projectile" object_size

    // object_type_definition 0x0069b9a0 +0x10: what projectile_build_network_update (0x4c0f30)
    // passes to message_delta_encode_message as the message index. The three creation /
    // notification messages below are encoded as literals instead.
    k_projectile_network_delta_index = 1,
    k_message_projectile_creation = 0x1e,   // projectile_send_creation  0x4c0b10
    k_message_projectile_detonation = 0x30, // projectile_send_detonation 0x4bda60
    k_message_projectile_attach = 0x33,     // projectile_send_attach     0x4bf120

    // projectile_update gives up after ten collision responses inside one tick and forces
    // _projectile_state_detonating.
    k_projectile_maximum_collisions_per_tick = 10,

    // Super-combining explosion (Projectile has_super_combining_explosion). Both the attach
    // path (0x4bf1c0 / 0x4bf390) and projectile_detonate (0x4c0670) walk the child list of the
    // parent, counting siblings that share the definition_tag of this one and do not have
    // _projectile_super_detonation_counted_bit set. The attach path sets
    // _projectile_super_detonation_bit once it has seen more than five, and
    // projectile_detonate switches to the super_detonation effect once it has seen more than
    // six, zeroing the timers of the surplus and randomising the last six.
    k_projectile_super_combine_attach_threshold = 5,
    k_projectile_super_combine_detonate_threshold = 6,

    // The two collision-test masks projectile_collision_test (0x4c0450) hands the collision
    // module at 0x00505880: the fat one for the point sweep, the thin one for the two offset
    // sweeps that widen the ray by Projectile.collision_radius.
    k_projectile_collision_mask_point = 0x1000e9,
    k_projectile_collision_mask_radius = 0x89,

    // Thresholds the responder (0x4bf390) and the integrator use, read from the shared .data
    // float pool rather than being immediates:
    //   0x00672c94 = 0.3    collision_result.normal.k above which the surface counts as
    //                       ground: sets _projectile_hit_ground_bit, and on a stopped
    //                       projectile also object flag 0x20
    //   0x00672bbc = 0.0001 squared speed below which the projectile is _projectile_at_rest
    //   0x00672ac8 = 30.0   ticks per second, the divisor turning tag seconds into timer rates
    //   0x0069c52c = 0.00356518  gravity, world units per tick squared
    k_projectile_detonation_timer_infinite = 0 // rate 0.0 means "never": projectile_new only
                                               // stores 0x244 / 0x24c when the tag time is at
                                               // least one tick, so a zero rate parks the timer
} projectile_constants;

// ---------------------------------------------------------------------------
// projectile_flags  (the uint32 at object 0x22c, i.e. projectile_data.flags)
// projectile_new (0x4bd7c0) stores the literal 2 here, so every projectile starts life as a
// tracer and the weapon that fired it clears the bit for non-tracer rounds.
// ---------------------------------------------------------------------------
typedef enum projectile_flags {
    _projectile_rotation_valid_bit = 0x01,   // rotation_axis / rotation_sine / rotation_cosine
                                             //   hold a real rotation. Owned entirely by
                                             //   projectile_compute_rotation (0x4c0180), which
                                             //   sets it from a non-zero object
                                             //   angular_velocity and clears it otherwise
    _projectile_tracer_bit = 0x02,           // the contrail attachment survives. Set at create;
                                             //   the first projectile_update tick of a
                                             //   projectile without it contrail_deletes
                                             //   attachment_handles[contrail_attachment_index]
                                             //   and forgets the index.
                                             //   projectile_update_function_values reports it as
                                             //   projectilefunctionin_tracer
    _projectile_hit_ground_bit = 0x04,       // the last collision normal had k > 0.3.
                                             //   projectile_update sets it after the response,
                                             //   0x4bf390 sets it together with 0x10 when an
                                             //   attach response lands on a structure surface
    _projectile_attached_bit = 0x08,         // stuck to something: velocity and angular_velocity
                                             //   are zeroed, object flag 0x20 is set, the
                                             //   integrator is skipped and the detonation timer
                                             //   is allowed to run. Set by the attach response
                                             //   (0x4bf390) and by projectile_attach_apply
                                             //   (0x4bf1c0); cleared by projectile_force_detonate
    _projectile_at_rest_bit = 0x10,          // squared speed fell below 0.0001. This is the bit
                                             //   projectile_update tests for
                                             //   projectiledetonationtimerstarts_after_first_bounce
                                             //   and _when_at_rest
    _projectile_detonation_timer_started_bit = 0x20, // detonation_timer is counting. Set by
                                             //   projectile_update the first tick the timer is
                                             //   allowed to run; 0x4bf390 plays the tag
                                             //   detonation_started effect only while it is clear
    _projectile_super_detonation_counted_bit = 0x40, // already accounted for by a
                                             //   super-combining explosion, so later sibling
                                             //   sweeps skip it. Set by projectile_detonate on
                                             //   the last six siblings it randomises
    _projectile_super_detonation_bit = 0x80  // more than five siblings of the same definition
                                             //   are attached to the same parent
} projectile_flags;

// ---------------------------------------------------------------------------
// projectile_definition_flags  (the ProjectileFlags bitfield at Projectile tag 0x17c)
// types/tags.h carries ProjectileFlags only as a uint32 typedef with the field names in a
// comment; these are the four bits this module actually tests, in the tag's own bit order.
// ---------------------------------------------------------------------------
typedef enum projectile_definition_flags {
    _projectile_definition_oriented_along_velocity_bit = 0x01,  // projectile_update (0x4bdc00)
                                                 //   only rebuilds the orientation from the
                                                 //   velocity when this is set
    _projectile_definition_ai_must_use_ballistic_aiming_bit = 0x02, // read by the AI aiming
                                                 //   helper 0x4beec0, not by this module
    _projectile_definition_detonation_max_time_if_attached_bit = 0x04, // the attach paths
                                                 //   (0x4bf1c0 / 0x4bf390) seed the detonation
                                                 //   timer from timer[1] instead of timer[0]
    _projectile_definition_has_super_combining_explosion_bit = 0x08, // gates the sibling sweep in
                                                 //   the attach paths and in projectile_detonate
    _projectile_definition_combine_initial_velocity_with_parent_bit = 0x10, // read by
                                                 //   projectile_new (0x4bd7c0), outside this range
    _projectile_definition_random_attached_detonation_time_bit = 0x20, // the attach paths pick a
                                                 //   uniform time between timer[0] and timer[1]
    _projectile_definition_minimum_unattached_detonation_time_bit = 0x40
} projectile_definition_flags;

// ---------------------------------------------------------------------------
// projectile_state  (the int16 at object 0x230, i.e. projectile_data.state)
// Only ever raised, never lowered: projectile_request_state (0x4bf0f0) is
// "if (state < requested) state = requested", eax = projectile index, cx = requested state.
// projectile_update acts on it at the end of the tick.
// ---------------------------------------------------------------------------
typedef enum projectile_state {
    _projectile_state_flying = 0,        // projectile_new writes this
    _projectile_state_detonating = 1,    // run projectile_detonate, then delete. Requested by
                                         //   the detonation timer completing, by
                                         //   projectileresponse_detonate, by a speed below
                                         //   Projectile.minimum_velocity, by exceeding
                                         //   maximum_range, by ten collisions in one tick and
                                         //   by the network detonation message
    _projectile_state_disappearing = 2   // delete with no detonation. Requested only by
                                         //   projectileresponse_disappear
} projectile_state;

// ---------------------------------------------------------------------------
// projectile_network_state  (object 0x27c and object 0x298)
// The payload half of every projectile delta: projectile_network_baseline_take (0x4c0ed0)
// fills the first copy from object.position / object.velocity, projectile_build_network_update
// (0x4c0f30) hands its address to message_delta_encode_message as the "items" pointer, and
// projectile_apply_network_update (0x4c1070) writes the second copy.
// ---------------------------------------------------------------------------
typedef struct projectile_network_state {
    real_point3d position;               // 0x00
    real_vector3d velocity;              // 0x0c
} projectile_network_state;              // size 0x18

// ---------------------------------------------------------------------------
// projectile_data  (object 0x1f4 .. 0x2b0, the only extension the projectile chain adds)
// Established by projectile_new (0x4bd7c0), projectile_update (0x4bdc00 and its promoted
// mid-body label 0x4be1b0), projectile_response (0x4bf390), projectile_compute_rotation
// (0x4c0180), projectile_update_function_values (0x4c0250), projectile_compute_deceleration
// (0x4c0310), projectile_detonate (0x4c0670), projectile_attach_apply (0x4bf1c0),
// projectile_notify_object_deleted (0x4bf0c0), projectile_force_detonate (0x4c0ac0), the
// network quartet (0x4c0b10 / 0x4c0ca0 / 0x4c0ed0 / 0x4c0f30 / 0x4c1070) and
// projectile_send_detonation (0x4bda60).
// ---------------------------------------------------------------------------
typedef struct projectile_data {
    uint8_t unknown_1f4[0x38];       // 0x1f4 UNRESOLVED. Not one of the 22 Ghidra functions nor
                                     //       any of the five recovered from the type row reads
                                     //       or writes a byte of it, and projectile_new does not
                                     //       initialize it either, so it arrives zeroed from the
                                     //       object pool. Its readers live outside this module.
    uint32_t flags;                  // 0x22c projectile_flags, initialized to
                                     //       _projectile_tracer_bit
    int16_t state;                   // 0x230 projectile_state
    int16_t material_response_index; // 0x232 index into the Projectile tag
                                     //       projectile_material_response block (stride 0xa0);
                                     //       -1 at create, then the global material type of the
                                     //       last surface hit, and the value out of
                                     //       damage_data after object damage adjusted it.
                                     //       Out of range (< 0 or >= count) falls back on the
                                     //       static record at 0x00695e20. projectile_detonate
                                     //       reads detonation_effect.tag_id (+0x74) from the row
    datum_index ignore_object_index; // 0x234 the object the collision sweep skips.
                                     //       projectile_new walks object.creating_object (0xc4)
                                     //       up the parent chain and stores the root, i.e. the
                                     //       firing unit; projectile_update also uses the value
                                     //       to suppress the flyby sound for the local player,
                                     //       forces it to -1 after every collision, and the
                                     //       overpenetrate response stores the object just
                                     //       passed through
    datum_index tracked_object_index;// 0x238 the guidance target. -1 at create;
                                     //       projectile_notify_object_deleted (0x4bf0c0) scrubs
                                     //       it. projectile_update only steers when it is set
                                     //       and Projectile.guided_angular_velocity > 0, turning
                                     //       guided_angular_velocity/30 radians per tick toward
                                     //       the target bounding_center with a periodic wobble
    int32_t contrail_attachment_index;// 0x23c index into object.attachment_types /
                                     //       attachment_handles. projectile_new scans the Object
                                     //       tag attachments block (count 0x140, pointer 0x144,
                                     //       stride 0x48) for the first entry whose tag group is
                                     //       'cont' and stores its index, else -1
    float detonation_timer;          // 0x240 0..1 progress; at 1.0 projectile_update requests
                                     //       _projectile_state_detonating.
                                     //       projectile_update_function_values reports it as
                                     //       projectilefunctionin_time_remaining
    float detonation_timer_rate;     // 0x244 per-tick increment. projectile_new and both attach
                                     //       paths compute it as 1/(t*30) where t is
                                     //       Projectile.timer[1] for detonation_max_time_if
                                     //       _attached, timer[0] for minimum_unattached
                                     //       _detonation_time, and random_real_range(timer[0],
                                     //       timer[1]) otherwise; left at 0 when t*30 < 1
    float arming_timer;              // 0x248 0..1 progress, advanced every tick regardless of
                                     //       state. While state is _projectile_state_detonating
                                     //       and this has not reached 1.0 the projectile keeps
                                     //       flying and does not detonate
    float arming_timer_rate;         // 0x24c per-tick increment, 1/(Projectile.arming_time*30)
                                     //       from projectile_new; 0 means "already armed",
                                     //       which is what makes the state-1 gate above
                                     //       fall through
    float distance_travelled;        // 0x250 accumulated world units. Compared against
                                     //       Projectile.maximum_range each tick (detonate on
                                     //       overrun, with the tick clipped at the range) and
                                     //       divided by it for
                                     //       projectilefunctionin_range_remaining
    float deceleration_delay;        // 0x254 0..1 ramp; the velocity decay below is skipped
                                     //       while it is under 1.0
    float deceleration_delay_rate;   // 0x258 per-tick increment. projectile_compute_deceleration
                                     //       sets it to damage_range[0]/initial_velocity, and
                                     //       forces delay 1.0 / rate 0.0 when damage_range[0]
                                     //       is not positive. See the note file: the quotient
                                     //       is the reciprocal of the obvious intent
    float deceleration;              // 0x25c world units per tick squared, from
                                     //       (initial_velocity^2 - final_velocity^2) /
                                     //       (2*(damage_range[1] - damage_range[0])) in 0x4c03f0,
                                     //       or 0 when the two velocities are equal. Subtracted
                                     //       from the speed each tick until final_velocity
    float deceleration_end_range;    // 0x260 the distance_travelled past which a projectile with
                                     //       no deceleration expires. See the note file: both
                                     //       branches of projectile_compute_deceleration store
                                     //       water_damage_range[1] (tag 0x1e0), so the air
                                     //       branch uses the wrong field -- a retail bug, kept
    real_vector3d rotation_axis;     // 0x264 normalized object.angular_velocity
    float rotation_sine;             // 0x270 sin / cos of the angular speed, so the renderer can
    float rotation_cosine;           // 0x274 spin the model without recomputing a trig pair.
                                     //       0 / 1 when the projectile is not rotating.
                                     //       The exact analogue of item_data rotation_sine
    uint8_t thrown_grenade;          // 0x278 0 at create. The only writer of a 1 in the whole
                                     //       image is unit_throw_grenade_move_to_hand
                                     //       (0x0056e41a), and the only reader is
                                     //       projectile_update, which broadcasts the detonation
                                     //       message when it is set and this machine is the
                                     //       authority (object.network_role == 0)
    uint8_t network_state_valid;     // 0x279 network_state below is live. Set by
                                     //       projectile_create_from_network (0x4c0ca0) and by
                                     //       projectile_network_baseline_take (0x4c0ed0);
                                     //       projectile_new clears all four of these bytes when
                                     //       the game is networked (0x00719720 is 1 or 2)
    uint8_t network_baseline_index;  // 0x27a bumped by projectile_network_baseline_take each time
                                     //       a new baseline is taken; an incoming update whose
                                     //       own baseline byte differs is dropped
    uint8_t network_sequence;        // 0x27b wrapping 0..0xff, reset to 0 with each baseline and
                                     //       incremented by projectile_build_network_update
                                     //       (which folds 0xff back to 0). An update is stale
                                     //       unless its sequence is newer by less than 0x1e
    projectile_network_state network_state;      // 0x27c the outgoing baseline the delta encoder
                                     //       diffs against
    uint8_t last_update_valid;       // 0x294 last_update_state below is live; only
                                     //       projectile_apply_network_update sets it
    uint8_t pad_295[3];              // 0x295 never read or written
    projectile_network_state last_update_state;  // 0x298 the most recent state accepted from the
                                     //       network, ending exactly on object_size 0x2b0
} projectile_data;                   // size 0xbc at object 0x1f4, fixed by the type row

// A projectile object as one struct: the common object header (types/objects.h) followed by
// projectile_data, so a field is one fixed offset from the object like the original code uses.
typedef struct projectile_object {
    object base;                        // 0x000
    projectile_data projectile;         // 0x1f4
} projectile_object;
typedef char projectile_object_projectile_at_1f4[offsetof(projectile_object, projectile) == 0x1f4 ? 1 : -1];

// ---------------------------------------------------------------------------
// collision_result  (the 0x50-byte record the collision module fills)
// NOT owned by this module: projectile_collision_test (0x4c0450) only allocates it and hands it
// to 0x00505880, and projectile_response (0x4bf390) only reads it. It is spelled here because
// the projectile responder is the module that pins the layout, and because the same record is
// what item_update sweeps with. Move it to a types/collision.h when that module is written.
//
// The size is bounded on both sides: projectile_update reserves the buffer as the top local of
// a frame that ends 0x50 bytes above it (sub esp,0x15c plus three pushes, buffer at esp+0x118),
// and projectile_response reads a word at +0x4e.
// ---------------------------------------------------------------------------
typedef enum collision_result_type {
    _collision_result_type_water_surface = 0, // the overpenetrate response toggles object flag
                                              //   0x10 (in water), recomputes the projectile
                                              //   deceleration for the new medium and nudges the
                                              //   position 0.001 back along the normal. Also
                                              //   produced by 0x505880's fog-plane test and read
                                              //   by point_physics 0x50b530
    _collision_result_type_unknown_1 = 1,     // never produced by anything this module calls
    _collision_result_type_structure = 2,     // a structure BSP surface; surface_flags bit 0x08
                                              //   routes breakable-surface damage to 0x004ffde0
    _collision_result_type_object = 3         // an object; drives object_apply_damage and the
                                              //   attach response
} collision_result_type;

typedef struct collision_result {
    int16_t type;                   // 0x00 collision_result_type
    int16_t unknown_02;             // 0x02
    int32_t first_leaf;             // 0x04 leaf of the segment start point (0x5058c2 DWORD,
                                    //      0x5060e4..); with first_cluster it is laid out like a
                                    //      bsp_leaf_reference
    int16_t first_cluster;          // 0x08 cluster of that leaf (0x5058c5 WORD)
    int16_t unknown_0a;             // 0x0a
    bsp_leaf_reference leaf;        // 0x0c the endpoint leaf/cluster pair (0x5058c9 DWORD +0x0c,
                                    //      0x5058cb WORD +0x10); projectile_response forwards it
                                    //      to object_set_cluster_and_parent and to the
                                    //      breakable-surface damage call
    float t;                        // 0x14 fraction of the swept segment consumed before the
                                    //      hit; projectile_update turns it into the 1 - t
                                    //      remainder it integrates the rest of the tick with
    real_point3d point;             // 0x18 contact point. projectile_response returns it as the
                                    //      new position of the projectile, and hands it to the impact
                                    //      noise call as the sound origin
    real_plane3d plane;             // 0x24 the surface plane: plane.normal is the surface normal
                                    //      (used for the reflection, for the "normal" entry of
                                    //      the effect coordinate system, and normal.k > 0.3 is
                                    //      the ground test) and plane.d (+0x30) its distance
    int16_t material_type;          // 0x34 global material type index; this is what seeds
                                    //      projectile_data.material_response_index
    int16_t unknown_36;             // 0x36
    datum_index object_index;       // 0x38 the object hit, -1 for a structure surface
    int16_t region_index;           // 0x3c collision region hit (WORD stores 0x5056f2, 0x5057d3);
                                    //      object_apply_damage's fourth argument
    int16_t node_index;             // 0x3e the collision node hit (formerly marker_index):
                                    //      object_attach_to_object uses it as the attachment
                                    //      marker, object_apply_damage as its third argument, and
                                    //      the attach message carries it
    int16_t permutation_index;      // 0x40 (WORD store 0x5056fa)
    int16_t pad_42;                 // 0x42 padding
    int32_t surface_index;          // 0x44 forwarded to the breakable-surface damage routine
                                    //      0x004ffde0. UNSURE of the name
    int32_t plane_index;            // 0x48 the BSP plane hit; the sign bit set means a back-face
                                    //      hit (DWORD stores 0x505701, 0x5057f4, 0x505a02)
    uint8_t surface_flags;          // 0x4c bit 0x08 = the surface can be damaged/broken
    uint8_t breakable_surface_index;// 0x4d (BYTE stores 0x505707, 0x505a0c, 0x5060b9); packed
                                    //      into the low half of the breakable-surface damage
                                    //      argument
    int16_t collision_material_index; // 0x4e index into ScenarioStructureBSP.collision_materials
                                    //      (WORD stores 0x50570a, 0x505a0f); object_apply_damage's
                                    //      fifth argument
} collision_result;                 // size 0x50

// ---------------------------------------------------------------------------
// network records
// All four are built on the stack and handed to message_delta_encode_message, which bit-packs
// them; the sizes below are the stack footprint, not the wire size. Every object handle is run
// through hash_table_get against the object network-id table at PTR_DAT_00687130 + 0x0c first,
// and a lookup that returns -1 is written as 0.
// ---------------------------------------------------------------------------

// projectile_send_creation (0x4c0b10), message 0x1e. The receiver is
// projectile_create_from_network (0x4c0ca0), which orthonormalizes forward/up, builds an
// object_placement_data with flag 0x02, calls object_new_with_datum_role_control, then seeds
// network_state, network_baseline_index, network_state_valid = 1, network_sequence = 0 and
// copies the velocity into object.velocity.
typedef struct projectile_creation_message {
    datum_index definition_tag;     // 0x00 object.definition_tag
    int32_t object_hash;            // 0x04 network id of the projectile itself
    int16_t owner_team;             // 0x08 object.owner_team (object 0xb8, formerly name_index)
    uint8_t pad_0a[2];              // 0x0a
    int32_t owner_hash;             // 0x0c network id of object.owner_linkage (object 0xc0)
    int32_t creating_object_hash;   // 0x10 network id of object 0xc4, the firing object
    real_point3d position;          // 0x14 projectile_data.network_state.position
    real_vector3d forward;          // 0x20 object.forward
    real_vector3d up;               // 0x2c object.up
    real_vector3d velocity;         // 0x38 projectile_data.network_state.velocity
    real_vector3d angular_velocity; // 0x44 object.angular_velocity
    uint8_t baseline_index;         // 0x50 projectile_data.network_baseline_index
    uint8_t pad_51[3];              // 0x51
} projectile_creation_message;      // size 0x54

// projectile_send_detonation (0x4bda60), message 0x30. Sent only for a thrown_grenade
// projectile detonating on the authority. The sender also forces object.network_role to 3. The
// receiver is 0x4bdb40, which sets role 3, repositions the object, runs projectile_detonate,
// raises the state and deletes it.
typedef struct projectile_detonation_message {
    int32_t object_hash;            // 0x00 network id of the projectile
    real_point3d position;          // 0x04 object.position at the moment of detonation
} projectile_detonation_message;    // size 0x10

// projectile_send_attach (0x4bf120), message 0x33. Sent by the attach response when the
// projectile stuck to an object and both ends are authoritative. The receiver is
// projectile_attach_apply (0x4bf1c0).
typedef struct projectile_attach_message {
    int32_t object_hash;            // 0x00 network id of the projectile
    int32_t parent_hash;            // 0x04 network id of the object it stuck to
    int16_t parent_marker_index;    // 0x08 collision_result.marker_index
} projectile_attach_message;        // size 0x0a

// projectile_build_network_update (0x4c0f30), message index
// object_type_definition[projectile] + 0x10 == 1. The header travels as the "changed" record of
// the delta encoder and projectile_network_state as its "items" record.
typedef struct projectile_network_update_header {
    int32_t object_hash;            // 0x00 network id of the projectile
    uint8_t baseline_index;         // 0x04 projectile_data.network_baseline_index
    uint8_t sequence;               // 0x05 projectile_data.network_sequence
    uint8_t is_delta;               // 0x06 1 for an incremental update, 0 for a baseline.
                                    //      projectile_apply_network_update treats a nonzero
                                    //      value as permission to adopt the incoming baseline
                                    //      index and state as its own. UNSURE of the name
} projectile_network_update_header; // size 0x07; the record the applier reaches through
                                    // update_record[0x11] is longer than this

// ---------------------------------------------------------------------------
// globals this module owns
// ---------------------------------------------------------------------------
// global 0x00695e20: ProjectileMaterialResponse projectile_default_material_response
//     0xa0 static bytes, referenced only from projectile_response (0x4bf390) and
//     projectile_detonate (0x4c0670), both of which substitute it whenever
//     projectile_data.material_response_index falls outside the tag block. Every field is zero
//     -- response projectileresponse_disappear, no noise, no friction -- except the three
//     effect dependencies, which are {'effe', "", 0, -1}. Ghidra also names three interior
//     offsets separately: 0x00695e80 is angular_noise (+0x60), 0x00695e84 velocity_noise
//     (+0x64) and 0x00695e94 detonation_effect.tag_id (+0x74).
// global 0x00695f80: char *projectile_effect_coordinate_system_names[5]
//     Read out of .data as {"normal", "incident", "negative incident", "reflection",
//     "gravity"} (pointers 0x00660708, 0x0066b1b0, 0x0066b19c, 0x0066b190, 0x0066050c).
//     projectile_response passes the table plus five matching vectors -- the surface normal,
//     the negated unit velocity, the unit velocity, the reflection of the velocity about the
//     normal, and the constant "down" vector at 0x0069672c -- to the effect spawner, which is
//     how each name is identified. projectile_detonate builds its own two-entry array
//     {"", "gravity"} on the stack instead.
// global 0x00696140: float projectile_network_update_position_tolerance   // 0.2 world units;
//     projectile_apply_network_update only relinks the object when an incremental update moved
//     the position further than this, and always relinks for a baseline.
//
// Globals the module reads but does not own, listed because every accessor in this header
// depends on them:
//   0x008603b0  data_array *object_data              // owned by types/objects.h
//   0x0087bc14  tag_instance *tag_instances          // owned by types/cache.h
//   0x0069bfdc  object_type_definition *object_type_definitions[12]
//   0x0069c52c  float k_gravity_per_tick_squared     // 0.00356518
//   0x00672ab8..0x00672f20  the shared .data float pool (0.2, 0.5, 0.0, 1.0, 30.0, 1/30, ...)
//   0x0065c218..0x0065c264  the shared constant vectors, via the pointers at 0x00696710
//   0x00719cd0  uint32_t random_seed_global          // owned by types/math.h
//   0x006b7af4  real_point3d *sphere_point_table     // owned by types/math.h
//   0x006b7af8  int16_t sphere_point_table_count
//   0x006f1d6c  void *game_time_globals              // +0x0c is the game tick
//   0x006f1d20  game_engine_definition *current_game_engine  // game.h (R04); non-NULL = a
//                                                    // multiplayer engine is loaded
//   0x00719720  int16_t network_game_mode            // 0 local, 1 client, 2 host
//   0x006894c8  int32_t k_projectile_minimum_age_ticks // projectile_is_old_enough (0x4c1270)
//                                                    //   compares object 0x0c + this against
//                                                    //   the game tick
//   0x00746f9c  void *structure_bsp_globals          // projectile_new probes it for the medium
//   0x0087a478  void *local_player_globals           // the flyby-sound listener
//   0x0087a480  data_array *player_data
//   0x00687130  void *object_network_id_table        // +0x0c hash table, +0x28 id -> handle
//   0x00687558  void *player_network_id_table
//   0x00871de0  uint8_t message_delta_buffer[]
//   0x0065512c  char k_empty_string[1]

#pragma pack(pop)
