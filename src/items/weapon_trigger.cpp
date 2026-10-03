#include "halo/core/slot_mask.hpp"
#include "halo/items/items.hpp"
#include "halo/scenario/api.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/items/vars.hpp"
#include "halo/core/libm.hpp"
#include "ai.h"
#include "halo/items/tag_flags.hpp"
#include "halo/projectiles/layout.hpp"
#include "halo/projectiles/api.hpp"

static auto &s_primary_trigger_marker = halo::link::ref<char []>(halo::items::vars().s_primary_trigger_marker);
static auto &s_secondary_trigger_marker = halo::link::ref<char []>(halo::items::vars().s_secondary_trigger_marker);

namespace halo::items {

static_assert(offsetof(WeaponTrigger, distribution_function) == 0x6c);
static_assert(offsetof(WeaponTrigger, projectiles_per_shot) == 0x6e);
static_assert(offsetof(WeaponTrigger, distribution_angle) == 0x70);
static_assert(offsetof(WeaponTrigger, minimum_error) == 0x78);
static_assert(offsetof(WeaponTrigger, error_angle) == 0x7c);
static_assert(offsetof(WeaponTrigger, first_person_offset) == 0x84);
static_assert(offsetof(WeaponTrigger, projectiles_between_contrails) == 0x26);
static_assert(offsetof(WeaponTrigger, projectile) + offsetof(TagDependency, tag_id) == 0xa0);
static_assert(sizeof(weapon_trigger_state) == 0x28);
static_assert(offsetof(weapon_object, weapon.triggers) == 0x260);
static_assert(offsetof(unit_object, unit.controlling_player) == 0x218);
static_assert(offsetof(unit_object, unit.actor_index) == 0x1f4);
static_assert(offsetof(unit_object, unit.gunner_unit_index) == 0x328);
static_assert(offsetof(Unit, unit_flags) == 0x17c);
static_assert(offsetof(Projectile, projectile_flags) == 0x17c);

namespace {

constexpr uint32_t k_object_type_mask_unit = 3;
constexpr uint32_t k_placement_flag_connect_to_map = 2;
constexpr int16_t k_actor_firing_state_holding = 4;
constexpr uint16_t k_unit_flag_fires_from_camera = 1u << 3;

object *object_at(datum_index handle)
{
    return ((object_header *)halo::objects::globals().object_data->data)[handle & halo::k_slot_mask].data;
}

void *tag_data_at(datum_index tag)
{
    return halo::cache::globals().tag_instances[tag & halo::k_slot_mask].data;
}

}  // namespace

/**
 * Member form of the original trigger_create_projectiles: create projectiles.
 *
 * @address 0x4c4c40
 */
void weapon_trigger_ref::create_projectiles(int16_t trigger_index, uint32_t role)
{
    uint32_t item_index = datum;
    weapon_object *item = (weapon_object *)object_at(item_index);
    Weapon *weapon_tag = (Weapon *)tag_data_at(item->base.definition_tag);
    WeaponTrigger *trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
    weapon_trigger_state *state = &item->weapon.triggers[trigger_index];
    datum_index holder = k_datum_index_none;
    uint32_t marker_object = item_index;
    static object_marker markers[0x40];
    int16_t marker_count;
    int16_t m;

    if (item->base.parent_object != k_datum_index_none &&
        halo::objects::object_try_and_get(item->base.parent_object, k_object_type_mask_unit) != 0) {
        holder = item->base.parent_object;
    }
    if ((item->base.flags & 1) && item->base.parent_object != k_datum_index_none) {
        marker_object = item->base.parent_object;
    }
    marker_count = (int16_t)halo::objects::object_get_node_local_transform(marker_object,
        trigger_index == 0 ? s_primary_trigger_marker : s_secondary_trigger_marker, markers, 0x40);
    if (marker_count == 0) {
        marker_count = 1;
    }
    if (!trigger_has(trigger->flags, weapon_trigger_tag_flag::projectiles_use_weapon_origin)) {
        marker_count = 1;
    }

    for (m = 0; m < marker_count; m++) {
        real_point3d origin = markers[m].node_transform.position;
        real_vector3d forward = markers[m].node_transform.forward;
        real speed = 0.0f;
        float error = 0.0f;
        unit_object *holder_object = 0;
        datum_index target = k_datum_index_none;
        datum_index projectile_tag;
        int16_t count;
        datum_index owner = k_datum_index_none;
        int16_t shot;

        if (holder != k_datum_index_none) {
            holder_object = (unit_object *)halo::objects::object_try_and_get(holder, k_object_type_mask_unit);
        }

        if (!trigger_has(trigger->flags, weapon_trigger_tag_flag::projectile_vector_cannot_be_adjusted) && holder_object != 0 &&
            !(holder_object->base.vitality_flags & _object_health_frozen_bit)) {
            Unit *holder_tag = (Unit *)tag_data_at(holder_object->base.definition_tag);
            datum_index player = holder_object->unit.controlling_player;
            datum_index actor = holder_object->unit.actor_index;
            uint8_t use_aiming_vector;
            uint8_t project_point = 1;

            if (holder_object->unit.gunner_unit_index != k_datum_index_none) {
                unit_object *controller = (unit_object *)object_at(holder_object->unit.gunner_unit_index);

                player = controller->unit.controlling_player;
                actor = controller->unit.actor_index;
            }
            use_aiming_vector = (uint8_t)((holder_tag->unit_flags >> 3) & 1);
            if (actor != k_datum_index_none &&
                ((::actor *)halo::ai::globals().actor_data->data)[actor & halo::k_slot_mask].firing_state == k_actor_firing_state_holding) {
                project_point = 0;
            }
            if (holder_object->unit.gunner_unit_index != k_datum_index_none) {
                project_point = 0;
            }
            halo::units::unit_project_onto_aiming_axis(holder, &speed, use_aiming_vector, project_point, &origin, &forward);
            if (player != k_datum_index_none) {
                real_vector3d left;
                real_vector3d up;
                real x = trigger->first_person_offset.x;
                real y = trigger->first_person_offset.y;
                real z = trigger->first_person_offset.z;

                halo::math::vector3d_cross_product(left, forward, *halo::math::globals().global_up3d_pointer);
                if (halo::math::vector3d_normalize_with_length(left) == 0.0f) {
                    left = *halo::math::globals().global_left3d_pointer;
                }
                halo::math::vector3d_cross_product(up, left, forward);
                halo::math::vector3d_normalize_with_length(up);
                origin.x = origin.x + forward.i * x + left.i * y + up.i * z;
                origin.y = origin.y + forward.j * x + left.j * y + up.j * z;
                origin.z = origin.z + forward.k * x + left.k * y + up.k * z;
                target = halo::game::camera_observer_update(player, &origin, &forward);
            } else if (actor != k_datum_index_none) {
                target = halo::ai::actor_compute_grenade_aim_direction(actor, &origin, &forward, &error);
            }
        }
        if (trigger_has(trigger->flags, weapon_trigger_tag_flag::projectiles_use_weapon_origin)) {
            origin = markers[m].node_transform.position;
        }

        if (trigger_index == 0 && item->weapon.alternate_shots_loaded > 0) {
            int16_t charge = item->weapon.alternate_shots_loaded;

            if (weapon_tag->secondary_trigger_mode == 4) {
                charge++;
            }
            projectile_tag = *(datum_index *)&((WeaponTrigger *)weapon_tag->triggers.pointer)[1].projectile.tag_id;
            count = (int16_t)((uint16_t)trigger->projectiles_per_shot * charge);
            item->weapon.alternate_shots_loaded = 0;
        } else {
            count = trigger->projectiles_per_shot;
            projectile_tag = *(datum_index *)&trigger->projectile.tag_id;
        }
        if (projectile_tag == k_datum_index_none) {
            continue;
        }
        {
            datum_index parent = object_at(item_index)->parent_object;
            object *parent_object = parent != k_datum_index_none ? halo::objects::object_try_and_get(parent, k_object_type_mask_unit) : 0;

            if (parent_object != 0) {
                owner = parent;
                if (((unit_object *)parent_object)->unit.gunner_unit_index != k_datum_index_none) {
                    owner = ((unit_object *)parent_object)->unit.gunner_unit_index;
                }
            }
        }

        for (shot = 0; shot < count; shot++) {
            object_placement_data placement;
            uint8_t tracer = 0;
            uint8_t from_player;
            datum_index projectile;
            Projectile *projectile_definition;

            halo::objects::object_placement_data_initialize(&placement, *(datum_index *)&trigger->projectile.tag_id, owner);
            placement.position = origin;
            placement.forward = forward;
            if (state->firing_rate == 0.0f) {
                tracer = 1;
                state->projectiles_since_tracer = 0;
            } else {
                int16_t n = state->projectiles_since_tracer;

                state->projectiles_since_tracer = n + 1;
                if (!(n < trigger->projectiles_between_contrails)) {
                    tracer = 1;
                    state->projectiles_since_tracer = 0;
                }
            }
            if (error == 0.0f) {
                real e = trigger_has(trigger->flags, weapon_trigger_tag_flag::analog_rate_of_fire) ? item->weapon.primary_trigger : state->error;

                error = (1.0f - e) * trigger->error_angle[0] + e * trigger->error_angle[1];
            }
            if (!trigger_has(trigger->flags, weapon_trigger_tag_flag::use_error_when_unzoomed) ||
                !(item->weapon.control_flags & _weapon_control_unknown_40_bit)) {
                halo::math::vector3d_randomize_direction(*((real_point3d *)&placement.forward), &placement.forward, halo::math::globals().random_seed_global,
                                             trigger->minimum_error, error);
            }
            {
                static real_vector3d first_direction;

                if (shot == 0) {
                    first_direction = placement.forward;
                }
                if (trigger_has(trigger->flags, weapon_trigger_tag_flag::projectiles_have_identical_error)) {
                    placement.forward = first_direction;
                }
            }
            halo::math::vector3d_build_perpendicular(placement.up, placement.forward);
            {
                double length = halo::libm::sqrt((double)placement.up.i * (double)placement.up.i +
                                     (double)placement.up.j * (double)placement.up.j +
                                     (double)placement.up.k * (double)placement.up.k);

                if (!(halo::libm::fabs(length) < 9.999999747378752e-05)) {
                    double inverse = 1.0 / length;

                    placement.up.i = (float)(placement.up.i * inverse);
                    placement.up.j = (float)(placement.up.j * inverse);
                    placement.up.k = (float)(placement.up.k * inverse);
                }
            }
            halo::items::weapon_trigger_barrel_spread_offset(&placement.forward, &placement.up, (uint16_t)shot, trigger->distribution_function,
                                                trigger->distribution_angle, (uint32_t)count);
            projectile_definition = (Projectile *)tag_data_at(projectile_tag);
            if (projectile_definition != 0 &&
                (projectile_definition->projectile_flags & (uint32_t)halo::projectiles::projectile_definition_flag::combine_initial_velocity_with_parent) &&
                holder != k_datum_index_none) {
                object *root = object_at(holder);

                while (root->parent_object != k_datum_index_none) {
                    root = object_at(root->parent_object);
                }
                placement.velocity = root->velocity;
            } else {
                placement.velocity.i = placement.forward.i * speed;
                placement.velocity.j = placement.forward.j * speed;
                placement.velocity.k = placement.forward.k * speed;
            }
            from_player = (uint8_t)(holder_object != 0 && holder_object->unit.controlling_player != k_datum_index_none);
            if (from_player) {
                placement.flags |= k_placement_flag_connect_to_map;
            }
            projectile = halo::objects::object_new_with_datum_role_control(&placement, role);
            if (projectile == k_datum_index_none) {
                continue;
            }
            if (from_player) {
                real_point3d camera;

                halo::units::unit_get_camera_position(holder, &camera);
                halo::objects::object_reposition_to_spawn_location(projectile, &camera, holder);
            }
            if (target != k_datum_index_none) {
                ((projectile_object *)object_at(projectile))->projectile.tracked_object_index = target;
            }
            if (!tracer) {
                ((projectile_object *)object_at(projectile))->projectile.flags &= ~(uint32_t)_projectile_tracer_bit;
            }
        }
    }
}

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
        real sin_angle = (real)halo::libm::sin((double)angle);
        real cos_angle = (real)halo::libm::cos((double)angle);
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    wd->triggers[trigger_index].effect_state_ticks = (int16_t)(tag_trigger->charged_time * 30.0f);
    wd->triggers[trigger_index].effect_state = _weapon_trigger_effect_charged;

    halo::items::weapon_set_state(item_index, trigger_index + 7, 1);
    action_handle = halo::interface::local_player_index_for_weapon(item_index);
    halo::interface::first_person_weapon_process_action(action_handle, 0x0e);
    if ((int16_t)action_handle == -1) {
        halo::interface::hud_play_pickup_notification(item_index, 0xe);
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    magazine = &wd->magazines[magazine_index];
    magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;

    if (item_obj->network_role == 1 &&
        (magazine->state == 0 || magazine->state == 2) &&
        halo::items::weapon_triggers_idle(item_index) == 0 &&
        wd->triggers[0].effect_state == _weapon_trigger_effect_out_of_ammo) {
        halo::items::weapon_trigger_effect_clear(item_index, 0);
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

            if (is_client_predicted == 1 && item_obj->network_role == 0 && halo::networking::globals().game_mode == 2) {
                halo::items::weapon_notify_reload_begin(item_index, magazine_index);
            }
            halo::items::weapon_set_state(item_index, magazine_index + 5, 0);
            halo::items::weapon_play_trigger_tag_effect(item_index, *(datum_index *)&magazine_tag->reloading_effect.tag_id,
                0.0f, 0.0f);
            halo::interface::weapon_action_notify_for_weapon(item_index, magazine->rounds_loaded != 0 ? 10 : 9);

            if (weapon_tag->weapon_type == 1) {
                int16_t remaining = magazine_tag->rounds_loaded_maximum - magazine->rounds_loaded;
                mode = (remaining == 1) ? (is_client_predicted == 0 ? 1 : 2)
                                        : (is_client_predicted == 0 ? -1 : 0);
            }

            magazine->state = _weapon_magazine_reloading;
            ticks = halo::items::weapon_get_first_person_animation_time(item_index, 7, 0, mode);
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;

    if (trigger_index + 1 < weapon_tag->triggers.count) {
        halo::items::weapon_fire_trigger(item_index, (int16_t)(trigger_index + 1));
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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
        halo::items::weapon_fire_trigger(item_index, 1);
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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

    if (halo::scenario::scenario_location_get_water_and_weather(&item_obj->position,
            (bsp_leaf_reference *)&item_obj->location_leaf_index, 0) == 0 && ready) {
        if (force == 0) {
            if (tag_trigger->charging_time > 0.0f) {
                if (weapon_has(weapon_tag->weapon_flags, weapon_tag_flag::cannot_fire_at_maximum_age) && wd->age >= 1.0f) {
                    halo::items::weapon_fire_trigger(item_index, trigger_index);
                    return;
                }
                if (weapon_tag->triggers.count < 2) {
                    if (wd->triggers[trigger_index].firing_rate <= 0.0f) {
                        wd->triggers[trigger_index].flags &= ~(uint32_t)_weapon_trigger_charge_effect_bit;
                    } else {
                        wd->triggers[trigger_index].flags |= _weapon_trigger_charge_effect_bit;
                        halo::items::weapon_fire_trigger(item_index, trigger_index);
                    }
                } else {
                    wd->triggers[trigger_index].effect_handle =
                        halo::items::weapon_play_trigger_tag_effect(item_index, *(datum_index *)&tag_trigger->charging_effect.tag_id, 0, 0);
                }
                halo::items::weapon_trigger_effect_set_state(item_index, trigger_index, _weapon_trigger_effect_charging,
                    (int16_t)(int32_t)(tag_trigger->charging_time * 30.0f));
                return;
            }
            if (tag_trigger->overload_time > 0.0f) {
                halo::items::weapon_trigger_effect_set_state(item_index, trigger_index, _weapon_trigger_effect_overloading,
                    (int16_t)(int32_t)(tag_trigger->overload_time * 30.0f));
                return;
            }
        }
        halo::items::weapon_fire_trigger(item_index, trigger_index);
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
    object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)weapon_index].data;
    Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)weapon_obj->definition_tag].data;

    if (trigger_index >= 0 && (int32_t)trigger_index < (int32_t)weapon_tag->triggers.count) {
        WeaponTrigger *trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
        Projectile *projectile_tag =
            (Projectile *)halo::cache::globals().tag_instances[(uint16_t)(*(datum_index *)&trigger->projectile.tag_id)].data;

        halo::ai::projectile_get_aiming_vector(target, 0, projectile_tag, origin, 0, 0, 0, use_high_arc,
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
    if (projectile_tag_id != k_datum_index_none) {
        projectile_tag = (Projectile *)halo::cache::globals().tag_instances[(uint16_t)projectile_tag_id].data;

        damage_tag_id = *(datum_index *)&projectile_tag->impact_damage.tag_id;
        if (damage_tag_id != k_datum_index_none) {
            damage_tag = (DamageEffect *)halo::cache::globals().tag_instances[(uint16_t)damage_tag_id].data;
            total = (damage_tag->damage_upper_bound[1] + damage_tag->damage_upper_bound[0]) * 0.5f;
        }

        damage_tag_id = *(datum_index *)&projectile_tag->attached_detonation_damage.tag_id;
        if (damage_tag_id != k_datum_index_none) {
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    if (tag_trigger->overcharged_action == 1) {
        halo::items::weapon_reload_recovery_finish(item_index);
    } else if (tag_trigger->overcharged_action == 2) {
        halo::items::weapon_trigger_enter_recovery(item_index, trigger_index);
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)halo::cache::globals().tag_instances[(uint16_t)item_obj->definition_tag].data;
    trigger = &wd->triggers[trigger_index];
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;

    rate = !trigger_has(tag_trigger->flags, weapon_trigger_tag_flag::analog_rate_of_fire) ? trigger->firing_rate : wd->primary_trigger;
    rate = (tag_trigger->maximum_rate_of_fire[1] - tag_trigger->maximum_rate_of_fire[0]) * rate +
           tag_trigger->maximum_rate_of_fire[0];

    ticks_per_shot = (rate <= 0.0001f) ? 0.0f : 30.0f / rate;
    if (weapon_tag->age_rate_of_fire_penalty > 0.0f) {
        ticks_per_shot = (wd->age * weapon_tag->age_rate_of_fire_penalty + 1.0f) * ticks_per_shot;
    }

    idle_plus_one = (real)trigger->idle_ticks + 1.0f;
    ready = idle_plus_one >= ticks_per_shot;

    if (trigger_has(tag_trigger->flags, weapon_trigger_tag_flag::does_not_repeat_automatically) && (id->flags & _item_held_by_player_bit) != 0 &&
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

    item_obj = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);

    wd->tracked_object_index = k_datum_index_none;
    wd->triggers[trigger_index].idle_ticks = 0;
    wd->triggers[trigger_index].effect_state = 0;
    wd->triggers[trigger_index].effect_state_ticks = 0;
}

}

namespace halo::items {

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
