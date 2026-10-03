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
#include "halo/game/api.hpp"

extern "C" {
extern int16_t network_game_mode;
extern network_client_globals *network_client;
extern uint8_t is_dedicated_server_flag;
extern Globals *global_globals;
extern uint8_t network_object_index_cache[];
extern void actor_notify_weapon_pickup_once(datum_index object_index);
extern int32_t actor_reassign_vehicle_seat(datum_index vehicle_object_index, datum_index self_object_index, int32_t seat_selector);
extern void actor_react_to_threat_event(datum_index self_object_index, datum_index other_object_index, int32_t event_kind, real magnitude, uint32_t extra_param, uint8_t suppress_vehicle_relay);
extern uint8_t network_index_cache_remove(uint8_t *container, int32_t key);
extern void player_update_history_free_all(void *history);
extern uint8_t unit_updates_suppressed;
extern uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context);
extern uint8_t object_collision_context_test_segment(object_collision_context *context, uint32_t flags, real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result);
extern real_vector3d *global_origin3d_pointer;
extern uint8_t *team_pair_data;
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
extern double sin(double x);
extern double cos(double x);
}

namespace halo::units {

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot((h))].data)
#define OBJECT_HEADER(h) (((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot((h))])
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot((t))].data)
namespace unit_apply_damage_effects_local {

static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = OBJECT_DATA(object_index);
    uint8_t *vehicle = OBJECT_DATA(vehicle_index);
    uint8_t *nodes = self + ((unit_object *)self)->base.nodes.offset;
    uint8_t *seat = (uint8_t *)((struct Unit *)TAG_DATA(*(datum_index *)vehicle))->seats.pointer + ((unit_object *)self)->unit.vehicle_seat_index * 0x11c;
    uint8_t *model_nodes;
    object_marker marker;
    real_point3d offset;
    real_point3d default_translation;
    real_point3d position;
    real_matrix4x3 basis;

    halo::objects::object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &marker, 1);
    offset.x = *(float *)(nodes + 0x28) - marker.node_transform.position.x;
    offset.y = *(float *)(nodes + 0x2c) - marker.node_transform.position.y;
    offset.z = *(float *)(nodes + 0x30) - marker.node_transform.position.z;
    model_nodes = *(uint8_t **)(TAG_DATA(*(datum_index *)&((struct Unit *)TAG_DATA(*(datum_index *)self))->base.model.tag_id) + 0xbc);
    default_translation = *(real_point3d *)(model_nodes + 0x28);
    if (((unit_object *)vehicle)->unit.driver_unit_index == object_index && (uint8_t)((struct unit_object *)vehicle)->unit.animation_state != 0x25 &&
        ((unit_object *)self)->base.parent_object != k_datum_index_none) {
        halo::units::UnitView(((unit_object *)self)->base.parent_object).try_set_animation_state(0x25);
    }
    ((unit_object *)self)->unit.last_parent_object_index = vehicle_index;
    ((unit_object *)self)->unit.last_seat_change_tick = halo::game::globals().game_time->game_time;
    if (((unit_object *)self)->unit.driver_unit_index == object_index) {
        ((unit_object *)self)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((unit_object *)self)->unit.gunner_unit_index == object_index) {
        ((unit_object *)self)->unit.gunner_unit_index = k_datum_index_none;
    }
    halo::objects::object_snap_to_parent_marker_and_detach(object_index);
    position.x = offset.x + ((unit_object *)self)->base.position.x;
    position.y = offset.y + ((unit_object *)self)->base.position.y;
    position.z = offset.z + ((unit_object *)self)->base.position.z - default_translation.z;
    halo::objects::object_set_position_and_orientation(object_index, 0, 0, &position);
    {
        uint8_t *reloaded = OBJECT_DATA(object_index);

        halo::math::matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((unit_object *)self)->base.forward.i = basis.forward;
    *(real_vector3d *)&((unit_object *)self)->base.up.i = basis.up;
    {
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag = TAG_DATA(*(datum_index *)object);

        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
            halo::objects::object_for_each_light_attachment(object_index, 0, 1);
        }
        if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
            clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
            OBJECT_HEADER(object_index).flags |= 2;
        }
    }
    ((unit_object *)self)->unit.vehicle_seat_index = -1;
    ((struct unit_object *)self)->unit.base_animation_state = 2;
    if (((unit_object *)vehicle)->unit.driver_unit_index == object_index) {
        ((unit_object *)vehicle)->unit.driver_unit_index = k_datum_index_none;
    }
    if (((unit_object *)vehicle)->unit.gunner_unit_index == object_index) {
        ((unit_object *)vehicle)->unit.gunner_unit_index = k_datum_index_none;
    }
    UnitView(vehicle_index).recompute_seat_occupants();
    UnitView(object_index).pick_and_ready_next_weapon();
    {
        int8_t request[2] = { 0x14, 0 };

        UnitView(object_index).update_animation_state_machine(request);
    }
    *(real_point3d *)(self + ((unit_object *)self)->base.node_function_values.offset + 0x10) = default_translation;
    if (((unit_object *)self)->base.type == 0) {
        UnitView(object_index).reset_orientation_and_find_position(vehicle_index);
    }
    halo::objects::object_recalculate_bounding_radius_recursive(object_index);
    if (UnitView(vehicle_index).all_seats_unoccupied() == 1) {
        uint8_t *empty = (uint8_t *)halo::objects::object_try_and_get(vehicle_index, 2);

        if (empty != 0) {
            *(int32_t *)(empty + 0x5ac) = halo::game::globals().game_time->game_time;
        }
    }
    if (network_game_mode == 1) {
        uint8_t *player = (uint8_t *)halo::memory::datum_get(((unit_object *)self)->unit.controlling_player, halo::game::globals().player_data);

        if (player != 0 && ((struct player *)player)->local_player_index == -1) {
            ((struct player *)player)->position_updates.read_index = 0;
            ((struct player *)player)->position_updates.write_index = 0;
            ((struct player *)player)->vehicle_updates.read_index = 0;
            ((struct player *)player)->vehicle_updates.write_index = 0;
        }
    }
}

static void biped_free_local_player_history(uint8_t *self)
{
    datum_index player_index = ((unit_object *)self)->unit.controlling_player;
    int16_t index = (int16_t)player_index;
    int16_t salt = (int16_t)(player_index >> 16);
    uint8_t *player;

    if (network_game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= halo::game::globals().player_data->maximum_count) {
        return;
    }
    player = (uint8_t *)halo::game::globals().player_data->data + halo::game::globals().player_data->size * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || ((struct player *)player)->local_player_index == -1) {
        return;
    }
    if (network_client != 0) {
        player_update_history_free_all(*(void **)&network_client->update_history);
    }
}

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
    uint8_t *obj = OBJECT_DATA(unit_index);
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)obj);
    uint8_t *effect_block = TAG_DATA(dd->damage_effect_tag) + 0x1c4;
    uint8_t killed = (uint8_t)(flags & 1);
    uint8_t knocked_down = 0;
    uint8_t violent = 0;
    uint32_t unit_flags;
    unit_state_change_record record;

    memset(&record, 0, sizeof(record));
    if (is_local == 1) {
        float recent = ((unit_object *)obj)->base.recent_body_damage + ((unit_object *)obj)->base.recent_shield_damage;

        if (recent > 0.0f) {
            ((struct unit_object *)obj)->unit.delayed_damage_category = *(int16_t *)(effect_block + 0x2);
            ((struct unit_object *)obj)->unit.delayed_damage_ticks = 0x2d;
            if (recent < ((struct unit_object *)obj)->unit.delayed_damage_amount) {
                recent = ((struct unit_object *)obj)->unit.delayed_damage_amount;
            }
            ((struct unit_object *)obj)->unit.delayed_damage_amount = recent;
            if (dd->responsible_object != k_datum_index_none) {
                ((struct unit_object *)obj)->unit.delayed_damage_responsible_object = dd->responsible_object;
            }
        }
    }
    unit_flags = ((unit_object *)obj)->unit.flags;
    if (unit_flags & 0x10) {
        float left = ((struct unit_object *)obj)->unit.active_camouflage_power - *(float *)(effect_block + 0x1c);

        ((struct unit_object *)obj)->unit.active_camouflage_power = left;
        if (left < 0.0f) {
            ((struct unit_object *)obj)->unit.active_camouflage_power = 0.0f;
        }
    }
    if (is_local == 1) {
        violent = (uint8_t)(killed && !(*(float *)(effect_block + 0x30) < 2.0f));
        if (!killed && (test_flag(unit_flags, units::unit_flag::unknown_2000)) && ((struct Unit *)unit_tag)->feign_death_threshold > 0.0f &&
            ((struct Unit *)unit_tag)->feign_death_time > 0.0f && ((unit_object *)obj)->base.body_vitality > 0.0f &&
            ((unit_object *)obj)->base.recent_body_damage > ((struct Unit *)unit_tag)->feign_death_threshold) {
            float ticks = (halo::math::random_real_range(0.0f, 1.0f) + ((struct Unit *)unit_tag)->feign_death_time) * 30.0f;

            set_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen);
            knocked_down = 1;
            if (1.0f > ticks) {
                ticks = 1.0f;
            }
            ((struct unit_object *)obj)->unit.feign_death_ticks = (int16_t)(int32_t)ticks;
        }
    }

    if (((unit_object *)obj)->unit.controlling_player == k_datum_index_none && killed && (effect_block[0x4] & 0x80) &&
        (test_flag(((struct Unit *)unit_tag)->unit_flags, tags::unit_tag_flag::runs_around_flaming))) {
        uint8_t *self;
        datum_index vehicle_index;

        if (((unit_object *)obj)->base.parent_object == k_datum_index_none) {
            goto stunned;
        }
        self = (uint8_t *)halo::objects::object_try_and_get(unit_index, 3);
        if (self == 0 || network_game_mode == 1 ||
            (vehicle_index = ((unit_object *)self)->base.parent_object) == k_datum_index_none ||
            ((unit_object *)self)->unit.vehicle_seat_index == -1) {
            goto record_check;
        }
        if (((unit_object *)self)->base.type == 1) {
            uint8_t *me = OBJECT_DATA(unit_index);

            if (((struct object *)me)->parent_object != k_datum_index_none && ((struct unit_object *)me)->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(unit_index, ((struct object *)me)->parent_object);
            }
            biped_free_local_player_history(me);
            goto record_check;
        }
        if (::halo::units::unit_state_is_scripted_animation((unit_data *)(self + k_unit_data_offset))) {
            goto record_check;
        }
        {
            uint8_t *self_tag = TAG_DATA(*(datum_index *)self);
            datum_index graph = *(datum_index *)&((struct Unit *)self_tag)->base.animation_graph.tag_id;
            uint8_t *seat_block = *(uint8_t **)(TAG_DATA(graph) + 0x10) + (int8_t)(uint8_t)((struct unit_object *)self)->unit.animation_definition_index * 0x64;
            int16_t death_animation;
            uint8_t *object;
            uint8_t *object_tag;

            if (*(int32_t *)(seat_block + 0x40) <= 8) {
                goto record_check;
            }
            death_animation = (*(int16_t **)(seat_block + 0x44))[8];
            if (death_animation == -1) {
                goto record_check;
            }
            if (((struct unit_object *)OBJECT_DATA(vehicle_index))->unit.driver_unit_index == unit_index) {
                UnitView((int32_t)vehicle_index).notify_weapon_removed();
            }
            UnitView(unit_index).set_custom_animation(*(datum_index *)&((struct Unit *)self_tag)->base.animation_graph.tag_id, halo::models::animation_choose_random_permutation(graph, death_animation, (animation_random_stream)1));
            object = OBJECT_DATA(unit_index);
            object_tag = TAG_DATA(*(datum_index *)object);
            if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1 && test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
                halo::objects::object_for_each_light_attachment(unit_index, 0, 1);
            }
            if (*(int32_t *)&((struct Object *)object_tag)->model.tag_id != -1) {
                clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                OBJECT_HEADER(unit_index).flags |= 2;
            }
            ((struct unit_object *)self)->unit.animation_state = 0x1b;
            actor_notify_weapon_pickup_once(unit_index);
            if (((unit_object *)self)->base.network_role == 0) {
                ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
            }
        }
stunned:
        if (is_local == 1) {
            UnitView(unit_index).enter_stunned_state(dd->responsible_object);
        }
        killed = 0;
        goto local_reactions;
    }

record_check:
    if ((dd->flags & 0x10) == 0 && (killed || knocked_down || !test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) &&
        !test_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unknown_800000) && (*(uint32_t *)(effect_block + 0x4) & 0x10) == 0) {
        uint32_t effect_flags = *(uint32_t *)(effect_block + 0x4);
        real_vector2d direction;
        real_vector2d forward;
        uint8_t stunned_flag = 0;
        uint8_t special = 0;
        uint8_t has_direction = 0;
        float angle = 0.0f;

        direction.i = dd->direction.i;
        direction.j = dd->direction.j;
        forward.i = ((unit_object *)obj)->base.forward.i;
        forward.j = ((unit_object *)obj)->base.forward.j;
        if (halo::math::vector2d_normalize_with_length(direction) > 0.0f && halo::math::vector2d_normalize_with_length(forward) > 0.0f) {
            angle = halo::math::vector2d_angle_between(forward, direction);
            has_direction = 1;
        }
        if (((uint8_t)((struct Unit *)unit_tag)->unit_flags & 0x80) && (effect_flags & 4) == 0) {
            stunned_flag = 1;
        }
        if ((uint8_t)((struct unit_object *)obj)->unit.flaming_ticks != 0) {
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
        if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
            uint8_t *player = (uint8_t *)halo::memory::datum_get(((unit_object *)obj)->unit.controlling_player, halo::game::globals().player_data);

            if (player != 0) {
                record.player_value = *(uint32_t *)&((struct player *)player)->respawn_timer;
            }
        }
        UnitView(unit_index).update_stance_and_jump(killed, knocked_down, violent, stunned_flag, special, angle, (int16_t)region_index, has_direction ? &direction : 0, is_local);
    } else {
        record.valid = 0;
    }

local_reactions:
    if (is_local == 1) {
        datum_index unit_player = ((unit_object *)obj)->unit.controlling_player;

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
    if (is_local == 1 && ((unit_object *)obj)->base.type == 0) {
        if (killed) {
            actor_reassign_vehicle_seat(dd->responsible_object, unit_index, *(uint16_t *)(effect_block + 0x2));
        } else if (!test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) {
            actor_react_to_threat_event(unit_index, dd->responsible_object, *(uint16_t *)(effect_block + 0x2), total,
                (uint32_t)(uintptr_t)&dd->direction, 0);
        }
    }

    if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none && *(float *)(effect_block + 0x20) > 0.0f &&
        (halo::game::globals().current_engine != 0 || is_dedicated_server_flag)) {
        uint8_t *shake = (uint8_t *)global_globals->player_information.pointer;
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
        if (cap > ((struct unit_object *)obj)->unit.stun) {
            float value = step + ((struct unit_object *)obj)->unit.stun;

            ((struct unit_object *)obj)->unit.stun = value;
            if (value > cap) {
                ((struct unit_object *)obj)->unit.stun = cap;
            }
        }
        add = (int16_t)(int32_t)(*(float *)(effect_block + 0x28) * 30.0f);
        low = (int16_t)(int32_t)(*(float *)(shake + 0x8c) * 30.0f);
        high = (int16_t)(int32_t)(*(float *)(shake + 0x90) * 30.0f);
        if (((struct unit_object *)obj)->unit.stun_ticks < low) {
            ((struct unit_object *)obj)->unit.stun_ticks = low;
        }
        ((struct unit_object *)obj)->unit.stun_ticks += add;
        if (((struct unit_object *)obj)->unit.stun_ticks > high) {
            ((struct unit_object *)obj)->unit.stun_ticks = high;
        }
    }

    if (is_local == 1 && (killed || knocked_down)) {
        UnitView(unit_index).release_transient_state(knocked_down);
        if (((unit_object *)obj)->base.network_role == 0 && killed == 1) {
            record.unit = unit_index;
            ::halo::units::unit_broadcast_state_change_event(record);
            if ((OBJECT_HEADER(unit_index).flags & 8) == 0) {
                network_index_cache_remove(network_object_index_cache, (int32_t)unit_index);
            }
            ((unit_object *)obj)->base.network_role = 3;
        }
    }
}
#undef OBJECT_DATA
#undef OBJECT_HEADER
#undef TAG_DATA

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
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    uint8_t *fall_table = (uint8_t *)global_globals->falling_damage.pointer;
    uint32_t exempt;

    exempt = (!test_flag(unit->flags, units::unit_flag::unknown_1000) && (int8_t)tag->biped_flags >= 0) ? 0 : 1;
    if (unit_updates_suppressed != 0) {
        exempt = 1;
    }

    if (halo::hs::fields::jetpack == 0 || unit->controlling_player == k_datum_index_none) {
        if (fall_speed <= *(float *)(fall_table + 0x90)) {
            if (!test_flag(tag->biped_flags, tags::biped_tag_flag::flying) && obj->velocity.k < -*(float *)(fall_table + 0x8c)) {
                if (!exempt && !test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen)) {
                    damage_data dd;
                    halo::objects::damage_data_initialize(&dd, *(datum_index *)(fall_table + 0x38));
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
            float harmless = *(float *)(fall_table + 0x90);
            float harmful = *(float *)(fall_table + 0x94);
            float blend = (fall_speed - harmless) / (harmful - harmless);

            dd.damage_effect_tag = *(datum_index *)(fall_table + 0x1c);
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

    if (*(int32_t *)&tag->melee_damage.tag_id == -1) {
        unit_data *unit0 = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
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
        unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
        damage_effect = *(datum_index *)&tag->melee_damage.tag_id;

        if (unit->current_weapon_index != -1) {
            datum_index weapon_index = unit->weapons[unit->current_weapon_index];
            if (weapon_index != k_datum_index_none) {
                object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
                Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
                if (test_flag(weapon_tag->weapon_flags, tags::weapon_tag_flag::ais_use_weapon_melee_damage)) {
                    damage_effect = *(datum_index *)&weapon_tag->player_melee_response.tag_id;
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
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

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
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    datum_index target = ((unit_object *)obj)->base.parent_object;
    uint8_t hit = 0;
    real_plane3d plane;
    real_point3d start;
    real_point3d hit_point;
    object_collision_context context;
    object_node_collision_result record;
    damage_data dd;

    if ((uint8_t)((struct unit_object *)obj)->unit.melee_state != 4 || target == k_datum_index_none || *(datum_index *)&((Unit *)tag)->melee_damage.tag_id == k_datum_index_none) {
        return;
    }
    if ((uint8_t)((struct unit_object *)obj)->unit.melee_damage_countdown == 0 && halo::physics::object_collision_context_build(target, &context)) {
        halo::objects::object_get_position(&start, unit_index);
        plane.normal.i = ((unit_object *)obj)->base.forward.i * 0.2f;
        plane.normal.j = ((unit_object *)obj)->base.forward.j * 0.2f;
        plane.normal.k = ((unit_object *)obj)->base.forward.k * 0.2f;
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
    dd.damage_effect_tag = *(datum_index *)&((Unit *)tag)->melee_damage.tag_id;
    dd.material_type = -1;
    dd.location_cluster_index = -1;
    dd.multiplier = 1.0f;
    dd.responsible_object = unit_index;
    dd.team_index = ((unit_object *)obj)->base.owner_team;
    dd.responsible_player = ((unit_object *)obj)->unit.controlling_player;
    dd.random_blend = 0.033333335f;
    if (hit) {
        dd.epicentre = hit_point;
        dd.origin = hit_point;
        dd.direction = *(real_vector3d *)&((unit_object *)obj)->base.forward.i;
        dd.flags |= 2;
        ((struct unit_object *)obj)->unit.melee_damage_countdown = 10;
        halo::objects::object_apply_damage(&dd, ((unit_object *)obj)->base.parent_object, record.node_index, record.region_index,
            *(int16_t *)((uint8_t *)&record + 0x1a), (uint32_t)(uintptr_t)&plane);
    } else {
        halo::objects::object_apply_damage(&dd, ((unit_object *)obj)->base.parent_object, -1, -1, -1, 0);
    }
    ((struct unit_object *)obj)->unit.melee_damage_countdown--;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot((h))].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot((t))].data)
/**
 * Engine function unit_process_melee_special_interaction.
 *
 * @address 0x56ff40
 */
void halo::units::unit_process_melee_special_interaction(uint32_t attacker_index, uint32_t target_index, uint32_t node_pair, uint32_t region_pair, uint32_t material, real_point3d *contact_point, real_plane3d *contact_plane, bsp_leaf_reference *contact_leaf)
{
    uint8_t *attacker = OBJECT_DATA(attacker_index);
    uint32_t unit_flags = ((struct Unit *)TAG_DATA(*(datum_index *)attacker))->unit_flags;
    uint8_t *target = OBJECT_DATA(target_index);

    if ((unit_flags & 0x2000) && ((struct object *)target)->type == 0 && ((struct object *)target)->shield_vitality > 0.0f &&
        (test_flag(((struct Unit *)TAG_DATA(*(datum_index *)target))->unit_flags, tags::unit_tag_flag::shields_fry_infection_forms))) {
        UnitView(attacker_index).cause_melee_damage(1, target_index, (int16_t)node_pair, (int16_t)region_pair, (int16_t)material, (uint32_t)contact_plane);
        halo::objects::object_set_health_frozen_flag(attacker_index);
        halo::objects::object_delete(attacker_index);
        return;
    }
    if (!(test_flag(unit_flags, tags::unit_tag_flag::impact_melee_attaches_to_unit)) || !((1u << ((uint8_t)((struct object *)target)->type & 0x1f)) & 3) || (test_flag(((struct object *)target)->vitality_flags, objects::vitality_flag::health_frozen))) {
        return;
    }
    {
        datum_index parent = ((struct object *)target)->parent_object;

        while (parent != k_datum_index_none) {
            uint8_t *p = OBJECT_DATA(parent);

            if (parent == attacker_index || ((struct object *)p)->type != 1) {
                return;
            }
            parent = ((struct object *)p)->parent_object;
        }
    }
    {
        real_vector3d *forward = (real_vector3d *)&((struct object *)attacker)->forward;
        real_vector3d *up = (real_vector3d *)&((struct object *)attacker)->up;
        real_vector3d left;

        *(real_vector3d *)&((struct object *)attacker)->velocity.i = *global_origin3d_pointer;
        *(real_vector3d *)&((struct object *)attacker)->angular_velocity.i = *global_origin3d_pointer;
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
    set_flag(((struct object *)attacker)->flags, objects::object_flag::at_rest);
    set_flag(((struct unit_object *)attacker)->unit.flags, units::unit_flag::detached);
    UnitView(attacker_index).try_ready_weapon(1, 0);
}
#undef OBJECT_DATA
#undef TAG_DATA

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
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
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
        if ((self_team < 0) || (9 < self_team) || (team_index < 0) || (9 < team_index)) {
            goto broadcast_check;
        }
        int32_t bit_index = team_index + self_team * 10;
        hostile = 1 - ((*(uint32_t *)(team_pair_data + 0xa4 + (bit_index >> 5) * 4) & (1u << (bit_index & 0x1f))) != 0);
    } else {
        hostile = self_team != team_index;
    }
    if (!hostile) {
        return;
    }

broadcast_check:
    {
        uint8_t *attacker = 0;
        uint32_t attacker_handle = k_datum_index_none;

        if (responsible_player != k_datum_index_none) {
            uint32_t controlled_unit = *(uint32_t *)((uint8_t *)halo::game::globals().player_data->data +
                                                     halo::datum_slot(responsible_player) * 0x200 + 0x34);

            if (controlled_unit != k_datum_index_none) {
                attacker = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(controlled_unit)].data;
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
                attacker = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(link)].data;
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
                ai_communication_broadcast(1, (datum_index)attacker_handle, k_datum_index_none, -1, k_datum_index_none,
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
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
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
        halo::math::matrix4x3_from_axis_angle(rotation, axis, (real)sin((double)length), (real)cos((double)length));
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
