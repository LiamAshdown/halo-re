#include "halo/objects/record_access.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/lcg.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "ai.h"
#include "items.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/units/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

namespace halo::units {

namespace unit_release_transient_state_local {

static void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index)
{
    uint8_t *self = halo::objects::object_record_bytes(object_index);
    uint8_t *vehicle = halo::objects::object_record_bytes(vehicle_index);
    uint8_t *nodes = self + ((unit_object *)self)->base.nodes.offset;
    uint8_t *seat = (uint8_t *)((struct Unit *)halo::objects::tag_record_bytes(*(datum_index *)vehicle))->seats.pointer + ((unit_object *)self)->unit.vehicle_seat_index * 0x11c;
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
    model_nodes = *(uint8_t **)(halo::objects::tag_record_bytes(halo::objects::tag_handle(((struct Unit *)halo::objects::tag_record_bytes(*(datum_index *)self))->base.model)) + 0xbc);
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
        uint8_t *reloaded = halo::objects::object_record_bytes(object_index);

        halo::math::matrix4x3_multiply((real_matrix4x3 *)(reloaded + ((struct object *)reloaded)->nodes.offset),
            (real_matrix4x3 *)(model_nodes + 0x68), &basis);
    }
    *(real_vector3d *)&((unit_object *)self)->base.forward.i = basis.forward;
    *(real_vector3d *)&((unit_object *)self)->base.up.i = basis.up;
    {
        uint8_t *object = halo::objects::object_record_bytes(object_index);
        uint8_t *object_tag = halo::objects::tag_record_bytes(*(datum_index *)object);

        if ((int32_t)halo::objects::tag_handle(((struct Object *)object_tag)->model) != -1 && test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
            halo::objects::object_for_each_light_attachment(object_index, 0, 1);
        }
        if ((int32_t)halo::objects::tag_handle(((struct Object *)object_tag)->model) != -1) {
            clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
            halo::objects::object_header_of(object_index).flags |= 2;
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
    if (halo::networking::globals().game_mode == 1) {
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

    if (halo::networking::globals().game_mode != 1 || player_index == k_datum_index_none || index < 0 ||
        index >= halo::game::globals().player_data->maximum_count) {
        return;
    }
    player = (uint8_t *)halo::game::globals().player_data->data + halo::game::globals().player_data->size * index;
    if (*(int16_t *)player == 0 || (salt != 0 && *(int16_t *)player != salt) || ((struct player *)player)->local_player_index == -1) {
        return;
    }
    if (halo::networking::globals().client != 0) {
        halo::networking::player_update_history_free_all((player_update_history *)(*(void **)&halo::networking::globals().client->update_history));
    }
}

}

/**
 * Engine function unit_release_transient_state.
 *
 * @address 0x568610
 */
void UnitView::release_transient_state(uint8_t is_light_reset)
{
    using namespace unit_release_transient_state_local;
    uint32_t unit_index = datum_handle;
    uint8_t *obj = halo::objects::object_record_bytes(unit_index);

    if (is_light_reset == 0) {
        ((struct unit_object *)obj)->unit.feign_death_ticks = 0;
        halo::objects::object_list_membership_set(unit_index, 1);
        if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
            halo::game::player_reset_after_unit_change(((unit_object *)obj)->unit.controlling_player);
            ((unit_object *)obj)->unit.controlling_player = k_datum_index_none;
        }
        if (((unit_object *)obj)->unit.actor_index != k_datum_index_none) {
            datum_index actor_index = ((unit_object *)obj)->unit.actor_index;
            uint8_t *actor_record = (uint8_t *)halo::ai::globals().actor_data->data + halo::datum_slot(actor_index) * 0x724;

            ((struct unit_object *)obj)->unit.encounter_index = *(int16_t *)&((actor *)actor_record)->encounter_index;
            ((struct unit_object *)obj)->unit.squad_index = ((actor *)actor_record)->squad_index;
            halo::ai::actor_attempt_grenade_throw(actor_index);
            ((unit_object *)obj)->unit.actor_index = k_datum_index_none;
        }
        if (((unit_object *)obj)->unit.swarm_actor_index != k_datum_index_none) {
            datum_index swarm_index = ((unit_object *)obj)->unit.swarm_actor_index;
            uint8_t *actor_record = (uint8_t *)halo::ai::globals().actor_data->data + halo::datum_slot(swarm_index) * 0x724;

            ((struct unit_object *)obj)->unit.encounter_index = *(int16_t *)&((actor *)actor_record)->encounter_index;
            ((struct unit_object *)obj)->unit.squad_index = ((actor *)actor_record)->squad_index;
            halo::ai::actor_release_from_cluster_or_delete(swarm_index, unit_index);
            ((unit_object *)obj)->unit.swarm_actor_index = k_datum_index_none;
        }
    } else {
        uint8_t *unit_tag = halo::objects::tag_record_bytes(*(datum_index *)obj);

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        if ((float)(int32_t)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale < ((struct Unit *)unit_tag)->feign_repeat_chance) {
            set_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unknown_2000);
        } else {
            clear_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unknown_2000);
        }
    }
    ((struct unit_object *)obj)->unit.death_time = halo::game::globals().game_time->game_time;
    clear_flag(((unit_object *)obj)->unit.flags, units::unit_flag::unattended | units::unit_flag::unknown_10);
    ((unit_object *)obj)->unit.control_flags = 0;
    if (((unit_object *)obj)->unit.current_weapon_index != -1) {
        uint8_t *unit = halo::objects::object_record_bytes(unit_index);
        int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;
        datum_index weapon_index = (slot != -1) ? *(datum_index *)(unit + 0x2f8 + slot * 4) : k_datum_index_none;
        uint8_t *weapon = halo::objects::object_record_bytes(weapon_index);

        *(int16_t *)&((struct weapon_object *)weapon)->weapon.control_flags = 0;
        ((struct weapon_object *)weapon)->weapon.primary_trigger = halo::math::transition_function_evaluate((transition_function_t)4, 0.0f);
    }
    clear_flag(((struct unit_object *)halo::objects::object_record_bytes(unit_index))->unit.flags, units::unit_flag::idle_turn_seeded);
    if (((unit_object *)obj)->base.parent_object != k_datum_index_none) {
        if (((unit_object *)obj)->unit.vehicle_seat_index == -1) {
            UnitView(unit_index).detach_reposition_and_nudge();
        } else if (halo::networking::globals().game_mode != 1) {
            uint8_t *me = halo::objects::object_record_bytes(unit_index);

            if (((struct object *)me)->parent_object != k_datum_index_none && ((struct unit_object *)me)->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(unit_index, ((struct object *)me)->parent_object);
            }
            biped_free_local_player_history(me);
        }
    }
    ((unit_object *)obj)->unit.pending_speech.priority = 0;
    UnitView(unit_index).drop_inventory_weapons();
    {
        uint8_t *holder = halo::objects::object_record_bytes(unit_index);

        if (((struct unit_object *)holder)->unit.equipment_object_index != k_datum_index_none) {
            UnitView(unit_index).drop_object_from_hand(((struct unit_object *)holder)->unit.equipment_object_index);
            ((struct unit_object *)holder)->unit.equipment_object_index = k_datum_index_none;
        }
    }
    UnitView(unit_index).drop_grenades();
    if ((uint8_t)((struct unit_object *)obj)->unit.delayed_weapon_drop_ticks == 0) {
        UnitView(unit_index).drop_current_weapon(1);
    }
    ((struct unit_object *)obj)->unit.overlays[1].animation_index = -1;
    ((struct unit_object *)obj)->unit.overlays[0].animation_index = -1;
    ((struct unit_object *)obj)->unit.melee_state = 0;
    if ((uint8_t)((struct unit_object *)obj)->unit.throwing_grenade_state == 1) {
        ((struct unit_object *)obj)->unit.throwing_grenade_state = 0;
    }
}

}
