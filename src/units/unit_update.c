// unit_update  (Ghidra: unit_update, misattributed; out/phase4/units_types_notes.md: "the
// unit row's +0x34 column is 0x5625b0 ... the real per-tick unit update", correcting the
// pre-existing name "unit_update" that belonged to the *biped* row's column, 0x5590a0)
// address 0x5625b0, size 4765 bytes
// name confidence: 0.85 (object_type_definition vtable proof; see file header of types/units.h)
// rewrite confidence: 0.15 -- by far the least certain file in this batch; see UNSURE notes
//   throughout. This function is the single largest and most cross-cutting one in the module,
//   and nearly every offset it touches is independently corroborated by a specific sentence in
//   out/phase4/units_types_notes.md written against this exact function, which is what makes a
//   rewrite possible at all; the parts of that documentation this file's evidence disagrees with
//   are called out inline.
// evidence: types/units.h unit_data (almost every field from 0x204 to 0x424, see inline
//   comments), biped_data.flags (0x4cc, bit 0 grounded -- via game_engine_is_valid_team_player's boolean result,
//   local_8); types/objects.h object.forward/up (0x74/0x80), .flags (0x10), .vitality_flags
//   (0x106); types/tags.h Unit.unit_flags (tag+0x17c, simple_creature 0x800, has_no_aiming
//   0x400), UnitFunctionIn_t fields (tag+0x264/0x268/0x270/0x274, the four turn/throttle rate
//   scalars -- see UNSURE), Biped.contact_point (tag+0x4e8, walked at line ~459); math.h
//   global_origin3d_pointer/global_forward3d_pointer (0x696714/0x696718),
//   vector3d_rotate_toward_with_acceleration, vector3d_cross_product, vector3d_angle_between.
// UNSURE: puVar4+0xae..0xb1 and +0xb2..0xb5 (the third argument of the two vector3d_rotate_toward_bounded calls,
//   an angular-acceleration carry-over the math helper writes back) land on the same four
//   floats types/units.h calls aiming_bounds/looking_bounds (0x2b8/0x2c8), which
//   unit_update_aiming_overlay_angles (0x563b50) also writes every tick with a completely
//   different meaning (a static yaw/pitch clamp box). Both are reproduced literally; whether
//   this is a genuine field reuse across two different subsystems or a boundary this batch
//   mapped slightly wrong is not resolved here.
// UNSURE: the four floats at tag+0x264/0x268/0x270/0x274 (the Unit tag's per-tick
// aiming/looking acceleration and velocity scalars fed into the two rotate-toward calls) have
// no named fields in types/tags.h's current Unit coverage; kept as raw tag offsets.
// UNSURE: object.flags bits 0x10000000 and 0x20000000 have no entry in object_flags
// (types/objects.h only documents up to 0x04000000); kept as raw literals.
// UNSURE: several zero-argument callees in this function (game_engine_is_valid_team_player, unit_check_weapon_use_permission,
// unit_refresh_targeting_flag_and_weapons, unit_clear_ground_adjust_dirty, unit_update_random_turn_angle, unit_update_autoaim_interaction, unit_melee_lunge_damage_tick, unit_update_look_delta_controls,
// weapon_set_ready_timer, effect_new_on_object, unit_get_weapon_object_index at line ~427) are declared with the
// exact argument counts/types Ghidra shows at each call site, which is not always consistent
// with how the same function is called elsewhere in this module; each call site is preserved
// literally rather than reconciled against a single guessed prototype.
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)
// reconciled: R04 0x006f1d20 int32_t network_predicted_state_flag -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

// PHASE-4 REVIEW CORRECTIONS (all five were field-identity errors found by re-deriving every
// puVar4 access from the Ghidra block; puVar4 is a `uint *`, so puVar4[i] is byte offset i*4
// while *(char *)((int)puVar4 + n) is byte offset n, and the earlier rewrite mixed the two):
//   0x0e0 object.body_vitality      was written as unit_data.animation_blend_weight
//   0x288 unit_data.aiming_speed    was written as unit_data.unknown_2a9
//   0x28b unit_data.unknown_28b     was written as unit_data.melee_damage_countdown (0x28a)
//   0x2e8 animation_blend_weight    was written as unit_data.unknown_338
//   0x344 unit_data.unknown_344     was written as unit_data.unknown_338, and its `< 0.0f`
//                                   test as `!= 0.0f`
// unit_data.unknown_338 is not touched by this function at all.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "effects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t unit_updates_suppressed; // 0x0071c419, DAT_0071c419
extern struct { int16_t threshold; int16_t highest; uint8_t claimed; } *ai_update_stagger; // 0x006ef910, DAT_006ef910
extern real_vector3d *global_origin3d_pointer;  // types/math.h spells it real_point3d *; the
                                                // same three floats, read here as a vector   // 0x00696714
extern real_vector3d *global_forward3d_pointer;  // 0x00696718
extern real_vector3d *global_up3d_and_neighbors_pointer; // 0x006966f8, UNSURE identity
extern game_engine_definition *current_game_engine;   // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern char *unit_base_animation_state_names[6]; // 0x0069fde4 (PTR_s_stand_0069fdec is &names[2])
extern uint8_t network_toggle_0087abc2;   // 0x0087abc2, DAT_0087abc2
extern data_array *player_data;                  // 0x0087a480
extern uint8_t *globals_tag_data; // 0x00746fa0, the globals tag data; +0x180 -> player info block

extern int32_t __ftol(); // 0x6391b4, MSVC 7.1 CRT float-to-int truncation; the double is on the x87 stack
extern uint8_t game_engine_is_valid_team_player(uint32_t unit_index);                  // 0x466b60, UNSURE: "is grounded" style predicate
extern int32_t player_index_from_unit_index(uint32_t unit_index);                 // 0x474db0
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX, CX
  // real signature (unit_get_weapon_object_index.c): datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); Ghidra recovered 1 of 2 args at this call site
extern void weapon_set_control_flags(datum_index item_index, uint16_t control_flags, real primary_trigger); // 0x4c2990, EAX, stack
extern void weapon_set_ready_timer(datum_index item_index, real value); // 0x4c2b20, EAX, stack
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, blam-cc: ECX, EDX
  // real signature (vector3d_angle_between_4cd4f0.c): real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); Ghidra recovered 0 of 2 args at this call site
extern void vector3d_rotate_toward_with_acceleration(real_vector3d *direction, real_vector3d *target_direction,
    real_vector3d *angular_velocity, real maximum_velocity, real acceleration);
    // 0x4cf530, blam-cc: ESI, EDI, stack (0x562f1d..0x562f38 / 0x5630cc..0x5630e7)
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up);
// vector3d_cross_product (0x4052c0) computes  *out = stack_operand x ecx_operand,  with out
// in EAX, ecx_operand in ECX and stack_operand pushed -- read out of the callee own
// decompilation (in_EAX / in_ECX / param_1) and matching
// src/objects/object_set_position_and_orientation.c.
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0
extern void sound_start_unspatialized(float amount);                            // 0x543dd0
extern void unit_clear_ground_adjust_dirty(uint32_t object_index);                                    // 0x55ad70, UNSURE: no traced args
extern void unit_dispatch_reaction_animation(int32_t unit_index, int16_t reaction_code); // 0x5614a0, ESI unit, stack code
extern void unit_update_animation_timers(uint32_t unit_index);      // 0x561620
extern uint8_t unit_is_look_target_valid(uint32_t unit_index);      // 0x562570
extern void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, float *bounds,
                          float max_velocity, float max_acceleration, real_vector3d *target,
                          real_matrix4x3 *transform); // 0x564ae0, src/math; blam-cc: stack (current, velocity,
                          // bounds, max_velocity, max_acceleration), ECX target, ESI transform.
// The third argument is the four-float (-yaw, +yaw, -pitch, +pitch) box: unit_data.aiming_bounds
// and .looking_bounds here, a stack copy of (-PI, PI, -PI/2, PI/2) in biped_update_facing.c.
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label,
                                                     char *weapon_label, uint8_t test_only); // 0x5651e0,
// unit_index in EAX; this matches the definition in unit_set_or_test_seat_and_weapon_label.c.
// The phase-4 review pass corrected the arity (Ghidra binds only the stack arguments at these
// call sites) and the return type (the callee returns a byte, tested in AL).
extern uint8_t unit_current_weapon_has_flag(uint32_t unit_index);    // 0x565b60
extern uint8_t unit_state_is_scripted_animation(unit_data *unit);    // 0x565c60
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset);        // 0x568610, UNSURE signature
extern uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction,
                                                          uint32_t which_bounds); // 0x5697a0
// UNSURE-CALL: Ghidra recovered 2 of the 3 arguments at every call site below (the unit index
// is register-passed); it is supplied here from the surrounding context.
extern int32_t unit_find_next_grenade_type_with_count(uint32_t unit_index, int32_t start_index, int16_t direction); // 0x5699a0, EAX, CX, stack
  // real signature (unit_find_next_grenade_type_with_count.c): int32_t unit_find_next_grenade_type_with_count(uint32_t unit_index, int32_t start_index, int16_t direction); Ghidra recovered 1 of 3 args at this call site
extern void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag); // 0x569bf0, stack, CL
  // real signature (unit_refresh_targeting_flag_and_weapons.c): void unit_refresh_targeting_flag_and_weapons(uint32_t unit_index, uint8_t initial_targeting_flag); Ghidra recovered 1 of 2 args at this call site
extern void unit_ready_desired_weapon(uint32_t unit_index, uint32_t flag); // 0x56d6e0
  // real signature (unit_ready_desired_weapon.c): void unit_ready_desired_weapon(uint32_t unit_index); Ghidra recovered 2 of 1 args at this call site
extern uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index); // 0x56da00, ESI, EDI
  // real signature (unit_check_weapon_use_permission.c): uint8_t unit_check_weapon_use_permission(uint32_t unit_index); Ghidra recovered 0 of 1 args at this call site
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern uint8_t unit_begin_throw_grenade(uint32_t unit_index, int32_t force_trigger); // 0x56e080, EDI, stack
  // real signature (unit_begin_throw_grenade.c): uint8_t unit_begin_throw_grenade(uint32_t unit_index, int32_t force_trigger); Ghidra recovered 1 of 2 args at this call site
extern void unit_throw_grenade_move_to_hand(uint32_t unit_index);     // 0x56e280
extern void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction); // 0x56e440
extern void unit_update_look_delta_controls(uint32_t object_index); // 0x56e820, EAX
  // real signature (unit_update_look_delta_controls.c): void unit_update_look_delta_controls(uint32_t object_index); Ghidra recovered 0 of 1 args at this call site
extern void unit_calculate_luminosity(uint32_t object_index); // 0x56ec60, EDI
  // real signature (unit_calculate_luminosity.c): void unit_calculate_luminosity(uint32_t object_index); Ghidra recovered 0 of 1 args at this call site
extern void unit_melee_lunge_damage_tick(uint32_t unit_index);                        // 0x56fc80
extern void unit_update_autoaim_interaction(uint32_t unit_index);                        // 0x570720
extern void unit_update_random_turn_angle(uint32_t object_index, real_vector3d *out_axis); // 0x570840, EAX, EDI
  // real signature (unit_update_random_turn_angle.c): void unit_update_random_turn_angle(uint32_t object_index, real_vector3d *out_axis); Ghidra recovered 0 of 2 args at this call site
extern void actor_react_to_threat_event(datum_index self_object_index, datum_index other_object_index, int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay); // 0x42be40, UNSURE signature
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six

// FIXED (register inputs, objdump; one stack argument remains, so no ordering question): the original never reads EAX; unit_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> unit_index
uint8_t unit_update(uint32_t unit_index) // blam-cc: param_1 (EAX) -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    uint8_t won_stagger_slot = 0;
    uint8_t seat_control_applied = 0;
    uint8_t is_grounded = game_engine_is_valid_team_player(unit_index); // UNSURE: see file header (local_8)

    if (!unit_updates_suppressed) {
        unit->update_tick_counter = unit->update_tick_counter + 1;
        int16_t tick = unit->update_tick_counter;
        if (ai_update_stagger->claimed == 0 && ai_update_stagger->threshold < tick) {
            ai_update_stagger->claimed = 1;
            won_stagger_slot = 1;
            unit->update_tick_counter = 0;
        } else {
            if (tick < ai_update_stagger->highest) {
                tick = ai_update_stagger->highest;
            }
            ai_update_stagger->highest = tick;
        }
    }

    // Seed the aiming/looking/facing triad. Unattended units (no actor/swarm) drive them off the
    // object's own basis instead of control input; idle_turn_seeded gates a one-time reseed via
    // unit_update_random_turn_angle instead of the full reset below.
    // Neither branch below touches aiming_velocity/looking_velocity; the only statement they
    // share is the final throttle.k write, so each branch just repeats it rather than using a
    // goto into a shared tail.
    if ((unit->flags & _unit_flag_idle_turn_seeded) == 0) {
        if ((unit->flags & _unit_flag_unattended) == 0) {
            unit->desired_looking_vector = obj->forward;
            unit->desired_aiming_vector = obj->forward;
            unit->desired_facing_vector = obj->forward;
            unit->throttle.i = global_origin3d_pointer->i;
            unit->throttle.j = global_origin3d_pointer->j;
            unit->control_flags = 0;
            unit->throttle.k = global_origin3d_pointer->k;
        }
        // (unattended set: this whole reset is skipped, matching the original)
    } else {
        unit_update_random_turn_angle(unit_index, &unit->desired_facing_vector); // 0x562661: EAX unit, EDI = object +0x224
        unit->desired_aiming_vector = unit->desired_facing_vector;
        unit->desired_looking_vector = unit->desired_facing_vector;
        unit->throttle.i = global_forward3d_pointer->i;
        unit->throttle.j = global_forward3d_pointer->j;
        unit->control_flags = 0;
        unit->throttle.k = global_forward3d_pointer->k;
    }

    if ((((Unit *)obj_tag)->unit_flags & 0x800) == 0) { // not simple_creature
        // Grenade-throw duration countdown and the driver/gunner control-input passthrough.
        int32_t throw_duration = unit->throwing_grenade_duration;
        uint8_t network_grenade_pending = 0;
        if (throw_duration > 0) {
            uint32_t flags = unit->control_flags | unit->unknown_214;
            unit->control_flags = flags;
            if ((unit->unknown_214 & 0x800) == 0) {
                unit->unknown_408 = 0.0f; // UNSURE: see below
            } else {
                if (throw_duration % 7 == 0) {
                    flags = flags | 0x800;
                } else {
                    flags = flags & ~0x800u;
                }
                unit->control_flags = flags;
                unit->unknown_408 = 1.0f;
            }
            unit->throwing_grenade_duration = throw_duration - 1;
            if (throw_duration - 1 == 0) {
                unit->unknown_214 = 0;
            }
        }

        if ((unit->flags & _unit_flag_unknown_8000000) == 0) {
            if (unit->driver_unit_index != (datum_index)-1 &&
                (obj->vitality_flags & _object_health_frozen_bit) == 0) {
                object *driver_obj = ((object_header *)object_data->data)[unit->driver_unit_index & 0xffff].data;
                unit_data *driver = (unit_data *)((uint8_t *)driver_obj + k_unit_data_offset);
                obj->owner_team = driver_obj->owner_team; // puVar4+0x2e -> object 0xb8
                network_grenade_pending = 1;
                if (driver->controlling_player != (datum_index)-1 ||
                    (driver->animation_state != 0x1b && driver->animation_state != 0x1a)) {
                    unit->control_flags = unit->control_flags | (driver->control_flags & 0x3f);
                    unit->desired_facing_vector = driver->desired_facing_vector;
                    unit->throttle = driver->throttle;
                }
            }
            if (unit->gunner_unit_index != (datum_index)-1 &&
                (obj->vitality_flags & _object_health_frozen_bit) == 0) {
                object *gunner_obj = ((object_header *)object_data->data)[unit->gunner_unit_index & 0xffff].data;
                unit_data *gunner = (unit_data *)((uint8_t *)gunner_obj + k_unit_data_offset);
                if (!network_grenade_pending) {
                    obj->owner_team = gunner_obj->owner_team;
                }
                if (gunner->controlling_player != (datum_index)-1 ||
                    (gunner->animation_state != 0x1b && gunner->animation_state != 0x1a)) {
                    // Both destinations copy the gunner's desired_aiming_vector (0x230), not
                    // desired_looking_vector -- reproduced literally from the original.
                    unit->desired_aiming_vector = gunner->desired_aiming_vector;
                    unit->desired_looking_vector = *(real_vector3d *)&gunner->desired_aiming_vector;
                    unit->control_flags = unit->control_flags | (gunner->control_flags & 0x7c00);
                    unit->primary_trigger = gunner->primary_trigger;
                }
            }
            if ((unit->control_flags & 0x7c00) == 0) {
                if (unit->unknown_322 < 0x7f) {
                    unit->unknown_322 = unit->unknown_322 + 1;
                }
            } else {
                unit->unknown_322 = 0;
            }
        }

        if (!unit_updates_suppressed) {
            // unknown_408 (damage accumulator) and unknown_424 (stun meter) 1/120 and 1/90 ramps.
            if ((unit->flags & 0x10) == 0) {
                float v = unit->unknown_37c - 0.008333334f; // UNSURE: field identity, see below
                unit->unknown_37c = (v < 0.0f) ? 0.0f : v;
            } else {
                float delta;
                if (current_game_engine == 0 || unit->unknown_422 == 0 || unit->unknown_422 != 1) {
                    delta = 0.008333334f;
                } else {
                    datum_index weapon = unit_get_weapon_object_index(unit_index, unit->current_weapon_index); // CX = +0x2f2
                    if (weapon == (datum_index)-1) {
                        delta = 0.008333334f;
                    } else {
                        object *weapon_obj = ((object_header *)object_data->data)[weapon & 0xffff].data;
                        void *weapon_tag = tag_instances[weapon_obj->definition_tag & 0xffff].data;
                        float rate = *(float *)((uint8_t *)weapon_tag + 0x4d0); // UNSURE: raw Weapon-tag field
                        delta = (rate == 0.0f) ? 0.008333334f : rate;
                    }
                }
                float sum = unit->unknown_37c + delta;
                unit->unknown_37c = sum;
                if (sum > 1.0f) {
                    unit->unknown_37c = 0.0f;
                    unit->unknown_422 = 0;
                }
            }
            if ((unit->flags & 0x20) == 0) {
                float v = unit->unknown_380 - 0.011111111f;
                unit->unknown_380 = (v < 0.0f) ? 0.0f : v;
            } else {
                float v = unit->unknown_380 + 0.011111111f;
                unit->unknown_380 = (v > 1.0f) ? 1.0f : v;
            }
            if (unit->unknown_428 > 0 && --unit->unknown_428 == 0) {
                unit->unknown_424 = 0.0f;
            }

            if ((unit->unknown_28c < 1 || --unit->unknown_28c != 0 ||
                 (unit_drop_current_weapon(unit_index, 1), !unit_updates_suppressed)) &&
                unit->unknown_420 > 0 && (obj->flags & _object_at_rest_bit) != 0 &&
                --unit->unknown_420 == 0) {
                // Ghidra: (float)puVar4[0x38], a dword index -> object + 0xe0 =
                // object.body_vitality. "the unit is dead" is what selects unit_release_transient_state
                // (which clears the actor/swarm handles); an animation blend weight here
                // was a pointer-stride mis-scale.
                if (obj->body_vitality <= 0.0f) {
                    unit_release_transient_state(unit_index, 0);
                } else {
                    uint32_t flags = unit->animation_state_flags;
                    obj->vitality_flags = obj->vitality_flags & ~4u;
                    unit_refresh_targeting_flag_and_weapons(unit_index, 1); // 0x562b41: CL = 1
                    unit_set_or_test_seat_and_weapon_label(unit_index, unit_base_animation_state_names[2], 0, 1); // "stand"
                    unit_try_set_animation_state(unit_index, (~(flags >> 3) & 1) | 0x22);
                    unit->animation_state_flags = (uint16_t)(unit->animation_state_flags & ~4u);
                    if (obj->type == 0) {
                        unit_clear_ground_adjust_dirty(unit_index); // index in a register
                    }
                    unit_dispatch_reaction_animation((int32_t)unit_index, 5); // 0x562b8f: ESI unit
                }
            }
        }
    }

    if ((((Unit *)obj_tag)->unit_flags & 0x400) == 0) { // not has_no_aiming
        if ((obj->vitality_flags & _object_health_frozen_bit) == 0 && !unit_updates_suppressed) {
            if ((obj->vitality_flags & 0x400) == 0) {
                if (unit->desired_weapon_index != unit->current_weapon_index &&
                    !unit_state_is_scripted_animation(unit) &&
                    unit_get_weapon_object_index(unit_index, unit->desired_weapon_index) != (datum_index)-1 &&
                    unit_check_weapon_use_permission(unit_index,
                        unit_get_weapon_object_index(unit_index, unit->desired_weapon_index)) != 0) { // 0x562c18..0x562c31
                    unit_ready_desired_weapon(unit_index, 1);
                }
            } else {
                unit_drop_current_weapon(unit_index, 1);
            }
            if (unit->desired_grenade_index != unit->current_grenade_index &&
                !unit_state_is_scripted_animation(unit)) {
                int16_t g = (int16_t)unit_find_next_grenade_type_with_count(unit_index, unit->current_grenade_index, 0); // 0x562c64: CX = +0x31d, stack 0
                if (g != -1) {
                    unit->current_grenade_index = (int8_t)g;
                }
            }
            if (network_toggle_0087abc2 != 0 && unit->controlling_player != (datum_index)-1) {
                for (int32_t i = 0; i < 2; i++) {
                    if (unit->grenade_counts[i] < 2) {
                        unit->grenade_counts[i] = 1;
                    }
                }
                if (unit->desired_grenade_index == -1) {
                    unit->desired_grenade_index = 0;
                }
            }
            if (unit->desired_zoom_level != unit->zoom_level) {
                int8_t new_zoom = unit->desired_zoom_level;
                unit->zoom_level = new_zoom;
                if (new_zoom == -1) {
                    obj->animation_frame = 0;
                }
                uint32_t p1 = player_index_from_unit_index(unit_index);
                if (p1 != (uint32_t)-1) {
                    uint32_t p2 = player_index_from_unit_index(unit_index);
                    if (*(int16_t *)((uint8_t *)player_data->data + (p2 & 0xffff) * 0x200 + 2) != -1) {
                        datum_index weapon = unit_get_weapon_object_index(unit_index, unit->current_weapon_index); // CX = +0x2f2
                        if (weapon != (datum_index)-1) {
                            object *weapon_obj = ((object_header *)object_data->data)[weapon & 0xffff].data;
                            void *weapon_tag = tag_instances[weapon_obj->definition_tag & 0xffff].data;
                            int32_t zoom_entry = (new_zoom == -1)
                                                      ? *(int32_t *)((uint8_t *)weapon_tag + 0x4bc)
                                                      : *(int32_t *)((uint8_t *)weapon_tag + 0x4ac); // UNSURE: raw Weapon-tag fields
                            float fraction = 1.0f;
                            int16_t zoom_count = *(int16_t *)((uint8_t *)weapon_tag + 0x3da);        // UNSURE
                            if (new_zoom != -1 && zoom_count > 1) {
                                fraction = (float)new_zoom / (float)(zoom_count - 1);
                            }
                            if (zoom_entry != -1) {
                                sound_start_unspatialized(fraction);
                            }
                        }
                    }
                }
            }
        }

        // Facing/aiming/looking rotation toward the desired vectors, then the current->smoothed
        // animation-control blend and the grenade throwing-state dispatch.
        // Ghidra: (char)puVar4[0xa2] == 1, a dword index -> object + 0x288 =
        // unit_data.aiming_speed (the low byte only, as an AL test).
        float aim_rate = (unit->aiming_speed == 1) ? *(float *)((uint8_t *)obj_tag + 0x26c) : 1.0f;

        real_vector3d *aiming = &unit->aiming_vector;
        real_vector3d previous_aiming = unit->aiming_vector; // 0x562df6..0x562e20 ([ebp-0x24])
        real_vector3d *aiming_velocity = &unit->aiming_velocity;
        float turn_accel = aim_rate * *(float *)((uint8_t *)obj_tag + 0x264) * 0.033333335f;
        float turn_rate = aim_rate * *(float *)((uint8_t *)obj_tag + 0x268) * 0.0011111111f;

        if (turn_accel == 0.0f && turn_rate == 0.0f) {
            unit->aiming_vector = unit->desired_aiming_vector;
            if (unit_is_look_target_valid(unit_index)) {
                unit_clamp_direction_to_aim_or_look_bounds(unit_index, aiming, 1);
            }
            unit->aiming_velocity = *global_origin3d_pointer;
        } else if (!unit->aiming_bounds_valid) {
            // FIXED (0x562f1d): ESI aiming, EDI desired aiming, stack (&aiming velocity, [ebp-0x18] from tag +0x264,
            // [ebp-0x14] from tag +0x268); the draft passed three arguments to this five-argument function.
            vector3d_rotate_toward_with_acceleration(aiming, &unit->desired_aiming_vector, aiming_velocity, turn_accel,
                turn_rate);
        } else {
            // 0x562e9f..0x562f13: the bounds are in the unit's own frame, built into a local
            // matrix (scale 1, orientation, left = up x forward, position = global origin)
            real_matrix4x3 frame;
            frame.scale = 1.0f;
            object_get_orientation(&frame.forward, unit_index, &frame.up);
            vector3d_cross_product(&frame.left, &frame.forward, &frame.up);
            frame.position = *(real_point3d *)global_origin3d_pointer;
            vector3d_rotate_toward_bounded(aiming, aiming_velocity, &unit->aiming_bounds[0], turn_accel, turn_rate,
                                           &unit->desired_aiming_vector, &frame);
        }

        {
            // FIXED (0x562f46..0x562fa0): the angle the aim moved this tick (vector3d_angle_between, ECX the aim
            // before, EDX after) over the tag's +0x264 times 1/30, clamped to [0, 1] (NaN kept), times 255 and
            // truncated (__ftol) into +0x323; 0 when +0x264 is 0.
            float aim_change = 0.0f;
            float per_tick = *(float *)((uint8_t *)obj_tag + 0x264);

            if (per_tick != 0.0f) {
                aim_change = vector3d_angle_between_4cd4f0(&previous_aiming, aiming) / (per_tick * 0.033333335f);
                if (aim_change < 0.0f) {
                    aim_change = 0.0f;
                } else if (aim_change > 1.0f) {
                    aim_change = 1.0f;
                }
            }
            unit->unknown_323 = (int8_t)(int32_t)(aim_change * 255.0f);
        }

        float look_accel = aim_rate * *(float *)((uint8_t *)obj_tag + 0x270) * 0.033333335f;
        float look_rate = aim_rate * *(float *)((uint8_t *)obj_tag + 0x274) * 0.0011111111f;
        if (look_accel == 0.0f && look_rate == 0.0f) {
            unit->looking_vector = unit->desired_looking_vector;
            unit_clamp_direction_to_aim_or_look_bounds(unit_index, &unit->looking_vector, 0);
            unit->looking_velocity = *global_origin3d_pointer;
        } else if (!unit->looking_bounds_valid) {
            // FIXED (0x5630cc): ESI looking, EDI desired looking, stack (&looking velocity, [ebp-0x14] from tag
            // +0x270, [ebp-0x18] from tag +0x274).
            vector3d_rotate_toward_with_acceleration(&unit->looking_vector, &unit->desired_looking_vector,
                &unit->looking_velocity, look_accel, look_rate);
        } else {
            // 0x563063..0x5630c2: the same local frame as the aiming branch
            real_matrix4x3 frame;
            frame.scale = 1.0f;
            object_get_orientation(&frame.forward, unit_index, &frame.up);
            vector3d_cross_product(&frame.left, &frame.forward, &frame.up);
            frame.position = *(real_point3d *)global_origin3d_pointer;
            vector3d_rotate_toward_bounded(&unit->looking_vector, &unit->looking_velocity, &unit->looking_bounds[0],
                         look_accel, look_rate, &unit->desired_looking_vector, &frame);
        }

        if (!unit_updates_suppressed) {
            uint32_t grenade_action = unit->control_flags >> 13;
            switch (unit->throwing_grenade_state) {
            case _unit_throwing_grenade_state_none:
                if ((grenade_action & 1) != 0) {
                    unit_begin_throw_grenade(unit_index, 0); // 0x56311a: EDI unit
                }
                break;
            case _unit_throwing_grenade_state_begin:
                if (obj->animation_frame > 1) {
                    unit_throw_grenade_move_to_hand(unit_index);
                }
                break;
            case _unit_throwing_grenade_state_in_hand:
                unit->throwing_grenade_counter = unit->throwing_grenade_counter + 1;
                if (unit->animation_state != _unit_animation_state_throwing_grenade) {
                    unit_release_thrown_grenade(unit_index, 1);
                }
                break;
            case _unit_throwing_grenade_state_released:
                if (unit->animation_state != _unit_animation_state_throwing_grenade &&
                    (grenade_action & 1) == 0) {
                    unit->throwing_grenade_state = (int8_t)(grenade_action & 1);
                }
                break;
            }
        }

        if (unit->current_weapon_index != -1 && !unit_updates_suppressed) {
            float trigger = unit->primary_trigger;
            uint32_t item_flags = 0;
            if (unit->current_weapon_index == unit->desired_weapon_index) {
                // recent_grenade mirrors the throw-countdown block at the top of this function
                // (unit->unknown_210/unknown_214, "local_5" in the original).
                uint8_t recent_grenade = unit->unknown_210 > 0 && (unit->unknown_214 & 0x800) != 0;
                if ((is_grounded != 0) && (unit->control_flags & 0x10) != 0) {
                    item_flags = 1;
                }
                if ((unit->control_flags & 0x800) != 0) {
                    item_flags |= 2;
                }
                if ((unit->control_flags & 0x1000) != 0) {
                    item_flags |= 4;
                }
                if ((((Unit *)obj_tag)->unit_flags & 0x800000) != 0) {
                    // 0x563214..0x56323a: the current weapon (EAX unit, CX +0x2f2) gets the ready timer +0x340
                    weapon_set_ready_timer(unit_get_weapon_object_index(unit_index, unit->current_weapon_index),
                        *(real *)&unit->unknown_340);
                }
                if ((unit->control_flags & 0x400) != 0) {
                    item_flags |= 8;
                }
                if (unit_state_is_scripted_animation(unit) && !recent_grenade) {
                    item_flags |= 0x10;
                }
                if (obj->type == 0) {
                    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
                    if (biped->unknown_505 > 0) {
                        item_flags |= 0x10;
                    }
                }
                if (unit->zoom_level != -1) {
                    item_flags |= 0x40;
                }
            } else {
                item_flags = 0x20;
            }
            // 0x5632a3..0x5632c2: EAX = the weapon in the current slot (+0x2f8[+0x2f2]) or -1.
            weapon_set_control_flags(unit->current_weapon_index == -1 ? k_datum_index_none :
                unit->weapons[unit->current_weapon_index], (uint16_t)item_flags, trigger);
        }
    }

    if ((((Unit *)obj_tag)->unit_flags & 0x800) == 0) { // not simple_creature
        if ((unit->animation_state_flags & _unit_animation_flag_aiming_enabled) != 0) {
            unit_update_look_delta_controls(unit_index);
            unit->animation_controls_smoothed[0] = unit->animation_controls_smoothed[0] * 0.7f + unit->animation_controls[0] * 0.3f;
            unit->animation_controls_smoothed[1] = unit->animation_controls_smoothed[1] * 0.7f + unit->animation_controls[1] * 0.3f;
            unit->animation_controls_smoothed[2] = unit->animation_controls_smoothed[2] * 0.7f + unit->animation_controls[2] * 0.3f;
        }

        // Unit.powered_seats (tag +0x2cc/+0x2d0, 0x44 each; 0x56334d): seat 0 (driver) is powered while
        // the unit has a driver or flag bit 0 is set, seat 1 (gunner) while it has a gunner who is not
        // the driver. The per-seat power level is the float run at object +0x338 (0x5633ae), ramped
        // over the seat's powerup time (+0x04) or powerdown time (+0x08) in seconds.
        Unit *unit_tag = (Unit *)obj_tag;
        int32_t contact_count = (int32_t)unit_tag->powered_seats.count;
        for (int32_t i = 0; i < contact_count; i++) {
            uint8_t *entry = (uint8_t *)unit_tag->powered_seats.pointer + i * 0x44;
            uint8_t active;
            if (i == 0) {
                active = (unit->driver_unit_index != (datum_index)-1) || (unit->flags & 1) != 0;
            } else {
                active = (unit->gunner_unit_index != (datum_index)-1) && unit->gunner_unit_index != unit->driver_unit_index;
            }
            float *wear = (float *)((uint8_t *)obj + 0x338 + i * 4);
            if ((obj->vitality_flags & _object_health_frozen_bit) == 0 && active) {
                if (*(uint32_t *)wear != 0x3f800000) {
                    float v = *wear + 1.0f / (*(float *)(entry + 4) * 30.0f);
                    *wear = v;
                    if (v > 1.0f) {
                        *wear = 1.0f;
                    }
                }
            } else if (*wear != 0.0f) {
                float v = *wear - 1.0f / (*(float *)(entry + 8) * 30.0f);
                *wear = v;
                if (v < 0.0f) {
                    *wear = 0.0f;
                }
            }
        }
    }

    if (unit->unknown_406 > 0 && --unit->unknown_406 == 0) {
        actor_react_to_threat_event(unit_index, unit->unknown_40c, unit->unknown_404, unit->unknown_408, 0, 1);
        unit->unknown_404 = 0;
        unit->unknown_40c = (datum_index)-1;
        unit->unknown_408 = 0.0f;
    }

    if (!unit_updates_suppressed) {
        unit_melee_lunge_damage_tick(unit_index);
        if (!unit_updates_suppressed) {
            unit_update_animation_timers(unit_index);
            if (!unit_updates_suppressed && (won_stagger_slot || unit->controlling_player != (datum_index)-1)) {
                unit_calculate_luminosity(unit_index);
            }
        }
    }

    // Ghidra addresses this one as *(char *)((int)puVar4 + 0x28b) -- a byte offset, so
    // unit_data.unknown_28b, not the melee_damage_countdown byte at 0x28a next to it.
    if (unit->unknown_28b != 0) {
        if (unit_updates_suppressed) {
            return 1;
        }
        int8_t remaining = unit->unknown_28b - 1;
        unit->unknown_28b = remaining;
        if (remaining == 0) {
            unit_update_autoaim_interaction(unit_index);
        }
    } else if (unit_updates_suppressed) {
        return 1;
    }

    // Relaxes unit_data.animation_blend_weight (0x2e8) toward zero at no more than 0.1 per
    // tick. Ghidra spells it puVar4[0xba]: a dword index, so 0xba * 4 = 0x2e8 -- not 0x338.
    float blend_delta = -unit->animation_blend_weight;
    if (blend_delta < -0.1f) {
        blend_delta = -0.1f;
    } else if (blend_delta > 0.1f) {
        blend_delta = 0.1f;
    }
    unit->animation_blend_weight = blend_delta + unit->animation_blend_weight;

    uint32_t flags = unit->flags;
    uint8_t network_create_seen = seat_control_applied;
    if ((flags & _unit_flag_idle_turn_seeded) != 0) {
        network_create_seen = 1;
        if ((flags & _unit_flag_unknown_80000) != 0) {
            network_create_seen = seat_control_applied;
        }
        unit->flags = flags & ~(uint32_t)_unit_flag_idle_turn_seeded;
    }
    flags = unit->flags;
    if ((flags & 0x20000000) != 0) { // UNSURE: undocumented bit
        if ((flags & _unit_flag_unknown_80000) != 0) {
            network_create_seen = 1;
        }
        unit->flags = flags & ~0x20000000u;
    }

    // Ghidra: (uVar10 & 0x10) with uVar10 = puVar4[0x82] (= 0x208 control_flags), then
    // ((float)puVar4[0xd1] < 0.0 != ((float)puVar4[0xd1] == 0.0)) -- the MSVC 7.1 spelling of
    // a plain `x < 0.0f` on unit_data.unknown_344 (0xd1 * 4 = 0x344). Neither operand is 0x338,
    // and the test is "negative", not "non-zero".
    if ((unit->control_flags & 0x10) != 0 || unit->unknown_344 < 0.0f || network_create_seen) {
        if (!is_grounded) {
            if ((unit->flags & _unit_flag_unknown_4000000) != 0) {
                unit->flags = unit->flags & ~(uint32_t)_unit_flag_unknown_4000000;
            }
            if ((unit->flags & _unit_flag_unknown_80000) == 0) {
                goto skip_luma_toggle;
            }
            unit->flags = (unit->flags & ~(uint32_t)_unit_flag_unknown_80000) | 0x10;
        } else {
            if (unit_current_weapon_has_flag(unit_index)) {
                if ((unit->control_flags & 0x10) != 0) {
                    int32_t player_effect;
                    if ((unit->flags & _unit_flag_unknown_4000000) == 0) {
                        player_effect = *(int32_t *)(*(uint8_t **)(globals_tag_data + 0x180) + 0x54);
                    } else {
                        player_effect = *(int32_t *)(*(uint8_t **)(globals_tag_data + 0x180) + 100);
                    }
                    if (player_effect != -1) {
                        // 0x56364e..0x56365c: EAX = the unit, ECX = player_effect, stack: unit, -1, 0..
                        effect_new_on_object(unit_index, (datum_index)player_effect, unit_index, -1, 0.0f, 0.0f,
                            0, 0);
                    }
                    unit->flags = unit->flags ^ _unit_flag_unknown_4000000;
                }
                if ((unit->control_flags & 0x10) != 0) {
                    goto skip_luma_toggle;
                }
            }
            if (((unit->flags & _unit_flag_unknown_80000) == 0 && unit->unknown_344 <= 0.2f) ||
                obj->parent_object != (datum_index)-1) {
                goto skip_luma_toggle;
            }
            // 0x56369b..0x5636b2: EAX = the unit, ECX = the Unit tag's +0x194 effect
            effect_new_on_object(unit_index, *(datum_index *)((uint8_t *)obj_tag + 0x194), unit_index, -1,
                0.0f, 0.0f, 0, 0);
            unit->flags = unit->flags ^ _unit_flag_unknown_80000;
        }
    }

skip_luma_toggle:
    if ((unit->flags & _unit_flag_unknown_80000) == 0) {
        if (unit->unknown_344 < 1.0f) {
            unit->unknown_344 = unit->unknown_344 + 0.0011111111f;
        }
        if (unit->unknown_340 != 0.0f) {
            float v = unit->unknown_340 - 0.041666668f;
            unit->unknown_340 = (v < 0.0f) ? 0.0f : v;
        }
    } else {
        if ((((Unit *)obj_tag)->unit_flags & 0x1000000) == 0) {
            unit->unknown_344 = unit->unknown_344 - 0.00027777778f;
        }
        if (obj->parent_object != (datum_index)-1 || (obj->vitality_flags & _object_health_frozen_bit) != 0) {
            unit->flags = unit->flags & ~(uint32_t)_unit_flag_unknown_80000;
        }
        if (unit->unknown_340 != 1.0f) {
            float v = unit->unknown_340 + 0.16666667f;
            unit->unknown_340 = (v > 1.0f) ? 1.0f : v;
        }
    }

    if (unit_current_weapon_has_flag(unit_index)) {
        if ((unit->flags & _unit_flag_unknown_4000000) == 0) {
            if (unit->unknown_348 != 0.0f) {
                float v = unit->unknown_348 - 0.041666668f;
                unit->unknown_348 = (v < 0.0f) ? 0.0f : v;
            }
        } else if (unit->unknown_348 != 1.0f) {
            float v = unit->unknown_348 + 0.083333336f;
            if (v > 1.0f) {
                unit->unknown_348 = 1.0f;
                return 1;
            }
            unit->unknown_348 = v;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x5625b0):

undefined4 FUN_005625b0(uint param_1)

{
  uint *puVar1;
  float fVar2;
  float fVar3;
  uint *puVar4;
  bool bVar5;
  undefined *puVar6;
  char cVar7;
  undefined1 uVar8;
  short sVar9;
  uint uVar10;
  char *pcVar11;
  int iVar12;
  uint uVar13;
  int iVar14;
  int iVar15;
  undefined4 *puVar16;
  undefined1 local_48 [12];
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  uint local_28;
  uint local_24;
  uint local_20;
  float local_1c;
  float local_18;
  int local_14;
  float local_10;
  int local_c;
  char local_8;
  char local_7;
  char local_6;
  char local_5;

  local_14 = (param_1 & 0xffff) * 0xc;
  puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_14);
  local_c = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_6 = '\0';
  local_7 = '\0';
  local_8 = FUN_00466b60(param_1);
  if (DAT_0071c419 == '\0') {
    *(short *)(puVar4 + 0x83) = (short)puVar4[0x83] + 1;
    sVar9 = (short)puVar4[0x83];
    if (((char)DAT_006ef910[2] == '\0') && (*DAT_006ef910 < sVar9)) {
      *(undefined1 *)(DAT_006ef910 + 2) = 1;
      local_6 = '\x01';
      *(undefined2 *)(puVar4 + 0x83) = 0;
    }
    else {
      if (sVar9 < DAT_006ef910[1]) {
        sVar9 = DAT_006ef910[1];
      }
      DAT_006ef910[1] = sVar9;
    }
  }
  if ((puVar4[0x81] & 0x2000000) == 0) {
    if ((puVar4[0x81] & 1) == 0) {
      puVar1 = puVar4 + 0x1d;
      puVar4[0x95] = *puVar1;
      puVar4[0x96] = puVar4[0x1e];
      puVar4[0x97] = puVar4[0x1f];
      puVar4[0x8c] = *puVar1;
      puVar4[0x8d] = puVar4[0x1e];
      puVar4[0x8e] = puVar4[0x1f];
      puVar4[0x89] = *puVar1;
      puVar4[0x8a] = puVar4[0x1e];
      puVar6 = PTR_DAT_00696714;
      puVar4[0x8b] = puVar4[0x1f];
      puVar4[0x9e] = *(uint *)puVar6;
      puVar4[0x9f] = *(uint *)(puVar6 + 4);
      uVar10 = *(uint *)(puVar6 + 8);
      puVar4[0x82] = 0;
      goto LAB_0056272d;
    }
  }
  else {
    FUN_00570840();
    puVar4[0x8c] = puVar4[0x89];
    puVar4[0x8d] = puVar4[0x8a];
    puVar4[0x8e] = puVar4[0x8b];
    puVar4[0x95] = puVar4[0x89];
    puVar4[0x96] = puVar4[0x8a];
    puVar4[0x97] = puVar4[0x8b];
    puVar6 = PTR_DAT_00696718;
    puVar4[0x9e] = *(uint *)PTR_DAT_00696718;
    puVar4[0x9f] = *(uint *)(puVar6 + 4);
    uVar10 = *(uint *)(puVar6 + 8);
    puVar4[0x82] = 0;
LAB_0056272d:
    puVar4[0xa0] = uVar10;
  }
  if ((*(uint *)(local_c + 0x17c) & 0x800) == 0) {
    uVar10 = puVar4[0x84];
    local_5 = '\0';
    if (0 < (int)uVar10) {
      uVar13 = puVar4[0x82] | puVar4[0x85];
      puVar4[0x82] = uVar13;
      if ((puVar4[0x85] & 0x800) == 0) {
        puVar4[0xa1] = 0;
      }
      else {
        if ((int)uVar10 % 7 == 0) {
          uVar13 = uVar13 | 0x800;
        }
        else {
          uVar13 = uVar13 & 0xfffff7ff;
        }
        puVar4[0x82] = uVar13;
        puVar4[0xa1] = 0x3f800000;
      }
      puVar4[0x84] = uVar10 - 1;
      if (uVar10 - 1 == 0) {
        puVar4[0x85] = 0;
      }
    }
    if ((puVar4[0x81] & 0x8000000) == 0) {
      if ((puVar4[0xc9] != 0xffffffff) && ((*(byte *)((int)puVar4 + 0x106) & 4) == 0)) {
        iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar4[0xc9] & 0xffff) * 0xc);
        *(undefined2 *)(puVar4 + 0x2e) = *(undefined2 *)(iVar14 + 0xb8);
        local_5 = '\x01';
        if ((*(int *)(iVar14 + 0x218) != -1) ||
           ((*(char *)(iVar14 + 0x2a3) != '\x1b' && (*(char *)(iVar14 + 0x2a3) != '\x1a')))) {
          puVar4[0x82] = puVar4[0x82] | *(uint *)(iVar14 + 0x208) & 0x3f;
          puVar4[0x89] = *(uint *)(iVar14 + 0x224);
          puVar4[0x8a] = *(uint *)(iVar14 + 0x228);
          puVar4[0x8b] = *(uint *)(iVar14 + 0x22c);
          puVar4[0x9e] = *(uint *)(iVar14 + 0x278);
          puVar4[0x9f] = *(uint *)(iVar14 + 0x27c);
          puVar4[0xa0] = *(uint *)(iVar14 + 0x280);
        }
      }
      if ((puVar4[0xca] != 0xffffffff) && ((*(byte *)((int)puVar4 + 0x106) & 4) == 0)) {
        iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar4[0xca] & 0xffff) * 0xc);
        if (local_5 == '\0') {
          *(undefined2 *)(puVar4 + 0x2e) = *(undefined2 *)(iVar14 + 0xb8);
        }
        if ((*(int *)(iVar14 + 0x218) != -1) ||
           ((*(char *)(iVar14 + 0x2a3) != '\x1b' && (*(char *)(iVar14 + 0x2a3) != '\x1a')))) {
          puVar4[0x8c] = *(uint *)(iVar14 + 0x230);
          puVar4[0x8d] = *(uint *)(iVar14 + 0x234);
          puVar4[0x8e] = *(uint *)(iVar14 + 0x238);
          puVar4[0x95] = *(uint *)(iVar14 + 0x230);
          puVar4[0x96] = *(uint *)(iVar14 + 0x234);
          puVar4[0x97] = *(uint *)(iVar14 + 0x238);
          puVar4[0x82] = puVar4[0x82] | *(uint *)(iVar14 + 0x208) & 0x7c00;
          puVar4[0xa1] = *(uint *)(iVar14 + 0x284);
        }
      }
      if ((puVar4[0x82] & 0x7c00) == 0) {
        if (*(char *)((int)puVar4 + 0x322) < '\x7f') {
          *(char *)((int)puVar4 + 0x322) = *(char *)((int)puVar4 + 0x322) + '\x01';
        }
      }
      else {
        *(undefined1 *)((int)puVar4 + 0x322) = 0;
      }
    }
    if (DAT_0071c419 == '\0') {
      if ((puVar4[0x81] & 0x10) == 0) {
        fVar2 = (float)puVar4[0xdf];
        puVar4[0xdf] = (uint)(fVar2 - 0.008333334);
        if (fVar2 - 0.008333334 < 0.0) {
          puVar4[0xdf] = 0;
        }
      }
      else {
        if (((DAT_006f1d20 == 0) || (*(short *)((int)puVar4 + 0x422) == 0)) ||
           (*(short *)((int)puVar4 + 0x422) != 1)) {
LAB_005629e9:
          fVar2 = 0.008333334;
        }
        else {
          iVar14 = *(int *)(DAT_008603b0 + 0x34);
          uVar10 = unit_get_weapon_object_index();
          if ((uVar10 == 0xffffffff) ||
             (iVar14 = *(int *)((**(uint **)(iVar14 + 8 + (uVar10 & 0xffff) * 0xc) & 0xffff) * 0x20
                                + 0x14 + DAT_0087bc14), *(float *)(iVar14 + 0x4d0) == 0.0))
          goto LAB_005629e9;
          fVar2 = *(float *)(iVar14 + 0x4d0);
        }
        fVar3 = (float)puVar4[0xdf];
        puVar4[0xdf] = (uint)(fVar2 + fVar3);
        if (1.0 < fVar2 + fVar3) {
          puVar4[0xdf] = 0x3f800000;
          *(undefined2 *)((int)puVar4 + 0x422) = 0;
        }
      }
      if ((puVar4[0x81] & 0x20) == 0) {
        fVar2 = (float)puVar4[0xe0] - 0.011111111;
        puVar4[0xe0] = (uint)fVar2;
        if (fVar2 < 0.0) {
          puVar4[0xe0] = 0;
        }
      }
      else {
        fVar2 = (float)puVar4[0xe0] + 0.011111111;
        puVar4[0xe0] = (uint)fVar2;
        if (1.0 < fVar2) {
          puVar4[0xe0] = 0x3f800000;
        }
      }
      if ((0 < (short)puVar4[0x10a]) &&
         (sVar9 = (short)puVar4[0x10a] + -1, *(short *)(puVar4 + 0x10a) = sVar9, sVar9 == 0)) {
        puVar4[0x109] = 0;
      }
      if ((((((char)puVar4[0xa3] < '\x01') ||
            (cVar7 = (char)puVar4[0xa3] + -1, *(char *)(puVar4 + 0xa3) = cVar7, cVar7 != '\0')) ||
           (unit_drop_current_weapon(param_1,1), DAT_0071c419 == '\0')) &&
          ((0 < (short)puVar4[0x108] && ((puVar4[4] & 0x20) != 0)))) &&
         (sVar9 = (short)puVar4[0x108] + -1, *(short *)(puVar4 + 0x108) = sVar9, sVar9 == 0)) {
        if ((float)puVar4[0x38] <= 0.0) {
          FUN_00568610(param_1,0);
        }
        else {
          uVar10 = puVar4[0xa6];
          *(ushort *)((int)puVar4 + 0x106) = *(ushort *)((int)puVar4 + 0x106) & 0xfffb;
          FUN_00569bf0(param_1);
          unit_set_or_test_seat_and_weapon_label(PTR_s_stand_0069fdec,0,1);
          unit_try_set_animation_state(param_1,~((byte)uVar10 >> 3) & 1 | 0x22);
          *(ushort *)(puVar4 + 0xa6) = (ushort)puVar4[0xa6] & 0xfffb;
          if ((short)puVar4[0x2d] == 0) {
            FUN_0055ad70();
          }
          FUN_005614a0(5);
        }
      }
    }
  }
  if ((*(uint *)(local_c + 0x17c) & 0x400) == 0) {
    if (((*(ushort *)((int)puVar4 + 0x106) & 4) == 0) && (DAT_0071c419 == '\0')) {
      if ((*(ushort *)((int)puVar4 + 0x106) & 0x400) == 0) {
        if (((((short)puVar4[0xbd] != *(short *)((int)puVar4 + 0x2f2)) &&
             (cVar7 = FUN_00565c60(), cVar7 == '\0')) &&
            (iVar14 = unit_get_weapon_object_index(), iVar14 != -1)) &&
           (cVar7 = FUN_0056da00(), cVar7 != '\0')) {
          unit_ready_desired_weapon(param_1,1);
        }
      }
      else {
        unit_drop_current_weapon(param_1,1);
      }
      if (((*(char *)((int)puVar4 + 0x31d) != (char)puVar4[199]) &&
          (cVar7 = FUN_00565c60(), cVar7 == '\0')) && (sVar9 = FUN_005699a0(0), sVar9 != -1)) {
        *(char *)(puVar4 + 199) = (char)sVar9;
      }
      if ((DAT_0087abc2 != '\0') && (puVar4[0x86] != 0xffffffff)) {
        pcVar11 = (char *)((int)puVar4 + 0x31e);
        iVar14 = 2;
        do {
          cVar7 = *pcVar11;
          if (cVar7 < '\x02') {
            cVar7 = '\x01';
          }
          *pcVar11 = cVar7;
          pcVar11 = pcVar11 + 1;
          iVar14 = iVar14 + -1;
        } while (iVar14 != 0);
        if (*(char *)((int)puVar4 + 0x31d) == -1) {
          *(undefined1 *)((int)puVar4 + 0x31d) = 0;
        }
      }
      cVar7 = *(char *)((int)puVar4 + 0x321);
      if (cVar7 != (char)puVar4[200]) {
        *(char *)(puVar4 + 200) = cVar7;
        if (cVar7 == -1) {
          puVar4[0xd2] = 0;
        }
        iVar14 = FUN_00474db0(param_1);
        if ((iVar14 != -1) &&
           (uVar10 = FUN_00474db0(param_1),
           *(short *)((uVar10 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1)) {
          iVar14 = *(int *)(DAT_008603b0 + 0x34);
          uVar10 = unit_get_weapon_object_index();
          if (uVar10 != 0xffffffff) {
            iVar14 = *(int *)((**(uint **)(iVar14 + 8 + (uVar10 & 0xffff) * 0xc) & 0xffff) * 0x20 +
                              0x14 + DAT_0087bc14);
            cVar7 = (char)puVar4[200];
            if (cVar7 == -1) {
              iVar12 = *(int *)(iVar14 + 0x4bc);
            }
            else {
              iVar12 = *(int *)(iVar14 + 0x4ac);
            }
            local_1c = 1.0;
            if ((cVar7 != -1) && (1 < *(short *)(iVar14 + 0x3da))) {
              local_1c = (float)(int)cVar7 / (float)(*(short *)(iVar14 + 0x3da) + -1);
            }
            if (iVar12 != -1) {
              FUN_00543dd0(local_1c);
            }
          }
        }
      }
    }
    if ((char)puVar4[0xa2] == '\x01') {
      local_10 = *(float *)(local_c + 0x26c);
    }
    else {
      local_10 = 1.0;
    }
    puVar1 = puVar4 + 0x8f;
    local_1c = local_10 * *(float *)(local_c + 0x264) * 0.033333335;
    local_28 = *puVar1;
    local_24 = puVar4[0x90];
    local_20 = puVar4[0x91];
    local_18 = local_10 * *(float *)(local_c + 0x268) * 0.0011111111;
    if ((local_1c == 0.0) && (local_18 == 0.0)) {
      *puVar1 = puVar4[0x8c];
      puVar4[0x90] = puVar4[0x8d];
      puVar4[0x91] = puVar4[0x8e];
      cVar7 = FUN_00562570();
      if (cVar7 != '\0') {
        FUN_005697a0(puVar1,1);
      }
      puVar6 = PTR_DAT_00696714;
      puVar4[0x92] = *(uint *)PTR_DAT_00696714;
      puVar4[0x93] = *(uint *)(puVar6 + 4);
      puVar4[0x94] = *(uint *)(puVar6 + 8);
      puVar16 = (undefined4 *)PTR_DAT_006966f8;
    }
    else if (*(char *)((int)puVar4 + 0x2b6) == '\0') {
      vector3d_rotate_toward_with_acceleration(puVar4 + 0x92,local_1c,local_18);
      puVar16 = (undefined4 *)PTR_DAT_006966f8;
    }
    else {
      object_get_orientation(local_48);
      vector3d_cross_product(local_48);
      puVar16 = (undefined4 *)PTR_DAT_006966f8;
      local_3c = *(undefined4 *)PTR_DAT_006966f8;
      local_38 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
      local_34 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
      FUN_00564ae0(puVar4 + 0x8f,puVar4 + 0x92,puVar4 + 0xae,local_1c,local_18);
    }
    iVar14 = local_c;
    if (*(float *)(local_c + 0x264) != 0.0) {
      vector3d_angle_between_4cd4f0();
    }
    uVar8 = __ftol();
    *(undefined1 *)((int)puVar4 + 0x323) = uVar8;
    local_18 = local_10 * *(float *)(iVar14 + 0x270) * 0.033333335;
    local_1c = local_10 * *(float *)(iVar14 + 0x274) * 0.0011111111;
    if ((local_18 == 0.0) && (local_1c == 0.0)) {
      puVar4[0x98] = puVar4[0x95];
      puVar4[0x99] = puVar4[0x96];
      puVar4[0x9a] = puVar4[0x97];
      FUN_005697a0(puVar4 + 0x98,0);
      puVar6 = PTR_DAT_00696714;
      puVar4[0x9b] = *(uint *)PTR_DAT_00696714;
      puVar4[0x9c] = *(uint *)(puVar6 + 4);
      puVar4[0x9d] = *(uint *)(puVar6 + 8);
    }
    else if (*(char *)((int)puVar4 + 0x2b7) == '\0') {
      vector3d_rotate_toward_with_acceleration(puVar4 + 0x9b,local_18,local_1c);
    }
    else {
      object_get_orientation(local_48);
      vector3d_cross_product(local_48);
      local_3c = *puVar16;
      local_38 = puVar16[1];
      local_34 = puVar16[2];
      FUN_00564ae0(puVar4 + 0x98,puVar4 + 0x9b,puVar4 + 0xb2,local_18,local_1c);
    }
    if (DAT_0071c419 == '\0') {
      uVar10 = puVar4[0x82] >> 0xd;
      switch(*(undefined1 *)((int)puVar4 + 0x28d)) {
      case 0:
        if ((uVar10 & 1) != 0) {
          unit_begin_throw_grenade(0);
        }
        break;
      case 1:
        if (1 < *(short *)((int)puVar4 + 0xd2)) {
          unit_throw_grenade_move_to_hand(param_1);
        }
        break;
      case 2:
        *(short *)((int)puVar4 + 0x28e) = *(short *)((int)puVar4 + 0x28e) + 1;
        if (*(char *)((int)puVar4 + 0x2a3) != '!') {
          unit_release_thrown_grenade(param_1,1);
        }
        break;
      case 3:
        if ((*(char *)((int)puVar4 + 0x2a3) != '!') && ((uVar10 & 1) == 0)) {
          *(byte *)((int)puVar4 + 0x28d) = (byte)uVar10 & 1;
        }
      }
    }
    if ((*(short *)((int)puVar4 + 0x2f2) != -1) && (DAT_0071c419 == '\0')) {
      local_1c = (float)puVar4[0xa1];
      uVar10 = 0;
      if (*(short *)((int)puVar4 + 0x2f2) == (short)puVar4[0xbd]) {
        if (((int)puVar4[0x84] < 1) || (local_5 = '\x01', (puVar4[0x85] & 0x800) == 0)) {
          local_5 = '\0';
        }
        if ((local_8 != '\0') && ((puVar4[0x82] & 0x10) != 0)) {
          uVar10 = 1;
        }
        if ((puVar4[0x82] & 0x800) != 0) {
          uVar10 = uVar10 | 2;
        }
        if ((puVar4[0x82] & 0x1000) != 0) {
          uVar10 = uVar10 | 4;
        }
        if ((*(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x17c) & 0x800000
            ) != 0) {
          unit_get_weapon_object_index(puVar4[0xd0]);
          FUN_004c2b20();
        }
        if ((puVar4[0x82] & 0x400) != 0) {
          uVar10 = uVar10 | 8;
        }
        cVar7 = FUN_00565c60();
        if ((cVar7 != '\0') && (local_5 == '\0')) {
          uVar10 = uVar10 | 0x10;
        }
        if (((short)puVar4[0x2d] == 0) && ('\0' < *(char *)((int)puVar4 + 0x505))) {
          uVar10 = uVar10 | 0x10;
        }
        if ((char)puVar4[200] != -1) {
          uVar10 = uVar10 | 0x40;
        }
      }
      else {
        uVar10 = 0x20;
      }
      item_set_permutation(uVar10,local_1c);
    }
  }
  iVar14 = local_c;
  if ((*(uint *)(local_c + 0x17c) & 0x800) == 0) {
    if ((puVar4[0xa6] & 2) != 0) {
      FUN_0056e820();
      puVar4[0xd9] = (uint)((float)puVar4[0xd9] * 0.7 + (float)puVar4[0xdc] * 0.3);
      puVar4[0xda] = (uint)((float)puVar4[0xda] * 0.7 + (float)puVar4[0xdd] * 0.3);
      puVar4[0xdb] = (uint)((float)puVar4[0xdb] * 0.7 + (float)puVar4[0xde] * 0.3);
    }
    sVar9 = 0;
    if (0 < *(int *)(iVar14 + 0x2cc)) {
      iVar12 = 0;
      do {
        iVar15 = iVar12 * 0x44 + *(int *)(iVar14 + 0x2d0);
        if (sVar9 == 0) {
          if ((puVar4[0xc9] == 0xffffffff) && ((puVar4[0x81] & 1) == 0)) {
LAB_0056339f:
            bVar5 = false;
          }
          else {
            bVar5 = true;
          }
        }
        else {
          if ((puVar4[0xca] == 0xffffffff) || (puVar4[0xca] == puVar4[0xc9])) goto LAB_0056339f;
          bVar5 = true;
        }
        if (((*(byte *)((int)puVar4 + 0x106) & 4) == 0) && (bVar5)) {
          if ((puVar4[iVar12 + 0xce] != 0x3f800000) &&
             (fVar2 = 1.0 / (*(float *)(iVar15 + 4) * 30.0) + (float)puVar4[iVar12 + 0xce],
             puVar4[iVar12 + 0xce] = (uint)fVar2, 1.0 < fVar2)) {
            puVar4[iVar12 + 0xce] = 0x3f800000;
          }
        }
        else if (((float)puVar4[iVar12 + 0xce] != 0.0) &&
                (fVar2 = (float)puVar4[iVar12 + 0xce] - 1.0 / (*(float *)(iVar15 + 8) * 30.0),
                puVar4[iVar12 + 0xce] = (uint)fVar2, fVar2 < 0.0)) {
          puVar4[iVar12 + 0xce] = 0;
        }
        sVar9 = sVar9 + 1;
        iVar12 = (int)sVar9;
      } while (iVar12 < *(int *)(iVar14 + 0x2cc));
    }
  }
  if ((0 < *(short *)((int)puVar4 + 0x406)) &&
     (sVar9 = *(short *)((int)puVar4 + 0x406) + -1, *(short *)((int)puVar4 + 0x406) = sVar9,
     sVar9 == 0)) {
    FUN_0042be40(param_1,puVar4[0x103],(short)puVar4[0x101],puVar4[0x102],0,1);
    *(undefined2 *)(puVar4 + 0x101) = 0;
    puVar4[0x103] = 0xffffffff;
    puVar4[0x102] = 0;
  }
  if ((((DAT_0071c419 == '\0') && (FUN_0056fc80(param_1), DAT_0071c419 == '\0')) &&
      (FUN_00561620(), DAT_0071c419 == '\0')) && ((local_6 != '\0' || (puVar4[0x86] != 0xffffffff)))
     ) {
    unit_calculate_luminosity();
  }
  if (*(char *)((int)puVar4 + 0x28b) == '\0') {
LAB_00563524:
    if (DAT_0071c419 != '\0') {
      return 1;
    }
  }
  else {
    if (DAT_0071c419 != '\0') {
      return 1;
    }
    cVar7 = *(char *)((int)puVar4 + 0x28b) + -1;
    *(char *)((int)puVar4 + 0x28b) = cVar7;
    if (cVar7 == '\0') {
      FUN_00570720(param_1);
      goto LAB_00563524;
    }
  }
  fVar2 = -(float)puVar4[0xba];
  if (-0.1 <= fVar2) {
    if (0.1 < fVar2) {
      fVar2 = 0.1;
    }
  }
  else {
    fVar2 = -0.1;
  }
  puVar4[0xba] = (uint)(fVar2 + (float)puVar4[0xba]);
  uVar10 = puVar4[0x81];
  cVar7 = local_7;
  if ((uVar10 & 0x10000000) != 0) {
    cVar7 = '\x01';
    if ((uVar10 & 0x80000) != 0) {
      cVar7 = local_7;
    }
    puVar4[0x81] = uVar10 & 0xefffffff;
  }
  uVar10 = puVar4[0x81];
  if ((uVar10 & 0x20000000) != 0) {
    if ((uVar10 & 0x80000) != 0) {
      cVar7 = '\x01';
    }
    puVar4[0x81] = uVar10 & 0xdfffffff;
  }
  uVar10 = puVar4[0x82];
  if ((((uVar10 & 0x10) != 0) || ((float)puVar4[0xd1] < 0.0 != ((float)puVar4[0xd1] == 0.0))) ||
     (cVar7 != '\0')) {
    if (local_8 == '\0') {
      if ((puVar4[0x81] & 0x4000000) != 0) {
        puVar4[0x81] = puVar4[0x81] & 0xfbffffff;
      }
      if ((puVar4[0x81] & 0x80000) == 0) goto LAB_005636c8;
      uVar10 = puVar4[0x81] & 0xfff7ffff | 0x10;
    }
    else {
      cVar7 = FUN_00565b60();
      if (cVar7 != '\0') {
        if ((uVar10 & 0x10) != 0) {
          if ((puVar4[0x81] & 0x4000000) == 0) {
            iVar14 = *(int *)(*(int *)(DAT_00746fa0 + 0x180) + 0x54);
          }
          else {
            iVar14 = *(int *)(*(int *)(DAT_00746fa0 + 0x180) + 100);
          }
          if (iVar14 != -1) {
            FUN_004507a0(param_1,0xffffffff,0,0,0,0);
          }
          puVar4[0x81] = puVar4[0x81] ^ 0x4000000;
        }
        if ((puVar4[0x82] & 0x10) != 0) goto LAB_005636c8;
      }
      if ((((puVar4[0x81] & 0x80000) == 0) && ((float)puVar4[0xd1] <= 0.2)) ||
         (puVar4[0x47] != 0xffffffff)) goto LAB_005636c8;
      FUN_004507a0(param_1,0xffffffff,0,0,0,0);
      uVar10 = puVar4[0x81] ^ 0x80000;
    }
    puVar4[0x81] = uVar10;
  }
LAB_005636c8:
  if ((puVar4[0x81] & 0x80000) == 0) {
    if ((float)puVar4[0xd1] < 1.0) {
      puVar4[0xd1] = (uint)((float)puVar4[0xd1] + 0.0011111111);
    }
    if (((float)puVar4[0xd0] != 0.0) &&
       (fVar2 = (float)puVar4[0xd0], puVar4[0xd0] = (uint)(fVar2 - 0.041666668),
       fVar2 - 0.041666668 < 0.0)) {
      puVar4[0xd0] = 0;
    }
  }
  else {
    if ((*(uint *)(local_c + 0x17c) & 0x1000000) == 0) {
      puVar4[0xd1] = (uint)((float)puVar4[0xd1] - 0.00027777778);
    }
    if ((puVar4[0x47] != 0xffffffff) || ((*(byte *)((int)puVar4 + 0x106) & 4) != 0)) {
      puVar4[0x81] = puVar4[0x81] & 0xfff7ffff;
    }
    if ((puVar4[0xd0] != 0x3f800000) &&
       (fVar2 = (float)puVar4[0xd0], puVar4[0xd0] = (uint)(fVar2 + 0.16666667),
       1.0 < fVar2 + 0.16666667)) {
      puVar4[0xd0] = 0x3f800000;
    }
  }
  cVar7 = FUN_00565b60();
  if (cVar7 != '\0') {
    if ((puVar4[0x81] & 0x4000000) == 0) {
      if (((float)puVar4[0xd2] != 0.0) &&
         (fVar2 = (float)puVar4[0xd2], puVar4[0xd2] = (uint)(fVar2 - 0.041666668),
         fVar2 - 0.041666668 < 0.0)) {
        puVar4[0xd2] = 0;
      }
    }
    else if ((puVar4[0xd2] != 0x3f800000) &&
            (fVar2 = (float)puVar4[0xd2], puVar4[0xd2] = (uint)(fVar2 + 0.083333336),
            1.0 < fVar2 + 0.083333336)) {
      puVar4[0xd2] = 0x3f800000;
      return 1;
    }
  }
  return 1;
}
#endif
