// actor_update_firing_state  (Ghidra: actor_update_firing_state, renamed)
// address 0x40e7b0, size 3752 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: it is the per-tick driver of the five-state machine at actor+0x5f2 (states 0..4
//   with the tick counter at 0x5f4), it decrements the four countdowns at 0x5f4/0x5f6/0x5f8/
//   0x5fc, it resolves the current threat into the 0x60c..0x638 block, and it calls
//   actor_update_aim_wander @0x40fcb0 and actor_reseed_movement_pause_timer @0x4104e0 on the
//   state transitions. Every tag field it reads lines up with types/tags.h:
//   ActorVariant.maximum_firing_distance / special_fire_mode / special_fire_situation /
//   special_fire_chance / special_fire_delay / super_ballistic_range / bombardment_range /
//   target_tracking / target_leading, and Actor.flags bit 9 must_crouch_to_shoot, bit 13
//   start_firing_before_aligned, Actor.more_flags bit 1 must_stand_to_fire and bit 2
//   must_stop_to_fire, plus the standing / crouching gun offsets at Actor+0x34 and +0x40.
// register convention: actor_index is the one Ghidra-recognized stack parameter.
//
// UNSURE (0.3): this function is heavily register-aliased in the export. weapon_trigger_get_aiming_vector,
// weapon_trigger_projectile_time_fraction, actor_grenade_trajectory_blocked, unit_add_marker_relative_offset, unit_get_camera_position, weapon_get_zoom_fov_resolved and
// unit_set_grenade_type_and_count_delta are all called with some or all of their arguments invisible; local_18 /
// local_14 / local_10 are read on one path before any visible assignment; and local_30 is
// reused first as the Actor tag pointer and then, in the tail, as a byte pair fed to
// actor_set_override_target. The control flow is preserved exactly. Do not trust the individual argument
// lists without a hook comparison.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t *player_globals;    // 0x0087a478, the per-player bitset this reads at +0x18

extern real random_real_range(real min, real max);          // 0x401050
extern real random_real(void);                                // 0x4019f0
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern void point3d_add_scaled(real_point3d *point, float scale); // 0x401930
extern real vector3d_distance(const real_point3d *a, const real_point3d *b); // 0x4088b0
extern real vector3d_magnitude_squared(real_vector3d *v);                     // 0x401000, src/math; blam-cc: EAX v
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b);      // 0x401020, src/math; blam-cc: EAX a, ECX b

extern uint8_t actor_grenade_behavior_kind_allowed(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_grenade_behavior_kind_allowed at 0x40f670
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_target_is_visible_or_object_count_ok(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_target_is_visible_or_object_count_ok at 0x40f700
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_get_aim_from_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_get_aim_from_position at 0x40f9b0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, this module
extern void actor_update_aim_wander(datum_index actor_index);  // 0x40fcb0, this module
extern void actor_reseed_movement_pause_timer(datum_index actor_index); // 0x4104e0, this module
extern uint8_t actor_should_hold_position(datum_index actor_index);               // 0x4105c0, this module
extern void actor_select_stance_offset_pair(datum_index actor_index, uint8_t *base, uint8_t **out_a, uint8_t **out_b); // 0x4106b0
extern uint8_t actor_action_has_queued_secondary(void);        // 0x417b70
extern datum_index actor_get_threat_weapon_object_index(void); // 0x4282c0
extern uint8_t actor_has_unshielded_threat_weapon(void);                             // 0x428370, not yet rewritten
extern void actor_set_override_target(uint32_t flags, uint32_t value);      // 0x42a5e0, not yet rewritten
extern uint8_t actor_grenade_trajectory_blocked(float a, void *b, float *c);       // 0x42b190, not yet rewritten
extern int16_t actor_evaluate_engagement_reachability(uint32_t kind, uint32_t enabled, uint32_t object_index,
                            uint32_t in_vehicle);              // 0x42b270, not yet rewritten
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.
extern float weapon_get_zoom_fov_resolved(void);                               // 0x46fe70: difficulty scale
extern uint8_t weapon_trigger_get_aiming_vector(datum_index weapon_index, int16_t trigger_index,
    real_point3d *origin, real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction,
    real *out_time, real *out_range, uint8_t *out_used_straight_line);
    // 0x4c2b40, src/items; blam-cc: EAX weapon_index, CX trigger_index, 7 stack args
extern float weapon_trigger_projectile_time_fraction(uint32_t handle);                    // 0x4c2be0, not yet rewritten
extern void unit_get_camera_position(void);                    // 0x568f80
extern void unit_add_marker_relative_offset(datum_index unit_index, uint32_t mode, void *point, void *direction,
                         void *offset);                        // 0x569190
extern void unit_set_grenade_type_and_count_delta(int32_t a);                           // 0x56d160, not yet rewritten

// blam-cc: stack -> actor_index
// One tick of the firing state machine. Ages the countdowns, re-derives which threat the
// actor is shooting at and how far away it is, decides whether firing is allowed at all,
// advances the burst state machine (0 idle, 1 hold, 2 swing, 3 pause, 4 special fire), and
// on the swing state builds the actual aim point and tests whether the shot is on target.
// Ends by handing the resulting fire / no-fire decision to 0x42a5e0 and setting or clearing
// actor.flags bit 0x1000.
void actor_update_firing_state(datum_index actor_index)
{
    actor *self;
    Actor *actor_definition;
    ActorVariant *variant;       // from actor.actor_variant_tag
    ActorVariant *aim_variant;   // from actor_get_actor_definition
    void *weapon_definition;
    prop *target;
    datum_index weapon_object;
    int16_t kind;
    int16_t next_state;
    uint8_t may_fire;
    uint8_t off_target;
    uint8_t want_special;
    uint8_t blocked;
    uint8_t stationary;
    uint32_t fire_flag;
    uint32_t fire_value;
    float delay;
    float scale;
    real_point3d aim_from;
    real_vector3d to_target;
    real_point3d *offset;
    float lead;
    int8_t hold_flag;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    actor_definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;
    aim_variant = actor_get_actor_definition(actor_index);

    weapon_definition = (void *)0;
    weapon_object = actor_get_threat_weapon_object_index();
    if (weapon_object != (datum_index)0xffffffff) {
        object *weapon = ((object_header *)object_data->data)[weapon_object & 0xffff].data;
        weapon_definition = tag_instances[weapon->definition_tag & 0xffff].data;
    }
    off_target = 0;
    weapon_object = actor_get_threat_weapon_object_index();

    if (self->unknown_5f4 > 0) { self->unknown_5f4 = self->unknown_5f4 - 1; }
    if (self->unknown_5f6 > 0) { self->unknown_5f6 = self->unknown_5f6 - 1; }
    if (self->unknown_5f8 > 0) { self->unknown_5f8 = self->unknown_5f8 - 1; }
    if (self->unknown_5fc > 0) { self->unknown_5fc = self->unknown_5fc - 1; }
    if (self->unknown_60c > 0) { self->unknown_61c = self->unknown_61c + 1; }

    // ------------------------------------------------------------------ resolve the threat
    if (self->unknown_5f2 != 2) {
        kind = 0;
        if (self->unknown_454 != 0) {
            if (self->unknown_45d == 0) {
                if (self->target_unit_index != (datum_index)0xffffffff) {
                    kind = 1;
                }
            } else {
                kind = 2;
            }
        }
        if (kind != self->unknown_60c ||
            (kind == 1 && self->target_unit_index != (datum_index)self->unknown_610) ||
            // 0x40e949..0x40e955: EAX = actor + 0x610, ECX = actor + 0x460 (both points here)
            (kind == 2 && vector3d_distance_squared((real_point3d *)((uint8_t *)self + 0x610),
                              (real_point3d *)((uint8_t *)self + 0x460)) > 0.25f)) {
            self->unknown_61c = 0;
        }
        self->unknown_60c = kind;
        if (kind == 1) {
            self->unknown_610 = (datum_index)self->target_unit_index;
        } else if (kind == 2) {
            self->unknown_610 = *(datum_index *)((uint8_t *)self + 0x460);
            *(uint32_t *)((uint8_t *)self + 0x614) = *(uint32_t *)((uint8_t *)self + 0x464);
            *(uint32_t *)((uint8_t *)self + 0x618) = *(uint32_t *)((uint8_t *)self + 0x468);
        }
    }

    self->unknown_628 = 0;
    self->vitality_wait_time = (actor_has_unshielded_threat_weapon() == 0) ? 0.0f : aim_variant->maximum_firing_distance;

    may_fire = 0;
    if (self->unknown_45c != 0) {
        // The actor is out of the fight entirely: force the machine back to state 0 and
        // broadcast the "cannot fire" event once.
        if (variant->grenade_type != -1 &&
            *(char *)((uint8_t *)((object_header *)object_data->data)[self->unit_index & 0xffff].data
                      + 0x31e + variant->grenade_type) == 0) {
            unit_set_grenade_type_and_count_delta(1);
        }
        self->flags = self->flags | 0x2000;
        ai_communication_broadcast(9, self->unit_index, 0xffffffff, 0xffffffff, 0xffffffff,
                                   0xffffffff, 0);
        self->unknown_5f2 = 0;
    } else if (actor_has_unshielded_threat_weapon() == 0) {
        self->unknown_5f2 = 0;
    } else if (self->unknown_5f2 == 4) {
        may_fire = 1;
    } else {
        // ---------------------------------------------------- special fire mode roll
        if (aim_variant->special_fire_mode > 0 && self->unknown_5f2 != 2 &&
            self->unknown_5fc < 1 && self->unknown_5fe < 1) {
            object *weapon = ((object_header *)object_data->data)[weapon_object & 0xffff].data;
            void *weapon_tag = tag_instances[weapon->definition_tag & 0xffff].data;
            uint8_t eligible = 0;
            weapon_get_zoom_fov_resolved();
            if (aim_variant->special_fire_mode == 1) {
                weapon_get_zoom_fov_resolved();
                if (*(int32_t *)((uint8_t *)weapon_tag + 0x4fc) > 0) {
                    eligible = 1;
                }
            } else if (aim_variant->special_fire_mode != 2 ||
                       *(int32_t *)((uint8_t *)weapon_tag + 0x4fc) > 1) {
                eligible = 1;
            }
            if (eligible != 0 && actor_grenade_behavior_kind_allowed(aim_variant->special_fire_situation) != 0) {
                delay = random_real_range(0.0f, 1.5f) + aim_variant->special_fire_delay;
                random_real();
                self->unknown_5fc = (int16_t)(int32_t)delay;
                if (delay < aim_variant->special_fire_chance &&
                    actor_target_is_visible_or_object_count_ok(aim_variant->special_fire_situation) != 0) {
                    if (aim_variant->special_fire_situation == 3) {
                        self->unknown_5fe = 3;
                    }
                    if (aim_variant->special_fire_mode == 1) {
                        self->unknown_602 = 1;
                    } else if (aim_variant->special_fire_mode == 2) {
                        self->unknown_604 = 1;
                    }
                }
            }
        }

        // ---------------------------------------------------- refresh the threat block
        if (self->unknown_60c > 0) {
            if (self->unknown_60c == 1) {
                target = &((prop *)prop_data->data)[self->unknown_610 & 0xffff];
                self->wander_unknown_638 = target->distance;
                self->wander_unknown_62c = *(float *)((uint8_t *)target + 0xc8);
                self->wander_unknown_630 = *(float *)((uint8_t *)target + 0xcc);
                self->wander_unknown_634 = *(float *)((uint8_t *)target + 0xd0);
                *(int16_t *)((uint8_t *)self + 0x626) = target->unknown_38;
                *(uint8_t *)((uint8_t *)self + 0x621) = target->unknown_118;
                *(uint8_t *)((uint8_t *)self + 0x624) = 1;
                if (*(int16_t *)((uint8_t *)target + 0x100) != -1) {
                    int16_t player = *(int16_t *)((uint8_t *)target + 0x100);
                    *(uint8_t *)((uint8_t *)self + 0x624) =
                        (uint8_t)(1 - ((player_globals[6 + (player >> 5)] &
                                        (1u << (((uint8_t)player) & 0x1f))) != 0));
                }
            } else {
                self->wander_unknown_62c = *(float *)((uint8_t *)self + 0x610);
                self->wander_unknown_630 = *(float *)((uint8_t *)self + 0x614);
                self->wander_unknown_634 = *(float *)((uint8_t *)self + 0x618);
                self->wander_unknown_638 = vector3d_distance(&self->body_position,
                                                             (real_point3d *)&self->wander_unknown_62c);
                *(uint8_t *)((uint8_t *)self + 0x621) = 0;
                *(uint8_t *)((uint8_t *)self + 0x624) = 0;
                if (self->unknown_61c % 10 == 0) {
                    *(int16_t *)((uint8_t *)self + 0x626) =
                        actor_evaluate_engagement_reachability(0, 0, 0xffffffff,
                                     (uint32_t)(self->active_unit_index !=
                                                (datum_index)0xffffffff));
                }
            }

            *(uint8_t *)((uint8_t *)self + 0x622) = 0;
            if (aim_variant->super_ballistic_range > 0.0f &&
                aim_variant->super_ballistic_range < self->wander_unknown_638) {
                *(uint8_t *)((uint8_t *)self + 0x622) = 1;
            }
            *(uint8_t *)((uint8_t *)self + 0x623) =
                (uint8_t)(self->unknown_455[0] != 0 && aim_variant->bombardment_range > 0.0f);

            // 0x40ed7b..0x40eda7: EAX = [esp+0x1c] (actor_get_threat_weapon_object_index's
            // result), CX = 0 (the first trigger)
            if (weapon_trigger_get_aiming_vector(weapon_object, 0, (real_point3d *)&self->aim_origin,
                             (real_point3d *)&self->wander_unknown_62c,
                             *(uint8_t *)((uint8_t *)self + 0x622),
                             (real_vector3d *)((uint8_t *)self + 0x63c), 0, (real *)((uint8_t *)self + 0x648),
                             (uint8_t *)&blocked) == 0) {
                self->unknown_60c = 0;
            }
        }

        // ---------------------------------------------------- the gate on firing at all
        hold_flag = (int8_t)*(uint8_t *)((uint8_t *)self + 0x457);
        if (self->unknown_60c == 0 || *(uint8_t *)((uint8_t *)self + 0x624) != 0 ||
            (hold_flag == 0 && self->unknown_5f6 > 0) ||
            actor_action_has_queued_secondary() != 0 ||
            (hold_flag == 0 &&
             ((self->unknown_15c != 0 && self->flying == 0 &&
               (((uint8_t *)variant)[0] & 1) == 0) ||
              ((actor_definition->flags & 0x200) != 0 /* must_crouch_to_shoot */ &&
               self->unknown_508 == 0))) ||
            (hold_flag == 0 &&
             (((actor_definition->more_flags & 2) != 0 /* must_stand_to_fire */ &&
               self->unknown_508 != 0) ||
              ((actor_definition->more_flags & 4) != 0 /* must_stop_to_fire */ &&
               self->unknown_504 != 0))) ||
            *(uint8_t *)((uint8_t *)self + 0x621) != 0 || self->unknown_15d != 0 ||
            (weapon_definition != (void *)0 &&
             *(float *)((uint8_t *)weapon_definition + 0x40c) > 0.0f &&
             self->wander_unknown_638 < *(float *)((uint8_t *)weapon_definition + 0x40c)) ||
            self->vocalization_unknown_3e8 == 0 ||
            self->vocalization_unknown_3ec != 2 ||
            *(uint8_t *)((uint8_t *)self + 0x58c) != 0) {
            self->unknown_5f2 = 0;
        } else if (self->unknown_5f2 == 2) {
            may_fire = 1;
        } else {
            int16_t result = *(int16_t *)((uint8_t *)self + 0x626);
            *(uint8_t *)((uint8_t *)self + 0x620) = (uint8_t)(result == 0 || result == 1);
            if ((*(uint8_t *)((uint8_t *)self + 0x620) == 0 &&
                 *(uint8_t *)((uint8_t *)self + 0x623) == 0) ||
                (hold_flag == 0 && self->vitality_wait_time <= self->wander_unknown_638)) {
                self->unknown_5f2 = 0;
            } else {
                may_fire = 1;
                self->unknown_628 = 1;
                if ((actor_definition->flags & 0x2000) == 0 /* start_firing_before_aligned */) {
                    float cone = (self->wander_unknown_638 >= 1.5f)
                                     ? 0.97f
                                     : self->wander_unknown_638 * 0.17526217f + 0.70710677f;
                    actor_get_aim_from_position();
                    if (aim_from.x * *(float *)((uint8_t *)self + 0x63c) +
                        aim_from.z * *(float *)((uint8_t *)self + 0x644) +
                        aim_from.y * *(float *)((uint8_t *)self + 0x640) < cone) {
                        off_target = 1;
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------ state machine
    next_state = self->unknown_5f2;
    switch (self->unknown_5f2) {
    case 0:
        if (may_fire == 0) {
            goto after_switch;
        }
        next_state = 1;
        break;
    case 1:
    case 3:
        if (off_target != 0 || self->unknown_5f4 != 0) {
            goto after_switch;
        }
        next_state = 2;
        break;
    case 2:
        if (self->unknown_5f4 != 0) {
            goto after_switch;
        }
        next_state = 3;
        break;
    case 4:
        if (self->unknown_5f4 != 0) {
            goto after_switch;
        }
        next_state = 0;
        break;
    default:
        goto after_switch;
    }

    if (next_state == 1) {
        if (actor_should_hold_position(actor_index) == 0 && off_target == 0) {
            next_state = 2;
            actor_update_aim_wander(actor_index);
        }
    } else if (next_state == 2) {
        actor_update_aim_wander(actor_index);
    } else if (next_state == 3) {
        actor_reseed_movement_pause_timer(actor_index);
    }
    self->unknown_5f2 = next_state;

after_switch:
    off_target = 0;
    may_fire = 0;
    *(uint8_t *)((uint8_t *)self + 0x688) = 0;

    if (self->unknown_5f2 == 4) {
        off_target = 1;
    } else if (self->unknown_5f2 == 2) {
        // ---------------------------------------------------- build the aim point
        float *aim_point = (float *)((uint8_t *)self + 0x658);
        float *fire_point = (float *)&self->grenade_aim_direction;

        aim_point[0] = self->wander_unknown_64c.i;
        aim_point[1] = self->wander_unknown_64c.j;
        aim_point[2] = self->wander_unknown_64c.k;
        lead = -3.4028235e+38f;

        if (self->unknown_60c == 1) {
            target = &((prop *)prop_data->data)[self->unknown_610 & 0xffff];
            scale = aim_variant->target_tracking;
            lead = *(float *)((uint8_t *)target + 0x114);
            if ((weapon_get_zoom_fov_resolved() + scale >= 1.0f || weapon_get_zoom_fov_resolved() + scale > 0.0f) &&
                *(uint8_t *)((uint8_t *)self + 0x623) == 0) {
                to_target.i = *(float *)((uint8_t *)target + 0xc8) - self->wander_unknown_64c.i;
                to_target.j = *(float *)((uint8_t *)target + 0xcc) - self->wander_unknown_64c.j;
                to_target.k = *(float *)((uint8_t *)target + 0xd0) - self->wander_unknown_64c.k;
                point3d_add_scaled((real_point3d *)aim_point, aim_variant->target_tracking);
            }
            scale = aim_variant->target_leading;
            if (weapon_get_zoom_fov_resolved() + scale >= 1.0f || weapon_get_zoom_fov_resolved() + scale > 0.0f) {
                float flight = weapon_trigger_projectile_time_fraction(*(uint32_t *)((uint8_t *)self + 0x648));
                float weight = aim_variant->target_leading;
                aim_point[0] = flight * *(float *)((uint8_t *)target + 0xd4) * weight + aim_point[0];
                aim_point[1] = flight * *(float *)((uint8_t *)target + 0xd8) * weight + aim_point[1];
                aim_point[2] = flight * *(float *)((uint8_t *)target + 0xdc) * weight + aim_point[2];
            }
        }

        self->wander_unknown_664.i = self->wander_unknown_670.i + self->wander_unknown_664.i;
        self->wander_unknown_664.j = self->wander_unknown_670.j + self->wander_unknown_664.j;
        self->wander_unknown_664.k = self->wander_unknown_670.k + self->wander_unknown_664.k;
        fire_point[0] = self->wander_unknown_664.i + aim_point[0];
        fire_point[1] = aim_point[1] + self->wander_unknown_664.j;
        fire_point[2] = aim_point[2] + self->wander_unknown_664.k;

        // ---------------------------------------------------- pick the firing origin
        if (self->active_unit_index != (datum_index)0xffffffff) {
            unit_get_camera_position();
        } else {
            offset = (real_point3d *)0;
            if (self->unknown_508 == 0) {
                offset = (real_point3d *)&aim_variant->custom_stand_gun_offset;
                if (aim_variant->custom_stand_gun_offset.i * aim_variant->custom_stand_gun_offset.i +
                    aim_variant->custom_stand_gun_offset.j * aim_variant->custom_stand_gun_offset.j +
                    aim_variant->custom_stand_gun_offset.k * aim_variant->custom_stand_gun_offset.k <=
                        0.0001f) {
                    offset = (real_point3d *)&actor_definition->standing_gun_offset;
                    if (vector3d_magnitude_squared((real_vector3d *)offset) <= 0.0001f) { // 0x40f327..0x40f330: EAX = offset
                        offset = (real_point3d *)0;
                    }
                }
            } else {
                offset = (real_point3d *)&aim_variant->custom_crouch_gun_offset;
                if (aim_variant->custom_crouch_gun_offset.i * aim_variant->custom_crouch_gun_offset.i +
                    aim_variant->custom_crouch_gun_offset.j * aim_variant->custom_crouch_gun_offset.j +
                    aim_variant->custom_crouch_gun_offset.k * aim_variant->custom_crouch_gun_offset.k <=
                        0.0001f) {
                    offset = (real_point3d *)&actor_definition->crouching_gun_offset;
                    if (vector3d_magnitude_squared((real_vector3d *)offset) <= 0.0001f) { // 0x40f327..0x40f330: EAX = offset
                        offset = (real_point3d *)0;
                    }
                }
            }
            if (offset != (real_point3d *)0) {
                to_target.i = self->body_position.x - fire_point[0];
                to_target.j = self->body_position.y - fire_point[1];
                to_target.k = self->body_position.z - fire_point[2];
                if (vector2d_normalize_with_length((real_vector2d *)&to_target) <= 0.0f) {
                    to_target.i = self->facing.i;
                    to_target.j = self->facing.j;
                    to_target.k = self->facing.k;
                } else {
                    to_target.k = 0.0f;
                }
                unit_add_marker_relative_offset(self->unit_index, 3, &self->body_position, &to_target, offset);
            } else {
                aim_from.x = self->aim_origin.x;
                aim_from.y = self->aim_origin.y;
                aim_from.z = self->aim_origin.z;
            }
        }

        // ---------------------------------------------------- is the shot on target
        // 0x40f3db..0x40f40b: EAX = the same weapon, CX = (unknown_603 != 0), the secondary trigger
        weapon_trigger_get_aiming_vector(weapon_object, (int16_t)(self->unknown_603 != 0),
                     (real_point3d *)&aim_from, (real_point3d *)fire_point,
                     *(uint8_t *)((uint8_t *)self + 0x622),
                     (real_vector3d *)((uint8_t *)self + 0x68c), 0, (real *)0, (uint8_t *)&blocked);
        *(uint8_t *)((uint8_t *)self + 0x688) = (uint8_t)(blocked == 0);
        to_target.i = fire_point[0] - aim_from.x;
        to_target.j = fire_point[1] - aim_from.y;
        to_target.k = fire_point[2] - aim_from.z;

        if (actor_grenade_trajectory_blocked(lead, &aim_from, &scale) == 0) {
            self->unknown_5fa = self->unknown_5fa + 1;
            self->unknown_5f4 = self->unknown_5f4 + 1;
            if (self->unknown_5fa > 0x2c && self->target_combat_status > 6) {
                uint32_t object_index = 0xffffffff;
                if (scale != -3.4028235e+38f) {
                    object_index = (uint32_t)
                        ((prop *)prop_data->data)[(uint32_t)scale & 0xffff].object_index;
                }
                ai_communication_broadcast(0xe, self->unit_index, object_index, 2, 0xffffffff,
                                           0xffffffff, 0);
                self->unknown_5fa = 0;
            }
        } else {
            self->unknown_5fa = 0;
            if (self->unknown_603 == 0) {
                off_target = 1;
            } else {
                may_fire = 1;
            }
        }
    }

    // ------------------------------------------------------------------ commit
    fire_flag = 0;
    fire_value = 0;
    stationary = 0;

    if (off_target != 0) {
        scale = aim_variant->rate_of_fire;
        if (self->unknown_602 != 0) {
            self->unknown_602 = 0;
        } else if (scale != 0.0f) {
            if (self->unknown_5f8 == 0) {
                float *burst_a = (float *)0;
                float *burst = (float *)0;
                fire_flag = 1;
                fire_value = 0x3f800000; // 1.0f as a bit pattern, exactly as the original
                weapon_get_zoom_fov_resolved();
                actor_select_stance_offset_pair(actor_index, (uint8_t *)aim_variant,
                                                (uint8_t **)&burst_a, (uint8_t **)&burst);
                scale = 30.0f;
                if (burst != (float *)0 && burst[2] > 0.0f) {
                    scale = 30.0f / burst[2];
                }
                self->unknown_5f8 = (scale < 2.0f) ? 2 : (int16_t)(int32_t)(scale + 0.5f);
            }
        } else {
            fire_flag = 1;
            fire_value = 0x3f800000;
        }
    } else if (may_fire != 0) {
        if (self->unknown_603 == 0) {
            stationary = 1;
        } else {
            self->unknown_603 = 0;
        }
    } else if (self->unknown_602 != 0) {
        fire_flag = 1;
        fire_value = 0x3f800000;
    }

    actor_set_override_target(fire_flag, fire_value);

    if (stationary != 0) {
        self->flags = self->flags | 0x1000;
    } else {
        self->flags = self->flags & 0xffffefff;
    }
}

#if 0
Original Ghidra decompilation (0x40e7b0): run `python tools/pack.py 0x40e7b0`.
The listing is 300+ lines with the frame spelled out as aliased locals (local_30 is first the
Actor tag pointer and later a byte pair, local_24 is first the weapon tag pointer and later a
cosine, local_18 / local_14 / local_10 are read before assignment on one branch). It is kept
in out/phase2/ai rather than duplicated here; the aliasing is described in the UNSURE block
at the top of this file.
#endif
