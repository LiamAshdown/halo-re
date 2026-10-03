#include "halo/objects/object_damage.hpp"
#include "game.h"
#include "units.h"
#include "effects.h"
#include "networking.h"
#include "physics.h"
#include "hs.h"
#include "projectiles.h"
#include <stdint.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"

extern "C" {
extern uint8_t actor_apply_perception_scale(datum_index actor_index, const uint8_t *zone, float *in_out_value);
extern game_engine_definition *current_game_engine;
extern void damage_data_initialize(damage_data *dd, datum_index damage_effect_tag);
extern void damage_effect_new_at_location(datum_index effect_tag, int16_t node_index, real_vector3d *normal, real_vector3d *incident, real_point3d *impact_position, uint32_t object_index);
extern ModelCollisionGeometryMaterial default_collision_material;
extern uint8_t g_00689481;
extern uint8_t g_006f1cf4;
extern uint8_t g_0087abc0;
extern uint8_t g_0087abc5;
extern uint8_t g_0087abc7;
extern void game_engine_attribute_player_death(datum_index victim_unit, datum_index killer, datum_index death_object, int32_t killer_team, char credit_kills);
extern float game_engine_compute_time_scale(int32_t param_a, int32_t param_b);
extern void game_engine_on_player_death(datum_index killer, datum_index death_object, datum_index victim, char is_suicide);
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
extern game_engine_state game_engine_state_value;
extern uint8_t game_engine_teams_enabled_flag;
extern game_time_globals *game_time;
extern real_vector3d *global_down3d_pointer;
extern Globals *global_globals;
extern real_vector3d *global_origin3d_pointer;
extern int16_t global_structure_bsp_index;
extern int32_t hash_table_get(hash_table *table, uint32_t key);
extern void hud_unit_meter_apply_predictive_damage(datum_index player_index, float damage);
extern player_globals *local_player_globals;
extern game_main_globals *main_game_globals;
extern int8_t message_delta_decode_compound_field(void *globals, void *out_value);
extern uint8_t message_delta_decode_compound_field_staged(void **context);
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern int16_t network_game_mode;
extern uint8_t network_message_scratch[0x7ff8];
extern network_server_globals *network_server;
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern void object_apply_body_damage(uint32_t target_index, int32_t region_index, int32_t node_index, void *plane, uint8_t *geometry, uint8_t *material, uint8_t *effect_block, damage_data *dd, uint32_t *notify_flags, float *body_damage_out, float *material_multiplier_out, float damage, uint8_t is_local);
extern void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t node_index, int16_t region_index, int16_t material_index, uint32_t plane);
extern void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *delta_velocity);
extern void object_apply_shield_damage(uint32_t target_index, uint8_t *geometry, uint8_t *material, uint8_t *effect_block, uint32_t *notify_flags, float *shield_damage_out, float *remaining_damage, uint8_t is_local, uint8_t apply_state, object_shield_impulse_result *record);
extern void object_damage_apply_line_of_sight(damage_data *dd, datum_index target_index, int8_t continue_flag);
extern void object_damage_notify_and_impulse(uint32_t target_index, damage_data *dd, uint32_t notify_flags, float shield_damage, float body_damage, uint32_t unused_6, int32_t region_index, uint32_t is_local);
extern data_array *object_data;
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings);
extern void object_delete_teardown(uint32_t object_index);
extern void object_delete_unparented(uint32_t object_index);
extern void object_destroy_region(uint32_t object_index, int32_t region_index);
extern void object_dispatch_effect_notify(uint32_t forwarded_eax, uint32_t forwarded_ecx);
extern uint16_t object_find_in_sphere(int32_t unknown_0, int32_t unknown_1, int32_t *location_leaf_index, real_point3d *center, float radius, datum_index *out_buffer, int32_t max_count);
extern int32_t object_get_controlling_player_index(datum_index object_index);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern uint32_t object_get_root_object_index(uint32_t object_index);
extern network_id_table *object_network_id_table;
extern void object_notify_pickup_or_refresh_probe(uint32_t object_index, datum_index player_index);
extern void object_queue_pickup_denied_event(void *param_1, int32_t key, uint32_t *source);
extern void object_regions_reset_permutation_lock(uint32_t object_index, int8_t unlock);
extern void object_set_health_frozen_flag(uint32_t object_index);
extern void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter, char use_matched_index);
extern void object_set_shield_depleted_flag(uint32_t object_index);
extern int32_t object_sound_event_last_tick;
extern void object_throttled_multiplayer_sound_event(void);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void object_type_definitions_notify_region_damage(uint32_t object_index, uint32_t argument_1, uint32_t argument_2);
extern data_array *player_data;
extern datum_index player_index_from_unit_index(datum_index unit_index);
extern uint8_t *team_pair_data;
extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b);
extern uint8_t unit_any_flagged_seat_occupied(uint32_t unit_index);
extern void unit_apply_damage_effects(datum_index unit_index, damage_data *dd, uint32_t flags, float shield_damage, float body_damage, int32_t region_index, uint8_t is_local);
extern void unit_apply_impulse(uint32_t object_index, real_vector3d *impulse);
extern void unit_apply_impulse_to_seat(uint32_t unit_index, real_vector3d *impulse);
extern uint8_t unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index);
extern void unit_update_stance_and_jump(uint32_t unit_index, uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still);
extern real weapon_get_zoom_fov(int16_t zoom_table_index, int16_t magnification);
extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index);
}

/**
 * Initialises the object's vitality maxima and stun thresholds, optionally from override values.
 *
 * Original register convention: uint32_t object_index in EAX (in_EAX); float *override_max_body_vitality.
 *
 * @address 0x004ed440
 */
void halo::objects::ObjectDamage::initialize_shield_stun_thresholds(float *override_max_body_vitality,
    float *override_max_shield_vitality)
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    float max_body_vitality = 0.0f;
    float max_shield_vitality = 0.0f;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;
    TagID collision_model = definition->collision_model.tag_id;

    if (collision_model.index != 0xffff) {
        ModelCollisionGeometry *geometry = (ModelCollisionGeometry *)halo::cache::globals().tag_instances[collision_model.index].data;
        if (geometry != 0) {
            max_body_vitality = geometry->maximum_body_vitality;
            max_shield_vitality = geometry->maximum_shield_vitality;
        }
    }

    if (override_max_body_vitality != 0) {
        max_body_vitality = *override_max_body_vitality;
    }
    if (override_max_shield_vitality != 0) {
        max_shield_vitality = *override_max_shield_vitality;
    }

    obj->maximum_shield_vitality = max_shield_vitality;
    obj->maximum_body_vitality = max_body_vitality;
    obj->body_vitality = (max_body_vitality > 0.0f) ? 1.0f : 0.0f;
    obj->shield_vitality = (max_shield_vitality > 0.0f) ? 1.0f : 0.0f;
}

namespace {
static void object_decay_damage_timer(int32_t *ticks, float *current, float *recent)
{
    int32_t t = *ticks;
    float current_value;
    float recent_value;

    if (t == -1) {
        return;
    }
    t++;
    *ticks = t;
    if (t >= 0) {
        *current = *current - 0.016666668f;
    }
    if (t >= 0x3c) {
        *recent = *recent - 0.016666668f;
    }
    current_value = (0.0f > *current) ? 0.0f : *current;
    *current = current_value;
    recent_value = (0.0f > *recent) ? 0.0f : *recent;
    *recent = recent_value;
    if (current_value == 0.0f && recent_value == 0.0f) {
        *ticks = -1;
    }
}
}

/**
 * Applies shield and body regeneration, damage timers and the resulting state changes for the tick.
 *
 * Original register convention: stack=object_index.
 *
 * @address 0x004ed510
 */
void halo::objects::ObjectDamage::update_vitality_and_regeneration()
{
    uint32_t object_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)obj & 0xffff].data;
    uint16_t *vitality_flags = (uint16_t *)(obj + 0x106);
    float *shield = (float *)(obj + 0xe4);

    if (*(datum_index *)&((struct Object *)object_tag)->collision_model.tag_id != k_datum_index_none) {
        uint8_t *geometry = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)&((struct Object *)object_tag)->collision_model.tag_id & 0xffff].data;

        if (geometry != 0) {
            uint16_t flags = *vitality_flags;
            uint16_t kill_request = (uint16_t)(flags & 0x2000);

            if (kill_request || (flags & 0x60)) {
                datum_index effect = *(datum_index *)((uint8_t *)global_globals->falling_damage.pointer + 0x1c);

                if ((flags & 4) == 0 && effect != k_datum_index_none) {
                    damage_data dd;

                    damage_data_initialize(&dd, effect);
                    dd.flags |= 4;
                    dd.random_blend = 1.0f;
                    if (flags & 0x40) {
                        dd.flags |= 0x10;
                    }
                    if (kill_request) {
                        dd.flags |= 0x80;
                    }
                    object_apply_damage(&dd, object_index, -1, -1, -1, 0);
                }
                *vitality_flags &= 0xdf9f;
            }
            obj[0x107] &= 0xef;
            if (((object *)obj)->maximum_shield_vitality > 0.0f && (*vitality_flags & 4) == 0) {
                uint16_t current = *vitality_flags;

                if (current & 0x10) {
                    float value = *shield + 0.033333335f;

                    *shield = value;
                    if (value < 3.0f) {
                        *vitality_flags = (uint16_t)(current | 0x1000);
                    } else {
                        *shield = 3.0f;
                        *vitality_flags = (uint16_t)(current & 0xffef);
                    }
                } else if (*shield > 1.0f && current_game_engine != 0) {
                    datum_index player_index = player_index_from_unit_index(object_index);
                    float excess = *shield - 1.0f;

                    if (0.00074074074f > excess) {
                        *shield = 1.0f;
                        hud_unit_meter_apply_predictive_damage(player_index, excess);
                    } else {
                        *shield = *shield - 0.00074074074f;
                        hud_unit_meter_apply_predictive_damage(player_index, 0.00074074074f);
                    }
                } else if (*shield < 1.0f) {
                    int16_t stun = ((object *)obj)->shield_stun_ticks;

                    if (stun == 0) {
                        float rate = weapon_get_zoom_fov_resolved(3, ((object *)obj)->owner_team) *
                            *(float *)(geometry + 0x1c0);
                        float value;

                        if (obj[0x106] & 8) {
                            object_dispatch_effect_notify(object_index, *(uint32_t *)(geometry + 0x1b4));
                            obj[0x106] &= 0xf7;
                            object_regions_reset_permutation_lock(object_index, 1);
                        }
                        obj[0x107] |= 0x10;
                        value = rate + *shield;
                        *shield = value;
                        if (value > 1.0f) {
                            *shield = 1.0f;
                            *vitality_flags &= 0xefff;
                        }
                    } else if (((object *)obj)->network_role == 3 || ((object *)obj)->network_role == 0) {
                        ((object *)obj)->shield_stun_ticks = (int16_t)(stun - 1);
                    }
                }
            }
            object_decay_damage_timer((int32_t *)(obj + 0x100), (float *)(obj + 0xec), (float *)(obj + 0xf8));
            object_decay_damage_timer((int32_t *)(obj + 0xfc), (float *)(obj + 0xe8), (float *)(obj + 0xf4));
        }
    }
    if (((object *)obj)->type == 0 && ((object *)obj)->network_role == 0) {
        uint8_t *object = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;

        if ((uint32_t)(((struct object *)object)->shield_stun_ticks > 0) != (uint32_t)object[0x538]) {
            ((struct object *)object)->flags |= 0x4000000;
        }
    }
}

/**
 * Initialises a damage_data record for the given damage effect tag with default flags, multipliers and unset
 * responsible party.
 *
 * Original register convention: damage_data *dd in EDX (in_EDX); datum_index damage_effect_tag on the.
 *
 * @address 0x004ed990
 */
void halo::objects::DamageDataView::initialize(datum_index damage_effect_tag)
{
    damage_data *dd = self;
    uint32_t *words = (uint32_t *)dd;
    int i;

    for (i = 0; i < 0x15; i++) {
        words[i] = 0;
    }

    dd->damage_effect_tag = damage_effect_tag;
    dd->material_type = (int16_t)0xffff;
    dd->responsible_player = (datum_index)0xffffffff;
    dd->responsible_object = (datum_index)0xffffffff;
    dd->team_index = (int16_t)0xffff;
    dd->location_cluster_index = (int16_t)0xffff;
    dd->random_blend = 1.0f;
    dd->multiplier = 1.0f;
}

/**
 * Restores the object's body vitality to its maximum; returns whether anything changed.
 *
 * Original register convention: uint32_t object_index in EAX (in_EAX). Returns a bool (1 when the.
 *
 * @address 0x004ed9d0
 */
uint8_t halo::objects::ObjectDamage::restore_full_body_vitality()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return 0;
    }

    if (obj->body_vitality < 1.0f) {
        obj->body_vitality = 1.0f;
        obj->flags |= _object_changed_bit;
        return 1;
    }

    return 0;
}

/**
 * Sets the object's health frozen flag and updates dependent state.
 *
 * Original register convention: uint32_t object_index in EAX (in_EAX).
 *
 * @address 0x004eda20
 */
void halo::objects::ObjectDamage::set_health_frozen_flag()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return;
    }

    obj->vitality_flags |= _object_health_frozen_bit;

    if (((Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data)->collision_model.tag_id.index != 0xffff) {

        halo::effects::effect_new_on_object(object_index,
            *(datum_index *)((uint8_t *)halo::cache::globals().tag_instances[((Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data)
                ->collision_model.tag_id.index].data + 0xb4),
            object_index, -1, 0.0f, 0.0f, 0, 0);
    }

    if (obj->type == _object_type_vehicle) {
        datum_index child_index = obj->first_child_object;
        while (child_index != (datum_index)0xffffffff) {
            object *child = headers[child_index & 0xffff].data;
            uint8_t *child_bytes = (uint8_t *)child;

            if (child->type == _object_type_biped &&
                (*(int32_t *)(child_bytes + 0x218) == -1 || g_0087abc0 == 0) &&
                *(int16_t *)(child_bytes + 0x2f0) != -1) {
                child->vitality_flags |= 0x0020;
            }
            child_index = child->next_object;
        }
    }

    object_set_shield_depleted_flag(object_index);
}

/**
 * Sets the shield depleted flag and starts the recharge delay.
 *
 * Original register convention: uint32_t object_index in EDI (unaff_EDI).
 *
 * @address 0x004edb10
 */
void halo::objects::ObjectDamage::set_shield_depleted_flag()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if ((obj->vitality_flags & _object_shield_depleted_bit) != 0) {
        return;
    }

    {
        datum_index collision_model = *(datum_index *)&((Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data)->collision_model.tag_id;

        if (collision_model != k_datum_index_none) {

            halo::effects::effect_new_on_object(object_index,
                *(datum_index *)((uint8_t *)halo::cache::globals().tag_instances[collision_model & 0xffff].data + 0x1a4),
                object_index, -1, 0.0f, 0.0f, 0, 0);
        }
    }

    obj->vitality_flags |= _object_shield_depleted_bit;
    obj->current_shield_damage = 0.0f;

    object_regions_reset_permutation_lock(object_index, 0);
}

/**
 * Starts shield recharging; returns whether it started.
 *
 * Original register convention: uint32_t object_index in EAX (in_EAX). Returns a bool (1 when recharge.
 *
 * @address 0x004edba0
 */
uint8_t halo::objects::ObjectDamage::shield_recharge_start()
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;

    if (obj->shield_vitality <= 1.0f) {
        obj->vitality_flags |= _object_shield_recharging_bit;
        if (obj->shield_vitality == 0.0f) {
            obj->shield_vitality = 0.01f;
        }
        obj->shield_stun_ticks = 0;
        *((uint8_t *)obj + 0x122) = 1;

        return 1;
    }

    return 0;
}

/**
 * Applies a damage event with an area of effect to the objects within its radius, collecting up to 64 candidates.
 *
 * Original register convention: damage_data *dd on the stack (param_1).
 *
 * @address 0x004edd30
 */
void halo::objects::DamageDataView::apply_area_effect()
{
    damage_data *dd = self;
    datum_index candidates[k_maximum_damage_candidates + 1];
    DamageEffect *effect = (DamageEffect *)halo::cache::globals().tag_instances[dd->damage_effect_tag & 0xffff].data;
    int16_t found = (int16_t)object_find_in_sphere(0, 0, &dd->location_leaf_index, &dd->epicentre,
        effect->radius[1], candidates, k_maximum_damage_candidates);

    if (found > 0) {
        int32_t i;
        for (i = 0; i < found; i++) {
            object_damage_apply_line_of_sight(dd, candidates[i], 0);
        }
    }

    halo::physics::breakable_surface_damage_in_blast_radius(dd);
}

namespace {
#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)
}

/**
 * Applies a damage event to a target after testing line of sight from the epicentre.
 *
 * Original register convention: stack -> damage, object_index, recurse_siblings.
 *
 * @address 0x004eddb0
 */
void halo::objects::ObjectDamage::apply_line_of_sight(damage_data *dd, int8_t continue_flag)
{
    datum_index target_index = handle;
    uint8_t *effect = TAG_DATA(dd->damage_effect_tag);
    real_point3d *origin = &dd->origin;

    for (;;) {
        uint8_t *target = OBJECT_DATA(target_index);
        uint8_t *target_tag = TAG_DATA(*(datum_index *)target);
        uint8_t apply = (uint8_t)(~target[0x10] & 1);
        uint8_t applied = 0;
        uint8_t spared_player = 0;
        uint8_t blocked;
        uint32_t flags;

        if (apply && ((1u << (target[0xb4] & 0x1f)) & 3) && *(float *)(effect + 0x1cc) > 9.999999747378752e-05f) {

            real_vector3d to_center;
            real_vector3d side_a;
            real_vector3d side_b;
            int32_t i;

            blocked = 1;
            to_center.i = ((struct object *)target)->bounding_center.x - origin->x;
            to_center.j = ((struct object *)target)->bounding_center.y - origin->y;
            to_center.k = ((struct object *)target)->bounding_center.z - origin->z;
            halo::math::vector3d_build_perpendicular(side_a, to_center);
            halo::math::vector3d_normalize_with_length(side_a);
            halo::math::vector3d_cross_product(side_b, side_a, to_center);
            halo::math::vector3d_normalize_with_length(side_b);
            for (i = 0; i < 4; i++) {
                real radius = *(float *)(effect + 0x1cc);
                real_vector3d *side = i < 2 ? &side_a : &side_b;
                real_vector3d offset;
                real_point3d sample;
                real_vector3d back;
                collision_result hit;

                if (i & 1) {
                    radius = -radius;
                }
                offset.i = side->i * radius;
                offset.j = side->j * radius;
                offset.k = side->k * radius;
                halo::physics::collision_test_movement_segment(0xc221, origin, &offset, object_get_root_object_index(target_index), &hit);
                sample = hit.point;
                back.i = ((struct object *)target)->bounding_center.x - sample.x;
                back.j = ((struct object *)target)->bounding_center.y - sample.y;
                back.k = ((struct object *)target)->bounding_center.z - sample.z;
                if (!halo::physics::collision_test_movement_segment(0xc221, &sample, &back, object_get_root_object_index(target_index), &hit)) {
                    blocked = 0;
                }
            }
        } else {

            datum_index root = k_datum_index_none;
            datum_index walk = target_index;
            real_vector3d to_center;
            collision_result hit;

            while (walk != k_datum_index_none) {
                root = walk;
                walk = *(datum_index *)(OBJECT_DATA(walk) + 0x11c);
            }
            to_center.i = ((struct object *)target)->bounding_center.x - origin->x;
            to_center.j = ((struct object *)target)->bounding_center.y - origin->y;
            to_center.k = ((struct object *)target)->bounding_center.z - origin->z;
            blocked = halo::physics::collision_test_movement_segment(0xc221, origin, &to_center, root, &hit);
        }
        if (blocked) {
            apply = 0;
        }

        flags = *(uint32_t *)(effect + 0x1c8);
        if ((flags & 1) && target_index == dd->responsible_object) {
            apply = 0;
        }
        if ((flags & 8) && !teams_are_enemies(dd->team_index, ((struct object *)target)->owner_team)) {
            apply = 0;
        } else if (apply && (flags & 0x1000)) {
            apply = 0;
            if (((1u << (target[0xb4] & 0x1f)) & 3) &&
                (*(uint32_t *)(TAG_DATA(*(datum_index *)target) + 0x17c) & 0x80000) &&
                target_index != dd->responsible_object) {
                real scale = weapon_get_zoom_fov(8, main_game_globals->difficulty);

                apply = 1;
                if ((scale > 0.0f || (flags & 0x400)) && (dd->flags & 0x40)) {
                    apply = 0;
                }
                if (scale > 0.0f && scale * 0.25f > halo::math::random_real()) {
                    apply = 0;
                }
                spared_player = 1;
            }
        }
        dd->flags |= 1;

        if (apply) {

            real distance;
            real blend;
            real range;

            dd->direction.i = ((struct object *)target)->bounding_center.x - origin->x;
            dd->direction.j = ((struct object *)target)->bounding_center.y - origin->y;
            dd->direction.k = ((struct object *)target)->bounding_center.z - origin->z;
            distance = halo::math::vector3d_normalize_with_length(dd->direction);
            range = *(float *)(effect + 0x4) - *(float *)(effect + 0x0);
            if (range > 0.0f) {
                blend = 1.0f - (distance - *(float *)(effect + 0x0)) / range;
                if (!(blend >= 0.0f)) {
                    blend = 0.0f;
                } else if (!(blend <= 1.0f)) {
                    blend = 1.0f;
                }
            } else {
                blend = 1.0f;
            }
            if (!(effect[0xc] & 1)) {
                dd->random_blend = blend;
            }
            if (blend > 0.0f) {
                object_apply_damage(dd, target_index, -1, -1, -1, 0);
                applied = 1;
            }
            {
                datum_index model = *(datum_index *)(target_tag + 0x7c);

                if (model != k_datum_index_none && (*TAG_DATA(model) & 8) &&
                    ((struct object *)target)->first_child_object != k_datum_index_none) {
                    object_damage_apply_line_of_sight(dd, ((struct object *)target)->first_child_object, 1);
                }
            }
        }
        if (spared_player && (!apply || !applied)) {
            dd->flags |= 0x40;
        }
        if (!continue_flag || ((struct object *)target)->next_object == k_datum_index_none) {
            return;
        }
        continue_flag = 1;
        target_index = ((struct object *)target)->next_object;
    }
}
#undef OBJECT_DATA
#undef TAG_DATA

/**
 * Raises a multiplayer sound event at a throttled rate.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004ee370
 */
void halo::objects::DamageSystem::throttled_multiplayer_sound_event()
{
    if (g_00689481 == 1 && (uint32_t)(object_sound_event_last_tick + 2) < (uint32_t)game_time->game_time) {
        game_engine_queue_multiplayer_sound(0x2b, 0xffffffff, 0);
        object_sound_event_last_tick = game_time->game_time;
    }
}

/**
 * Decodes a networked shield charge message, applies it and notifies the object.
 *
 * Original register convention: a pointer-to-pointer parameter in EAX (in_EAX).
 *
 * @address 0x004ee4d0
 */
void halo::objects::DamageSystem::apply_shield_charge_and_notify(void **message)
{
    int32_t network_id;
    float shield_damage;
    int8_t notify;

    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged(0);
        return;
    }

    if (message_delta_decode_compound_field(message, &network_id) == 0) {
        return;
    }

    if (network_id != 0) {
        int32_t effect = ((int32_t *)object_network_id_table->handles)[network_id];

        if (effect != -1) {
            object *target = object_try_and_get(effect, _object_mask_unit);

            if (target != 0) {
                if (0.0f < shield_damage) {
                    target->shield_damage_ticks = 0;
                    if ((target->vitality_flags & _object_shield_depleted_bit) == 0) {
                        target->current_shield_damage = 1.0f;
                    }
                    target->recent_shield_damage = shield_damage + target->recent_shield_damage;
                    if (1.0f < target->current_shield_damage) {
                        target->current_shield_damage = 1.0f;
                    }
                    if (1.0f < target->recent_shield_damage) {
                        target->recent_shield_damage = 1.0f;
                    }
                }
                if (notify == 1) {
                    object_set_shield_depleted_flag(0);
                }
                unit_update_stance_and_jump(effect, 0, 0, 0, 0, 0, 0, 0xffffffff, 0, 0);
            }
        }
    }

    object_throttled_multiplayer_sound_event();
}

namespace {
static uint8_t *object_get(datum_index object_index)
{
    return (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;
}

static uint8_t *tag_get(datum_index tag_index)
{
    return (uint8_t *)halo::cache::globals().tag_instances[tag_index & 0xffff].data;
}

static uint8_t teams_are_friends(int32_t bit)
{
    return (uint8_t)((*(uint32_t *)(team_pair_data + 0xa4 + (bit >> 5) * 4) >> (bit & 0x1f)) & 1);
}

static player *player_try_get(datum_index player_index)
{
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    int16_t identifier;
    uint8_t *record;

    if (player_index == k_datum_index_none || index < 0 || index >= player_data->maximum_count) {
        return 0;
    }
    record = (uint8_t *)player_data->data + player_data->size * index;
    identifier = *(int16_t *)record;
    if (identifier == 0 || (salt != 0 && identifier != salt)) {
        return 0;
    }
    return (player *)record;
}
}

/**
 * Applies a damage event to a target object: resolves the hit region and material, splits the damage between shield
 * and body and triggers the follow-up notifications.
 *
 * Original register convention: stack=(dd, target_object_index, node_index, region_index, material_index, plane).
 *
 * @address 0x004ee5e0
 */
void halo::objects::ObjectDamage::apply_damage(damage_data *dd, int16_t hit_node_index, int16_t hit_region_index,
    int16_t hit_material_index, uint32_t hit_plane)
{
    uint32_t target_object_index = handle;
    datum_index target_index = target_object_index;
    int16_t node_index = hit_node_index;
    int16_t region_index = hit_region_index;
    int16_t material_index = hit_material_index;
    uint8_t *target = object_get(target_index);
    uint8_t *effect_block = tag_get(dd->damage_effect_tag) + 0x1c4;
    int32_t target_role = ((struct object *)target)->network_role;
    uint8_t target_is_local = (target_role == 0 || target_role == 3);
    uint8_t difficulty_scaled = 0;
    uint8_t parents_take_damage = 1;
    uint8_t no_random_range;
    uint8_t reported = 0;
    datum_index list[17];
    int16_t count = 0;
    int16_t remaining;
    uint32_t flags;
    uint8_t *target_tag;
    float amount;

    if (dd->responsible_player != k_datum_index_none && player_try_get(dd->responsible_player) == 0) {
        dd->responsible_player = k_datum_index_none;
    }
    halo::math::globals().random_seed_global = halo::math::globals().random_seed_global * 0x19660d + 0x3c6ef35f;
    amount = ((*(float *)(effect_block + 0x14) - *(float *)(effect_block + 0x10)) *
              ((float)(int32_t)(halo::math::globals().random_seed_global >> 16) * 1.5259022e-05f) + *(float *)(effect_block + 0x10)) *
             dd->random_blend + (1.0f - dd->random_blend) * *(float *)(effect_block + 0x0c);
    amount = amount * dd->multiplier;
    if (dd->responsible_object != k_datum_index_none) {
        uint8_t *responsible = (uint8_t *)object_try_and_get(dd->responsible_object, 3);

        if (responsible != 0) {
            datum_index actor;

            if (*(datum_index *)(responsible + 0x328) != k_datum_index_none) {
                responsible = object_get(*(datum_index *)(responsible + 0x328));
            }
            actor = ((unit_object *)responsible)->unit.swarm_actor_index;
            if (actor == k_datum_index_none) {
                actor = ((unit_object *)responsible)->unit.actor_index;
            }
            if (actor != k_datum_index_none) {
                actor_apply_perception_scale(actor, (const uint8_t *)dd, &amount);
            }
        }
    }
    if (current_game_engine != 0) {
        int32_t target_player = object_get_controlling_player_index(target_index);
        int32_t responsible_player = object_get_controlling_player_index(dd->responsible_object);

        amount = game_engine_compute_time_scale(responsible_player, target_player) * amount;
    } else if (dd->team_index != -1) {
        int16_t team = dd->team_index;

        if (team < 0 || team >= 10 || !teams_are_friends(team * 10 + 1)) {
            amount = weapon_get_zoom_fov(0, main_game_globals->difficulty) * amount;
            difficulty_scaled = 1;
        }
    }

    flags = dd->flags;
    if ((flags & 1) || (flags & 4)) {
        list[0] = target_index;
        count = 1;
    } else if (target_index != k_datum_index_none) {
        datum_index id = target_index;

        do {
            list[count++] = id;
            id = *(datum_index *)(object_get(id) + 0x11c);
        } while (id != k_datum_index_none);
    }
    target_tag = tag_get(*(datum_index *)target);
    if (*(datum_index *)(target_tag + 0x7c) != k_datum_index_none) {
        parents_take_damage = (uint8_t)(~(*(uint32_t *)tag_get(*(datum_index *)(target_tag + 0x7c)) >> 4) & 1);
    }
    if (((struct object *)target)->damage_owner != k_datum_index_none) {
        list[count++] = ((struct object *)target)->damage_owner;
    }

    if ((flags & 1) == 0 && ((struct object *)target)->type == 1) {
        float rider_fraction = (1.0f - *(float *)(effect_block + 0x18)) * *(float *)(target_tag + 0x184);
        datum_index child;

        dd->multiplier = rider_fraction;
        if (current_game_engine != 0 && ((struct object *)target)->first_child_object != k_datum_index_none) {
            int32_t players = 0;

            for (child = ((struct object *)target)->first_child_object; child != k_datum_index_none;
                 child = *(datum_index *)(object_get(child) + 0x114)) {
                uint8_t *rider = object_get(child);

                if (((struct object *)rider)->type == 0 && *(datum_index *)(rider + 0x218) != k_datum_index_none) {
                    players++;
                }
            }
            if (players != 0) {
                dd->multiplier = rider_fraction / (float)players;
            }
        }
        for (child = ((struct object *)target)->first_child_object; child != k_datum_index_none;
             child = *(datum_index *)(object_get(child) + 0x114)) {
            uint8_t *rider = object_get(child);

            if (((struct object *)rider)->type != 0) {
                continue;
            }
            if (*(datum_index *)(rider + 0x218) == k_datum_index_none) {
                if (child != *(datum_index *)(target + 0x324)) {
                    continue;
                }
                dd->flags |= 0x20;
            } else {
                dd->flags &= ~0x20u;
            }
            object_apply_damage(dd, child, -1, -1, -1, 0);
            dd->flags &= ~0x20u;
        }
        dd->multiplier = 1.0f;
    }

    no_random_range = (*(float *)(effect_block + 0x10) == 0.0f && *(float *)(effect_block + 0x14) == 0.0f);
    {
        int16_t i;

        for (i = 0; i < count; i++) {
            datum_index id = list[i];
            int16_t index = (int16_t)id;
            int16_t salt = (int16_t)(id >> 16);
            uint8_t *header;
            uint8_t *unit;
            datum_index player_index;

            if (id == k_datum_index_none || index < 0 || index >= object_data->maximum_count) {
                continue;
            }
            header = (uint8_t *)object_data->data + object_data->size * index;
            if (*(int16_t *)header == 0 || (salt != 0 && *(int16_t *)header != salt)) {
                continue;
            }
            if (((1u << (header[3] & 0x1f)) & 3) == 0) {
                continue;
            }
            unit = *(uint8_t **)(header + 0x8);
            if (unit == 0) {
                continue;
            }
            player_index = ((unit_object *)unit)->unit.controlling_player;
            if (player_index != k_datum_index_none) {
                switch (network_game_mode) {
                case 0:
                    halo::effects::player_effect_mark_damage_direction(player_index, dd, &dd->direction, dd->random_blend, amount);
                    break;
                case 1:
                    if (no_random_range == 1) {
                        halo::effects::player_effect_mark_damage_direction(player_index, dd, &dd->direction, dd->random_blend,
                            amount);
                    }
                    break;
                case 2:
                    if (no_random_range) {
                        halo::effects::player_effect_mark_damage_direction(player_index, dd, &dd->direction, dd->random_blend,
                            amount);
                    } else {
                        halo::effects::player_effect_send_network_update(player_index, &dd->direction, dd, dd->random_blend,
                            amount);
                    }
                    break;
                }
            } else if (g_0087abc5) {
                datum_index first_local = local_player_globals->local_players[0];

                if (network_game_mode == 0) {
                    halo::effects::player_effect_mark_damage_direction(first_local, dd, &dd->direction, dd->random_blend, amount);
                } else if (network_game_mode == 2) {
                    halo::effects::player_effect_send_network_update(first_local, &dd->direction, dd, dd->random_blend, amount);
                }
            }
        }
    }

    if (!(amount > 0.0f)) {
        return;
    }
    remaining = count;
    for (;;) {
        int16_t before = remaining;
        int16_t i;
        datum_index id;
        uint8_t *obj;
        uint8_t *object_tag;
        uint8_t *geometry;
        uint8_t *material;
        uint32_t notify_flags = 0;
        float shield_damage = 0.0f;
        float body_damage = 0.0f;
        float material_multiplier = 0.0f;
        int32_t region = -1;
        object_shield_impulse_result record;
        uint8_t kill;
        uint8_t friendly = 0;
        uint8_t shield_allowed = 1;
        uint8_t body_allowed = 1;
        uint8_t apply_state;

        remaining--;
        if (before <= 0) {
            break;
        }
        i = remaining;
        id = list[i];
        obj = object_get(id);
        object_tag = tag_get(*(datum_index *)obj);
        if (*(datum_index *)(object_tag + 0x7c) == k_datum_index_none) {
            goto notify;
        }
        geometry = tag_get(*(datum_index *)(object_tag + 0x7c));
        kill = (uint8_t)((dd->flags >> 2) & 1);
        *(datum_index *)record.unknown_00 = id;
        record.shield_damage_dealt = 0.0f;
        record.depleted_this_call = 0;
        if (((object *)obj)->network_role == 3 || ((object *)obj)->network_role == 0) {
            apply_state = 1;
        } else {
            player *responsible = player_try_get(dd->responsible_player);

            apply_state = (responsible != 0 && responsible->local_player_index != -1) ? 0 : 1;
        }
        if (current_game_engine != 0 && game_engine_teams_enabled_flag) {
            datum_index owner = player_index_from_unit_index(id);

            if (owner != k_datum_index_none && owner != dd->responsible_player) {
                player *owner_record = player_try_get(owner);

                if (owner_record != 0) {
                    friendly = (uint8_t)(teams_are_enemies(dd->team_index,
                        *(int16_t *)&((struct player *)owner_record)->team) == 0);
                    if (friendly) {
                        switch (g_006f1cf4) {
                        case 0:
                            shield_allowed = 0;
                            body_allowed = 0;
                            break;
                        case 2:
                            shield_allowed = 1;
                            body_allowed = 0;
                            break;
                        case 3:
                            if ((*(uint8_t *)(effect_block + 0x4) & 0x20) == 0) {
                                shield_allowed = 0;
                                body_allowed = 0;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (node_index >= 0 && node_index < *(int32_t *)(geometry + 0x28c)) {
            *(int16_t *)&region = *(int16_t *)(*(uint8_t **)(geometry + 0x290) + node_index * 0x40 + 0x32);
        }
        if (difficulty_scaled) {
            notify_flags = 0x20;
        }
        if (dd->team_index != -1) {
            int16_t team = ((object *)obj)->owner_team;

            if (current_game_engine != 0) {
                if (team == dd->team_index) {
                    notify_flags |= 0x10;
                }
            } else if (team >= 0 && team < 10 && dd->team_index >= 0 && dd->team_index < 10) {
                if (teams_are_friends(team * 10 + dd->team_index)) {
                    notify_flags |= 0x10;
                }
            }
        }
        if (i == 0 && material_index >= 0 && material_index < *(int32_t *)(geometry + 0x234)) {
            material = *(uint8_t **)(geometry + 0x238) + material_index * 0x48;
        } else if (*(int16_t *)(geometry + 0x4) >= 0 && *(int16_t *)(geometry + 0x4) < *(int32_t *)(geometry + 0x234)) {
            material = *(uint8_t **)(geometry + 0x238) + *(int16_t *)(geometry + 0x4) * 0x48;
        } else {
            material = (uint8_t *)&default_collision_material;
        }
        dd->material_type = *(int16_t *)(material + 0x24);
        if (g_0087abc7 && dd->responsible_player != k_datum_index_none) {
            kill = 1;
        }
        if (*(int16_t *)effect_block == 2 && unit_point_in_front_and_asleep(&dd->origin, id) &&
            (obj[0x107] & 8) == 0) {
            kill = 1;
        }
        if (target_is_local == 1 && kill && (obj[0x106] & 4) == 0 && (!friendly || body_allowed)) {
            ((object *)obj)->body_vitality = 0.0f;
            object_set_health_frozen_flag(id);
            notify_flags |= 0x41;
        }
        if ((dd->flags & 0x20) == 0 && (*(uint32_t *)(effect_block + 0x4) & 0x200) == 0 &&
            ((object *)obj)->maximum_shield_vitality > 0.0f && (!friendly || shield_allowed) && (i == 0 || (*geometry & 1))) {
            object_apply_shield_damage(id, geometry, material, effect_block, &notify_flags, &shield_damage, &amount,
                target_is_local, apply_state, &record);
        }
        if ((i == 0 || (parents_take_damage && (*geometry & 2))) &&
            (*(uint8_t *)(effect_block + 0x4) & 0x40) == 0) {
            if (((*geometry & 0x20) && (*(uint8_t *)(effect_block + 0x4) & 0x20) == 0) ||
                (friendly && !body_allowed)) {
                amount = 0.0f;
            }
            object_apply_body_damage(id, (i == 0) ? region_index : -1, (i == 0) ? node_index : -1,
                (void *)(uintptr_t)((i == 0) ? hit_plane : 0), geometry, material, effect_block, dd, &notify_flags,
                &body_damage, &material_multiplier, amount, target_is_local);
            remaining = 0;
        }
        if (!reported && (shield_damage > 0.0001f || body_damage > 0.0001f)) {
            if (shield_damage > body_damage) {
                dd->material_type = *(int16_t *)(geometry + 0xd2);
                dd->remaining_vitality = *(uint32_t *)&((object *)obj)->shield_vitality;
            } else {
                float vitality = ((object *)obj)->body_vitality;

                if (vitality < 0.0f) {
                    vitality = 0.0f;
                } else if (vitality > 1.0f) {
                    vitality = 1.0f;
                }
                *(float *)&dd->remaining_vitality = vitality;
            }
            reported = 1;
        }
        object_notify_pickup_or_refresh_probe(id, dd->responsible_player);
        if (shield_damage > 0.0f && ((object *)obj)->type == 0) {
            obj[0x122] = 1;
        }
notify:
        object_damage_notify_and_impulse(id, dd, notify_flags, shield_damage, body_damage,
            *(uint32_t *)&material_multiplier, region, target_is_local);
        if (notify_flags & 4) {
            int32_t role = *(int32_t *)(object_get(id) + 0x4);

            if (role == 0) {
                object_delete_unparented(id);
                object_delete_recursive(id, 0);
            } else if (role == 3) {
                object_delete_recursive(id, 0);
            }
        }
        if (!(amount > 0.0f)) {
            break;
        }
    }
}

/**
 * Applies body (non-shield) damage to a target object region and reports the notification flags and damage done.
 *
 * Original register convention: stack=(target_index, region_index, node_index, plane, geometry, material,
 * effect_block, dd, notify_flags, body_damage_out, material_multiplier_out, damage, is_local).
 *
 * @address 0x004ef2a0
 */
void halo::objects::ObjectDamage::apply_body_damage(int32_t region_index, int32_t node_index, void *plane,
    uint8_t *geometry, uint8_t *material, uint8_t *effect_block, damage_data *dd, uint32_t *notify_flags,
    float *body_damage_out, float *material_multiplier_out, float damage, uint8_t is_local)
{
    uint32_t target_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[target_index & 0xffff].data;
    uint8_t *vitality_flags = obj + 0x106;
    float *vitality = (float *)(obj + 0xe0);
    float body = damage * *(float *)(material + 0x3c);
    float maximum;
    float inverse_maximum;
    float value;
    float taken;
    uint8_t unscaled = 0;

    if ((*geometry & 0x40) && ((object *)obj)->type == 1 && *(datum_index *)(obj + 0x324) == k_datum_index_none) {
        body = 0.0f;
    }
    if (current_game_engine == 0 && *(int16_t *)(effect_block + 0x2) == 1 && ((object *)obj)->owner_team == 1) {
        unscaled = 1;
    }
    maximum = ((object *)obj)->maximum_body_vitality;
    if (!unscaled) {
        maximum = weapon_get_zoom_fov_resolved(1, ((object *)obj)->owner_team) * maximum;
    }
    inverse_maximum = (maximum > 0.0f) ? 1.0f / maximum : 0.0f;
    value = body;
    if (*notify_flags & 0x10) {
        value = (1.0f - *(float *)(geometry + 0x44)) * body;
        if (*notify_flags & 0x20) {
            real multiplier = weapon_get_zoom_fov(0, main_game_globals->difficulty);

            if (multiplier > 0.0f) {
                value = value / multiplier;
            }
        }
    }
    taken = value * inverse_maximum * *(float *)(effect_block + 0x3c + *(int16_t *)(material + 0x24) * 4);
    if (*(uint16_t *)vitality_flags & 0x800) {
        if (is_local != 1) {
            goto bookkeeping;
        }
    } else {
        if (body > 0.0f && (*(uint8_t *)(geometry + 0x20) & 1)) {
            uint32_t effect_flags = *(uint32_t *)(effect_block + 0x4);

            if (effect_flags & 2) {
                if (!(current_game_engine == 0 && ((object *)obj)->type == 0 &&
                      *(datum_index *)(obj + 0x218) != k_datum_index_none)) {
                    if (is_local == 1) {
                        *vitality = 0.0f;
                    }
                    *notify_flags |= 0x40;
                    if (current_game_engine != 0) {
                        *notify_flags |= 0x80;
                    }
                }
            } else if ((effect_flags & 0x800) && current_game_engine != 0) {
                taken = taken + taken;
                if (taken > *vitality) {
                    *notify_flags |= 0x80;
                }
            }
        }
        if (is_local != 1) {
            goto bookkeeping;
        }
        *vitality = *vitality - taken;
    }
    if ((int16_t)region_index != -1) {
        int32_t region = (int16_t)region_index;

        if ((((object *)obj)->destroyed_region_flags & (1u << (region & 0x1f))) == 0) {
            uint8_t *region_block = *(uint8_t **)(geometry + 0x244) + region * 0x54;
            uint8_t region_damage = (uint8_t)(int32_t)(taken * 255.0f + (int32_t)obj[0x178 + region]);

            obj[0x178 + region] = region_damage;
            if (*(float *)(region_block + 0x28) > 0.0f &&
                (float)region_damage * 0.0039215689f > *(float *)(region_block + 0x28)) {
                object_destroy_region(target_index, region_index);
                *notify_flags |= 2;
            }
        }
    }
bookkeeping:
    {
        float current = taken + ((object *)obj)->current_body_damage;
        float recent;

        ((object *)obj)->body_damage_ticks = 0;
        ((object *)obj)->current_body_damage = current;
        recent = taken + ((object *)obj)->recent_body_damage;
        ((object *)obj)->recent_body_damage = recent;
        if (current > 1.0f) {
            ((object *)obj)->current_body_damage = 1.0f;
        }
        if (recent > 1.0f) {
            ((object *)obj)->recent_body_damage = 1.0f;
        }
    }
    if (g_0087abc0 && *vitality < 0.0f && ((1u << (obj[0xb4] & 0x1f)) & 3)) {
        if (*(datum_index *)(obj + 0x218) != k_datum_index_none) {
            *vitality = 0.0f;
        } else if (((object *)obj)->type == 1) {
            datum_index child = ((object *)obj)->first_child_object;

            while (child != k_datum_index_none) {
                uint8_t *child_obj = (uint8_t *)((object_header *)object_data->data)[child & 0xffff].data;

                if (((1u << (child_obj[0xb4] & 0x1f)) & 3) &&
                    *(datum_index *)(child_obj + 0x218) != k_datum_index_none) {
                    *vitality = 0.0f;
                    break;
                }
                child = ((object *)child_obj)->next_object;
            }
        }
    }
    if (is_local == 1) {
        float absolute = weapon_get_zoom_fov_resolved(1, ((object *)obj)->owner_team) * ((object *)obj)->maximum_body_vitality *
            *vitality;
        float destroyed = *(float *)(geometry + 0xb8);

        if (destroyed < 0.0f && absolute < destroyed) {
            object_delete_teardown(target_index);
            *notify_flags |= 5;
        } else if (absolute < 0.0f) {
            if ((*vitality_flags & 4) == 0) {
                int16_t i;

                for (i = 0; i < *(int32_t *)(geometry + 0x240); i++) {
                    if (*(*(uint8_t **)(geometry + 0x244) + i * 0x54 + 0x20) & 4) {
                        object_destroy_region(target_index, i);
                    }
                }
                object_set_health_frozen_flag(target_index);
                *notify_flags |= 1;
            }
        } else if (absolute < *(float *)(geometry + 0x94) && (*vitality_flags & 1) == 0) {
            halo::effects::effect_new_on_object(target_index, *(datum_index *)(geometry + 0xa4), target_index, -1, 0.0f, 0.0f, 0, 0);
            *vitality_flags |= 1;
        }
    }
    if ((dd->flags & 2) && *(datum_index *)(geometry + 0x7c) != k_datum_index_none) {
        damage_effect_new_at_location(*(datum_index *)(geometry + 0x7c), (int16_t)node_index, &dd->direction,
            (real_vector3d *)plane, &dd->origin, target_index);
    }
    if ((dd->flags & 1) && body > *(float *)(geometry + 0x80) &&
        *(datum_index *)(geometry + 0x90) != k_datum_index_none && *(int16_t *)(effect_block + 0x2) != 7) {
        halo::effects::effect_new_on_object(target_index, *(datum_index *)(geometry + 0x90), target_index, -1, 0.0f, 0.0f, 0, 0);
    }
    *body_damage_out = body;
    *material_multiplier_out = *(float *)(material + 0x3c);
}

/**
 * Applies shield damage to a target object and reports the shield damage done and the damage left over.
 *
 * Original register convention: EBX -> record, stack=(target_index, geometry, material, effect_block, notify_flags,
 * shield_damage_out, remaining_damage, is_local, apply_state).
 *
 * @address 0x004ef820
 */
void halo::objects::ObjectDamage::apply_shield_damage(uint8_t *geometry, uint8_t *material, uint8_t *effect_block,
    uint32_t *notify_flags, float *shield_damage_out, float *remaining_damage, uint8_t is_local, uint8_t apply_state,
    object_shield_impulse_result *record)
{
    uint32_t target_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[target_index & 0xffff].data;
    float *shield = (float *)(obj + 0xe4);
    uint16_t *vitality_flags = (uint16_t *)(obj + 0x106);
    float passthrough = *remaining_damage;
    float to_shield = *remaining_damage;
    float maximum;
    float inverse_maximum;
    uint8_t negligible = 0;
    uint8_t unscaled = 0;

    record->shield_damage_dealt = 0.0f;
    record->depleted_this_call = 0;
    if (current_game_engine == 0 && *(int16_t *)(effect_block + 0x2) == 1 && ((object *)obj)->owner_team == 1) {
        unscaled = 1;
    }
    if (!(*shield > 0.0f)) {
        to_shield = 0.0f;
        if (is_local != 1) {
            goto done;
        }
        *shield = 0.0f;
        goto stun;
    }
    maximum = ((object *)obj)->maximum_shield_vitality;
    if (!unscaled) {
        maximum = weapon_get_zoom_fov_resolved(2, ((object *)obj)->owner_team) * maximum;
    }
    inverse_maximum = (maximum > 0.0f) ? 1.0f / maximum : 0.0f;
    if ((*notify_flags & 0x10) == 0 || (*geometry & 4) == 0) {
        float threshold = *(float *)(geometry + 0xf0);

        to_shield = (1.0f - *(float *)(material + 0x28)) * passthrough;
        if (*shield <= threshold && threshold > 0.0f) {
            real t = halo::math::transition_function_evaluate(*(transition_function_t *)(geometry + 0xec), *shield / threshold);
            float leak = *(float *)(geometry + 0xf4);

            to_shield = ((1.0f - leak) * t + leak) * to_shield;
        }
    }
    if (*vitality_flags & 0x10) {
        to_shield = passthrough;
        passthrough = 0.0f;
    } else {
        float scaled;
        float dealt;

        if (to_shield < 0.0f) {
            to_shield = 0.0f;
        }
        passthrough = passthrough - to_shield;
        if ((*notify_flags & 0x10) && (*notify_flags & 0x20)) {
            real multiplier = weapon_get_zoom_fov(0, main_game_globals->difficulty);

            if (multiplier > 0.0f) {
                to_shield = to_shield / multiplier;
            }
        }
        scaled = to_shield * *(float *)(material + 0x2c) *
            *(float *)(effect_block + 0x3c + *(int16_t *)(geometry + 0xd2) * 4);
        if (scaled < 0.0001f) {
            negligible = 1;
        }
        dealt = inverse_maximum * scaled;
        if (dealt > *shield || *(int16_t *)effect_block == 3) {
            float overflow = scaled - maximum * *shield;

            if (overflow > 0.0f) {
                passthrough = overflow + passthrough;
            }
            if (is_local == 1) {
                *shield = 0.0f;
            }
            if ((*vitality_flags & 8) == 0 && apply_state == 1) {
                object_set_shield_depleted_flag(target_index);
                *notify_flags |= 8;
                record->depleted_this_call = 1;
            }
        } else {
            if (is_local == 1 && (*vitality_flags & 0x800) == 0) {
                *shield = *shield - dealt;
            }
            if ((*vitality_flags & 2) == 0 && *shield < *(float *)(geometry + 0x184)) {
                object_dispatch_effect_notify(target_index, *(uint32_t *)(geometry + 0x194));
                *vitality_flags |= 2;
            }
        }
        if (negligible) {
            goto local_stun;
        }
    }
    if (apply_state == 1) {
        float fraction = (*remaining_damage - passthrough) * inverse_maximum;
        float recent;

        ((object *)obj)->shield_damage_ticks = 0;
        if ((*vitality_flags & 8) == 0) {
            ((object *)obj)->current_shield_damage = 1.0f;
        }
        recent = fraction + ((object *)obj)->recent_shield_damage;
        ((object *)obj)->recent_shield_damage = recent;
        if (((object *)obj)->current_shield_damage > 1.0f) {
            ((object *)obj)->current_shield_damage = 1.0f;
        }
        if (recent > 1.0f) {
            ((object *)obj)->recent_shield_damage = 1.0f;
        }
        record->shield_damage_dealt = fraction;
    }
local_stun:
    if (is_local != 1) {
        goto done;
    }
stun:
    if (!(to_shield < *(float *)(geometry + 0x108)) || *shield == 0.0f) {
        ((object *)obj)->shield_stun_ticks = (int16_t)(int32_t)(*(float *)(geometry + 0x10c) * 30.0f);
    }
done:
    *shield_damage_out = to_shield;
    *remaining_damage = passthrough;
}

/**
 * Queues a networked pickup-denied event for a player.
 *
 * Original register convention: ECX -> key, EDI -> source, stack -> param_1.
 *
 * @address 0x004efbf0
 */
void halo::objects::DamageSystem::queue_pickup_denied_event(void *param_1, int32_t key, uint32_t *source)
{
    struct {
        int32_t looked_up;
        void *original_param_1;
        uint32_t source0;
        uint32_t source1;
        uint32_t source2;
    } block;

    block.looked_up = 0;
    if (key != -1) {
        block.looked_up = hash_table_get(&object_network_id_table->id_to_index, key);
    }
    block.source1 = source[1];
    block.source0 = source[0];
    block.original_param_1 = param_1;
    block.source2 = source[2];

    {
        void *items[1];
        items[0] = &block;
        network_session_broadcast_to_flagged(message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x31, 0,
                                             items, 0, 1, 0), network_server, 1, network_message_scratch, 0, 0, 0, 3);
    }
}

/**
 * Decodes a networked linked-impulse message and applies it to the target object.
 *
 * Original register convention: a pointer-to-pointer parameter in EAX (in_EAX).
 *
 * @address 0x004efc80
 */
void halo::objects::DamageSystem::apply_linked_impulse(void **message)
{
    int32_t network_id;
    float impulse_scale, direction_i, direction_j, direction_k;

    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged(0);
        return;
    }

    if (message_delta_decode_compound_field(message, &network_id) != 0 && network_id != 0 &&
        ((int32_t *)object_network_id_table->handles)[network_id] != -1) {
        real_vector3d impulse;

        impulse.i = direction_i * impulse_scale;
        impulse.j = direction_j * impulse_scale;
        impulse.k = direction_k * impulse_scale;
        halo::items::item_accelerate((uint32_t)((int32_t *)object_network_id_table->handles)[network_id], &impulse, 0);
    }
}

namespace {
static void (*const item_accelerate__as_object_damage_notify_and_impulse)(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer) = reinterpret_cast<void (*)(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer)>(&halo::items::item_accelerate);
}

/**
 * Sends the HUD and network notifications for a damage result and applies the resulting impulse to the target.
 *
 * Original register convention: stack=(target_index, dd, notify_flags, shield_damage, body_damage, unused_6,
 * region_index, is_local).
 *
 * @address 0x004efcf0
 */
void halo::objects::ObjectDamage::notify_and_impulse(damage_data *dd, uint32_t notify_flags, float shield_damage,
    float body_damage, uint32_t unused_6, int32_t region_index, uint32_t is_local)
{
    uint32_t target_index = handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[target_index & 0xffff].data;
    uint8_t *object_tag = (uint8_t *)halo::cache::globals().tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *effect = (uint8_t *)halo::cache::globals().tag_instances[dd->damage_effect_tag & 0xffff].data;
    int16_t type;

    if (((struct Object *)object_tag)->acceleration_scale > 0.0001f) {
        real_vector3d direction;
        real_vector3d impulse;
        float scale;

        direction = dd->direction;
        direction.k = direction.k + 0.45f;
        halo::math::vector3d_normalize_with_length(direction);
        type = ((object *)obj)->type;
        scale = ((struct Object *)object_tag)->acceleration_scale * *(float *)(effect + 0x1f4) * 0.033333335f;
        impulse.i = direction.i * scale;
        impulse.j = direction.j * scale;
        impulse.k = scale * direction.k;
        switch (type) {
        case 0:
        case 1:
            if (*(float *)(effect + 0x1f4) > 0.0001f && (*(uint32_t *)(obj + 0x204) & 0x800000) == 0) {
                if (type == 0) {
                    unit_apply_impulse(target_index, &impulse);
                } else {
                    if (*(uint32_t *)(effect + 0x1c8) & 0x20) {
                        impulse.i = impulse.i + impulse.i;
                        impulse.j = impulse.j + impulse.j;
                        impulse.k = impulse.k + impulse.k;
                    }
                    if (((object *)obj)->network_role != 1 || unit_any_flagged_seat_occupied(target_index) == 1) {
                        unit_apply_impulse_to_seat(target_index, &impulse);
                    }
                }
            }
            break;
        case 2:
        case 3:
        case 4: {
            uint8_t significant = 0;
            int32_t role;

            if (!(impulse.k * impulse.k + impulse.i * impulse.i + impulse.j * impulse.j < 0.0001f) ||
                (*(uint8_t *)(obj + 0x1f4) & 8) == 0) {
                significant = 1;
            }
            if (((object *)obj)->network_role == 0 && significant == 1) {
                object_queue_pickup_denied_event(*(void **)&scale, (int32_t)target_index, (uint32_t *)&direction);
            }
            role = ((object *)obj)->network_role;
            if (role == 0 || role == 3 || !significant) {
                item_accelerate__as_object_damage_notify_and_impulse(target_index, &impulse,
                    (uint8_t)((dd->random_blend > 0.5f && (*(uint8_t *)(effect + 0x1c8) & 0x20)) ? 1 : 0));
            }
            break;
        }
        case 5:
            object_apply_impulse_and_spin(target_index, &impulse);
            break;
        default:
            break;
        }
    }
    if ((uint8_t)is_local == 1 && (current_game_engine == 0 || game_engine_state_value == 0)) {
        if ((dd->flags & 0x80) == 0) {
            if (notify_flags & 1) {
                game_engine_attribute_player_death(target_index, dd->responsible_player, dd->responsible_object,
                    (int32_t)(uint16_t)dd->team_index, 1);
            }
        } else {
            datum_index player_index = player_index_from_unit_index(target_index);

            game_engine_on_player_death(player_index, target_index, player_index, 1);
        }
    }
    if ((1u << (obj[0xb4] & 0x1f)) & 3) {
        unit_apply_damage_effects(target_index, dd, notify_flags, shield_damage, body_damage, region_index,
            (uint8_t)is_local);
    }
}

/**
 * Forwards an effect notification (register arguments) to the object's type definition.
 *
 * Original register convention: EAX -> forwarded_eax, ECX -> forwarded_ecx (both pass straight through to.
 *
 * @address 0x004efff0
 */
void halo::objects::DamageSystem::dispatch_effect_notify(uint32_t forwarded_eax, uint32_t forwarded_ecx)
{
    halo::effects::effect_new_on_object(forwarded_eax, forwarded_ecx, forwarded_eax, -1, 0.0f, 0.0f, 0, 0);
}

/**
 * Spawns the effect attached to a damage effect tag at an impact location, passing the surface normal and incident
 * vectors and the object that was hit.
 *
 * Original register convention: EAX=normal, ECX=incident, EBX=impact_position, EDI=object_index, stack=(effect_tag,
 * node_index).
 *
 * @address 0x004f0010
 */
void halo::objects::DamageSystem::effect_new_at_location(datum_index effect_tag, int16_t node_index,
    real_vector3d *normal, real_vector3d *incident, real_point3d *impact_position, uint32_t object_index)
{
    static const char *const k_names[5] = { "normal", "incident", "negative incident", "reflection", "gravity" };
    const char *names[5];
    real_vector3d gravity = *global_down3d_pointer;
    real_vector3d direction = *normal;
    real_vector3d vectors[5];
    real_point3d positions[5];
    int32_t i;

    for (i = 0; i < 5; i++) {
        names[i] = k_names[i];
    }
    if (halo::math::vector3d_normalize_with_length(direction) == 0.0f) {
        direction = *halo::math::globals().global_forward3d_pointer;
    }
    vectors[1].i = direction.i * -1.0f;
    vectors[1].j = direction.j * -1.0f;
    vectors[1].k = direction.k * -1.0f;
    vectors[2] = direction;
    if (incident == 0) {
        real_point3d object_position;
        real_vector3d away;
        float dot2;

        object_get_position(&object_position, object_index);
        away.i = impact_position->x - object_position.x;
        away.j = impact_position->y - object_position.y;
        away.k = impact_position->z - object_position.z;
        if (halo::math::vector3d_normalize_with_length(away) == 0.0f) {
            away = *(real_vector3d *)((uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data + 0x74);
        }
        vectors[0] = away;
        dot2 = away.j * direction.j + direction.k * away.k + direction.i * away.i;
        dot2 = dot2 + dot2;
        vectors[3].i = direction.i - away.i * dot2;
        vectors[3].j = direction.j - away.j * dot2;
        vectors[3].k = direction.k - away.k * dot2;
    } else {
        float dot2;

        vectors[0] = *incident;
        dot2 = direction.k * incident->k + direction.j * incident->j + direction.i * incident->i;
        dot2 = dot2 + dot2;
        vectors[3].i = direction.i - dot2 * incident->i;
        vectors[3].j = direction.j - dot2 * incident->j;
        vectors[3].k = direction.k - dot2 * incident->k;
    }
    vectors[4] = gravity;
    for (i = 0; i < 5; i++) {
        positions[i] = *impact_position;
    }
    if (object_index != 0xffffffff && node_index != -1) {
        halo::effects::effect_new_on_object_with_node_table(object_index, effect_tag, object_index, (uint16_t)node_index, 5,
            (uint32_t)(uintptr_t)names, (uint32_t)(uintptr_t)positions, (uint32_t)(uintptr_t)vectors,
            1.0f, 0.0f, 0, 0);
        return;
    }
    halo::effects::effect_new_with_color(effect_tag, object_index, global_origin3d_pointer, 5, (uint32_t)(uintptr_t)names,
        positions, (uint32_t)(uintptr_t)vectors, 1.0f, 0.0f, 0, 0, 0);
}

namespace {
static void (*const effect_new_on_object_with_node_table__as_object_damage_effect_dispatch)() = reinterpret_cast<void (*)()>(&halo::effects::effect_new_on_object_with_node_table);
}

/**
 * Dispatches a damage effect at a node of an object.
 *
 * Original register convention: none visible; whatever registers effect_new_on_object_with_node_table needs pass
 * through.
 *
 * @address 0x004f0250
 */
void halo::objects::DamageSystem::effect_dispatch(int32_t push_value, int32_t node_object)
{
    (void)push_value; (void)node_object;
    effect_new_on_object_with_node_table__as_object_damage_effect_dispatch();
}

/**
 * Destroys a model region of an object.
 *
 * Original register convention: uint32_t object_index in EAX (in_EAX); int32_t region_index on the stack.
 *
 * @address 0x004f02d0
 */
void halo::objects::ObjectDamage::destroy_region(int32_t region_index)
{
    uint32_t object_index = handle;
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[obj->definition_tag & 0xffff].data;

    if (definition->collision_model.tag_id.index != 0xffff) {
        if ((obj->destroyed_region_flags & (1 << (region_index & 0x1f))) == 0) {
            ModelCollisionGeometry *geometry =
                (ModelCollisionGeometry *)halo::cache::globals().tag_instances[definition->collision_model.tag_id.index].data;
            ModelCollisionGeometryRegion *region = &((ModelCollisionGeometryRegion *)geometry->regions.pointer)[region_index];

            halo::effects::effect_new_on_object(object_index, *(datum_index *)&((struct ModelCollisionGeometryRegion *)region)->destroyed_effect.tag_id, object_index, -1,
                0.0f, 0.0f, 0, 0);
            object_set_permutation_by_name(object_index, (char *)"~damaged", (int16_t)region_index, 1);

            if ((region->flags & 0x20) != 0) {
                obj->vitality_flags |= _object_region_response_80_bit;
            }
            if ((region->flags & 0x40) != 0) {
                obj->vitality_flags |= _object_region_response_100_bit;
            }
            if ((region->flags & 0x80) != 0) {
                *((uint8_t *)obj + 0x107) |= 2;
            }
            if ((region->flags & 0x100) != 0) {
                *((uint8_t *)obj + 0x107) |= 4;
            }
            if ((region->flags & 2) != 0) {
                object_set_health_frozen_flag(object_index);
            }

            obj->destroyed_region_flags |= (uint16_t)(1 << (region_index & 0x1f));
            object_type_definitions_notify_region_damage(object_index, (uint32_t)region_index, region->flags);

        }
    }
}

/**
 * Marks every breakable surface of the structure BSP as intact again.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004ffd40
 */
void halo::objects::DamageSystem::breakable_surfaces_reset()
{
    breakable_surface_globals *table = halo::physics::globals().breakable_surface_state;
    int32_t group, i;

    table->initialized = 1;

    for (group = 0; group < 16; group++) {
        for (i = 0; i < 8; i++) {
            table->active[group][i] = 0xffffffff;
        }
    }
    for (group = 0; group < 16; group++) {
        for (i = 0; i < 256; i++) {
            table->health[group][i] = 1.0f;
        }
    }
}

/**
 * Returns whether the breakable surface with the given bit index in the current structure BSP is still intact.
 *
 * @address 0x004ffda0
 */
int8_t halo::objects::DamageSystem::breakable_surface_is_intact(int16_t bit_index)
{
    if (bit_index != -1) {
        uint32_t word = halo::physics::globals().breakable_surface_state->active[global_structure_bsp_index][bit_index >> 5];
        if ((word & (1 << (bit_index & 0x1f))) == 0) {
            return 0;
        }
    }
    return 1;
}
