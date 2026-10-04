#include "halo/math/constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/units/flags.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/core/bit_cast.hpp"
#include "halo/units/records.hpp"
#include "halo/game/records.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/tags/objects_layout.hpp"
#include "halo/hs/script_globals.hpp"
#include "halo/objects/object_damage.hpp"
#include "halo/tags/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/network_constants.hpp"
#include "game.h"
#include "units.h"
#include "items.h"
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
#include "halo/scenario/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/units/vars.hpp"

static auto &default_collision_material = halo::link::ref<ModelCollisionGeometryMaterial>(halo::objects::vars().default_collision_material);
static auto &g_006f1cf4 = halo::link::ref<uint8_t>(halo::objects::vars().g_006f1cf4);
static auto &global_down3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_down3d_pointer);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &global_origin3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &object_sound_event_last_tick = halo::link::ref<int32_t>(halo::objects::vars().object_sound_event_last_tick);
static auto &team_pair_data = halo::link::ref<team_pair_globals *>(halo::ai::vars().team_pair_data);

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
    object *obj = headers[halo::datum_slot(object_index)].data;
    float max_body_vitality = 0.0f;
    float max_shield_vitality = 0.0f;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    TagID collision_model = definition->collision_model.tag_id;

    if (collision_model.index != halo::k_word_none) {
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
/** The per-material damage multiplier of a damage effect (the dirt..ice floats are consecutive, indexed by material type). */
static float material_damage_multiplier(const DamageEffect &effect, int32_t material_type)
{
    return (&effect.dirt)[material_type];
}
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
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));
    Object *object_tag = halo::objects::tag_as<Object>(obj->base.definition_tag);
    uint16_t *vitality_flags = &obj->base.vitality_flags;
    float *shield = &obj->base.shield_vitality;

    if (halo::objects::tag_handle(object_tag->collision_model) != k_datum_index_none) {
        ModelCollisionGeometry *geometry = halo::objects::tag_as<ModelCollisionGeometry>(halo::objects::tag_handle(object_tag->collision_model));

        if (geometry != 0) {
            uint16_t flags = *vitality_flags;
            uint16_t kill_request = (uint16_t)(flags & static_cast<uint16_t>(vitality_flag::shield_stationary));

            if (kill_request || test_flag(flags, vitality_flag::die_act_of_god | vitality_flag::die_act_of_god_silent)) {
                datum_index effect = halo::objects::tag_handle(halo::objects::block_element<GlobalsFallingDamage>(global_globals->falling_damage, 0).falling_damage);

                if (!test_flag(flags, vitality_flag::health_frozen) && effect != k_datum_index_none) {
                    damage_data dd;

                    halo::objects::damage_data_initialize(&dd, effect);
                    set_flag(dd.flags, damage_data_flag::kill_target);
                    dd.random_blend = 1.0f;
                    if (test_flag(flags, vitality_flag::die_act_of_god_silent)) {
                        set_flag(dd.flags, damage_data_flag::kill_silently);
                    }
                    if (kill_request) {
                        set_flag(dd.flags, damage_data_flag::suppress_death_notification);
                    }
                    halo::objects::object_apply_damage(&dd, object_index, -1, -1, -1, 0);
                }
                clear_flag(*vitality_flags, vitality_flag::shield_stationary | vitality_flag::die_act_of_god | vitality_flag::die_act_of_god_silent);
            }
            clear_flag(obj->base.vitality_flags, vitality_flag::stunned);
            if (obj->base.maximum_shield_vitality > 0.0f && (*vitality_flags & 4) == 0) {
                uint16_t current = *vitality_flags;

                if (current & 0x10) {
                    float value = *shield + halo::math::k_seconds_per_tick;

                    *shield = value;
                    if (value < 3.0f) {
                        *vitality_flags = (uint16_t)(current | to_bits(vitality_flag::stunned));
                    } else {
                        *shield = 3.0f;
                        *vitality_flags = (uint16_t)(current & ~to_bits(vitality_flag::shield_recharging));
                    }
                } else if (*shield > 1.0f && halo::game::globals().current_engine != 0) {
                    datum_index player_index = halo::game::player_index_from_unit_index(object_index);
                    float excess = *shield - 1.0f;

                    if (0.00074074074f > excess) {
                        *shield = 1.0f;
                        halo::interface::hud_unit_meter_apply_predictive_damage(player_index, excess);
                    } else {
                        *shield = *shield - 0.00074074074f;
                        halo::interface::hud_unit_meter_apply_predictive_damage(player_index, 0.00074074074f);
                    }
                } else if (*shield < 1.0f) {
                    int16_t stun = obj->base.shield_stun_ticks;

                    if (stun == 0) {
                        float rate = halo::game::weapon_get_zoom_fov_resolved(3, obj->base.owner_team) *
                            geometry->shield_recharge_rate;
                        float value;

                        if ((uint8_t)obj->base.vitality_flags & 8) {
                            halo::objects::object_dispatch_effect_notify(object_index, halo::objects::tag_handle(geometry->shield_recharging_effect));
                            clear_flag(obj->base.vitality_flags, vitality_flag::shield_depleted);
                            halo::objects::object_regions_reset_permutation_lock(object_index, 1);
                        }
                        set_flag(obj->base.vitality_flags, vitality_flag::stunned);
                        value = rate + *shield;
                        *shield = value;
                        if (value > 1.0f) {
                            *shield = 1.0f;
                            clear_flag(*vitality_flags, vitality_flag::stunned);
                        }
                    } else if (obj->base.network_role == 3 || obj->base.network_role == 0) {
                        obj->base.shield_stun_ticks = (int16_t)(stun - 1);
                    }
                }
            }
            object_decay_damage_timer(&obj->base.body_damage_ticks, &obj->base.current_body_damage, &obj->base.recent_body_damage);
            object_decay_damage_timer(&obj->base.shield_damage_ticks, &obj->base.current_shield_damage, &obj->base.recent_shield_damage);
        }
    }
    if (obj->base.type == _object_type_biped && obj->base.network_role == 0) {
        unit_object *object = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));

        if ((uint32_t)(object->base.shield_stun_ticks > 0) != (uint32_t)halo::units::biped_data_of(object)->network_shield_stunned) {
            set_flag(object->base.flags, objects::object_flag::changed);
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
    *dd = damage_data{};

    dd->damage_effect_tag = damage_effect_tag;
    dd->material_type = static_cast<int16_t>(-1);
    dd->responsible_player = k_datum_index_none;
    dd->responsible_object = k_datum_index_none;
    dd->team_index = static_cast<int16_t>(-1);
    dd->location_cluster_index = static_cast<int16_t>(-1);
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
    object *obj = headers[halo::datum_slot(object_index)].data;

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
    object *obj = headers[halo::datum_slot(object_index)].data;

    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return;
    }

    obj->vitality_flags |= _object_health_frozen_bit;

    if (((Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data)->collision_model.tag_id.index != halo::k_word_none) {

        halo::effects::effect_new_on_object(object_index,
            halo::objects::tag_handle(halo::objects::tag_as<ModelCollisionGeometry>(((Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data)
                ->collision_model.tag_id.index)->body_depleted_effect),
            object_index, -1, 0.0f, 0.0f, 0, 0);
    }

    if (obj->type == _object_type_vehicle) {
        datum_index child_index = obj->first_child_object;
        while (child_index != k_datum_index_none) {
            object *child = headers[halo::datum_slot(child_index)].data;
            unit_object *child_unit = reinterpret_cast<unit_object *>(child);

            if (child->type == _object_type_biped &&
                (child_unit->unit.controlling_player == k_datum_index_none || halo::hs::fields::deathless_player == 0) &&
                child_unit->unit.vehicle_seat_index != -1) {
                set_flag(child->vitality_flags, objects::vitality_flag::die_act_of_god);
            }
            child_index = child->next_object;
        }
    }

    halo::objects::object_set_shield_depleted_flag(object_index);
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
    object *obj = headers[halo::datum_slot(object_index)].data;

    if ((obj->vitality_flags & _object_shield_depleted_bit) != 0) {
        return;
    }

    {
        datum_index collision_model = halo::objects::tag_handle(((Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data)->collision_model);

        if (collision_model != k_datum_index_none) {

            halo::effects::effect_new_on_object(object_index,
                *(datum_index *)(halo::objects::tag_record_bytes(collision_model) + 0x1a4),
                object_index, -1, 0.0f, 0.0f, 0, 0);
        }
    }

    obj->vitality_flags |= _object_shield_depleted_bit;
    obj->current_shield_damage = 0.0f;

    halo::objects::object_regions_reset_permutation_lock(object_index, 0);
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
    object *obj = headers[halo::datum_slot(object_index)].data;

    if (obj->shield_vitality <= 1.0f) {
        obj->vitality_flags |= _object_shield_recharging_bit;
        if (obj->shield_vitality == 0.0f) {
            obj->shield_vitality = 0.01f;
        }
        obj->shield_stun_ticks = 0;
        obj->shield_update_pending = 1;

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
    DamageEffect *effect = (DamageEffect *)halo::cache::globals().tag_instances[halo::datum_slot(dd->damage_effect_tag)].data;
    int16_t found = (int16_t)halo::objects::object_find_in_sphere(0, 0, &dd->location_leaf_index, &dd->epicentre,
        effect->radius[1], candidates, k_maximum_damage_candidates);

    if (found > 0) {
        int32_t i;
        for (i = 0; i < found; i++) {
            halo::objects::object_damage_apply_line_of_sight(dd, candidates[i], 0);
        }
    }

    halo::physics::breakable_surface_damage_in_blast_radius(dd);
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
    DamageEffect *effect = halo::objects::tag_as<DamageEffect>(dd->damage_effect_tag);
    real_point3d *origin = &dd->origin;

    for (;;) {
        unit_object *target = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(target_index));
        Unit *target_tag = halo::objects::tag_as<Unit>(target->base.definition_tag);
        uint8_t apply = (uint8_t)(~(uint8_t)target->base.flags & 1);
        uint8_t applied = 0;
        uint8_t spared_player = 0;
        uint8_t blocked;
        uint32_t flags;

        if (apply && ((1u << ((uint8_t)target->base.type & 0x1f)) & _object_mask_unit) && effect->damage_aoe_core_radius > 9.999999747378752e-05f) {

            real_vector3d to_center;
            real_vector3d side_a;
            real_vector3d side_b;
            int32_t i;

            blocked = 1;
            to_center.i = target->base.bounding_center.x - origin->x;
            to_center.j = target->base.bounding_center.y - origin->y;
            to_center.k = target->base.bounding_center.z - origin->z;
            halo::math::vector3d_build_perpendicular(side_a, to_center);
            halo::math::vector3d_normalize_with_length(side_a);
            halo::math::vector3d_cross_product(side_b, side_a, to_center);
            halo::math::vector3d_normalize_with_length(side_b);
            for (i = 0; i < 4; i++) {
                real radius = effect->damage_aoe_core_radius;
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
                halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::object_vehicle | halo::collision_test_flag::object_scenery | halo::collision_test_flag::object_machine), origin, &offset, halo::objects::object_get_root_object_index(target_index), &hit);
                sample = hit.point;
                back.i = target->base.bounding_center.x - sample.x;
                back.j = target->base.bounding_center.y - sample.y;
                back.k = target->base.bounding_center.z - sample.z;
                if (!halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::object_vehicle | halo::collision_test_flag::object_scenery | halo::collision_test_flag::object_machine), &sample, &back, halo::objects::object_get_root_object_index(target_index), &hit)) {
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
                walk = ((struct object *)halo::objects::object_record_bytes(walk))->parent_object;
            }
            to_center.i = target->base.bounding_center.x - origin->x;
            to_center.j = target->base.bounding_center.y - origin->y;
            to_center.k = target->base.bounding_center.z - origin->z;
            blocked = halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::object_vehicle | halo::collision_test_flag::object_scenery | halo::collision_test_flag::object_machine), origin, &to_center, root, &hit);
        }
        if (blocked) {
            apply = 0;
        }

        flags = effect->damage_flags;
        if (test_flag(flags, tags::damage_effect_damage_tag_flag::does_not_hurt_owner) && target_index == dd->responsible_object) {
            apply = 0;
        }
        if (test_flag(flags, tags::damage_effect_damage_tag_flag::does_not_hurt_friends) && !halo::game::teams_are_enemies(dd->team_index, target->base.owner_team)) {
            apply = 0;
        } else if (apply && test_flag(flags, tags::damage_effect_damage_tag_flag::infection_form_pop)) {
            apply = 0;
            if (((1u << ((uint8_t)target->base.type & 0x1f)) & _object_mask_unit) &&
                (halo::objects::tag_as<Item>(target->base.definition_tag)->item_flags & 0x80000) &&
                target_index != dd->responsible_object) {
                real scale = halo::game::weapon_get_zoom_fov(8, halo::main::globals().game_globals->difficulty);

                apply = 1;
                if ((scale > 0.0f || test_flag(flags, tags::damage_effect_damage_tag_flag::only_hurts_one_infection_form)) && test_flag(dd->flags, damage_data_flag::player_spared)) {
                    apply = 0;
                }
                if (scale > 0.0f && scale * 0.25f > halo::math::random_real()) {
                    apply = 0;
                }
                spared_player = 1;
            }
        }
        set_flag(dd->flags, damage_data_flag::area_damage);

        if (apply) {

            real distance;
            real blend;
            real range;

            dd->direction.i = target->base.bounding_center.x - origin->x;
            dd->direction.j = target->base.bounding_center.y - origin->y;
            dd->direction.k = target->base.bounding_center.z - origin->z;
            distance = halo::math::vector3d_normalize_with_length(dd->direction);
            range = effect->radius[1] - effect->radius[0];
            if (range > 0.0f) {
                blend = 1.0f - (distance - effect->radius[0]) / range;
                if (!(blend >= 0.0f)) {
                    blend = 0.0f;
                } else if (!(blend <= 1.0f)) {
                    blend = 1.0f;
                }
            } else {
                blend = 1.0f;
            }
            if (!(effect->flags & 1)) {
                dd->random_blend = blend;
            }
            if (blend > 0.0f) {
                halo::objects::object_apply_damage(dd, target_index, -1, -1, -1, 0);
                applied = 1;
            }
            {
                datum_index model = halo::objects::tag_handle(target_tag->base.collision_model);

                if (model != k_datum_index_none && (*halo::objects::tag_record_bytes(model) & 8) &&
                    target->base.first_child_object != k_datum_index_none) {
                    halo::objects::object_damage_apply_line_of_sight(dd, target->base.first_child_object, 1);
                }
            }
        }
        if (spared_player && (!apply || !applied)) {
            set_flag(dd->flags, damage_data_flag::player_spared);
        }
        if (!continue_flag || target->base.next_object == k_datum_index_none) {
            return;
        }
        continue_flag = 1;
        target_index = target->base.next_object;
    }
}

/**
 * Raises a multiplayer sound event at a throttled rate.
 *
 * Original register convention: none (no parameters).
 *
 * @address 0x004ee370
 */
void halo::objects::DamageSystem::throttled_multiplayer_sound_event()
{
    if (halo::hs::fields::should_play_multiplayer_hit_sound == 1 && (uint32_t)(object_sound_event_last_tick + 2) < (uint32_t)halo::game::globals().game_time->game_time) {
        halo::game::game_engine_queue_multiplayer_sound(0x2b, k_datum_index_none, 0);
        object_sound_event_last_tick = halo::game::globals().game_time->game_time;
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
    struct {
        int32_t network_id;
        float shield_damage;
        int8_t notify;
    } decoded;

    if (halo::networking::delta_context(message)->state->incremental != 0) {
        halo::networking::message_delta_decode_compound_field_staged(message);
        return;
    }

    if (halo::networking::message_delta_decode_compound_field(message, &decoded) == 0) {
        return;
    }

    if (decoded.network_id != 0) {
        datum_index effect = object_network_id_table->handles[decoded.network_id];

        if (effect != k_datum_index_none) {
            object *target = halo::objects::object_try_and_get(effect, _object_mask_unit);

            if (target != 0) {
                if (0.0f < decoded.shield_damage) {
                    target->shield_damage_ticks = 0;
                    if ((target->vitality_flags & _object_shield_depleted_bit) == 0) {
                        target->current_shield_damage = 1.0f;
                    }
                    target->recent_shield_damage = decoded.shield_damage + target->recent_shield_damage;
                    if (1.0f < target->current_shield_damage) {
                        target->current_shield_damage = 1.0f;
                    }
                    if (1.0f < target->recent_shield_damage) {
                        target->recent_shield_damage = 1.0f;
                    }
                }
                if (decoded.notify == 1) {
                    halo::objects::object_set_shield_depleted_flag(effect);
                }
                halo::units::unit_update_stance_and_jump(effect, 0, 0, 0, 0, 0, 0, k_datum_index_none, 0, 0);
            }
        }
    }

    halo::objects::object_throttled_multiplayer_sound_event();
}

namespace {
static unit_object *object_get(datum_index object_index)
{
    return reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));
}

template <typename T>
static T *tag_get(datum_index tag_index)
{
    return halo::objects::tag_as<T>(tag_index);
}

static uint8_t teams_are_friends(int32_t bit)
{
    return (uint8_t)((team_pair_data->enemy_bits[bit >> 5] >> (bit & 0x1f)) & 1);
}

static player *player_try_get(datum_index player_index)
{
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    player *record;

    if (player_index == k_datum_index_none || index < 0 || index >= halo::game::globals().player_data->maximum_count) {
        return 0;
    }
    record = halo::game::player_at(static_cast<uint32_t>(index));
    if (record->identifier == 0 || (salt != 0 && record->identifier != salt)) {
        return 0;
    }
    return record;
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
    unit_object *target = object_get(target_index);
    DamageEffect *effect_block = tag_get<DamageEffect>(dd->damage_effect_tag);
    int32_t target_role = target->base.network_role;
    uint8_t target_is_local = (target_role == 0 || target_role == 3);
    uint8_t difficulty_scaled = 0;
    uint8_t parents_take_damage = 1;
    uint8_t no_random_range;
    uint8_t reported = 0;
    datum_index list[17];
    int16_t count = 0;
    int16_t remaining;
    uint32_t flags;
    Unit *target_tag;
    float amount;

    if (dd->responsible_player != k_datum_index_none && player_try_get(dd->responsible_player) == 0) {
        dd->responsible_player = k_datum_index_none;
    }
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    amount = ((effect_block->damage_upper_bound[1] - effect_block->damage_upper_bound[0]) *
              ((float)(int32_t)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale) + effect_block->damage_upper_bound[0]) *
             dd->random_blend + (1.0f - dd->random_blend) * effect_block->damage_lower_bound;
    amount = amount * dd->multiplier;
    if (dd->responsible_object != k_datum_index_none) {
        unit_object *responsible = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(dd->responsible_object, 3));

        if (responsible != 0) {
            datum_index actor;

            if (responsible->unit.gunner_unit_index != k_datum_index_none) {
                responsible = object_get(responsible->unit.gunner_unit_index);
            }
            actor = responsible->unit.swarm_actor_index;
            if (actor == k_datum_index_none) {
                actor = responsible->unit.actor_index;
            }
            if (actor != k_datum_index_none) {
                halo::ai::actor_apply_perception_scale(actor, (const uint8_t *)dd, &amount);
            }
        }
    }
    if (halo::game::globals().current_engine != 0) {
        int32_t target_player = halo::objects::object_get_controlling_player_index(target_index);
        int32_t responsible_player = halo::objects::object_get_controlling_player_index(dd->responsible_object);

        amount = halo::game::game_engine_compute_time_scale(responsible_player, target_player) * amount;
    } else if (dd->team_index != -1) {
        int16_t team = dd->team_index;

        if (team < 0 || team >= 10 || !teams_are_friends(team * 10 + 1)) {
            amount = halo::game::weapon_get_zoom_fov(0, halo::main::globals().game_globals->difficulty) * amount;
            difficulty_scaled = 1;
        }
    }

    flags = dd->flags;
    if (test_flag(flags, damage_data_flag::area_damage | damage_data_flag::kill_target)) {
        list[0] = target_index;
        count = 1;
    } else if (target_index != k_datum_index_none) {
        datum_index id = target_index;

        do {
            list[count++] = id;
            id = object_get(id)->base.parent_object;
        } while (id != k_datum_index_none);
    }
    target_tag = tag_get<Unit>(target->base.definition_tag);
    if (halo::objects::tag_handle(target_tag->base.collision_model) != k_datum_index_none) {
        parents_take_damage = (uint8_t)(~(tag_get<ModelCollisionGeometry>(halo::objects::tag_handle(target_tag->base.collision_model))->flags >> 4) & 1);
    }
    if (target->base.damage_owner != k_datum_index_none) {
        list[count++] = target->base.damage_owner;
    }

    if ((flags & 1) == 0 && target->base.type == _object_type_vehicle) {
        float rider_fraction = (1.0f - effect_block->damage_vehicle_passthrough_penalty) * target_tag->rider_damage_fraction;
        datum_index child;

        dd->multiplier = rider_fraction;
        if (halo::game::globals().current_engine != 0 && target->base.first_child_object != k_datum_index_none) {
            int32_t players = 0;

            for (child = target->base.first_child_object; child != k_datum_index_none;
                 child = object_get(child)->base.next_object) {
                unit_object *rider = object_get(child);

                if (rider->base.type == _object_type_biped && rider->unit.controlling_player != k_datum_index_none) {
                    players++;
                }
            }
            if (players != 0) {
                dd->multiplier = rider_fraction / (float)players;
            }
        }
        for (child = target->base.first_child_object; child != k_datum_index_none;
             child = object_get(child)->base.next_object) {
            unit_object *rider = object_get(child);

            if (rider->base.type != _object_type_biped) {
                continue;
            }
            if (rider->unit.controlling_player == k_datum_index_none) {
                if (child != target->unit.driver_unit_index) {
                    continue;
                }
                set_flag(dd->flags, damage_data_flag::recursing_into_child);
            } else {
                clear_flag(dd->flags, damage_data_flag::recursing_into_child);
            }
            halo::objects::object_apply_damage(dd, child, -1, -1, -1, 0);
            clear_flag(dd->flags, damage_data_flag::recursing_into_child);
        }
        dd->multiplier = 1.0f;
    }

    no_random_range = (effect_block->damage_upper_bound[0] == 0.0f && effect_block->damage_upper_bound[1] == 0.0f);
    {
        int16_t i;

        for (i = 0; i < count; i++) {
            datum_index id = list[i];
            int16_t index = (int16_t)id;
            int16_t salt = (int16_t)(id >> 16);
            object_header *header;
            object *unit;
            datum_index player_index;

            if (id == k_datum_index_none || index < 0 || index >= object_data->maximum_count) {
                continue;
            }
            header = &halo::objects::object_header_of(static_cast<uint32_t>(index));
            if (header->identifier == 0 || (salt != 0 && header->identifier != salt)) {
                continue;
            }
            if (((1u << (header->type & 0x1f)) & _object_mask_unit) == 0) {
                continue;
            }
            unit = header->data;
            if (unit == 0) {
                continue;
            }
            player_index = reinterpret_cast<unit_object *>(unit)->unit.controlling_player;
            if (player_index != k_datum_index_none) {
                switch (halo::networking::globals().game_mode) {
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
            } else if (halo::hs::fields::reflexive_damage_effects) {
                datum_index first_local = halo::game::globals().local_player_globals->local_players[0];

                if (halo::networking::globals().game_mode == halo::networking::k_game_mode_local) {
                    halo::effects::player_effect_mark_damage_direction(first_local, dd, &dd->direction, dd->random_blend, amount);
                } else if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
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
        unit_object *obj;
        Object *object_tag;
        ModelCollisionGeometry *geometry;
        ModelCollisionGeometryMaterial *material;
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
        object_tag = tag_get<Object>(obj->base.definition_tag);
        if (halo::objects::tag_handle(object_tag->collision_model) != k_datum_index_none) {
            geometry = tag_get<ModelCollisionGeometry>(halo::objects::tag_handle(object_tag->collision_model));
            kill = (uint8_t)test_flag(dd->flags, damage_data_flag::kill_target);
            *(datum_index *)record.unknown_00 = id;
            record.shield_damage_dealt = 0.0f;
            record.depleted_this_call = 0;
            if (obj->base.network_role == 3 || obj->base.network_role == 0) {
                apply_state = 1;
            } else {
                player *responsible = player_try_get(dd->responsible_player);

                apply_state = (responsible != 0 && responsible->local_player_index != -1) ? 0 : 1;
            }
            if (halo::game::globals().current_engine != 0 && halo::game::globals().teams_enabled) {
                datum_index owner = halo::game::player_index_from_unit_index(id);

                if (owner != k_datum_index_none && owner != dd->responsible_player) {
                    player *owner_record = player_try_get(owner);

                    if (owner_record != 0) {
                        friendly = (uint8_t)(halo::game::teams_are_enemies(dd->team_index,
                            static_cast<int16_t>(owner_record->team)) == 0);
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
                                if ((effect_block->damage_flags & 0x20) == 0) {
                                    shield_allowed = 0;
                                    body_allowed = 0;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            if (node_index >= 0 && node_index < geometry->nodes.count) {
                region = static_cast<int32_t>((static_cast<uint32_t>(region) & 0xffff0000u) | static_cast<uint16_t>(halo::objects::block_element<ModelCollisionGeometryNode>(geometry->nodes, node_index).name_thing));
            }
            if (difficulty_scaled) {
                notify_flags = 0x20;
            }
            if (dd->team_index != -1) {
                int16_t team = obj->base.owner_team;

                if (halo::game::globals().current_engine != 0) {
                    if (team == dd->team_index) {
                        notify_flags |= 0x10;
                    }
                } else if (team >= 0 && team < 10 && dd->team_index >= 0 && dd->team_index < 10) {
                    if (teams_are_friends(team * 10 + dd->team_index)) {
                        notify_flags |= 0x10;
                    }
                }
            }
            if (i == 0 && material_index >= 0 && material_index < geometry->materials.count) {
                material = &halo::objects::block_element<ModelCollisionGeometryMaterial>(geometry->materials, material_index);
            } else if (geometry->indirect_damage_material >= 0 && geometry->indirect_damage_material < geometry->materials.count) {
                material = &halo::objects::block_element<ModelCollisionGeometryMaterial>(geometry->materials, geometry->indirect_damage_material);
            } else {
                material = &default_collision_material;
            }
            dd->material_type = material->material_type;
            if (halo::hs::fields::omnipotent && dd->responsible_player != k_datum_index_none) {
                kill = 1;
            }
            if (effect_block->damage_side_effect == damageeffectsideeffect_lethal_to_the_unsuspecting && halo::units::unit_point_in_front_and_asleep(&dd->origin, id) &&
                ((uint8_t)(obj->base.vitality_flags >> 8) & 8) == 0) {
                kill = 1;
            }
            if (target_is_local == 1 && kill && !test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen) && (!friendly || body_allowed)) {
                obj->base.body_vitality = 0.0f;
                halo::objects::object_set_health_frozen_flag(id);
                notify_flags |= 0x41;
            }
            if (!test_flag(dd->flags, damage_data_flag::recursing_into_child) && !test_flag(effect_block->damage_flags, tags::damage_effect_damage_tag_flag::skips_shields) &&
                obj->base.maximum_shield_vitality > 0.0f && (!friendly || shield_allowed) && (i == 0 || test_flag(geometry->flags, tags::model_collision_geometry_tag_flag::takes_shield_damage_for_children))) {
                halo::objects::object_apply_shield_damage(id, geometry, material, effect_block, &notify_flags, &shield_damage, &amount,
                    target_is_local, apply_state, &record);
            }
            if ((i == 0 || (parents_take_damage && test_flag(geometry->flags, tags::model_collision_geometry_tag_flag::takes_body_damage_for_children))) &&
                !test_flag(effect_block->damage_flags, tags::damage_effect_damage_tag_flag::only_hurts_shields)) {
                if ((test_flag(geometry->flags, tags::model_collision_geometry_tag_flag::only_damaged_by_explosives) && !test_flag(effect_block->damage_flags, tags::damage_effect_damage_tag_flag::detonates_explosives)) ||
                    (friendly && !body_allowed)) {
                    amount = 0.0f;
                }
                halo::objects::object_apply_body_damage(id, (i == 0) ? region_index : -1, (i == 0) ? node_index : -1,
                    (void *)(uintptr_t)((i == 0) ? hit_plane : 0), geometry, material, effect_block, dd, &notify_flags,
                    &body_damage, &material_multiplier, amount, target_is_local);
                remaining = 0;
            }
            if (!reported && (shield_damage > 0.0001f || body_damage > 0.0001f)) {
                if (shield_damage > body_damage) {
                    dd->material_type = geometry->shield_material_type;
                    dd->remaining_vitality = halo::bit_cast<uint32_t>(obj->base.shield_vitality);
                } else {
                    float vitality = obj->base.body_vitality;

                    if (vitality < 0.0f) {
                        vitality = 0.0f;
                    } else if (vitality > 1.0f) {
                        vitality = 1.0f;
                    }
                    dd->remaining_vitality = halo::bit_cast<uint32_t>(vitality);
                }
                reported = 1;
            }
            halo::objects::object_notify_pickup_or_refresh_probe(id, dd->responsible_player);
            if (shield_damage > 0.0f && obj->base.type == _object_type_biped) {
                obj->base.shield_update_pending = 1;
            }
        }
        halo::objects::object_damage_notify_and_impulse(id, dd, notify_flags, shield_damage, body_damage,
            halo::bit_cast<uint32_t>(material_multiplier), region, target_is_local);
        if (notify_flags & 4) {
            int32_t role = object_get(id)->base.network_role;

            if (role == 0) {
                halo::objects::object_delete_unparented(id);
                halo::objects::object_delete_recursive(id, 0);
            } else if (role == 3) {
                halo::objects::object_delete_recursive(id, 0);
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
    ModelCollisionGeometry *geometry, ModelCollisionGeometryMaterial *material, DamageEffect *effect_block, damage_data *dd, uint32_t *notify_flags,
    float *body_damage_out, float *material_multiplier_out, float damage, uint8_t is_local)
{
    uint32_t target_index = handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(target_index));
    uint16_t *vitality_flags = &obj->base.vitality_flags;
    float *vitality = &obj->base.body_vitality;
    float body = damage * material->body_damage_multiplier;
    float maximum;
    float inverse_maximum;
    float value;
    float taken;
    uint8_t unscaled = 0;

    if ((geometry->flags & 0x40) && obj->base.type == _object_type_vehicle && obj->unit.driver_unit_index == k_datum_index_none) {
        body = 0.0f;
    }
    if (halo::game::globals().current_engine == 0 && effect_block->damage_category == 1 && obj->base.owner_team == 1) {
        unscaled = 1;
    }
    maximum = obj->base.maximum_body_vitality;
    if (!unscaled) {
        maximum = halo::game::weapon_get_zoom_fov_resolved(1, obj->base.owner_team) * maximum;
    }
    inverse_maximum = (maximum > 0.0f) ? 1.0f / maximum : 0.0f;
    value = body;
    if (*notify_flags & 0x10) {
        value = (1.0f - geometry->friendly_damage_resistance) * body;
        if (*notify_flags & 0x20) {
            real multiplier = halo::game::weapon_get_zoom_fov(0, halo::main::globals().game_globals->difficulty);

            if (multiplier > 0.0f) {
                value = value / multiplier;
            }
        }
    }
    taken = value * inverse_maximum * material_damage_multiplier(*effect_block, material->material_type);
    bool skip_region_damage = false;
    if (test_flag(*vitality_flags, vitality_flag::hash_flag)) {
        if (is_local != 1) {
            skip_region_damage = true;
        }
    } else {
        if (body > 0.0f && (geometry->unknown_20 & 1)) {
            uint32_t effect_flags = effect_block->damage_flags;

            if (test_flag(effect_flags, tags::damage_effect_damage_tag_flag::can_cause_headshots)) {
                if (!(halo::game::globals().current_engine == 0 && obj->base.type == _object_type_biped &&
                      obj->unit.controlling_player != k_datum_index_none)) {
                    if (is_local == 1) {
                        *vitality = 0.0f;
                    }
                    *notify_flags |= 0x40;
                    if (halo::game::globals().current_engine != 0) {
                        *notify_flags |= 0x80;
                    }
                }
            } else if (test_flag(effect_flags, tags::damage_effect_damage_tag_flag::can_cause_multiplayer_headshots) && halo::game::globals().current_engine != 0) {
                taken = taken + taken;
                if (taken > *vitality) {
                    *notify_flags |= 0x80;
                }
            }
        }
        if (is_local != 1) {
            skip_region_damage = true;
        } else {
            *vitality = *vitality - taken;
        }
    }
    if (!skip_region_damage && (int16_t)region_index != -1) {
        int32_t region = (int16_t)region_index;

        if ((obj->base.destroyed_region_flags & (1u << (region & 0x1f))) == 0) {
            ModelCollisionGeometryRegion *region_block = &halo::objects::block_element<ModelCollisionGeometryRegion>(geometry->regions, region);
            uint8_t region_damage = (uint8_t)(int32_t)(taken * 255.0f + (int32_t)obj->base.region_vitality[region]);

            obj->base.region_vitality[region] = region_damage;
            if (region_block->damage_threshold > 0.0f &&
                (float)region_damage * 0.0039215689f > region_block->damage_threshold) {
                halo::objects::object_destroy_region(target_index, region_index);
                *notify_flags |= 2;
            }
        }
    }
    {
        float current = taken + obj->base.current_body_damage;
        float recent;

        obj->base.body_damage_ticks = 0;
        obj->base.current_body_damage = current;
        recent = taken + obj->base.recent_body_damage;
        obj->base.recent_body_damage = recent;
        if (current > 1.0f) {
            obj->base.current_body_damage = 1.0f;
        }
        if (recent > 1.0f) {
            obj->base.recent_body_damage = 1.0f;
        }
    }
    if (g_0087abc0 && *vitality < 0.0f && ((1u << ((uint8_t)obj->base.type & 0x1f)) & _object_mask_unit)) {
        if (obj->unit.controlling_player != k_datum_index_none) {
            *vitality = 0.0f;
        } else if (obj->base.type == _object_type_vehicle) {
            datum_index child = obj->base.first_child_object;

            while (child != k_datum_index_none) {
                unit_object *child_obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(child));

                if (((1u << ((uint8_t)child_obj->base.type & 0x1f)) & _object_mask_unit) &&
                    child_obj->unit.controlling_player != k_datum_index_none) {
                    *vitality = 0.0f;
                    break;
                }
                child = child_obj->base.next_object;
            }
        }
    }
    if (is_local == 1) {
        float absolute = halo::game::weapon_get_zoom_fov_resolved(1, obj->base.owner_team) * obj->base.maximum_body_vitality *
            *vitality;
        float destroyed = geometry->body_destroyed_threshold;

        if (destroyed < 0.0f && absolute < destroyed) {
            halo::objects::object_delete_teardown(target_index);
            *notify_flags |= 5;
        } else if (absolute < 0.0f) {
            if ((*vitality_flags & 4) == 0) {
                int16_t i;

                for (i = 0; i < geometry->regions.count; i++) {
                    if (halo::objects::block_element<ModelCollisionGeometryRegion>(geometry->regions, i).flags & 4) {
                        halo::objects::object_destroy_region(target_index, i);
                    }
                }
                halo::objects::object_set_health_frozen_flag(target_index);
                *notify_flags |= 1;
            }
        } else if (absolute < geometry->body_damaged_threshold && (*vitality_flags & 1) == 0) {
            halo::effects::effect_new_on_object(target_index, halo::objects::tag_handle(geometry->body_damaged_effect), target_index, -1, 0.0f, 0.0f, 0, 0);
            *vitality_flags |= 1;
        }
    }
    if (test_flag(dd->flags, damage_data_flag::localized_damage) && halo::objects::tag_handle(geometry->localized_damage_effect) != k_datum_index_none) {
        halo::objects::damage_effect_new_at_location(halo::objects::tag_handle(geometry->localized_damage_effect), (int16_t)node_index, &dd->direction,
            (real_vector3d *)plane, &dd->origin, target_index);
    }
    if (test_flag(dd->flags, damage_data_flag::area_damage) && body > geometry->area_damage_effect_threshold &&
        halo::objects::tag_handle(geometry->area_damage_effect) != k_datum_index_none && effect_block->damage_category != 7) {
        halo::effects::effect_new_on_object(target_index, halo::objects::tag_handle(geometry->area_damage_effect), target_index, -1, 0.0f, 0.0f, 0, 0);
    }
    *body_damage_out = body;
    *material_multiplier_out = material->body_damage_multiplier;
}

/**
 * Applies shield damage to a target object and reports the shield damage done and the damage left over.
 *
 * Original register convention: EBX -> record, stack=(target_index, geometry, material, effect_block, notify_flags,
 * shield_damage_out, remaining_damage, is_local, apply_state).
 *
 * @address 0x004ef820
 */
void halo::objects::ObjectDamage::apply_shield_damage(ModelCollisionGeometry *geometry, ModelCollisionGeometryMaterial *material, DamageEffect *effect_block,
    uint32_t *notify_flags, float *shield_damage_out, float *remaining_damage, uint8_t is_local, uint8_t apply_state,
    object_shield_impulse_result *record)
{
    uint32_t target_index = handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(target_index));
    float *shield = &obj->base.shield_vitality;
    uint16_t *vitality_flags = &obj->base.vitality_flags;
    float passthrough = *remaining_damage;
    float to_shield = *remaining_damage;
    float maximum;
    float inverse_maximum;
    uint8_t negligible = 0;
    uint8_t unscaled = 0;

    record->shield_damage_dealt = 0.0f;
    record->depleted_this_call = 0;
    if (halo::game::globals().current_engine == 0 && effect_block->damage_category == 1 && obj->base.owner_team == 1) {
        unscaled = 1;
    }
    if (!(*shield > 0.0f)) {
        to_shield = 0.0f;
        if (is_local == 1) {
            *shield = 0.0f;
        }
    } else {
        maximum = obj->base.maximum_shield_vitality;
        if (!unscaled) {
            maximum = halo::game::weapon_get_zoom_fov_resolved(2, obj->base.owner_team) * maximum;
        }
        inverse_maximum = (maximum > 0.0f) ? 1.0f / maximum : 0.0f;
        if ((*notify_flags & 0x10) == 0 || (geometry->flags & 4) == 0) {
            float threshold = geometry->shield_failure_threshold;

            to_shield = (1.0f - material->shield_leak_percentage) * passthrough;
            if (*shield <= threshold && threshold > 0.0f) {
                real t = halo::math::transition_function_evaluate(geometry->shield_failure_function, *shield / threshold);
                float leak = geometry->failing_shield_leak_fraction;

                to_shield = ((1.0f - leak) * t + leak) * to_shield;
            }
        }
        if (*vitality_flags & halo::to_bits(vitality_flag::shield_recharging)) {
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
                real multiplier = halo::game::weapon_get_zoom_fov(0, halo::main::globals().game_globals->difficulty);

                if (multiplier > 0.0f) {
                    to_shield = to_shield / multiplier;
                }
            }
            scaled = to_shield * material->shield_damage_multiplier *
                material_damage_multiplier(*effect_block, geometry->shield_material_type);
            if (scaled < 0.0001f) {
                negligible = 1;
            }
            dealt = inverse_maximum * scaled;
            if (dealt > *shield || effect_block->damage_side_effect == damageeffectsideeffect_emp) {
                float overflow = scaled - maximum * *shield;

                if (overflow > 0.0f) {
                    passthrough = overflow + passthrough;
                }
                if (is_local == 1) {
                    *shield = 0.0f;
                }
                if ((*vitality_flags & 8) == 0 && apply_state == 1) {
                    halo::objects::object_set_shield_depleted_flag(target_index);
                    *notify_flags |= 8;
                    record->depleted_this_call = 1;
                }
            } else {
                if (is_local == 1 && (*vitality_flags & 0x800) == 0) {
                    *shield = *shield - dealt;
                }
                if ((*vitality_flags & 2) == 0 && *shield < geometry->shield_damaged_threshold) {
                    halo::objects::object_dispatch_effect_notify(target_index, halo::objects::tag_handle(geometry->shield_damaged_effect));
                    *vitality_flags |= 2;
                }
            }
        }
        if (!negligible && apply_state == 1) {
            float fraction = (*remaining_damage - passthrough) * inverse_maximum;
            float recent;

            obj->base.shield_damage_ticks = 0;
            if ((*vitality_flags & 8) == 0) {
                obj->base.current_shield_damage = 1.0f;
            }
            recent = fraction + obj->base.recent_shield_damage;
            obj->base.recent_shield_damage = recent;
            if (obj->base.current_shield_damage > 1.0f) {
                obj->base.current_shield_damage = 1.0f;
            }
            if (recent > 1.0f) {
                obj->base.recent_shield_damage = 1.0f;
            }
            record->shield_damage_dealt = fraction;
        }
    }
    if (is_local == 1) {
        if (!(to_shield < geometry->minimum_stun_damage) || *shield == 0.0f) {
            obj->base.shield_stun_ticks = (int16_t)(int32_t)(geometry->stun_time * halo::game::k_ticks_per_second_f);
        }
    }
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
        block.looked_up = halo::objects::hash_table_get(&object_network_id_table->id_to_index, key);
    }
    block.source1 = source[1];
    block.source0 = source[0];
    block.original_param_1 = param_1;
    block.source2 = source[2];

    {
        void *items[1];
        items[0] = &block;
        halo::networking::network_session_broadcast_to_flagged(halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 0x31, 0,
                                             items, 0, 1, 0), halo::networking::globals().server, 1, network_message_scratch, 0, 0, 0, 3);
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
    struct {
        int32_t network_id;
        float impulse_scale;
        float direction_i;
        float direction_j;
        float direction_k;
    } decoded;

    if (halo::networking::delta_context(message)->state->incremental != 0) {
        halo::networking::message_delta_decode_compound_field_staged(message);
        return;
    }

    if (halo::networking::message_delta_decode_compound_field(message, &decoded) != 0 && decoded.network_id != 0 &&
        object_network_id_table->handles[decoded.network_id] != k_datum_index_none) {
        real_vector3d impulse;

        impulse.i = decoded.direction_i * decoded.impulse_scale;
        impulse.j = decoded.direction_j * decoded.impulse_scale;
        impulse.k = decoded.direction_k * decoded.impulse_scale;
        halo::items::item_accelerate(object_network_id_table->handles[decoded.network_id], &impulse, 0);
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
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(target_index));
    Object *object_tag = halo::objects::tag_as<Object>(obj->base.definition_tag);
    DamageEffect *effect = halo::objects::tag_as<DamageEffect>(dd->damage_effect_tag);
    int16_t type;

    if (object_tag->acceleration_scale > 0.0001f) {
        real_vector3d direction;
        real_vector3d impulse;
        float scale;

        direction = dd->direction;
        direction.k = direction.k + 0.45f;
        halo::math::vector3d_normalize_with_length(direction);
        type = obj->base.type;
        scale = object_tag->acceleration_scale * effect->damage_instantaneous_acceleration.i * halo::math::k_seconds_per_tick;
        impulse.i = direction.i * scale;
        impulse.j = direction.j * scale;
        impulse.k = scale * direction.k;
        switch (type) {
        case _object_type_biped:
        case _object_type_vehicle:
            if (effect->damage_instantaneous_acceleration.i > 0.0001f && (obj->unit.flags & halo::to_bits(units::unit_flag::impervious)) == 0) {
                if (type == _object_type_biped) {
                    halo::units::unit_apply_impulse(target_index, &impulse);
                } else {
                    if (effect->damage_flags & 0x20) {
                        impulse.i = impulse.i + impulse.i;
                        impulse.j = impulse.j + impulse.j;
                        impulse.k = impulse.k + impulse.k;
                    }
                    if (obj->base.network_role != 1 || halo::units::unit_any_flagged_seat_occupied(target_index) == 1) {
                        halo::units::unit_apply_impulse_to_seat(target_index, &impulse);
                    }
                }
            }
            break;
        case _object_type_weapon:
        case _object_type_equipment:
        case _object_type_garbage: {
            uint8_t significant = 0;
            int32_t role;

            if (!(impulse.k * impulse.k + impulse.i * impulse.i + impulse.j * impulse.j < 0.0001f) ||
                (reinterpret_cast<item_object *>(obj)->item.flags & _item_at_rest_on_structure_bit) == 0) {
                significant = 1;
            }
            if (obj->base.network_role == 0 && significant == 1) {
                halo::objects::object_queue_pickup_denied_event(*(void **)&scale, (int32_t)target_index, (uint32_t *)&direction);
            }
            role = obj->base.network_role;
            if (role == 0 || role == 3 || !significant) {
                item_accelerate__as_object_damage_notify_and_impulse(target_index, &impulse,
                    (uint8_t)((dd->random_blend > 0.5f && (effect->damage_flags & 0x20)) ? 1 : 0));
            }
            break;
        }
        case _object_type_projectile:
            halo::objects::object_apply_impulse_and_spin(target_index, &impulse);
            break;
        default:
            break;
        }
    }
    if ((uint8_t)is_local == 1 && (halo::game::globals().current_engine == 0 || halo::game::globals().state == 0)) {
        if (!test_flag(dd->flags, damage_data_flag::suppress_death_notification)) {
            if (notify_flags & 1) {
                halo::game::game_engine_attribute_player_death(target_index, dd->responsible_player, dd->responsible_object,
                    (int32_t)(uint16_t)dd->team_index, 1);
            }
        } else {
            datum_index player_index = halo::game::player_index_from_unit_index(target_index);

            halo::game::game_engine_on_player_death(player_index, target_index, player_index, 1);
        }
    }
    if ((1u << ((uint8_t)obj->base.type & 0x1f)) & _object_mask_unit) {
        halo::units::unit_apply_damage_effects(target_index, dd, notify_flags, shield_damage, body_damage, region_index,
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

        halo::objects::object_get_position(&object_position, object_index);
        away.i = impact_position->x - object_position.x;
        away.j = impact_position->y - object_position.y;
        away.k = impact_position->z - object_position.z;
        if (halo::math::vector3d_normalize_with_length(away) == 0.0f) {
            away = *(real_vector3d *)(halo::objects::object_record_bytes(object_index) + 0x74);
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
    if (object_index != k_datum_index_none && node_index != -1) {
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
    object *obj = headers[halo::datum_slot(object_index)].data;
    Object *definition = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if (definition->collision_model.tag_id.index != halo::k_word_none) {
        if ((obj->destroyed_region_flags & (1 << (region_index & 0x1f))) == 0) {
            ModelCollisionGeometry *geometry =
                (ModelCollisionGeometry *)halo::cache::globals().tag_instances[definition->collision_model.tag_id.index].data;
            ModelCollisionGeometryRegion *region = &((ModelCollisionGeometryRegion *)geometry->regions.pointer)[region_index];

            halo::effects::effect_new_on_object(object_index, halo::objects::tag_handle(((struct ModelCollisionGeometryRegion *)region)->destroyed_effect), object_index, -1,
                0.0f, 0.0f, 0, 0);
            halo::objects::object_set_permutation_by_name(object_index, "~damaged", (int16_t)region_index, 1);

            if (test_flag(region->flags, tags::model_collision_geometry_region_tag_flag::inhibits_melee_attack)) {
                obj->vitality_flags |= _object_region_response_80_bit;
            }
            if (test_flag(region->flags, tags::model_collision_geometry_region_tag_flag::inhibits_weapon_attack)) {
                obj->vitality_flags |= _object_region_response_100_bit;
            }
            if (test_flag(region->flags, tags::model_collision_geometry_region_tag_flag::inhibits_walking)) {
                set_flag(obj->vitality_flags, vitality_flag::region_response_200);
            }
            if (test_flag(region->flags, tags::model_collision_geometry_region_tag_flag::forces_drop_weapon)) {
                set_flag(obj->vitality_flags, vitality_flag::region_response_400);
            }
            if (test_flag(region->flags, tags::model_collision_geometry_region_tag_flag::forces_object_to_die)) {
                halo::objects::object_set_health_frozen_flag(object_index);
            }

            obj->destroyed_region_flags |= (uint16_t)(1 << (region_index & 0x1f));
            halo::objects::object_type_definitions_notify_region_damage(object_index, (uint32_t)region_index, region->flags);

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
            table->active[group][i] = k_datum_index_none;
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
        uint32_t word = halo::physics::globals().breakable_surface_state->active[halo::scenario::globals().structure_bsp_index][bit_index >> 5];
        if ((word & (1 << (bit_index & 0x1f))) == 0) {
            return 0;
        }
    }
    return 1;
}
