#include "halo/units/seat_detach.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/game/records.hpp"
#include "halo/units/records.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/hs/script_globals.hpp"
#include <string.h>
#include "halo/models/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/core/lcg.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "physics.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &is_dedicated_server_flag = halo::link::ref<uint8_t>(halo::units::vars().is_dedicated_server_flag);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &network_object_index_cache = halo::link::ref<uint8_t []>(halo::units::vars().network_object_index_cache);
static auto &unit_updates_suppressed = halo::link::ref<uint8_t>(halo::units::vars().unit_updates_suppressed);
static auto &global_origin3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &team_pair_data = halo::link::ref<uint8_t *>(halo::ai::vars().team_pair_data);

namespace halo::units {

namespace unit_apply_damage_effects_local {

}

/**
 * Engine function unit_apply_damage_effects.
 *
 * @address 0x5674a0
 */
void UnitView::apply_damage_effects(damage_data *dd, uint32_t flags, float shield_damage, float body_damage, int32_t region_index, uint8_t is_local)
{
    using namespace unit_apply_damage_effects_local;
    datum_index unit_index = datum_handle;
    float total = shield_damage + body_damage;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)obj);
    uint8_t *effect_block = halo::objects::tag_record_bytes(dd->damage_effect_tag) + 0x1c4;
    uint8_t killed = (uint8_t)(flags & 1);
    uint8_t knocked_down = 0;
    uint8_t violent = 0;
    uint32_t unit_flags;
    unit_state_change_record record;

    memset(&record, 0, sizeof(record));
    if (is_local == 1) {
        float recent = obj->base.recent_body_damage + obj->base.recent_shield_damage;

        if (recent > 0.0f) {
            obj->unit.delayed_damage_category = *(int16_t *)(effect_block + 0x2);
            obj->unit.delayed_damage_ticks = 0x2d;
            if (recent < obj->unit.delayed_damage_amount) {
                recent = obj->unit.delayed_damage_amount;
            }
            obj->unit.delayed_damage_amount = recent;
            if (dd->responsible_object != k_datum_index_none) {
                obj->unit.delayed_damage_responsible_object = dd->responsible_object;
            }
        }
    }
    unit_flags = obj->unit.flags;
    if (unit_flags & 0x10) {
        float left = obj->unit.active_camouflage_power - *(float *)(effect_block + 0x1c);

        obj->unit.active_camouflage_power = left;
        if (left < 0.0f) {
            obj->unit.active_camouflage_power = 0.0f;
        }
    }
    if (is_local == 1) {
        violent = (uint8_t)(killed && !(*(float *)(effect_block + 0x30) < 2.0f));
        if (!killed && (test_flag(unit_flags, units::unit_flag::unknown_2000)) && unit_tag->feign_death_threshold > 0.0f &&
            unit_tag->feign_death_time > 0.0f && obj->base.body_vitality > 0.0f &&
            obj->base.recent_body_damage > unit_tag->feign_death_threshold) {
            float ticks = (halo::math::random_real_range(0.0f, 1.0f) + unit_tag->feign_death_time) * 30.0f;

            set_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen);
            knocked_down = 1;
            if (1.0f > ticks) {
                ticks = 1.0f;
            }
            obj->unit.feign_death_ticks = (int16_t)(int32_t)ticks;
        }
    }

    bool skip_record_check = false;
    if (obj->unit.controlling_player == k_datum_index_none && killed && (effect_block[0x4] & 0x80) &&
        (test_flag(unit_tag->unit_flags, tags::unit_tag_flag::runs_around_flaming))) {
        const bool enter_stunned = [&]() -> bool {
            unit_object *self;
            datum_index vehicle_index;

            if (obj->base.parent_object == k_datum_index_none) {
                return true;
            }
            self = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(unit_index, 3));
            if (self == 0 || halo::networking::globals().game_mode == 1 ||
                (vehicle_index = self->base.parent_object) == k_datum_index_none ||
                self->unit.vehicle_seat_index == -1) {
                return false;
            }
            if (self->base.type == 1) {
                unit_object *me = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));

                if (me->base.parent_object != k_datum_index_none && me->unit.vehicle_seat_index != -1) {
                    biped_detach_from_seat(unit_index, me->base.parent_object);
                }
                biped_free_local_player_history(me);
                return false;
            }
            if (::halo::units::unit_state_is_scripted_animation((unit_data *)(reinterpret_cast<uint8_t *>(self) + k_unit_data_offset))) {
                return false;
            }
            {
                Unit *self_tag = halo::objects::tag_as<Unit>(*(datum_index *)self);
                datum_index graph = halo::objects::tag_handle(self_tag->base.animation_graph);
                uint8_t *seat_block = *(uint8_t **)(halo::objects::tag_record_bytes(graph) + 0x10) + (int8_t)(uint8_t)self->unit.animation_definition_index * 0x64;
                int16_t death_animation;
                uint8_t *object;
                Object *object_tag;

                if (*(int32_t *)(seat_block + 0x40) <= 8) {
                    return false;
                }
                death_animation = (*(int16_t **)(seat_block + 0x44))[8];
                if (death_animation == -1) {
                    return false;
                }
                if (((struct unit_object *)halo::objects::object_record_bytes(vehicle_index))->unit.driver_unit_index == unit_index) {
                    UnitView((int32_t)vehicle_index).notify_weapon_removed();
                }
                UnitView(unit_index).set_custom_animation(halo::objects::tag_handle(self_tag->base.animation_graph), halo::models::animation_choose_random_permutation(graph, death_animation, (animation_random_stream)1));
                object = halo::objects::object_record_bytes(unit_index);
                object_tag = halo::objects::tag_as<Object>(*(datum_index *)object);
                if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1 && test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
                    halo::objects::object_for_each_light_attachment(unit_index, 0, 1);
                }
                if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
                    clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                    halo::objects::object_header_of(unit_index).flags |= 2;
                }
                self->unit.animation_state = animation_state_value(unit_animation_state_id::seat_exit);
                halo::ai::actor_notify_weapon_pickup_once(unit_index);
                if (self->base.network_role == 0) {
                    ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
                }
            }
            return true;
        }();
        if (enter_stunned) {
            if (is_local == 1) {
                UnitView(unit_index).enter_stunned_state(dd->responsible_object);
            }
            killed = 0;
            skip_record_check = true;
        }
    }

    if (!skip_record_check) {
        if ((dd->flags & 0x10) == 0 && (killed || knocked_down || !test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen)) &&
            !test_flag(obj->unit.flags, units::unit_flag::unknown_800000) && (*(uint32_t *)(effect_block + 0x4) & 0x10) == 0) {
            uint32_t effect_flags = *(uint32_t *)(effect_block + 0x4);
            real_vector2d direction;
            real_vector2d forward;
            uint8_t stunned_flag = 0;
            uint8_t special = 0;
            uint8_t has_direction = 0;
            float angle = 0.0f;

            direction.i = dd->direction.i;
            direction.j = dd->direction.j;
            forward.i = obj->base.forward.i;
            forward.j = obj->base.forward.j;
            if (halo::math::vector2d_normalize_with_length(direction) > 0.0f && halo::math::vector2d_normalize_with_length(forward) > 0.0f) {
                angle = halo::math::vector2d_angle_between(forward, direction);
                has_direction = 1;
            }
            if (((uint8_t)unit_tag->unit_flags & 0x80) && (effect_flags & 4) == 0) {
                stunned_flag = 1;
            }
            if ((uint8_t)obj->unit.flaming_ticks != 0) {
                stunned_flag = 1;
            }
            if (flags & 0x8a) {
                special = 1;
            }
            record.valid = 1;
            record.killed = killed;
            record.knocked_down = knocked_down;
            record.violent = violent;
            record.stunned = stunned_flag;
            record.special = special;
            record.region_index = (int16_t)region_index;
            record.angle = angle;
            if (has_direction == 1) {
                record.no_direction = 0;
                record.direction = direction;
            } else {
                record.no_direction = 1;
            }
            record.player_value = 0;
            if (obj->unit.controlling_player != k_datum_index_none) {
                uint8_t *player = (uint8_t *)halo::memory::datum_get(obj->unit.controlling_player, halo::game::globals().player_data);

                if (player != 0) {
                    record.player_value = *(uint32_t *)&((struct player *)player)->respawn_timer;
                }
            }
            UnitView(unit_index).update_stance_and_jump(killed, knocked_down, violent, stunned_flag, special, angle, (int16_t)region_index, has_direction ? &direction : 0, is_local);
        } else {
            record.valid = 0;
        }
    }

    if (is_local == 1) {
        datum_index unit_player = obj->unit.controlling_player;

        if (dd->responsible_player != k_datum_index_none && unit_player != k_datum_index_none &&
            halo::game::globals().current_engine != 0 && halo::game::globals().current_engine->unknown_64 != 0) {
            ((void (*)(datum_index, datum_index, uint32_t))halo::game::globals().current_engine->unknown_64)(
                dd->responsible_player, unit_player, (flags >> 4) & 0xffffff01);
        }
        if (dd->responsible_player != k_datum_index_none || dd->responsible_object != k_datum_index_none) {
            UnitView(unit_index).record_recent_damage_and_react(total, (int16_t)*(uint16_t *)(effect_block + 0x2), killed, dd->responsible_player, (int16_t)(uint16_t)dd->team_index, dd->responsible_object);
        }
        if ((dd->flags & 0x10) == 0 && ((flags & 1) || body_damage > 0.0f || shield_damage > 0.0f)) {
            UnitView(unit_index).choose_combat_reaction_animation((const datum_index *)dd, (uint8_t)(knocked_down | killed), (uint8_t)((flags >> 6) & 0xffffff01), body_damage);
        }
    }
    if (body_damage > 0.0f || shield_damage > 0.0f) {
        UnitView(unit_index).validate_and_clear_weapon_switch();
    }
    if (is_local == 1 && obj->base.type == 0) {
        if (killed) {
            halo::ai::actor_reassign_vehicle_seat(dd->responsible_object, unit_index, *(uint16_t *)(effect_block + 0x2));
        } else if (!test_flag(obj->base.vitality_flags, objects::vitality_flag::health_frozen)) {
            halo::ai::actor_react_to_threat_event(unit_index, dd->responsible_object, *(uint16_t *)(effect_block + 0x2), total,
                (uint32_t)(uintptr_t)&dd->direction, 0);
        }
    }

    if (obj->unit.controlling_player != k_datum_index_none && *(float *)(effect_block + 0x20) > 0.0f &&
        (halo::game::globals().current_engine != 0 || is_dedicated_server_flag)) {
        GlobalsPlayerInformation *shake = halo::objects::block_elements<GlobalsPlayerInformation>(global_globals->player_information);
        float step = dd->random_blend * *(float *)(effect_block + 0x20);
        float cap = *(float *)(effect_block + 0x24) * dd->random_blend;
        int16_t add;
        int16_t low;
        int16_t high;

        if (step < 0.0f) {
            step = 0.0f;
        }
        if (cap < 0.0f) {
            cap = 0.0f;
        } else if (!(cap < 1.0f)) {
            cap = 1.0f;
        }
        if (cap > obj->unit.stun) {
            float value = step + obj->unit.stun;

            obj->unit.stun = value;
            if (value > cap) {
                obj->unit.stun = cap;
            }
        }
        add = (int16_t)(int32_t)(*(float *)(effect_block + 0x28) * 30.0f);
        low = (int16_t)(int32_t)(shake->minimum_stun_time * 30.0f);
        high = (int16_t)(int32_t)(shake->maximum_stun_time * 30.0f);
        if (obj->unit.stun_ticks < low) {
            obj->unit.stun_ticks = low;
        }
        obj->unit.stun_ticks += add;
        if (obj->unit.stun_ticks > high) {
            obj->unit.stun_ticks = high;
        }
    }

    if (is_local == 1 && (killed || knocked_down)) {
        UnitView(unit_index).release_transient_state(knocked_down);
        if (obj->base.network_role == 0 && killed == 1) {
            record.unit = unit_index;
            ::halo::units::unit_broadcast_state_change_event(record);
            if ((halo::objects::object_header_of(unit_index).flags & 8) == 0) {
                halo::networking::network_index_cache_remove(network_object_index_cache, (int32_t)unit_index);
            }
            obj->base.network_role = 3;
        }
    }
}

/**
 * Applies scaled fall damage to a unit when its downward velocity (param_2, positive) exceeds the tag-defined
 * safe threshold, unless the unit is exempt (unattended and the Biped tag's own exemption bit is set, or
 * updates are suppressed, or it's a non-local-player unit while the multiplayer-fall-damage toggle is off).
 * Below the harmful threshold, applies the instant "out of bounds" damage effect (and deletes the unit if
 * it's still marked for deferred delete);
 *
 * @address 0x55e4f0
 */
void UnitView::apply_fall_damage(float fall_speed)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    GlobalsFallingDamage *fall_table = halo::objects::block_elements<GlobalsFallingDamage>(global_globals->falling_damage);
    uint32_t exempt;

    exempt = (!test_flag(unit->flags, units::unit_flag::unknown_1000) && (int8_t)tag->biped_flags >= 0) ? 0 : 1;
    if (unit_updates_suppressed != 0) {
        exempt = 1;
    }

    if (halo::hs::fields::jetpack == 0 || unit->controlling_player == k_datum_index_none) {
        if (fall_speed <= fall_table->harmful_falling_velocity[0]) {
            if (!test_flag(tag->biped_flags, tags::biped_tag_flag::flying) && obj->velocity.k < -fall_table->maximum_falling_velocity) {
                if (!exempt && !test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen)) {
                    damage_data dd;
                    halo::objects::damage_data_initialize(&dd, halo::objects::tag_handle(fall_table->distance_damage));
                    halo::objects::object_apply_damage(&dd, object_index, -1, -1, -1, 0);
                }
                if (halo::game::globals().current_engine == 0 && test_flag(obj->flags, objects::object_flag::outside_map)) {
                    if (halo::game::player_index_from_unit_index(object_index) == -1) {
                        halo::objects::object_delete(object_index);
                    }
                }
            }
        } else if (!exempt) {
            damage_data dd = {0};
            float harmless = fall_table->harmful_falling_velocity[0];
            float harmful = fall_table->harmful_falling_velocity[1];
            float blend = (fall_speed - harmless) / (harmful - harmless);

            dd.damage_effect_tag = halo::objects::tag_handle(fall_table->falling_damage);
            dd.responsible_player = k_datum_index_none;
            dd.responsible_object = k_datum_index_none;
            dd.team_index = -1;
            dd.location_cluster_index = -1;
            dd.material_type = -1;
            dd.multiplier = 1.0f;
            dd.random_blend = (blend < 0.0f) ? 0.0f : (blend > 1.0f ? 1.0f : blend);

            halo::objects::object_apply_damage(&dd, object_index, -1, -1, -1, 0);
        }
    }
}

/**
 * Performs the unit's melee attack: locates the "melee" marker (falling back to the unit's bounding center,
 * and re-checking line of sight from the center to the marker), resolves the melee damage effect (the Unit
 * tag's default, or the current weapon's own response effect when its ais_use_weapon_melee_damage flag is
 * set), and applies it either to a specific target object or as an area effect at the impact point. Clears
 * melee_state either way.
 *
 * @address 0x56f2d0
 */
void UnitView::cause_melee_damage(uint8_t suppress_effect, uint32_t target_object_index, int16_t damage_param4, int16_t damage_param5, int16_t damage_param6, uint32_t damage_param7)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    real_point3d origin_pos = obj->bounding_center;
    real_point3d target_pos;
    object_marker melee_marker;
    int16_t found;
    datum_index damage_effect;
    damage_data dd;

    if ((int32_t)halo::objects::tag_handle(tag->melee_damage) == -1) {
        unit_data *unit0 = halo::units::unit_data_of(obj);
        unit0->melee_state = 0;
        return;
    }

    found = halo::objects::object_get_node_local_transform(unit_index, (char *)"melee", &melee_marker, 1);
    if (found == 1) {
        real_vector3d delta;
        uint8_t scratch[0x54];

        target_pos = melee_marker.node_transform.position;
        delta.i = target_pos.x - origin_pos.x;
        delta.j = target_pos.y - origin_pos.y;
        delta.k = target_pos.z - origin_pos.z;

        if (halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::ignore_invisible | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::water_surface | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::unstick), &origin_pos, &delta, k_datum_index_none, (collision_result *)scratch) != 0) {
            target_pos = origin_pos;
        }
    } else {
        target_pos = origin_pos;
    }

    obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    {
        unit_data *unit = halo::units::unit_data_of(obj);
        damage_effect = halo::objects::tag_handle(tag->melee_damage);

        if (unit->current_weapon_index != -1) {
            datum_index weapon_index = unit->weapons[unit->current_weapon_index];
            if (weapon_index != k_datum_index_none) {
                object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
                Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
                if (test_flag(weapon_tag->weapon_flags, tags::weapon_tag_flag::ais_use_weapon_melee_damage)) {
                    damage_effect = halo::objects::tag_handle(weapon_tag->player_melee_response);
                }
            }
        }

        dd.damage_effect_tag = damage_effect;
        dd.responsible_player = unit->controlling_player;
        dd.responsible_object = unit_index;
        dd.team_index = obj->owner_team;
        dd.location_leaf_index = obj->location_leaf_index;
        *(int32_t *)&dd.location_cluster_index = *(int32_t *)&obj->location_cluster_index;
        dd.epicentre = target_pos;
        dd.origin = origin_pos;
        dd.random_blend = 1.0f;
        dd.multiplier = 1.0f;
        dd.material_type = -1;

        if (target_object_index == k_datum_index_none) {
            halo::objects::damage_apply_area_effect(&dd);
        } else {
            halo::objects::object_apply_damage(&dd, target_object_index, damage_param4, damage_param5, damage_param6, damage_param7);
        }

        if (suppress_effect == 0 && dd.material_type != -1) {
            ::halo::units::unit_trigger_material_hit_effect(dd.material_type, damage_effect, unit_index);
        }

        unit->melee_state = 0;
    }
}

/**
 * Puts the unit into a disoriented/stunned state: drops its current weapon, sets the disoriented flag and
 * adjusts vitality flags, and -- if not already stunned -- picks a random stun duration, records the
 * responsible object, and kicks off the random idle-turn wander.
 *
 * @address 0x5705a0
 */
void UnitView::enter_stunned_state(uint32_t responsible_object)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    UnitView(unit_index).drop_current_weapon(1);
    unit->flags |= _unit_flag_disoriented;
    obj->vitality_flags = (obj->vitality_flags & 0xfffb) | 0x800;

    if (unit->flaming_ticks == 0) {
        int16_t duration;

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        duration = (int16_t)(((int32_t)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * 0x5a) >> 0x10) + 0x3c;

        if (duration == 0) {
            duration = 1;
        } else if (duration > 0xff) {
            duration = 0xff;
        }

        unit->flaming_ticks = (int8_t)duration;
        unit->flaming_responsible_object = responsible_object;
        UnitView(unit_index).initialize_random_turn_angle();
    }
}

/**
 * Engine function unit_melee_lunge_damage_tick.
 *
 * @address 0x56fc80
 */
void UnitView::melee_lunge_damage_tick()
{
    uint32_t unit_index = datum_handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    Unit *tag = halo::objects::tag_as<Unit>(*(datum_index *)obj);
    datum_index target = obj->base.parent_object;
    uint8_t hit = 0;
    real_plane3d plane;
    real_point3d start;
    real_point3d hit_point;
    object_collision_context context;
    object_node_collision_result record;
    damage_data dd;

    if ((uint8_t)obj->unit.melee_state != 4 || target == k_datum_index_none || halo::objects::tag_handle(tag->melee_damage) == k_datum_index_none) {
        return;
    }
    if ((uint8_t)obj->unit.melee_damage_countdown == 0 && halo::physics::object_collision_context_build(target, &context)) {
        halo::objects::object_get_position(&start, unit_index);
        plane.normal.i = obj->base.forward.i * 0.2f;
        plane.normal.j = obj->base.forward.j * 0.2f;
        plane.normal.k = obj->base.forward.k * 0.2f;
        start.x -= plane.normal.i * 0.5f;
        start.y -= plane.normal.j * 0.5f;
        start.z -= plane.normal.k * 0.5f;
        if (halo::physics::object_collision_context_test_segment(&context, 3, &start, &plane.normal, &record)) {
            float fraction = *(float *)((uint8_t *)&record + 0x08);

            hit_point.x = plane.normal.i * fraction + start.x;
            hit_point.y = plane.normal.j * fraction + start.y;
            hit_point.z = plane.normal.k * fraction + start.z;
            halo::math::matrix4x3_transform_plane(plane,
                *(real_matrix4x3 *)((uint8_t *)context.nodes + record.node_index * 0x34),
                *(*(real_plane3d **)((uint8_t *)&record + 0x0c)));
            if (*(int32_t *)((uint8_t *)&record + 0x14) < 0) {
                plane.normal.i = -plane.normal.i;
                plane.normal.j = -plane.normal.j;
                plane.normal.k = -plane.normal.k;
                plane.d = -plane.d;
            }
            hit = 1;
        }
    }
    memset(&dd, 0, sizeof(dd));
    dd.damage_effect_tag = halo::objects::tag_handle(tag->melee_damage);
    dd.material_type = -1;
    dd.location_cluster_index = -1;
    dd.multiplier = 1.0f;
    dd.responsible_object = unit_index;
    dd.team_index = obj->base.owner_team;
    dd.responsible_player = obj->unit.controlling_player;
    dd.random_blend = 0.033333335f;
    if (hit) {
        dd.epicentre = hit_point;
        dd.origin = hit_point;
        dd.direction = *(real_vector3d *)&obj->base.forward.i;
        dd.flags |= 2;
        obj->unit.melee_damage_countdown = 10;
        halo::objects::object_apply_damage(&dd, obj->base.parent_object, record.node_index, record.region_index,
            *(int16_t *)((uint8_t *)&record + 0x1a), (uint32_t)(uintptr_t)&plane);
    } else {
        halo::objects::object_apply_damage(&dd, obj->base.parent_object, -1, -1, -1, 0);
    }
    obj->unit.melee_damage_countdown--;
}

/**
 * Engine function unit_process_melee_special_interaction.
 *
 * @address 0x56ff40
 */
void halo::units::unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index, uint32_t node_pair, uint32_t region_pair, uint32_t material, real_point3d *contact_point, real_plane3d *contact_plane, bsp_leaf_reference *contact_leaf)
{
    unit_object *attacker = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(attacker_index));
    uint32_t unit_flags = halo::objects::tag_as<Unit>(*(datum_index *)attacker)->unit_flags;
    object *target = reinterpret_cast<object *>(halo::objects::object_record_bytes(target_index));

    if ((unit_flags & 0x2000) && target->type == 0 && target->shield_vitality > 0.0f &&
        (test_flag(halo::objects::tag_as<Unit>(*(datum_index *)target)->unit_flags, tags::unit_tag_flag::shields_fry_infection_forms))) {
        UnitView(attacker_index).cause_melee_damage(1, target_index, (int16_t)node_pair, (int16_t)region_pair, (int16_t)material, (uint32_t)contact_plane);
        halo::objects::object_set_health_frozen_flag(attacker_index);
        halo::objects::object_delete(attacker_index);
        return;
    }
    if (!(test_flag(unit_flags, tags::unit_tag_flag::impact_melee_attaches_to_unit)) || !((1u << ((uint8_t)target->type & 0x1f)) & 3) || (test_flag(target->vitality_flags, objects::vitality_flag::health_frozen))) {
        return;
    }
    {
        datum_index parent = target->parent_object;

        while (parent != k_datum_index_none) {
            object *p = reinterpret_cast<object *>(halo::objects::object_record_bytes(parent));

            if (parent == attacker_index || p->type != 1) {
                return;
            }
            parent = p->parent_object;
        }
    }
    {
        real_vector3d *forward = (real_vector3d *)&attacker->base.forward;
        real_vector3d *up = (real_vector3d *)&attacker->base.up;
        real_vector3d left;

        *(real_vector3d *)&attacker->base.velocity.i = *global_origin3d_pointer;
        *(real_vector3d *)&attacker->base.angular_velocity.i = *global_origin3d_pointer;
        *forward = contact_plane->normal;
        forward->i = -forward->i;
        forward->j = -forward->j;
        forward->k = -forward->k;
        halo::math::vector3d_cross_product(left, *forward, *up);
        if (halo::math::vector3d_normalize_with_length(left) == 0.0f) {
            halo::math::vector3d_cross_product(left, *forward, *halo::math::globals().global_up3d_pointer);
            if (halo::math::vector3d_normalize_with_length(left) == 0.0f) {
                left = *halo::math::globals().global_forward3d_pointer;
            }
        }
        halo::math::vector3d_cross_product(*up, left, *forward);
    }
    halo::objects::object_set_position_and_relink(contact_point, attacker_index, contact_leaf);
    halo::objects::object_attach_to_object(target_index, attacker_index, (int16_t)node_pair);
    set_flag(attacker->base.flags, objects::object_flag::at_rest);
    set_flag(attacker->unit.flags, units::unit_flag::detached);
    UnitView(attacker_index).try_ready_weapon(1, 0);
}

/**
 * Records a recent damage or contact event into a small per-unit cache, used to avoid repeating an associated
 * response too often
 *
 * @address 0x568230
 */
void UnitView::record_recent_damage_and_react(float damage_amount, int16_t response_index, uint8_t allow_broadcast, uint32_t responsible_player, int16_t team_index, uint32_t responsible_object)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    int32_t current_tick = halo::game::globals().game_time->game_time;
    uint8_t merged = 0;

    unit_recent_damage *slot = unit->recent_damage;
    for (int32_t i = 4; i != 0; i--, slot++) {
        if (((responsible_player != k_datum_index_none) && (slot->responsible_player == responsible_player)) ||
            (slot->responsible_unit == responsible_object)) {
            slot->tick = current_tick;
            merged = 1;
            slot->damage += damage_amount;
        }
    }

    if (!merged) {
        int16_t empty = 0;
        while (unit->recent_damage[empty].tick != -1) {
            empty++;
            if (empty >= 4) {
                empty = -1;
                break;
            }
        }
        int16_t chosen;
        if (empty != -1) {
            chosen = empty;
        } else {
            int16_t oldest_by_damage = 0;
            for (int16_t i = 1; i < 4; i++) {
                if (unit->recent_damage[oldest_by_damage].damage < unit->recent_damage[i].damage) {
                    oldest_by_damage = i;
                }
            }
            int16_t oldest_by_tick = -1;
            for (int16_t i = 0; i < 4; i++) {
                if ((i != oldest_by_damage) &&
                    ((oldest_by_tick == -1) || ((uint32_t)unit->recent_damage[i].tick < (uint32_t)unit->recent_damage[oldest_by_tick].tick))) {
                    oldest_by_tick = i;
                }
            }
            chosen = oldest_by_tick;
        }
        unit->recent_damage[chosen].responsible_unit = responsible_object;
        unit->recent_damage[chosen].responsible_player = responsible_player;
        unit->recent_damage[chosen].damage = damage_amount;
        unit->recent_damage[chosen].tick = current_tick;
    }

    if (!allow_broadcast) {
        return;
    }
    if (team_index == -1) {
        return;
    }

    int16_t self_team = ((struct object *)unit_obj)->owner_team;
    uint8_t hostile;
    if (halo::game::globals().current_engine == 0) {
        if (!((self_team < 0) || (9 < self_team) || (team_index < 0) || (9 < team_index))) {
            int32_t bit_index = team_index + self_team * 10;
            hostile = 1 - ((*(uint32_t *)(team_pair_data + 0xa4 + (bit_index >> 5) * 4) & (1u << (bit_index & 0x1f))) != 0);
            if (!hostile) {
                return;
            }
        }
    } else {
        hostile = self_team != team_index;
        if (!hostile) {
            return;
        }
    }

    {
        uint8_t *attacker = 0;
        uint32_t attacker_handle = k_datum_index_none;

        if (responsible_player != k_datum_index_none) {
            uint32_t controlled_unit = halo::game::player_at(responsible_player)->unit;

            if (controlled_unit != k_datum_index_none) {
                attacker = halo::objects::object_record_bytes(controlled_unit);
                attacker_handle = controlled_unit;
            }
        }
        if (attacker == 0) {
            object_header *hdr = 0;

            attacker_handle = responsible_object;
            if ((responsible_object != k_datum_index_none) && (0 <= (int16_t)responsible_object) &&
                ((int16_t)responsible_object < halo::objects::globals().object_data->maximum_count)) {
                object_header *candidate = (object_header *)((uint8_t *)halo::objects::globals().object_data->data +
                                                             (int16_t)responsible_object * halo::objects::globals().object_data->size);

                if ((candidate->identifier != 0) &&
                    (((int16_t)(responsible_object >> 16) == 0) ||
                     (candidate->identifier == (int16_t)(responsible_object >> 16)))) {
                    hdr = candidate;
                }
            }
            if (hdr != 0 && ((1 << (((uint8_t *)hdr)[3] & 0x1f)) & 3) != 0) {
                attacker = (uint8_t *)hdr->data;
            }
            if (attacker == 0) {
                return;
            }
        }

        {
            uint32_t link = *(uint32_t *)(attacker + (response_index == 9 ? 0x324 : 0x328));

            if (link != k_datum_index_none) {
                attacker_handle = link;
                attacker = halo::objects::object_record_bytes(link);
            }
        }

        if ((attacker[0x106] & 4) == 0) {
            int32_t tick = halo::game::globals().game_time->game_time;
            int32_t last = *(int32_t *)(attacker + 0x42c);
            int16_t threshold;

            if (last == -1 || !(last + 0x78 >= tick)) {
                *(int16_t *)(attacker + 0x42a) = 0;
            }
            *(int16_t *)(attacker + 0x42a) = (int16_t)(*(int16_t *)(attacker + 0x42a) + 1);
            *(int32_t *)(attacker + 0x42c) = tick;
            threshold = (*(uint32_t *)(attacker + 0x218) != k_datum_index_none) ? 5 : 3;
            if (*(int16_t *)(attacker + 0x42a) >= threshold) {
                halo::ai::ai_communication_broadcast(1, (datum_index)attacker_handle, k_datum_index_none, -1, k_datum_index_none,
                                           k_datum_index_none, 0);
                *(int16_t *)(attacker + 0x42a) = 0;
            }
        }
    }
    return;
}

/**
 * Decays the unit's camera/weapon recoil offset toward zero each tick while its recoil countdown timer
 * (vehicle_data.unknown_4ce) is active: exponentially decays velocity and angular velocity, rotates the
 * object's forward/up vectors by the resulting angular-velocity axis-angle, snaps both velocities to zero
 * once the countdown expires, and reapplies the resulting orientation and offset position. FIXED (register
 * inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it);
 *
 * @address 0x574780
 */
void UnitView::update_recoil_decay()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    vehicle_data *vehicle = halo::units::vehicle_data_of(obj);
    real_vector3d forward = obj->forward;
    real_vector3d up = obj->up;
    real_point3d target_point;
    real_vector3d axis;
    real length;

    vehicle->decay_ticks_remaining -= 1;

    obj->velocity.i *= 0.835f;
    obj->velocity.j *= 0.835f;
    obj->velocity.k *= 0.835f;
    obj->angular_velocity.i *= 0.835f;
    obj->angular_velocity.j *= 0.835f;
    obj->angular_velocity.k *= 0.835f;

    axis = obj->angular_velocity;
    target_point.x = obj->velocity.i + obj->position.x;
    target_point.y = obj->velocity.j + obj->position.y;
    target_point.z = obj->velocity.k + obj->position.z;

    length = halo::math::vector3d_normalize_with_length(axis);
    if (length == 0.0f) {
        forward = obj->forward;
        up = obj->up;
    } else {
        real_matrix4x3 rotation;
        halo::math::matrix4x3_from_axis_angle(rotation, axis, (real)halo::libm::sin((double)length), (real)halo::libm::cos((double)length));
        halo::math::matrix4x3_transform_vector(forward, obj->forward, rotation);
        halo::math::matrix4x3_transform_vector(up, obj->up, rotation);
    }

    if (vehicle->decay_ticks_remaining == 0) {
        obj->velocity.i = 0.0f;
        obj->velocity.j = 0.0f;
        obj->velocity.k = 0.0f;
        obj->angular_velocity.i = 0.0f;
        obj->angular_velocity.j = 0.0f;
        obj->angular_velocity.k = 0.0f;
    }

    halo::objects::object_set_position_and_orientation(object_index, &forward, &up, &target_point);
}

/**
 * REWRITTEN from objdump 0x561b80..0x561cab: each fraction is the value over its maximum (maximum body +0xd8,
 * shield +0xdc), 1 when the value reaches the maximum, 0 when the maximum is not positive; a positive current
 * shield (+0xe4) or body (+0xe0) that the new fraction takes to zero (or below) first calls
 * object_set_shield_depleted_flag (EDI unit) / object_set_health_frozen_flag (EAX unit). The draft called
 * both with no unit and with the conditions inverted.
 *
 * Original register convention: see file header.
 *
 * @address 0x561b80
 */
void UnitView::update_vitality_fractions(float body_delta, float shield_delta)
{
    uint32_t unit_index = datum_handle;
    object *obj;
    float shield_fraction;
    float body_fraction;

    if (unit_index == (uint32_t)-1) {
        return;
    }
    obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    if ((obj->vitality_flags & _object_health_frozen_bit) != 0) {
        return;
    }
    if (!(obj->maximum_shield_vitality > 0.0f)) {
        shield_fraction = 0.0f;
    } else if (!(shield_delta < obj->maximum_shield_vitality)) {
        shield_fraction = 1.0f;
    } else {
        shield_fraction = shield_delta / obj->maximum_shield_vitality;
    }
    if (!(obj->maximum_body_vitality > 0.0f)) {
        body_fraction = 0.0f;
    } else if (!(body_delta < obj->maximum_body_vitality)) {
        body_fraction = 1.0f;
    } else {
        body_fraction = body_delta / obj->maximum_body_vitality;
    }

    if (obj->shield_vitality > 0.0f && !(shield_fraction > 0.0f)) {
        halo::objects::object_set_shield_depleted_flag(unit_index);
    }
    obj->shield_vitality = shield_fraction;
    if (obj->body_vitality > 0.0f && !(body_fraction > 0.0f)) {
        halo::objects::object_set_health_frozen_flag(unit_index);
    }
    obj->body_vitality = body_fraction;
}

}
