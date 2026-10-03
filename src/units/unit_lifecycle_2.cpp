#include "halo/networking/game_mode.hpp"
#include "halo/units/seat_detach.hpp"
#include "halo/units/animation_states.hpp"
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
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));

    if (is_light_reset == 0) {
        obj->unit.feign_death_ticks = 0;
        halo::objects::object_list_membership_set(unit_index, 1);
        if (obj->unit.controlling_player != k_datum_index_none) {
            halo::game::player_reset_after_unit_change(obj->unit.controlling_player);
            obj->unit.controlling_player = k_datum_index_none;
        }
        if (obj->unit.actor_index != k_datum_index_none) {
            datum_index actor_index = obj->unit.actor_index;
            uint8_t *actor_record = (uint8_t *)halo::ai::globals().actor_data->data + halo::datum_slot(actor_index) * 0x724;

            obj->unit.encounter_index = *(int16_t *)&((actor *)actor_record)->encounter_index;
            obj->unit.squad_index = ((actor *)actor_record)->squad_index;
            halo::ai::actor_attempt_grenade_throw(actor_index);
            obj->unit.actor_index = k_datum_index_none;
        }
        if (obj->unit.swarm_actor_index != k_datum_index_none) {
            datum_index swarm_index = obj->unit.swarm_actor_index;
            uint8_t *actor_record = (uint8_t *)halo::ai::globals().actor_data->data + halo::datum_slot(swarm_index) * 0x724;

            obj->unit.encounter_index = *(int16_t *)&((actor *)actor_record)->encounter_index;
            obj->unit.squad_index = ((actor *)actor_record)->squad_index;
            halo::ai::actor_release_from_cluster_or_delete(swarm_index, unit_index);
            obj->unit.swarm_actor_index = k_datum_index_none;
        }
    } else {
        Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)obj);

        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        if ((float)(int32_t)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale < unit_tag->feign_repeat_chance) {
            set_flag(obj->unit.flags, units::unit_flag::unknown_2000);
        } else {
            clear_flag(obj->unit.flags, units::unit_flag::unknown_2000);
        }
    }
    obj->unit.death_time = halo::game::globals().game_time->game_time;
    clear_flag(obj->unit.flags, units::unit_flag::unattended | units::unit_flag::unknown_10);
    obj->unit.control_flags = 0;
    if (obj->unit.current_weapon_index != -1) {
        unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
        int16_t slot = unit->unit.current_weapon_index;
        datum_index weapon_index = (slot != -1) ? unit->unit.weapons[slot] : k_datum_index_none;
        weapon_object *weapon = reinterpret_cast<weapon_object *>(halo::objects::object_record_bytes(weapon_index));

        *(int16_t *)&weapon->weapon.control_flags = 0;
        weapon->weapon.primary_trigger = halo::math::transition_function_evaluate((transition_function_t)4, 0.0f);
    }
    clear_flag(((struct unit_object *)halo::objects::object_record_bytes(unit_index))->unit.flags, units::unit_flag::idle_turn_seeded);
    if (obj->base.parent_object != k_datum_index_none) {
        if (obj->unit.vehicle_seat_index == -1) {
            UnitView(unit_index).detach_reposition_and_nudge();
        } else if (halo::networking::globals().game_mode != halo::networking::k_game_mode_client) {
            unit_object *me = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));

            if (me->base.parent_object != k_datum_index_none && me->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(unit_index, me->base.parent_object);
            }
            biped_free_local_player_history(me);
        }
    }
    obj->unit.pending_speech.priority = 0;
    UnitView(unit_index).drop_inventory_weapons();
    {
        unit_object *holder = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));

        if (holder->unit.equipment_object_index != k_datum_index_none) {
            UnitView(unit_index).drop_object_from_hand(holder->unit.equipment_object_index);
            holder->unit.equipment_object_index = k_datum_index_none;
        }
    }
    UnitView(unit_index).drop_grenades();
    if ((uint8_t)obj->unit.delayed_weapon_drop_ticks == 0) {
        UnitView(unit_index).drop_current_weapon(1);
    }
    obj->unit.overlays[1].animation_index = -1;
    obj->unit.overlays[0].animation_index = -1;
    obj->unit.melee_state = _unit_melee_state_none;
    if ((uint8_t)obj->unit.throwing_grenade_state == _unit_throwing_grenade_state_begin) {
        obj->unit.throwing_grenade_state = _unit_throwing_grenade_state_none;
    }
}

}
