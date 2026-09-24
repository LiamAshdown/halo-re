// projectile_update  (Ghidra: FUN_004bdc00; renamed per
// out/phase4/projectiles_types_notes.md "Renames this pass establishes". Ghidra separately
// promoted the address 0x4be1b0 to a bogus second "function" it named
// resolution_list_add_resolution; that address is NOT rewritten as its own file -- it is the
// mid-body loop of THIS function (same LAB_004be5ad / LAB_004be5c1 labels appear in both
// decompilations, 0x4be1b0 has zero callers, and it decompiles with unaff_EBX/unaff_ESI/
// unaff_EDI/in_stack_... inputs and no prologue of its own, Ghidra's signature for a promoted
// label). The pack for 0x4bdc00 already contains the complete decompiled function body,
// including everything Ghidra separately re-printed at 0x4be1b0; that second printout was used
// only to cross-check field offsets (it prints them as absolute object-relative numbers where
// this function prints `puVar3[0x8b]`-style dword indices).
// address 0x4bdc00, real size 0xd30 bytes (0x4bdc00..0x4beb30; the "size=1456" the batch metadata
// reports is the truncated boundary Ghidra used before it mis-split 0x4be1b0 off; confirmed by
// disassembling straight through to the padding int3s at 0x4beb21).
// name confidence: 0.85   rewrite confidence: 0.7 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
//   in this batch -- see the UNSURE blocks below for the guided-projectile "wander" steering,
//   the flyby-sound listener check and the two mid-loop calls to projectile_request_state whose
//   "return value" is really just whatever float was already sitting on the x87 stack)
// evidence: every projectile_data field below is established in out/phase4/projectiles_types_notes.md's
//   "projectile_data" table with this function cited as (co-)establishing it; types/projectiles.h
//   documents the same fields with the same evidence. `objdump -d -M intel bin/halo.exe` for the
//   whole 0x4bdc00..0x4beb30 range resolved every call Ghidra printed with empty parentheses
//   (hidden register arguments) and the true target of the vector3d project/normalize/rotate
//   helpers; anywhere Ghidra's own pseudo-C already showed concrete non-empty arguments for an
//   opaque out-of-range callee, that reading is trusted as-is (Ghidra's x87 comparison-to-boolean
//   resolution is reliable throughout this function; only its call-argument recovery needed
//   disassembly help).
// register convention: already a plain stack parameter in Ghidra's own output (`mov eax,[ebp+8]`
//   is the function's first real instruction), the same shape as item_update's own
//   `int __cdecl item_update(uint item_index)` -- this is the analogous projectile-row vtable
//   column (projectile row +0x34; the item row +0x34 is item_update itself, 0x4bc5c0).
// UNSURE (function-wide, same tradeoff item_update.c documents for FUN_00401a20 and friends):
//   weapon_get_zoom_fov (a difficulty/perception scalar keyed off global 0x006b0b80, called with a
//   literal stack argument of 0x13), unit_get_secondary_eye_marker_position (no visible arguments or return, called every
//   guided tick right before the two periodic_function_evaluate phase samples -- almost
//   certainly advances or reads the per-object noise phase state, not otherwise identified),
//   sound_definition_maximum_distance (EAX = tag->flyby_sound.tag_id, returns a float10 range in ST0; treated as the
//   sound's audible radius), and sound_start_at_location (the same opaque effect/sound bundle dispatcher
//   item_update.c already treats as opaque, called here with a five-vector bundle instead of
//   that function's three-vector one) are all out of this module's address range and preserved
//   as opaque calls with the exact arguments the disassembly shows.
//   Two local scratch areas are read with `+=`/by-address before this function (or, on later
//   iterations of the per-tick loop, the previous iteration) ever writes them: the wander target
//   accumulator (wander_target below) and the bsp_leaf_reference passed to
//   object_set_cluster_and_parent at the very end of a settled sub-step. Both are preserved
//   literally as ordinary uninitialized locals -- exactly what the original stack frame is --
//   rather than invented-zero-initialized, per this task's "no invented behaviour" rule. In
//   practice the tick loop runs at the same call depth every frame, so the same physical stack
//   words are likely being read back tick over tick; this is retail behaviour, not a rewrite bug.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern void *game_time_globals_006f1d6c; // 0x006f1d6c, +0x0c is the game tick
extern void *local_player_globals; // 0x0087a478
extern data_array *player_data; // 0x0087a480
extern real_point3d *global_origin3d_pointer; // 0x00696714, see src/items/item_update.c
extern uint8_t *unknown_006b0b80; // 0x006b0b80, UNSURE: some difficulty/skill globals block;
    // weapon_get_zoom_fov reads a word at +0x0e from it

extern void contrail_delete(datum_index attachment_handle); // 0x44cad0
extern void projectile_update_function_values(datum_index projectile_index); // 0x4c0250, this
    // module, out of range (>0x4bf390), not rewritten this pass
extern void projectile_request_state(datum_index projectile_index, int16_t requested_state); // 0x4bf0f0, this batch
extern uint8_t projectile_collision_test(datum_index projectile_index, real_point3d *swept_target,
    collision_result *out_hit); // 0x4c0450, this module, out of range, not rewritten this pass.
    // blam-cc: EAX -> projectile_index, EDI -> swept_target, stack -> out_hit (resolved from the
    // disassembly at the call site: `mov eax,esi; lea edi,[esp+0x6c]; push edx(&hit); call`)
extern void projectile_response(datum_index projectile_index, collision_result *hit,
    real_point3d *swept_target, real_vector3d *velocity_at_impact); // 0x4bf390, this batch (see
    // projectile_response.c). blam-cc: projectile_index/hit/swept_target on the stack (pushed in
    // that reverse order), velocity_at_impact in EAX. UNSURE: velocity_at_impact's identity --
    // resolved only as far as "a 12-byte record read as three floats and used, together with the
    // global up vector as a zero-length fallback, to build an effect coordinate system", which
    // matches types/projectiles.h's description of projectile_response's incident/reflection
    // vectors; not traced back to its exact source slot in this function's frame.
extern void ai_accumulate_repeated_event(datum_index object_index, real_point3d *origin, int32_t kind,
    ObjectNoise_t noise, int32_t param_5); // 0x42c610, opaque, out of range (plays the impact
    // noise at a world point). Five stack arguments, not four: 0x4be55f..0x4be572 pushes
    // 1 / impact_noise / 1 / &origin / projectile_index, and projectile_detonate's own call at
    // 0x4c0a98 pushes the same shape with kind = 2. The origin pointer is `lea ecx,[esp+0x134]`
    // where the collision_result handed to projectile_response is at [esp+0x11c], i.e. exactly
    // collision_result + 0x18 = collision_result.point, the contact point.
extern real weapon_get_zoom_fov(int16_t difficulty_word, uint32_t literal_0x13); // 0x46fe10, UNSURE
    // signature and role, opaque, out of range
extern void unit_get_secondary_eye_marker_position(void); // 0x569280, opaque, out of range, no visible arguments or return
extern real periodic_function_evaluate(periodic_function_t type, double time); // 0x4cc9b0, this
    // codebase's math module (src/math/periodic_function_evaluate.c); type is
    // _periodic_function_wander at both call sites here
extern double fcos(double x); // CRT
extern double fsin(double x); // CRT
extern double sqrt(double x); // a single x87 FSQRT instruction
extern real FUN_00401000(real_vector3d *v); // 0x401000, opaque helper confirmed by disassembly to
    // return dot(v, v) (squared length), EAX -> v
extern real sound_definition_maximum_distance(TagID sound_tag_id); // 0x545460, UNSURE signature (audible-radius
    // lookup for a sound tag?), opaque, out of range. blam-cc: EAX -> sound_tag_id
extern void vector3d_project_onto_axis(real_vector3d *parallel_out, real_vector3d *axis,
    real_vector3d *v, real_vector3d *perp_out); // 0x4cda90, src/math/vector3d_project_onto_axis.c
    // blam-cc: ECX -> parallel_out, EDX -> axis, ESI -> v, EDI -> perp_out
extern void sound_start_at_location(void *bundle); // 0x543d80, opaque effect/sound dispatcher, see
    // src/items/item_update.c; called here with a five real_vector3d bundle instead of that
    // call site's three-vector one
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0, out = stack_operand x ecx_operand
extern void vector3d_rotate_about_axis(real sin_angle, real cos_angle); // 0x4cd820, UNSURE args,
    // see src/items/item_update.c
extern void vector3d_build_perpendicular(void); // 0x4cd670, UNSURE args, opaque
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310
extern void contrail_advance(int32_t kind, real elapsed_seconds); // 0x44ca60, opaque, out of range.
    // blam-cc: EDI -> the contrail attachment handle
    // obj->attachment_handles[pd->contrail_attachment_index], reloaded at 0x4bea33 immediately
    // before the call; the two stack arguments are the literal 0 and (1 - remaining_fraction)/30,
    // i.e. how far into the tick the sub-step ended, in seconds. projectile_detonate's call at
    // 0x4c08ab is byte-for-byte the same idiom.
extern void projectile_send_detonation(datum_index projectile_index); // 0x4bda60, this batch
extern void projectile_detonate(uint32_t object_index, char first_collision,
                                real remaining_tick_fraction); // 0x4c0670, this module, out of range, see src/projectiles/projectile_detonation_message_apply.c
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0

// Per-tick update for a flying projectile: retires a spent tracer contrail, advances the arming
// and deceleration-delay timers, advances the detonation timer once it is allowed to start,
// refreshes the tag function-in values, then -- while still flying (or still armed-pending while
// "detonating") and not attached/resting/parented -- runs the ballistic integrator: guided
// steering toward a tracked object with a periodic "wander" wobble, velocity decay against the
// tag's damage range, gravity, a range-limit / velocity-floor check that can request
// _projectile_state_detonating or _projectile_state_disappearing outright, up to
// k_projectile_maximum_collisions_per_tick collision responses (each running
// projectile_collision_test then projectile_response), the flyby-sound listener check, the
// rotation-valid spin, and finally committing the new position/velocity and relinking the
// object's cluster. Once the sub-tick loop stops, a projectile requesting
// _projectile_state_detonating detonates (unless still arming) and a projectile requesting
// either terminal state is deleted (recursively if it has children, non-recursively otherwise).
int projectile_update(uint32_t projectile_index)
{
    object *obj = ((object_header *)object_data->data)[projectile_index & 0xffff].data;
    Projectile *tag = (Projectile *)tag_instances[obj->definition_tag & 0xffff].data;
    projectile_data *pd = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);

    real remaining_fraction = 1.0f;   // local_158: fraction of this tick still left to integrate
    int16_t collision_count = 0;      // sVar17
    uint8_t flyby_sound_played = 0;   // bVar13: has the flyby sound already fired this tick

    // A tracer whose flag has been cleared (by the weapon, for a non-tracer round) loses its
    // contrail attachment the first tick after creation.
    if ((pd->flags & _projectile_tracer_bit) == 0 && pd->contrail_attachment_index != -1) {
        if (obj->attachment_handles[pd->contrail_attachment_index] != (datum_index)0xffffffff) {
            contrail_delete(obj->attachment_handles[pd->contrail_attachment_index]);
        }
        obj->attachment_handles[pd->contrail_attachment_index] = (datum_index)0xffffffff;
        pd->contrail_attachment_index = -1;
    }

    pd->arming_timer += pd->arming_timer_rate;
    pd->deceleration_delay += pd->deceleration_delay_rate;

    {
        // detonation_timer only counts while it is "allowed to start": already started, or
        // attached, or the tag's detonation_timer_starts condition (at-rest / after first
        // bounce) is currently true.
        uint8_t at_rest_condition_met;
        if (tag->detonation_timer_starts == projectiledetonationtimerstarts_after_first_bounce ||
            tag->detonation_timer_starts == projectiledetonationtimerstarts_when_at_rest) {
            at_rest_condition_met = (pd->flags & _projectile_at_rest_bit) != 0;
        } else {
            at_rest_condition_met = 1; // "immediately"
        }
        if ((pd->flags & _projectile_detonation_timer_started_bit) != 0 ||
            (pd->flags & _projectile_attached_bit) != 0 ||
            at_rest_condition_met) {
            if ((pd->flags & _projectile_detonation_timer_started_bit) == 0) {
                pd->flags |= _projectile_detonation_timer_started_bit;
            }
            pd->detonation_timer += pd->detonation_timer_rate;
            if (1.0f <= pd->detonation_timer) {
                projectile_request_state(projectile_index, _projectile_state_detonating);
            }
        }
    }

    projectile_update_function_values(projectile_index);

    // Uninitialized until the guided-steering branch below first writes it -- see the file
    // header's UNSURE note. This is the accumulating "wander" target the guided branch nudges
    // the projectile toward every tick it has a tracked object with a positive
    // guided_angular_velocity.
    real_point3d wander_target;

    for (;;) {
        int16_t state_blocks_flight;
        real_vector3d velocity;         // fStack_150/14c + fVar2
        real speed;                     // fVar8
        real_vector3d aim_point;        // fStack_114/110/10c: the point the integrator steers
                                        //   the velocity toward this sub-step (copy of velocity
                                        //   direction, nudged by the guided/wander block)
        real turn_rate = 0.0f;          // fStack_154
        uint8_t collision_attempted = 0; // bVar12, reset every iteration
        real_vector3d new_velocity;     // fStack_150/14c/148 after friction is applied
        real_point3d swept_target;      // fStack_108/104/100 (auStack_58): the full-step end
                                        //   point projectile_collision_test sweeps toward and
                                        //   projectile_response is handed
        real avg_speed_for_range;       // fStack_130 ([esp+0x40]): the speed the range check
                                        //   integrates with
        real speed_after_decay;         // fStack_13c ([esp+0x34]): the end-of-sub-step speed the
                                        //   post-collision rescale divides by

        state_blocks_flight = pd->state != _projectile_state_flying &&
            (pd->state != _projectile_state_detonating ||
             pd->arming_timer_rate == 0.0f || 1.0f <= pd->arming_timer);
        if (state_blocks_flight ||
            (pd->flags & _projectile_attached_bit) != 0 ||
            (obj->flags & _object_at_rest_bit) != 0 ||
            obj->parent_object != (datum_index)0xffffffff) {
            break;
        }

        velocity = obj->velocity;
        speed = (real)sqrt((double)(velocity.k * velocity.k + velocity.j * velocity.j + velocity.i * velocity.i));
        aim_point = velocity;      // fStack_114/110/10c: steering may rotate this copy in place
        new_velocity = velocity;   // fStack_150/14c/148: the default -- only a blend branch
                                   //   below (always scaling *this* struct's own current value,
                                   //   never aim_point) changes it

        // ---------------- guided steering + periodic "wander" wobble ----------------
        // UNSURE (whole block): weapon_get_zoom_fov, unit_get_secondary_eye_marker_position and sound_definition_maximum_distance's exact roles are
        // not resolved beyond what the disassembly and the surrounding arithmetic show; see the
        // file header. The shape is preserved literally: while there is a tracked object and a
        // positive guided_angular_velocity, accumulate a slowly-drifting wander_target from two
        // independent periodic_function_evaluate("wander") phases keyed off the tick and the
        // projectile's own datum salt, fading in with distance to the tracked object, then turn
        // the aim point toward (wander_target - position) by turn_rate radians if that turn
        // would move toward the target at all.
        if (pd->tracked_object_index != (datum_index)0xffffffff && 0.0f < tag->guided_angular_velocity) {
            object *tracked = ((object_header *)object_data->data)[pd->tracked_object_index & 0xffff].data;
            real distance_to_target, fade;
            real dx, dy, dz;

            turn_rate = tag->guided_angular_velocity * 0.033333335f; // /30, ticks per second

            // UNSURE: `1 << (tracked->type & 0x1f) & 3` tests _object_mask_biped|_object_mask_vehicle;
            // tracked+0x218 is a unit-extension field types/objects.h does not name (see its note
            // "0x218 on bipeds and vehicles ... belong to the unit extension").
            if (((1 << (tracked->type & 0x1f)) & (_object_mask_biped | _object_mask_vehicle)) != 0 &&
                *(int32_t *)((uint8_t *)tracked + 0x218) != -1) {
                turn_rate *= weapon_get_zoom_fov(*(int16_t *)(unknown_006b0b80 + 0x0e), 0x13);
            }

            dx = obj->bounding_center.x - tracked->bounding_center.x;
            dy = obj->bounding_center.y - tracked->bounding_center.y;
            dz = obj->bounding_center.z - tracked->bounding_center.z;
            distance_to_target = (real)sqrt((double)(dy * dy + dz * dz + dx * dx));
            if (distance_to_target <= 10.0f) {
                if (distance_to_target <= 2.0f) {
                    fade = 0.0f;
                } else {
                    fade = (distance_to_target - 2.0f) * 0.125f;
                    if (fade < 0.0f) {
                        fade = 0.0f;
                    } else if (1.0f < fade) {
                        fade = 1.0f;
                    }
                }
            } else {
                fade = 1.0f;
            }

            unit_get_secondary_eye_marker_position();
            {
                int32_t game_tick = *(int32_t *)((uint8_t *)game_time_globals_006f1d6c + 0x0c);
                int32_t salt_high = (int32_t)projectile_index >> 0x10;
                real phase_a = periodic_function_evaluate(_periodic_function_wander,
                    (double)((real)(int32_t)((uint16_t)(salt_high * 7 + game_tick)) * 0.011111111));
                real phase_b = periodic_function_evaluate(_periodic_function_wander,
                    (double)((real)(int32_t)((uint16_t)(game_tick + salt_high * 3)) * 0.011111111));
                real angle_b = 3.1415927f - phase_b * 1.5707964f;
                real cos_b = (real)fcos((double)angle_b);
                real cos_a = (real)fcos((double)(phase_a * 6.2831855f));
                real sin_a = (real)fsin((double)(phase_a * 6.2831855f));
                real sin_b = (real)fsin((double)angle_b);
                real wander_x = cos_a * cos_b;
                real wander_y = sin_a * cos_b;
                real wander_z = sin_b;

                wander_target.x += wander_x * fade;
                wander_target.y += wander_y * fade;
                wander_target.z += wander_z * fade;
            }

            {
                real to_wander_x = wander_target.x - obj->position.x;
                real to_wander_y = wander_target.y - obj->position.y;
                real to_wander_z = wander_target.z - obj->position.z;
                real_vector3d cross;
                vector3d_cross_product(&cross, &aim_point, (real_vector3d *)&to_wander_x); // UNSURE operand order
                if (0.0f < to_wander_x * aim_point.i + to_wander_y * aim_point.j + to_wander_z * aim_point.k &&
                    0.0f < vector3d_normalize_with_length(&cross)) {
                    vector3d_rotate_about_axis((real)fsin((double)turn_rate), (real)fcos((double)turn_rate));
                }
            }
        }

        // ---------------- velocity decay against the tag's damage range ----------------
        // Every branch below either leaves new_velocity (i, j and the not-yet-gravity-adjusted
        // k already seeded above) exactly as read from the object, or scales its OWN current
        // value by a blend fraction -- never aim_point's. aim_point is left as the (possibly
        // steering-rotated) direction copy unless the two-part branch explicitly recomputes it
        // from new_velocity and the raw `velocity` reading (matching the original, which always
        // re-reads *pfVar1 / object.velocity for this, never the steering-rotated copy).
        avg_speed_for_range = speed; // fStack_130 default, overwritten by most branches below
        speed_after_decay = speed;   // fStack_13c default, likewise
        if (pd->deceleration_delay < 1.0f) {
            goto apply_gravity;
        }
        if (speed <= tag->final_velocity || pd->deceleration == 0.0f) {
            if (tag->maximum_range != 0.0f ||
                tag->timer[1] != 0.0f ||
                tag->minimum_velocity > tag->final_velocity || // 0x4be2e1 `fcomp`/`test ah,0x41`/
                                                              // `jp`: Ghidra prints this as the
                                                              // garbled `(a < b) == (a == b)`
                (pd->deceleration == 0.0f && pd->distance_travelled < pd->deceleration_end_range)) {
                if (speed < tag->final_velocity && 0.0f < speed) {
                    real blend = (tag->final_velocity / speed) * 0.99f;
                    new_velocity.i = velocity.i * blend;
                    new_velocity.j = velocity.j * blend;
                    new_velocity.k = velocity.k * blend;
                    avg_speed_for_range = speed;
                    speed_after_decay = speed;
                    goto apply_gravity;
                }
            } else {
                // UNSURE: Ghidra shows this as `item_update_max_permutation_reached()` returning
                // a float10 "consumed" by the next line; 0x4bf0f0 (projectile_request_state) is a
                // 6-instruction void function that never touches the FPU, so this is almost
                // certainly a decompiler artifact -- the real x87 value flowing into the velocity
                // blend below is whatever was already on the stack from the comparison chain
                // just above, not a genuine return. The two real, disassembly-confirmed effects
                // are: request _projectile_state_disappearing, and leave new_velocity untouched
                // (no blend) for this sub-step.
                projectile_request_state(projectile_index, _projectile_state_disappearing);
            }
            avg_speed_for_range = speed;
            goto apply_gravity;
        }
        {
            real decel_step = remaining_fraction * pd->deceleration;
            real speed_after = speed - decel_step;
            speed_after_decay = speed_after;
            if (speed_after > tag->final_velocity) { // same garbled-compare idiom as above
                real blend = speed_after / speed;
                new_velocity.i = velocity.i * blend;
                new_velocity.j = velocity.j * blend;
                new_velocity.k = velocity.k * blend;
                aim_point.i = (new_velocity.i + velocity.i) * 0.5f;
                aim_point.j = (new_velocity.j + velocity.j) * 0.5f;
                aim_point.k = (new_velocity.k + velocity.k) * 0.5f;
                avg_speed_for_range = speed - decel_step * 0.5f;
            } else {
                real fraction_to_floor = (speed - tag->final_velocity) / decel_step;
                real remainder = 1.0f - fraction_to_floor;
                real floor_speed = tag->final_velocity * 0.99f;
                speed_after_decay = floor_speed; // fStack_13c is reassigned in this branch
                real blend = floor_speed / speed;
                new_velocity.i = velocity.i * blend;
                new_velocity.j = velocity.j * blend;
                new_velocity.k = velocity.k * blend;
                aim_point.i = remainder * new_velocity.i + (new_velocity.i + velocity.i) * fraction_to_floor * 0.5f;
                aim_point.j = remainder * new_velocity.j + (new_velocity.j + velocity.j) * fraction_to_floor * 0.5f;
                aim_point.k = remainder * new_velocity.k + (new_velocity.k + velocity.k) * fraction_to_floor * 0.5f;
                // 0x4be1b8..0x4be1d6: the first term multiplies the UNSCALED
                // Projectile.final_velocity (`fmul [esi+0x1e8]`), while only the term inside the
                // average uses the 0.99-scaled floor speed. An earlier rewrite of this file used
                // floor_speed in both places; that was wrong.
                avg_speed_for_range = remainder * tag->final_velocity +
                                      (floor_speed + speed) * fraction_to_floor * 0.5f;
            }
        }

    apply_gravity:
        {
            real gravity_scale = (obj->flags & _object_in_water_bit) == 0 ? tag->air_gravity_scale : tag->water_gravity_scale;
            real gravity_step = 0.00356518f * gravity_scale * remaining_fraction; // k_gravity_per_tick_squared
            real aim_k = aim_point.k - gravity_step * 0.5f;
            real blend_frac; // fVar19/fVar18-equivalent "how far this sub-step actually goes"

            new_velocity.k -= gravity_step;

            if (tag->maximum_range == 0.0f ||
                avg_speed_for_range * remaining_fraction + pd->distance_travelled <= tag->maximum_range) {
                blend_frac = 1.0f;
            } else if (avg_speed_for_range == 0.0f) {
                // See the UNSURE note above: request _projectile_state_detonating (the tick
                // clips exactly at the range limit) and treat the fraction as 0.0.
                projectile_request_state(projectile_index, _projectile_state_detonating);
                blend_frac = 0.0f;
            } else {
                real numerator = tag->maximum_range - pd->distance_travelled;
                blend_frac = (numerator / avg_speed_for_range) * remaining_fraction;
                projectile_request_state(projectile_index, _projectile_state_detonating);
            }
            blend_frac *= remaining_fraction;
            swept_target.x = aim_point.i * blend_frac + obj->position.x;
            swept_target.y = aim_point.j * blend_frac + obj->position.y;
            swept_target.z = aim_k * blend_frac + obj->position.z;

            if (collision_count == k_projectile_maximum_collisions_per_tick) {
                projectile_request_state(projectile_index, _projectile_state_detonating);
                remaining_fraction = 0.0f;
                if (!collision_attempted) {
                    break;
                }
                goto settle_sub_step;
            }
            if (pd->state == _projectile_state_disappearing) {
                goto give_up_sub_step;
            }
            {
                collision_result hit;
                collision_attempted = 1;
                if (projectile_collision_test(projectile_index, &swept_target, &hit) == 0) {
                    goto give_up_sub_step;
                }
                remaining_fraction = 1.0f - hit.t;
                // The raw (pre-friction, pre-gravity) velocity.k scaled by the fraction of the
                // sub-step left after the hit, blended on top of the already gravity-adjusted
                // new_velocity.k.
                new_velocity.k = velocity.k * remaining_fraction + new_velocity.k;
                // 0x4be4ef `fdiv [esp+0x34]`: this rescale is driven by fStack_13c
                // (speed_after_decay), NOT by the range check's fStack_130.
                if (speed_after_decay != 0.0f) {
                    real ratio = remaining_fraction * pd->deceleration + speed_after_decay;
                    if (speed < ratio) {
                        ratio = speed;
                    }
                    ratio /= speed_after_decay;
                    new_velocity.i *= ratio;
                    new_velocity.j *= ratio;
                    new_velocity.k *= ratio;
                }
                if (0.3f < hit.normal.k) {
                    pd->flags |= _projectile_hit_ground_bit;
                }
                pd->ignore_object_index = (datum_index)0xffffffff;
                projectile_response(projectile_index, &hit, &swept_target, (real_vector3d *)&new_velocity);
                collision_count++;
                ai_accumulate_repeated_event(projectile_index, &hit.point, 1, tag->impact_noise, 1);
                // 0x4be577 `mov al,[ebx+0x22c]` / `test al,8` / `je LAB_004be5c1`: the settle
                // tail runs when the response did NOT attach the projectile. If it did attach,
                // the tail is skipped and the loop falls straight to its `0.0 < remaining`
                // test, whose next top-of-loop check then breaks on the attached bit.
                if ((pd->flags & _projectile_attached_bit) == 0) {
                    goto settle_sub_step;
                }
            }
            continue;

        give_up_sub_step:
            remaining_fraction = 0.0f;
            if (!collision_attempted) {
                break;
            }

        settle_sub_step:
            {
                real moved_i = swept_target.x - obj->position.x;
                real moved_j = swept_target.y - obj->position.y;
                real moved_k = swept_target.z - obj->position.z;
                pd->distance_travelled += (real)sqrt((double)(moved_i * moved_i + moved_j * moved_j + moved_k * moved_k));

                // ---------------- flyby-sound listener check ----------------
                // UNSURE: see the file header. Plays tag->flyby_sound once per tick, at most
                // once, when the local player's own unit is the one this projectile is ignoring
                // and it is not the local player itself.
                if (!flyby_sound_played && *(int32_t *)&tag->flyby_sound.tag_id != -1 &&
                    *(uint32_t *)((uint8_t *)local_player_globals + 4) != (uint32_t)0xffffffff) {
                    uint32_t local_player_unit = *(uint32_t *)(((*(uint32_t *)((uint8_t *)local_player_globals + 4)) & 0xffff) * 0x200 + 0x34 +
                        *(int32_t *)((uint8_t *)player_data + 0x34));
                    if (local_player_unit != (uint32_t)0xffffffff && local_player_unit != pd->ignore_object_index) {
                        object *listener = ((object_header *)object_data->data)[local_player_unit & 0xffff].data;
                        real radius = sound_definition_maximum_distance(tag->flyby_sound.tag_id);
                        real_vector3d to_listener = {
                            listener->bounding_center.x - obj->position.x,
                            listener->bounding_center.y - obj->position.y,
                            listener->bounding_center.z - obj->position.z
                        };
                        real along;
                        real_vector3d projected, projected_perp;
                        // 0x4be692..0x4be6cf: ECX = &projected ([esp+0xb0]), EDX = the step
                        // displacement ([esp+0x44]) as the axis, ESI = &to_listener ([esp+0xa4]),
                        // EDI = &projected_perp ([esp+0x98]).
                        vector3d_project_onto_axis(&projected, (real_vector3d *)&moved_i,
                                                   &to_listener, &projected_perp);
                        along = projected.i * moved_i + projected.j * moved_j + projected.k * moved_k;
                        if (0.0f <= along && along < FUN_00401000((real_vector3d *)&moved_i) &&
                            FUN_00401000((real_vector3d *)&moved_i) < radius * radius) { // UNSURE:
                            // both comparisons reuse the moved_* vector per the disassembly
                            real_vector3d incident, up_or_scratch;
                            struct { real_point3d position; real_vector3d normal; real_point3d reference; datum_index leaf; int16_t cluster; } bundle;
                            incident.i = listener->bounding_center.x - swept_target.x;
                            incident.j = listener->bounding_center.y - swept_target.y;
                            incident.k = listener->bounding_center.z - swept_target.z;
                            vector3d_normalize_with_length(&incident);
                            bundle.position = *global_origin3d_pointer; // UNSURE: bundle shape guessed
                                // by analogy with item_update.c's sound_start_at_location bundle; not traced
                                // field-by-field here
                            sound_start_at_location(&bundle);
                            flyby_sound_played = 1;
                            (void)up_or_scratch;
                        }
                    }
                }

                // ---------------- rotation-valid spin ----------------
                if ((tag->projectile_flags & _projectile_definition_oriented_along_velocity_bit) == 0 ||
                    (obj->velocity.i == 0.0f && obj->velocity.j == 0.0f && obj->velocity.k == 0.0f)) {
                    if ((pd->flags & _projectile_rotation_valid_bit) != 0) {
                        vector3d_rotate_about_axis(pd->rotation_sine, pd->rotation_cosine);
                        vector3d_rotate_about_axis(pd->rotation_sine, pd->rotation_cosine);
                        vector3d_normalize_with_length(&obj->forward);
                        vector3d_cross_product(&obj->up, 0, 0); // UNSURE, see src/items/item_update.c's identical tail
                        vector3d_cross_product(0, 0, 0); // UNSURE
                        vector3d_normalize_with_length(0);
                    }
                } else {
                    real_vector3d saved_velocity = obj->velocity;
                    if (0.0f < vector3d_normalize_with_length(&saved_velocity)) {
                        obj->forward = saved_velocity;
                        vector3d_cross_product(0, 0, 0); // UNSURE
                        vector3d_cross_product(0, 0, 0); // UNSURE
                        if (vector3d_normalize_with_length(0) == 0.0f) {
                            vector3d_build_perpendicular();
                            vector3d_normalize_with_length(0);
                        }
                    }
                    vector3d_rotate_about_axis(pd->rotation_sine, pd->rotation_cosine);
                }

                // ---------------- commit position, relink cluster ----------------
                object_unlink_cluster_or_notify_parent(projectile_index);
                obj->position = swept_target;
                {
                    // UNSURE: passed by address without being initialized anywhere in this
                    // function -- see the file header's note on the second leftover-stack read.
                    bsp_leaf_reference location_hint;
                    object_set_cluster_and_parent(projectile_index, &location_hint);
                }
                obj->velocity = new_velocity;

                if (remaining_fraction != 0.0f && collision_count != 0 &&
                    pd->contrail_attachment_index != -1 &&
                    obj->attachment_handles[pd->contrail_attachment_index] != (datum_index)0xffffffff) {
                    object_recalculate_bounding_radius(projectile_index);
                    // EDI = obj->attachment_handles[pd->contrail_attachment_index]
                    contrail_advance(0, (1.0f - remaining_fraction) * 0.033333335f);
                }
            }
        }

        if (remaining_fraction <= 0.0f ||
            (pd->state != _projectile_state_flying &&
             (pd->state != _projectile_state_detonating || pd->arming_timer_rate == 0.0f || 1.0f <= pd->arming_timer)) ||
            (pd->flags & _projectile_attached_bit) != 0 || (obj->flags & _object_at_rest_bit) != 0 ||
            obj->parent_object != (datum_index)0xffffffff) {
            break;
        }
    }

    if (pd->state == _projectile_state_detonating) {
        if (pd->arming_timer_rate != 0.0f && pd->arming_timer < 1.0f) {
            return 1;
        }
        if (obj->network_role == 1) {
            return 1;
        }
        if (obj->network_role == 0 && pd->thrown_grenade == 1) {
            projectile_send_detonation(projectile_index);
        }
        // 0x4beac7..0x4bead9: `sete al` (collision_count == 0) and `mov edx,[esp+0x18]`
        // (remaining_fraction) are the two stack arguments; EBX carries the object index.
        projectile_detonate(projectile_index, collision_count == 0, remaining_fraction);
    } else if (pd->state != _projectile_state_disappearing) {
        return 1;
    }

    if (obj->network_role == 0) {
        object_delete_unparented(projectile_index);
    } else if (obj->network_role != 3) {
        return 1;
    }
    object_delete_recursive(projectile_index, 0);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bdc00), the complete function (Ghidra's own "size=1456" batch
metadata is wrong -- it is the byte offset of 0x4be1b0, which this decompilation already fully
absorbs; see the file header):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004bdc00(uint param_1)

{
  float *pfVar1;
  float fVar2;
  uint *puVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  bool bVar12;
  bool bVar13;
  char cVar14;
  byte bVar15;
  int iVar16;
  short sVar17;
  float10 fVar18;
  float10 fVar19;
  float10 fVar20;
  float10 fVar21;
  float10 extraout_ST1;
  float10 extraout_ST1_00;
  float local_158;
  float fStack_154;
  float fStack_150;
  float fStack_14c;
  float fStack_148;
  float fStack_140;
  float fStack_13c;
  float fStack_130;
  float fStack_114;
  float fStack_110;
  float fStack_10c;
  float fStack_108;
  float fStack_104;
  float fStack_100;
  float fStack_fc;
  float fStack_f8;
  float fStack_f4;
  float fStack_f0;
  float fStack_ec;
  float fStack_e8;
  float fStack_e4;
  uint uStack_e0;
  uint uStack_dc;
  float fStack_d8;
  float fStack_d4;
  float fStack_d0;
  float fStack_cc;
  float fStack_c8;
  float fStack_c4;
  float fStack_c0;
  float fStack_bc;
  float fStack_b8;
  float fStack_b4;
  float fStack_ac;
  float fStack_a8;
  float fStack_a4;
  float fStack_a0;
  float fStack_9c;
  float fStack_98;
  float fStack_94;
  undefined4 uStack_90;
  undefined4 uStack_8c;
  undefined4 uStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined1 auStack_58 [12];
  undefined4 uStack_4c;
  undefined4 uStack_48;
  float fStack_44;
  undefined1 auStack_40 [20];
  float fStack_2c;

  iVar16 = (param_1 & 0xffff) * 0xc;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_158 = 1.0;
  sVar17 = 0;
  bVar13 = false;
  if (((puVar3[0x8b] & 2) == 0) && (puVar3[0x8f] != 0xffffffff)) {
    if (puVar3[puVar3[0x8f] + 0x53] != 0xffffffff) {
      contrail_delete(puVar3[puVar3[0x8f] + 0x53]);
    }
    puVar3[puVar3[0x8f] + 0x53] = 0xffffffff;
    puVar3[0x8f] = 0xffffffff;
  }
  puVar3[0x92] = (uint)((float)puVar3[0x92] + (float)puVar3[0x93]);
  puVar3[0x95] = (uint)((float)puVar3[0x96] + (float)puVar3[0x95]);
  if ((*(short *)(iVar4 + 0x180) == 1) || (*(short *)(iVar4 + 0x180) == 2)) {
    bVar15 = (byte)(puVar3[0x8b] >> 4) & 1;
  }
  else {
    bVar15 = 1;
  }
  uVar5 = puVar3[0x8b];
  if ((((uVar5 & 0x20) != 0) || ((uVar5 & 8) != 0)) || (bVar15 != 0)) {
    if ((uVar5 & 0x20) == 0) {
      puVar3[0x8b] = uVar5 | 0x20;
    }
    fVar2 = (float)puVar3[0x90];
    puVar3[0x90] = (uint)((float)puVar3[0x91] + fVar2);
    if ((1.0 <= (float)puVar3[0x91] + fVar2) &&
       (iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16), *(short *)(iVar6 + 0x230) < 1)
       ) {
      *(undefined2 *)(iVar6 + 0x230) = 1;
    }
  }
  item_update_function_values(param_1);
  do {
    if ((((short)puVar3[0x8c] != 0) &&
        ((((short)puVar3[0x8c] != 1 || ((float)puVar3[0x93] == 0.0)) || (1.0 <= (float)puVar3[0x92])
         ))) || ((((puVar3[0x8b] & 8) != 0 || ((puVar3[4] & 0x20) != 0)) ||
                 (puVar3[0x47] != 0xffffffff)))) break;
    pfVar1 = (float *)(puVar3 + 0x1a);
    fStack_150 = *pfVar1;
    fStack_14c = (float)puVar3[0x1b];
    fVar2 = (float)puVar3[0x1c];
    bVar12 = false;
    fVar8 = SQRT((float)puVar3[0x1c] * (float)puVar3[0x1c] +
                 (float)puVar3[0x1b] * (float)puVar3[0x1b] + *pfVar1 * *pfVar1);
    fStack_114 = *pfVar1;
    fStack_110 = (float)puVar3[0x1b];
    fStack_10c = (float)puVar3[0x1c];
    uVar5 = puVar3[0x8d];
    if ((puVar3[0x8e] != 0xffffffff) && (0.0 < *(float *)(iVar4 + 0x1ec))) {
      fStack_154 = *(float *)(iVar4 + 0x1ec) * 0.033333335;
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (puVar3[0x8e] & 0xffff) * 0xc);
      if (((1 << (*(byte *)(iVar6 + 0xb4) & 0x1f) & 3U) != 0) && (*(int *)(iVar6 + 0x218) != -1)) {
        fVar18 = (float10)FUN_0046fe10();
        fStack_154 = (float)(fVar18 * (float10)fStack_154);
      }
      fVar9 = (float)puVar3[0x28] - *(float *)(iVar6 + 0xa0);
      fVar10 = (float)puVar3[0x29] - *(float *)(iVar6 + 0xa4);
      fVar11 = (float)puVar3[0x2a] - *(float *)(iVar6 + 0xa8);
      fVar9 = SQRT(fVar10 * fVar10 + fVar11 * fVar11 + fVar9 * fVar9);
      if (fVar9 <= 10.0) {
        if ((fVar9 <= 2.0) || (fStack_140 = (fVar9 - 2.0) * 0.125, fStack_140 < 0.0)) {
          fStack_140 = 0.0;
        }
        else if (1.0 < fStack_140) {
          fStack_140 = 1.0;
        }
      }
      else {
        fStack_140 = 1.0;
      }
      FUN_00569280();
      iVar6 = *(int *)(DAT_006f1d6c + 0xc);
      fVar18 = (float10)periodic_function_evaluate
                                  ((double)((float)(((int)param_1 >> 0x10) * 7 + iVar6 & 0xffff) *
                                           0.011111111));
      fVar19 = (float10)periodic_function_evaluate
                                  ((double)((float)(iVar6 + ((int)param_1 >> 0x10) * 3 & 0xffff) *
                                           0.011111111));
      fVar19 = (float10)3.1415927 - fVar19 * (float10)1.5707964;
      fVar20 = (float10)fcos(fVar19);
      fVar21 = (float10)fcos((float10)(float)(fVar18 * (float10)6.2831855));
      fStack_b4 = (float)(fVar21 * fVar20);
      fVar18 = (float10)fsin((float10)(float)(fVar18 * (float10)6.2831855));
      fVar19 = (float10)fsin(fVar19);
      fStack_ac = (float)fVar19;
      fStack_fc = fStack_b4 * fStack_140 + fStack_fc;
      fStack_f8 = (float)(fVar18 * fVar20 * (float10)fStack_140 + (float10)fStack_f8);
      fStack_f4 = fStack_ac * fStack_140 + fStack_f4;
      fStack_f0 = fStack_fc - (float)puVar3[0x17];
      fStack_ec = fStack_f8 - (float)puVar3[0x18];
      fStack_e8 = fStack_f4 - (float)puVar3[0x19];
      vector3d_cross_product(pfVar1);
      if ((0.0 < fStack_f0 * *pfVar1 +
                 fStack_ec * (float)puVar3[0x1b] + fStack_e8 * (float)puVar3[0x1c]) &&
         (fVar18 = (float10)vector3d_normalize_with_length(), (float10)0.0 < fVar18)) {
        fVar18 = (float10)fcos((float10)fStack_154);
        fVar19 = (float10)fsin((float10)fStack_154);
        vector3d_rotate_about_axis((float)fVar19,(float)fVar18);
      }
    }
    fStack_13c = fVar8;
    fStack_130 = fVar8;
    if ((float)puVar3[0x95] < 1.0) {
LAB_004be329:
      fVar18 = (float10)fVar2;
    }
    else {
      if ((fVar8 <= *(float *)(iVar4 + 0x1e8)) || ((float)puVar3[0x97] == 0.0)) {
        if (((*(float *)(iVar4 + 0x1c8) != 0.0) ||
            ((*(float *)(iVar4 + 0x1c0) != 0.0 ||
             (*(float *)(iVar4 + 0x1c4) < *(float *)(iVar4 + 0x1e8) ==
              (*(float *)(iVar4 + 0x1c4) == *(float *)(iVar4 + 0x1e8)))))) ||
           (((float)puVar3[0x97] == 0.0 && ((float)puVar3[0x94] < (float)puVar3[0x98])))) {
          if ((fVar8 < *(float *)(iVar4 + 0x1e8)) && (0.0 < fVar8)) {
            fVar18 = ((float10)*(float *)(iVar4 + 0x1e8) / (float10)fVar8) * (float10)0.99;
            fStack_150 = (float)((float10)fStack_150 * fVar18);
            fStack_14c = (float)((float10)fStack_14c * fVar18);
            fVar18 = fVar18 * (float10)fVar2;
            goto LAB_004be32d;
          }
        }
        else {
          item_update_max_permutation_reached();
        }
        goto LAB_004be329;
      }
      fVar9 = local_158 * (float)puVar3[0x97];
      fStack_13c = fVar8 - fVar9;
      if (fStack_13c < *(float *)(iVar4 + 0x1e8) == (fStack_13c == *(float *)(iVar4 + 0x1e8))) {
        fVar18 = (float10)fStack_13c / (float10)fVar8;
        fStack_150 = (float)((float10)fStack_150 * fVar18);
        fStack_14c = (float)((float10)fStack_14c * fVar18);
        fVar18 = fVar18 * (float10)fVar2;
        fStack_114 = (fStack_150 + *pfVar1) * 0.5;
        fStack_110 = (fStack_14c + (float)puVar3[0x1b]) * 0.5;
        fStack_10c = (float)((fVar18 + (float10)(float)puVar3[0x1c]) * (float10)0.5);
        fStack_130 = fVar8 - fVar9 * 0.5;
      }
      else {
        fVar9 = (fVar8 - *(float *)(iVar4 + 0x1e8)) / fVar9;
        fStack_13c = *(float *)(iVar4 + 0x1e8) * 0.99;
        fVar10 = 1.0 - fVar9;
        fVar18 = (float10)fStack_13c / (float10)fVar8;
        fStack_150 = (float)((float10)fStack_150 * fVar18);
        fStack_14c = (float)((float10)fStack_14c * fVar18);
        fVar18 = fVar18 * (float10)fVar2;
        fStack_114 = fVar10 * fStack_150 + (fStack_150 + *pfVar1) * fVar9 * 0.5;
        fStack_110 = fVar10 * fStack_14c + (fStack_14c + (float)puVar3[0x1b]) * fVar9 * 0.5;
        fStack_10c = (float)((float10)fVar10 * fVar18 +
                            (fVar18 + (float10)(float)puVar3[0x1c]) * (float10)fVar9 * (float10)0.5)
        ;
        fStack_130 = fVar10 * *(float *)(iVar4 + 0x1e8) + (fStack_13c + fVar8) * fVar9 * 0.5;
      }
    }
LAB_004be32d:
    if ((puVar3[4] & 0x10) == 0) {
      fVar2 = *(float *)(iVar4 + 0x1cc);
    }
    else {
      fVar2 = *(float *)(iVar4 + 0x1d8);
    }
    fVar2 = _DAT_0069c52c * fVar2;
    fVar19 = (float10)fVar2 * (float10)local_158;
    fStack_148 = (float)(fVar18 - fVar19);
    fVar18 = (float10)fStack_10c - fVar19 * (float10)0.5;
    if ((*(float *)(iVar4 + 0x1c8) == 0.0) ||
       (fStack_130 * local_158 + (float)puVar3[0x94] <= *(float *)(iVar4 + 0x1c8))) {
      fVar19 = (float10)1.0;
    }
    else if (fStack_130 == 0.0) {
      fVar19 = (float10)item_update_max_permutation_reached();
      fVar18 = extraout_ST1_00;
    }
    else {
      fVar19 = (float10)item_update_max_permutation_reached();
      fVar18 = extraout_ST1;
    }
    fVar19 = fVar19 * (float10)local_158;
    fStack_108 = (float)((float10)fStack_114 * fVar19 + (float10)(float)puVar3[0x17]);
    fStack_104 = (float)((float10)fStack_110 * fVar19 + (float10)(float)puVar3[0x18]);
    fStack_100 = (float)(fVar19 * fVar18 + (float10)(float)puVar3[0x19]);
    if (sVar17 == 10) {
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
      if (*(short *)(iVar6 + 0x230) < 1) {
        *(undefined2 *)(iVar6 + 0x230) = 1;
      }
LAB_004be5ad:
      local_158 = 0.0;
      if (!bVar12) break;
LAB_004be5c1:
      fVar2 = fStack_108 - (float)puVar3[0x17];
      fVar8 = fStack_104 - (float)puVar3[0x18];
      fVar9 = fStack_100 - (float)puVar3[0x19];
      puVar3[0x94] = (uint)(SQRT(fVar2 * fVar2 + fVar8 * fVar8 + fVar9 * fVar9) +
                           (float)puVar3[0x94]);
      if ((((!bVar13) && (*(int *)(iVar4 + 0x210) != -1)) &&
          (*(uint *)(DAT_0087a478 + 4) != 0xffffffff)) &&
         ((uVar7 = *(uint *)((*(uint *)(DAT_0087a478 + 4) & 0xffff) * 0x200 + 0x34 +
                            *(int *)(DAT_0087a480 + 0x34)), uVar7 != 0xffffffff && (uVar7 != uVar5))
         )) {
        iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
        fVar18 = (float10)FUN_00545460();
        fStack_cc = *(float *)(iVar6 + 0xa0) - (float)puVar3[0x17];
        fStack_c8 = *(float *)(iVar6 + 0xa4) - (float)puVar3[0x18];
        fStack_c4 = *(float *)(iVar6 + 0xa8) - (float)puVar3[0x19];
        vector3d_project_onto_axis();
        fVar10 = fStack_c0 * fVar2 + fStack_bc * fVar8 + fStack_b8 * fVar9;
        if ((0.0 <= fVar10) &&
           ((fVar19 = (float10)FUN_00401000(), (float10)fVar10 < fVar19 &&
            (fVar19 = (float10)FUN_00401000(),
            fVar19 < (float10)(float)fVar18 * (float10)(float)fVar18)))) {
          fStack_a8 = *(float *)(iVar6 + 0xa0) - fStack_d8;
          fStack_a4 = *(float *)(iVar6 + 0xa4) - fStack_d4;
          fStack_a0 = *(float *)(iVar6 + 0xa8) - fStack_d0;
          fStack_9c = fVar2;
          fStack_98 = fVar8;
          fStack_94 = fVar9;
          vector3d_normalize_with_length();
          uStack_90 = *(undefined4 *)PTR_DAT_00696714;
          uStack_8c = *(undefined4 *)(PTR_DAT_00696714 + 4);
          uStack_88 = *(undefined4 *)(PTR_DAT_00696714 + 8);
          uStack_84 = uStack_4c;
          uStack_80 = uStack_48;
          FUN_00543d80();
          bVar13 = true;
        }
      }
      if (((*(byte *)(iVar4 + 0x17c) & 1) == 0) ||
         ((((float)puVar3[0x1a] == 0.0 && ((float)puVar3[0x1b] == 0.0)) &&
          ((float)puVar3[0x1c] == 0.0)))) {
        if ((puVar3[0x8b] & 1) != 0) {
          vector3d_rotate_about_axis(puVar3[0x9c]);
          vector3d_rotate_about_axis(puVar3[0x9c],puVar3[0x9d]);
          vector3d_normalize_with_length();
          vector3d_cross_product(puVar3 + 0x20);
          vector3d_cross_product();
          vector3d_normalize_with_length();
        }
      }
      else {
        fStack_e4 = (float)puVar3[0x1a];
        uStack_e0 = puVar3[0x1b];
        uStack_dc = puVar3[0x1c];
        fVar18 = (float10)vector3d_normalize_with_length();
        if ((float10)0.0 < fVar18) {
          puVar3[0x1d] = (uint)fStack_e4;
          puVar3[0x1e] = uStack_e0;
          puVar3[0x1f] = uStack_dc;
          vector3d_cross_product();
          vector3d_cross_product();
          fVar18 = (float10)vector3d_normalize_with_length();
          if ((float10)0.0 == fVar18) {
            vector3d_build_perpendicular();
            vector3d_normalize_with_length();
          }
        }
        vector3d_rotate_about_axis(puVar3[0x9c]);
      }
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
      object_unlink_cluster_or_notify_parent();
      *(float *)(iVar6 + 0x5c) = fStack_108;
      *(float *)(iVar6 + 0x60) = fStack_104;
      *(float *)(iVar6 + 100) = fStack_100;
      object_set_cluster_and_parent(param_1);
      puVar3[0x1a] = (uint)fStack_150;
      puVar3[0x1b] = (uint)fStack_14c;
      puVar3[0x1c] = (uint)fStack_148;
      if (((local_158 != 0.0) && (sVar17 != 0)) &&
         ((puVar3[0x8f] != 0xffffffff && (puVar3[puVar3[0x8f] + 0x53] != 0xffffffff)))) {
        object_recalculate_bounding_radius();
        FUN_0044ca60(0);
      }
    }
    else {
      if ((short)puVar3[0x8c] == 2) goto LAB_004be5ad;
      bVar12 = true;
      cVar14 = FUN_004c0450();
      if (cVar14 == '\0') goto LAB_004be5ad;
      local_158 = 1.0 - fStack_44;
      fStack_148 = fVar2 * local_158 + fStack_148;
      if (fStack_13c != 0.0) {
        fVar2 = local_158 * (float)puVar3[0x97] + fStack_13c;
        if (fVar8 < fVar2) {
          fVar2 = fVar8;
        }
        fVar2 = fVar2 / fStack_13c;
        fStack_150 = fStack_150 * fVar2;
        fStack_14c = fStack_14c * fVar2;
        fStack_148 = fVar2 * fStack_148;
      }
      if (0.3 < fStack_2c) {
        puVar3[0x8b] = puVar3[0x8b] | 4;
      }
      puVar3[0x8d] = 0xffffffff;
      FUN_004bf390(param_1,auStack_58,&fStack_108);
      sVar17 = sVar17 + 1;
      FUN_0042c610(param_1,auStack_40,1,*(undefined2 *)(iVar4 + 0x182));
      if ((puVar3[0x8b] & 8) == 0) goto LAB_004be5c1;
    }
  } while (0.0 < local_158);
  if ((short)puVar3[0x8c] == 1) {
    if (((float)puVar3[0x93] != 0.0) && ((float)puVar3[0x92] < 1.0)) {
      return 1;
    }
    if (puVar3[1] == 1) {
      return 1;
    }
    if ((puVar3[1] == 0) && ((char)puVar3[0x9e] == '\x01')) {
      FUN_004bda60();
    }
    item_detonate(sVar17 == 0,local_158);
  }
  else if ((short)puVar3[0x8c] != 2) {
    return 1;
  }
  iVar4 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16) + 4);
  if (iVar4 == 0) {
    object_delete_unparented();
  }
  else if (iVar4 != 3) {
    return 1;
  }
  object_delete_recursive(param_1,0);
  return 1;
}
#endif
