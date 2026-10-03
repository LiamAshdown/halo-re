#include "halo/items/items.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern data_array *object_data;
extern data_array *actor_data;
extern char s_primary_trigger_marker[];
extern char s_secondary_trigger_marker[];
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *markers, uint32_t max_count);
extern void unit_project_onto_aiming_axis(datum_index unit_index, real *out_speed, uint8_t project_point, uint8_t use_unit_aiming_vector, real_point3d *point, real_vector3d *axis);
extern uint32_t camera_observer_update(datum_index player_index, real_point3d *observer_position, real_vector3d *fallback_facing);
extern uint32_t actor_compute_grenade_aim_direction(datum_index actor_index, real_point3d *target_point, real_vector3d *out_direction, float *out_698);
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role);
extern void weapon_trigger_barrel_spread_offset(real_vector3d *v, real_vector3d *axis, uint16_t barrel_index, int16_t distribution_function, real distribution_angle, uint32_t flags);
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role);
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out);
extern uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position, uint32_t ignore_object_index);
extern double fabs(double x);
extern double sqrt(double x);
extern double cos(double x);
extern double sin(double x);
extern int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force);
extern uint32_t local_player_index_for_weapon(datum_index item_index);
extern void first_person_weapon_process_action(uint32_t handle, int32_t action);
extern void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code);
extern int16_t network_game_mode;
extern int32_t weapon_triggers_idle(datum_index item_index);
extern void weapon_trigger_effect_clear(datum_index item_index, int16_t trigger_index);
extern void weapon_notify_reload_begin(datum_index item_index, int16_t magazine_index);
extern uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, real scale_a, real scale_b);
extern void weapon_action_notify_for_weapon(datum_index weapon_index, int32_t action_code);
extern int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index, int16_t category, int16_t mode);
extern void weapon_fire_trigger(datum_index item_index, int16_t trigger_index);
extern uint8_t scenario_location_get_water_and_weather(real_point3d *point, bsp_leaf_reference *leaf, int16_t *weather_index_out);
extern void weapon_trigger_effect_set_state(datum_index item_index, int16_t trigger_index, int8_t state, int16_t counter);
extern uint8_t projectile_get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag, real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override, uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed, real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line);
extern void weapon_reload_recovery_finish(datum_index item_index);
extern void weapon_trigger_enter_recovery(datum_index item_index, int16_t trigger_index);
void trigger_create_projectiles(uint32_t item_index, int16_t trigger_index, uint32_t role);
void weapon_trigger_become_charged(datum_index item_index, int16_t trigger_index);
void weapon_trigger_begin_reload(datum_index item_index, int16_t magazine_index, int8_t is_client_predicted);
void weapon_trigger_continue_burst(datum_index item_index, int16_t trigger_index);
void weapon_trigger_effect_set_out_of_ammo(datum_index item_index, int16_t trigger_index);
void weapon_trigger_finish_shot(datum_index item_index, int16_t trigger_index);
void weapon_trigger_fire_or_reload(datum_index item_index, int16_t trigger_index, int8_t force);
uint8_t weapon_trigger_get_aiming_vector(datum_index weapon_index, int16_t trigger_index, real_point3d *origin, real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction, real *out_time, real *out_range, uint8_t *out_used_straight_line);
real weapon_trigger_get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire);
real weapon_trigger_get_charge_fraction(datum_index item_index, int16_t trigger_index);
void weapon_trigger_handle_empty(datum_index item_index, int16_t trigger_index);
real weapon_trigger_projectile_time_fraction(datum_index item_index, int16_t trigger_index, real elapsed);
int32_t weapon_trigger_ready_to_fire(datum_index item_index, int16_t trigger_index);
void weapon_trigger_reset_tracking(datum_index item_index, int16_t trigger_index);
}

namespace halo::items {

#define F(p, o) (*(float *)((p) + (o)))

#define W(p, o) (*(int16_t *)((p) + (o)))

#define D(p, o) (*(datum_index *)((p) + (o)))

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & 0xffff].data)

/**
 * Member form of the original trigger_create_projectiles: create projectiles.
 *
 * @address 0x4c4c40
 */
void weapon_trigger_ref::create_projectiles(int16_t trigger_index, uint32_t role)
{
    uint32_t item_index = datum;
    uint8_t *item = OBJECT_DATA(item_index);
    uint8_t *weapon_tag = TAG_DATA(*(datum_index *)item);
    uint8_t *trigger = *(uint8_t **)(weapon_tag + 0x500) + trigger_index * 0x114;
    uint8_t *state = item + 0x260 + trigger_index * 0x28;
    datum_index holder = k_datum_index_none;
    uint32_t marker_object = item_index;
    static object_marker markers[0x40];
    int16_t marker_count;
    int16_t m;

    if (D(item, 0x11c) != k_datum_index_none && object_try_and_get(D(item, 0x11c), 3) != 0) {
        holder = D(item, 0x11c);
    }
    if ((((struct item_object *)item)->base.flags & 1) && D(item, 0x11c) != k_datum_index_none) {
        marker_object = D(item, 0x11c);
    }
    marker_count = (int16_t)object_get_node_local_transform(marker_object,
        trigger_index == 0 ? s_primary_trigger_marker : s_secondary_trigger_marker, markers, 0x40);
    if (marker_count == 0) {
        marker_count = 1;
    }
    if (!(*(uint32_t *)trigger & 0x20)) {
        marker_count = 1;
    }

    for (m = 0; m < marker_count; m++) {
        real_point3d origin = markers[m].node_transform.position;
        real_vector3d forward = markers[m].node_transform.forward;
        real speed = 0.0f;
        float error = 0.0f;
        uint8_t *holder_object = 0;
        datum_index target = k_datum_index_none;
        datum_index projectile_tag;
        int16_t count;
        datum_index owner = k_datum_index_none;
        int16_t shot;

        if (holder != k_datum_index_none) {
            int16_t index = (int16_t)holder;
            int16_t salt = (int16_t)(holder >> 16);

            if (index >= 0 && index < *(int16_t *)((uint8_t *)object_data + 0x20)) {
                uint8_t *header = (uint8_t *)object_data->data + *(int16_t *)((uint8_t *)object_data + 0x22) * index;

                if (*(int16_t *)header != 0 && (salt == 0 || *(int16_t *)header == salt) &&
                    ((1u << (header[3] & 0x1f)) & 3)) {
                    holder_object = *(uint8_t **)(header + 0x8);
                }
            }
        }

        if (!(*(uint32_t *)trigger & 0x800) && holder_object != 0 && !(holder_object[0x106] & 4)) {
            uint8_t *holder_tag = TAG_DATA(*(datum_index *)holder_object);
            datum_index player = D(holder_object, 0x218);
            datum_index actor = D(holder_object, 0x1f4);
            uint8_t use_aiming_vector;
            uint8_t project_point = 1;

            if (D(holder_object, 0x328) != k_datum_index_none) {
                uint8_t *controller = OBJECT_DATA(D(holder_object, 0x328));

                player = D(controller, 0x218);
                actor = D(controller, 0x1f4);
            }
            use_aiming_vector = (uint8_t)((*(uint32_t *)(holder_tag + 0x17c) >> 3) & 1);
            if (actor != k_datum_index_none &&
                W((uint8_t *)actor_data->data + (actor & 0xffff) * 0x724, 0x5f2) == 4) {
                project_point = 0;
            }
            if (D(holder_object, 0x328) != k_datum_index_none) {
                project_point = 0;
            }
            unit_project_onto_aiming_axis(holder, &speed, use_aiming_vector, project_point, &origin, &forward);
            if (player != k_datum_index_none) {
                real_vector3d left;
                real_vector3d up;
                real x = F(trigger, 0x84);
                real y = F(trigger, 0x88);
                real z = F(trigger, 0x8c);

                halo::math::vector3d_cross_product(left, forward, *halo::math::globals().global_up3d_pointer);
                if (halo::math::vector3d_normalize_with_length(left) == 0.0f) {
                    left = *halo::math::globals().global_left3d_pointer;
                }
                halo::math::vector3d_cross_product(up, left, forward);
                halo::math::vector3d_normalize_with_length(up);
                origin.x = origin.x + forward.i * x + left.i * y + up.i * z;
                origin.y = origin.y + forward.j * x + left.j * y + up.j * z;
                origin.z = origin.z + forward.k * x + left.k * y + up.k * z;
                target = camera_observer_update(player, &origin, &forward);
            } else if (actor != k_datum_index_none) {
                target = actor_compute_grenade_aim_direction(actor, &origin, &forward, &error);
            }
        }
        if (*(uint32_t *)trigger & 0x20) {
            origin = markers[m].node_transform.position;
        }

        if (trigger_index == 0 && W(item, 0x25c) > 0) {
            int16_t charge = W(item, 0x25c);

            if (W(weapon_tag, 0x32c) == 4) {
                charge++;
            }
            projectile_tag = D(*(uint8_t **)(weapon_tag + 0x500), 0x1b4);
            count = (int16_t)((uint16_t)W(trigger, 0x6e) * charge);
            W(item, 0x25c) = 0;
        } else {
            count = W(trigger, 0x6e);
            projectile_tag = D(trigger, 0xa0);
        }
        if (projectile_tag == k_datum_index_none) {
            continue;
        }
        {
            datum_index parent = D(OBJECT_DATA(item_index), 0x11c);
            object *parent_object = parent != k_datum_index_none ? object_try_and_get(parent, 3) : 0;

            if (parent_object != 0) {
                owner = parent;
                if (D((uint8_t *)parent_object, 0x328) != k_datum_index_none) {
                    owner = D((uint8_t *)parent_object, 0x328);
                }
            }
        }

        for (shot = 0; shot < count; shot++) {
            object_placement_data placement;
            uint8_t tracer = 0;
            uint8_t from_player;
            datum_index projectile;
            uint8_t *projectile_definition;

            object_placement_data_initialize(&placement, D(trigger, 0xa0), owner);
            placement.position = origin;
            placement.forward = forward;
            if (F(state, 0x10) == 0.0f) {
                tracer = 1;
                W(state, 0xe) = 0;
            } else {
                int16_t n = W(state, 0xe);

                W(state, 0xe) = n + 1;
                if (!(n < W(trigger, 0x26))) {
                    tracer = 1;
                    W(state, 0xe) = 0;
                }
            }
            if (error == 0.0f) {
                real e = (*(uint32_t *)trigger & 0x200) ? F(item, 0x234) : F(state, 0x1c);

                error = (1.0f - e) * F(trigger, 0x7c) + e * F(trigger, 0x80);
            }
            if (!(*(uint32_t *)trigger & 0x400) || !(item[0x230] & 0x40)) {
                halo::math::vector3d_randomize_direction(*((real_point3d *)&placement.forward), &placement.forward, halo::math::globals().random_seed_global,
                                             F(trigger, 0x78), error);
            }
            {
                static real_vector3d first_direction;

                if (shot == 0) {
                    first_direction = placement.forward;
                }
                if (*(uint32_t *)trigger & 0x1000) {
                    placement.forward = first_direction;
                }
            }
            halo::math::vector3d_build_perpendicular(placement.up, placement.forward);
            {
                double length = sqrt((double)placement.up.i * (double)placement.up.i +
                                     (double)placement.up.j * (double)placement.up.j +
                                     (double)placement.up.k * (double)placement.up.k);

                if (!(fabs(length) < 9.999999747378752e-05)) {
                    double inverse = 1.0 / length;

                    placement.up.i = (float)(placement.up.i * inverse);
                    placement.up.j = (float)(placement.up.j * inverse);
                    placement.up.k = (float)(placement.up.k * inverse);
                }
            }
            weapon_trigger_barrel_spread_offset(&placement.forward, &placement.up, (uint16_t)shot, W(trigger, 0x6c),
                                                F(trigger, 0x70), (uint32_t)count);
            projectile_definition = TAG_DATA(projectile_tag);
            if (projectile_definition != 0 && (*(uint32_t *)(projectile_definition + 0x17c) & 0x10) &&
                holder != k_datum_index_none) {
                uint8_t *root = OBJECT_DATA(holder);

                while (D(root, 0x11c) != k_datum_index_none) {
                    root = OBJECT_DATA(D(root, 0x11c));
                }
                placement.velocity = *(real_vector3d *)&((struct object *)root)->velocity.i;
            } else {
                placement.velocity.i = placement.forward.i * speed;
                placement.velocity.j = placement.forward.j * speed;
                placement.velocity.k = placement.forward.k * speed;
            }
            from_player = (uint8_t)(holder_object != 0 && D(holder_object, 0x218) != k_datum_index_none);
            if (from_player) {
                placement.flags |= 2;
            }
            projectile = object_new_with_datum_role_control(&placement, role);
            if (projectile == k_datum_index_none) {
                continue;
            }
            if (from_player) {
                real_point3d camera;

                unit_get_camera_position(holder, &camera);
                object_reposition_to_spawn_location(projectile, &camera, holder);
            }
            if (target != k_datum_index_none) {
                D(OBJECT_DATA(projectile), 0x238) = target;
            }
            if (!tracer) {
                *(uint32_t *)(OBJECT_DATA(projectile) + 0x22c) &= ~2u;
            }
        }
    }
}

#undef F

#undef W

#undef D

#undef OBJECT_DATA

#undef TAG_DATA

/**
 * Rotates `v` around `axis` by barrel_index's share of distribution_angle, for
 * distribution_function == 1 (rotate); otherwise does nothing.
 *
 * @address 0x4c54e0
 */
void weapon_trigger_ref::barrel_spread_offset(real_vector3d *v, real_vector3d *axis, uint16_t barrel_index, int16_t distribution_function, real distribution_angle, uint32_t flags)
{
    real index;

    if ((flags & 1) == 0) {
        index = (real)(int16_t)(barrel_index >> 1) - 0.5f;
        if ((barrel_index & 1) != 0) {
            index = -index;
        }
    } else if (barrel_index == 0) {
        index = 0.0f;
    } else {
        int16_t half = (int16_t)(barrel_index - 1) >> 1;
        if (((barrel_index - 1) & 1) == 0) {
            index = (real)(-half);
        } else {
            index = (real)half;
        }
    }

    if (distribution_function == 1) {
        real angle = index * distribution_angle;
        real sin_angle = (real)sin((double)angle);
        real cos_angle = (real)cos((double)angle);
        halo::math::vector3d_rotate_about_axis(*v, *axis, sin_angle, cos_angle);
    }
}

/**
 * Transitions a trigger into the "charged" effect state and its matching weapon_state, and
 * starts the first-person charged-loop action.
 *
 * @address 0x4c3bc0
 */
void weapon_trigger_ref::become_charged(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;
    uint32_t action_handle;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    wd->triggers[trigger_index].effect_state_ticks = (int16_t)(tag_trigger->charged_time * 30.0f);
    wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_charged;

    weapon_set_state(item_index, trigger_index + 7, 1);
    action_handle = local_player_index_for_weapon(item_index);
    first_person_weapon_process_action(action_handle, 0x0e);
    if ((int16_t)action_handle == -1) {
        hud_play_pickup_notification(item_index, 0xe);
    }
}

/**
 * Starts loading a fresh round into a weapon trigger's chamber/magazine. When called
 * client-side with is_client_predicted, first copies the client's own predicted round counts
 * into the magazine before deciding whether there is room to reload; when called host-side in a
 * network game, notifies observers that the reload began.
 *
 * @address 0x4c35b0
 */
void weapon_trigger_ref::begin_reload(int16_t magazine_index, int8_t is_client_predicted)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    weapon_magazine_state *magazine;
    WeaponMagazine *magazine_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine = &wd->magazines[magazine_index];
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;

    if (item_obj->network_role == 1 &&
        (magazine->state == 0 || magazine->state == 2) &&
        weapon_triggers_idle(item_index) == 0 &&
        wd->triggers[0].effect_state == _weapon_trigger_effect_out_of_ammo) {
        weapon_trigger_effect_clear(item_index, 0);
    }

    if ((magazine->state == 0 || magazine->state == 2) &&
        wd->triggers[0].effect_state == 0 && wd->triggers[1].effect_state == 0 && wd->state == 0) {
        if (item_obj->network_role == 1 && is_client_predicted == 1) {
            magazine->rounds_unloaded = wd->predicted_rounds_unloaded[magazine_index];
            magazine->rounds_loaded = wd->predicted_rounds_loaded[magazine_index];
        }

        if (magazine->rounds_unloaded > 0 && magazine->rounds_loaded < magazine_tag->rounds_loaded_maximum) {
            int16_t mode = -1;
            int16_t ticks;

            if (is_client_predicted == 1 && item_obj->network_role == 0 && network_game_mode == 2) {
                weapon_notify_reload_begin(item_index, magazine_index);
            }
            weapon_set_state(item_index, magazine_index + 5, 0);
            weapon_play_trigger_tag_effect(item_index, *(datum_index *)&magazine_tag->reloading_effect.tag_id,
                0.0f, 0.0f);
            weapon_action_notify_for_weapon(item_index, magazine->rounds_loaded != 0 ? 10 : 9);

            if (weapon_tag->weapon_type == 1) {
                int16_t remaining = magazine_tag->rounds_loaded_maximum - magazine->rounds_loaded;
                mode = (remaining == 1) ? (is_client_predicted == 0 ? 1 : 2)
                                        : (is_client_predicted == 0 ? -1 : 0);
            }

            magazine->state = _weapon_magazine_reloading;
            ticks = weapon_get_first_person_animation_time(item_index, 7, 0, mode);
            magazine->state_ticks = ticks;
            magazine->state_ticks_total = ticks;
        }
        wd->flags = wd->flags & ~(uint32_t)_weapon_ammo_prediction_pending_bit;
    }
}

/**
 * Member form of the original weapon_trigger_continue_burst: continue burst.
 *
 * @address 0x4c3c60
 */
void weapon_trigger_ref::continue_burst(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (trigger_index + 1 < weapon_tag->triggers.count) {
        weapon_fire_trigger(item_index, (int16_t)(trigger_index + 1));
    }

    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
    wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_overloading;
    wd->triggers[trigger_index].effect_state_ticks = (int16_t)(tag_trigger->overload_time * 30.0f);
}

/**
 * Clears a weapon trigger's effect state back to idle.
 *
 * @address 0x4c3e40
 */
void weapon_trigger_ref::effect_clear(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
}

/**
 * Marks a trigger's effect state as "out of ammo", parked forever.
 *
 * @address 0x4c3e70
 */
void weapon_trigger_ref::effect_set_out_of_ammo(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_out_of_ammo;
    wd->triggers[trigger_index].effect_state_ticks = k_weapon_trigger_effect_ticks_infinite;
}

/**
 * Sets one weapon trigger's effect-state byte and its tick counter to caller-supplied values.
 *
 * @address 0x4c49c0
 */
void weapon_trigger_ref::effect_set_state(int16_t trigger_index, int8_t state, int16_t counter)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    wd->triggers[trigger_index].effect_state = state;
    wd->triggers[trigger_index].effect_state_ticks = counter;
}

/**
 * Member form of the original weapon_trigger_enter_recovery: enter recovery.
 *
 * @address 0x4c3d00
 */
void weapon_trigger_ref::enter_recovery(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    if (tag_trigger->spew_time > 0.0f) {
        wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_spewing;
        wd->triggers[trigger_index].effect_state_ticks = (int16_t)(tag_trigger->spew_time * 30.0f);
        wd->triggers[trigger_index].firing_rate = 0.0f;
        return;
    }

    if (weapon_tag->triggers.count > 1) {
        weapon_fire_trigger(item_index, 1);
    }
    wd->triggers[trigger_index].idle_ticks = 0;
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
    wd->triggers[trigger_index].firing_rate = 0.0f;
}

/**
 * Member form of the original weapon_trigger_finish_shot: finish shot.
 *
 * @address 0x4c48f0
 */
void weapon_trigger_ref::finish_shot(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    wd->triggers[trigger_index].idle_ticks = 0;
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
}

/**
 * Decides what a pulled (or forced) weapon trigger should do this tick: refuse while reloading
 * or overheated, otherwise fire immediately, or enter the charging/overload effect state.
 *
 * @address 0x4c3280
 */
void weapon_trigger_ref::fire_or_reload(int16_t trigger_index, int8_t force)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;
    uint8_t ready;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    ready = 1;
    if (tag_trigger->magazine != (uint16_t)-1 && wd->magazines[tag_trigger->magazine].state != 0) {
        ready = 0;
    }
    if ((wd->flags & _weapon_overheated_bit) != 0) {
        ready = 0;
    }

    if (scenario_location_get_water_and_weather((real_point3d *)((uint8_t *)item_obj + 0x5c),
            (bsp_leaf_reference *)((uint8_t *)item_obj + 0x98), 0) == 0 && ready) {
        if (force == 0) {
            if (tag_trigger->charging_time > 0.0f) {
                if ((weapon_tag->weapon_flags & 0x800) != 0 && wd->age >= 1.0f) {
                    weapon_fire_trigger(item_index, trigger_index);
                    return;
                }
                if (weapon_tag->triggers.count < 2) {
                    if (wd->triggers[trigger_index].firing_rate <= 0.0f) {
                        wd->triggers[trigger_index].flags &= ~0x20;
                    } else {
                        wd->triggers[trigger_index].flags |= 0x20;
                        weapon_fire_trigger(item_index, trigger_index);
                    }
                } else {
                    wd->triggers[trigger_index].effect_handle =
                        weapon_play_trigger_tag_effect(item_index, *(datum_index *)&tag_trigger->charging_effect.tag_id, 0, 0);
                }
                weapon_trigger_effect_set_state(item_index, trigger_index, _weapon_trigger_effect_charging,
                    (int16_t)(int32_t)(tag_trigger->charging_time * 30.0f));
                return;
            }
            if (tag_trigger->overload_time > 0.0f) {
                weapon_trigger_effect_set_state(item_index, trigger_index, _weapon_trigger_effect_overloading,
                    (int16_t)(int32_t)(tag_trigger->overload_time * 30.0f));
                return;
            }
        }
        weapon_fire_trigger(item_index, trigger_index);
    }
}

/**
 * Solves the aiming direction for a shot from origin to target with the projectile of the
 * weapon's trigger_index-th trigger. Returns 1 when the trigger exists (whatever the solver
 * says), 0 otherwise.
 *
 * @address 0x4c2b40
 */
uint8_t weapon_trigger_ref::get_aiming_vector(int16_t trigger_index, real_point3d *origin, real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction, real *out_time, real *out_range, uint8_t *out_used_straight_line)
{
    datum_index weapon_index = datum;
    object *weapon_obj = ((object_header *)object_data->data)[(uint16_t)weapon_index].data;
    Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)weapon_obj->definition_tag].data;

    if (trigger_index >= 0 && (int32_t)trigger_index < (int32_t)weapon_tag->triggers.count) {
        WeaponTrigger *trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        Projectile *projectile_tag =
            (Projectile *)halo::cache::globals().tag_instances[(uint16_t)(*(datum_index *)&trigger->projectile.tag_id)].data;

        projectile_get_aiming_vector(target, 0, projectile_tag, origin, 0, 0, 0, use_high_arc,
            out_direction, 0, out_time, out_range, out_used_straight_line);
        return 1;
    }
    return 0;
}

/**
 * Averages the impact/attached-detonation damage of a weapon's first trigger's projectile, and
 * optionally reports that trigger's maximum rate of fire (upper bound of the two-entry range).
 *
 * @address 0x4c12b0
 */
real weapon_trigger_ref::get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire)
{
    Weapon *weapon_tag;
    WeaponTrigger *trigger;
    datum_index projectile_tag_id;
    Projectile *projectile_tag;
    datum_index damage_tag_id;
    DamageEffect *damage_tag;
    real total;

    total = 0.0f;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)weapon_tag_id].data;
    trigger = (WeaponTrigger *)weapon_tag->triggers.pointer;

    if (out_max_rate_of_fire != 0) {
        *out_max_rate_of_fire = trigger->maximum_rate_of_fire[1];
    }

    projectile_tag_id = *(datum_index *)&trigger->projectile.tag_id;
    if (projectile_tag_id != (datum_index)0xffffffff) {
        projectile_tag = (Projectile *)halo::cache::globals().tag_instances[(uint16_t)projectile_tag_id].data;

        damage_tag_id = *(datum_index *)&projectile_tag->impact_damage.tag_id;
        if (damage_tag_id != (datum_index)0xffffffff) {
            damage_tag = (DamageEffect *)halo::cache::globals().tag_instances[(uint16_t)damage_tag_id].data;
            total = (damage_tag->damage_upper_bound[1] + damage_tag->damage_upper_bound[0]) * 0.5f;
        }

        damage_tag_id = *(datum_index *)&projectile_tag->attached_detonation_damage.tag_id;
        if (damage_tag_id != (datum_index)0xffffffff) {
            damage_tag = (DamageEffect *)halo::cache::globals().tag_instances[(uint16_t)damage_tag_id].data;
            total = total + (damage_tag->damage_upper_bound[1] + damage_tag->damage_upper_bound[0]) * 0.5f;
        }
    }

    return total;
}

/**
 * Returns 1.0 once a trigger is fully charged, an increasing 0..1 fraction while it is
 * charging, or 0.0 for every other effect state.
 *
 * @address 0x4c3100
 */
real weapon_trigger_ref::get_charge_fraction(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    Weapon *weapon_tag;
    weapon_trigger_state *trigger;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    trigger = &wd->triggers[trigger_index];

    if (trigger->effect_state == _weapon_trigger_effect_charging) {
        tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        return 1.0f - ((real)trigger->effect_state_ticks * 0.033333335f) / tag_trigger->charging_time;
    }
    if (trigger->effect_state != _weapon_trigger_effect_charged) {
        return 0.0f;
    }
    return 1.0f;
}

/**
 * Member form of the original weapon_trigger_handle_empty: handle empty.
 *
 * @address 0x4c3de0
 */
void weapon_trigger_ref::handle_empty(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    if (tag_trigger->overcharged_action == 1) {
        weapon_reload_recovery_finish(item_index);
    } else if (tag_trigger->overcharged_action == 2) {
        weapon_trigger_enter_recovery(item_index, trigger_index);
    }
}

/**
 * Divides `elapsed` by trigger_index's projectile initial_velocity (offset 0x1e4 in the
 * Projectile tag), returning 0 when the trigger index is out of range or the projectile has no
 * positive initial velocity.
 *
 * @address 0x4c2be0
 */
real weapon_trigger_ref::projectile_time_fraction(int16_t trigger_index, real elapsed)
{
    datum_index item_index = datum;
    object *item_obj;
    Weapon *weapon_tag;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (trigger_index >= 0 && trigger_index < weapon_tag->triggers.count) {
        WeaponTrigger *trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        uint8_t *projectile_tag = (uint8_t *)halo::cache::globals().tag_instances[(uint16_t)(*(datum_index *)&trigger->projectile.tag_id)].data;
        real initial_velocity = ((Projectile *)projectile_tag)->initial_velocity;

        if (initial_velocity > 0.0f) {
            return elapsed / initial_velocity;
        }
    }
    return 0.0f;
}

/**
 * Evaluates whether a weapon trigger has aged long enough, given its (possibly analog) rate of
 * fire and the weapon's age penalty, to fire again. A does_not_repeat_automatically trigger that
 * is still held down and pulled is never ready.
 *
 * @address 0x4c3190
 */
int32_t weapon_trigger_ref::ready_to_fire(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    weapon_trigger_state *trigger;
    WeaponTrigger *tag_trigger;
    real rate;
    real ticks_per_shot;
    real idle_plus_one;
    int32_t ready;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    trigger = &wd->triggers[trigger_index];
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    rate = (tag_trigger->flags & 0x200) == 0 ? trigger->firing_rate : wd->primary_trigger;
    rate = (tag_trigger->maximum_rate_of_fire[1] - tag_trigger->maximum_rate_of_fire[0]) * rate +
           tag_trigger->maximum_rate_of_fire[0];

    ticks_per_shot = (rate <= 0.0001f) ? 0.0f : 30.0f / rate;
    if (weapon_tag->age_rate_of_fire_penalty > 0.0f) {
        ticks_per_shot = (wd->age * weapon_tag->age_rate_of_fire_penalty + 1.0f) * ticks_per_shot;
    }

    idle_plus_one = (real)trigger->idle_ticks + 1.0f;
    ready = idle_plus_one >= ticks_per_shot;

    if ((tag_trigger->flags & 8) != 0 && (id->flags & _item_held_by_player_bit) != 0 &&
        (trigger->flags & _weapon_trigger_not_pulled_bit) == 0) {
        ready = 0;
    }
    return ready;
}

/**
 * Fully resets a weapon's tracked-object reference and one trigger's effect state.
 *
 * @address 0x4c3eb0
 */
void weapon_trigger_ref::reset_tracking(int16_t trigger_index)
{
    datum_index item_index = datum;
    object *item_obj;
    weapon_data *wd;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    wd->tracked_object_index = (datum_index)0xffffffff;
    wd->triggers[trigger_index].idle_ticks = 0;
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
}

}

extern "C" {

void trigger_create_projectiles(uint32_t item_index, int16_t trigger_index, uint32_t role)
{
    halo::items::weapon_trigger_ref(item_index).create_projectiles(trigger_index, role);
}

void weapon_trigger_barrel_spread_offset(real_vector3d *v, real_vector3d *axis, uint16_t barrel_index, int16_t distribution_function, real distribution_angle, uint32_t flags)
{
    halo::items::weapon_trigger_ref::barrel_spread_offset(v, axis, barrel_index, distribution_function, distribution_angle, flags);
}

void weapon_trigger_become_charged(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).become_charged(trigger_index);
}

void weapon_trigger_begin_reload(datum_index item_index, int16_t magazine_index, int8_t is_client_predicted)
{
    halo::items::weapon_trigger_ref(item_index).begin_reload(magazine_index, is_client_predicted);
}

void weapon_trigger_continue_burst(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).continue_burst(trigger_index);
}

void weapon_trigger_effect_clear(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).effect_clear(trigger_index);
}

void weapon_trigger_effect_set_out_of_ammo(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).effect_set_out_of_ammo(trigger_index);
}

void weapon_trigger_effect_set_state(datum_index item_index, int16_t trigger_index, int8_t state, int16_t counter)
{
    halo::items::weapon_trigger_ref(item_index).effect_set_state(trigger_index, state, counter);
}

void weapon_trigger_enter_recovery(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).enter_recovery(trigger_index);
}

void weapon_trigger_finish_shot(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).finish_shot(trigger_index);
}

void weapon_trigger_fire_or_reload(datum_index item_index, int16_t trigger_index, int8_t force)
{
    halo::items::weapon_trigger_ref(item_index).fire_or_reload(trigger_index, force);
}

uint8_t weapon_trigger_get_aiming_vector(datum_index weapon_index, int16_t trigger_index, real_point3d *origin, real_point3d *target, uint8_t use_high_arc, real_vector3d *out_direction, real *out_time, real *out_range, uint8_t *out_used_straight_line)
{
    return halo::items::weapon_trigger_ref(weapon_index).get_aiming_vector(trigger_index, origin, target, use_high_arc, out_direction, out_time, out_range, out_used_straight_line);
}

real weapon_trigger_get_average_damage(datum_index weapon_tag_id, float *out_max_rate_of_fire)
{
    return halo::items::weapon_trigger_ref::get_average_damage(weapon_tag_id, out_max_rate_of_fire);
}

real weapon_trigger_get_charge_fraction(datum_index item_index, int16_t trigger_index)
{
    return halo::items::weapon_trigger_ref(item_index).get_charge_fraction(trigger_index);
}

void weapon_trigger_handle_empty(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).handle_empty(trigger_index);
}

real weapon_trigger_projectile_time_fraction(datum_index item_index, int16_t trigger_index, real elapsed)
{
    return halo::items::weapon_trigger_ref(item_index).projectile_time_fraction(trigger_index, elapsed);
}

int32_t weapon_trigger_ready_to_fire(datum_index item_index, int16_t trigger_index)
{
    return halo::items::weapon_trigger_ref(item_index).ready_to_fire(trigger_index);
}

void weapon_trigger_reset_tracking(datum_index item_index, int16_t trigger_index)
{
    halo::items::weapon_trigger_ref(item_index).reset_tracking(trigger_index);
}

}
