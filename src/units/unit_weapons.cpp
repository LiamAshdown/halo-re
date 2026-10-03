#include "halo/networking/game_mode.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/game/records.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/records.hpp"
#include "halo/objects/record_access.hpp"
#include <string.h>
#include "halo/projectiles/api.hpp"
#include "halo/models/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/core/lcg.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "projectiles.h"
#include "hs.h"
#include "networking.h"
#include "effects.h"
#include "crt.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/models/models.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/items/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/units/vars.hpp"

static auto &object_type_definitions = halo::link::ref<object_type_definition *[k_maximum_object_types]>(halo::game::vars().object_type_definitions);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &k_empty_string = halo::link::ref<char []>(halo::networking::vars().k_empty_string);
static auto &global_origin3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &s_left_hand_marker = halo::link::ref<char []>(halo::units::vars().s_left_hand_marker);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &weapon_bottomless_clip = halo::link::ref<uint8_t>(halo::items::vars().weapon_bottomless_clip);
static auto &game_engine_unknown_aa00 = halo::link::ref<uint32_t>(halo::game::vars().game_engine_unknown_aa00);
static auto &motion_sensor_override_value = halo::link::ref<uint32_t>(halo::ui::vars().motion_sensor_override_value);

namespace halo::units {

namespace unit_add_initial_weapons_local {

static object *object_from_index(uint32_t object_index)
{
    return ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
}

}

/**
 * Creates the unit tag's starting weapons and hands each one to the unit.
 *
 * @address 0x56cf10
 */
void UnitView::add_initial_weapons()
{
    using namespace unit_add_initial_weapons_local;
    uint32_t unit_index = datum_handle;
    Unit *tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(object_from_index(unit_index)->definition_tag)].data;
    int32_t i;

    for (i = 0; (int32_t)(int16_t)i < (int32_t)tag->weapons.count; i++) {
        uint32_t weapon_tag = halo::objects::tag_handle(((UnitWeapon *)tag->weapons.pointer)[i].weapon);
        object_placement_data placement;
        uint32_t role;
        datum_index weapon_index;
        int32_t network_role;

        if (weapon_tag == k_datum_index_none) {
            continue;
        }
        halo::objects::object_placement_data_initialize(&placement, weapon_tag, unit_index);
        role = 3;
        if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host &&
            object_type_definitions[((Object *)halo::cache::globals().tag_instances[halo::datum_slot(placement.definition_tag)].data)->object_type]
                ->network_delta_message_type != -1) {
            role = 0;
        }
        weapon_index = halo::objects::object_new_with_datum_role_control(&placement, role);
        if (weapon_index == k_datum_index_none) {
            continue;
        }
        if (halo::game::globals().current_engine != 0 &&
            UnitView(unit_index).has_weapon_of_type((int32_t)object_from_index(weapon_index)->definition_tag)) {
            halo::objects::object_delete(weapon_index);
            continue;
        }
        if (::halo::units::unit_pickup_weapon(0, weapon_index, unit_index)) {
            continue;
        }
        network_role = object_from_index(weapon_index)->network_role;
        if (network_role == 0) {
            halo::objects::object_delete_unparented(weapon_index);
        } else if (network_role != 3) {
            continue;
        }
        halo::objects::object_delete_recursive(weapon_index, 0);
    }
}

/**
 * Initiates the unit's grenade-throw sequence: validates the current mode, records timing/aim data, and
 * starts the throw animation state machine.
 *
 * Original register convention: EDI, stack.
 *
 * @address 0x56e080
 */
uint8_t UnitView::begin_throw_grenade(const real_vector2d *direction)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int16_t grenade_type = unit->current_grenade_index;
    if ((grenade_type == -1) || (unit->grenade_counts[grenade_type] <= 0)) {
        return 0;
    }

    if (is_scripted_animation_state(animation_state_id(unit->animation_state))) {
        return 0;
    }
    if (halo::items::weapon_prevents_grenade_throwing(current_weapon)   != 0) {
        return 0;
    }
    if (current_weapon != k_datum_index_none) {
        halo::items::weapon_reset_triggers(current_weapon);
    }
    object *biped_check = halo::objects::object_try_and_get(unit_index, _object_mask_biped);
    if (biped_check != nullptr) {
        reinterpret_cast<biped_object *>(biped_check)->biped.melee_ticks = 0;
    }
    unit->replacement_animation_state = 0;
    unit->overlays[0].animation_index = -1;

    if (UnitView(unit_index).try_set_animation_state(animation_state_value(unit_animation_state_id::throwing_grenade)) == 0) {
        return 0;
    }

    unit->throwing_grenade_state = _unit_throwing_grenade_state_begin;
    unit->throwing_grenade_counter = 0;
    ModelAnimations *graph = halo::objects::tag_as<ModelAnimations>(unit_tag->base.animation_graph.tag_id.index);
    ModelAnimationsAnimation *animations = (ModelAnimationsAnimation *)(halo::objects::block_elements<ModelAnimationsAnimation>(graph->animations));
    unit->throwing_grenade_duration = (animations[unit_obj->animation_index].key_frame_index - unit_obj->animation_frame) + 1;

    if (direction != 0) {
        UnitView(unit_index).set_throw_aim_direction(direction);
    } else {
        real_vector2d aim;

        aim.i = ((struct unit_object *)unit_obj)->unit.aiming_vector.i;
        aim.j = ((struct unit_object *)unit_obj)->unit.aiming_vector.j;
        if (0.0f < halo::math::vector2d_normalize_with_length(aim)) {
            UnitView(unit_index).set_throw_aim_direction(&aim);
        }
    }
    halo::interface::weapon_action_notify_for_unit(unit_index, 0x11);
    halo::game::unit_invalidate_local_player_zoom_level(unit_index);
    GlobalsGrenade *grenade_table_entry = &halo::objects::block_element<GlobalsGrenade>(global_globals->grenades, (int8_t)grenade_type);
    if (halo::objects::tag_handle(grenade_table_entry->throwing_effect) != -1) {
        halo::effects::effect_new_on_object(unit_index, halo::objects::tag_handle(grenade_table_entry->throwing_effect), unit_index, -1,
            0.0f, 0.0f, 0, 0);
    }
    return 1;
}

/**
 * Checks whether the unit's current seat allows using its equipped weapon, consulting an optional scripted
 * permission callback.
 *
 * Original register convention: ESI -> unit_index, EDI -> weapon_index.
 *
 * @address 0x56da00
 */
uint8_t UnitView::check_weapon_use_permission(uint32_t weapon_index)
{
    uint32_t unit_index = datum_handle;
    char *seat_name = UnitView(unit_index).get_seat_or_state_name();
    char *weapon_label = k_empty_string;
    if (weapon_index != k_datum_index_none) {
        object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
        weapon_label = halo::objects::tag_as<Weapon>(weapon_obj->definition_tag)->label.string;
    }

    if (UnitView(unit_index).set_or_test_seat_and_weapon_label(seat_name, weapon_label, 0) == 0) {
        return 0;
    }
    if (halo::game::globals().current_engine != 0 && halo::game::globals().current_engine->unknown_60 != nullptr) {
        uint8_t (*permission)(uint32_t, uint32_t) =
            reinterpret_cast<uint8_t (*)(uint32_t, uint32_t)>(halo::game::globals().current_engine->unknown_60);

        return permission(unit_index, weapon_index);
    }
    return 1;
}

/**
 * Clears the unit's currently selected secondary item field, releasing it first via unit_drop_object_from_hand.
 *
 * Original register convention: in_ECX.
 *
 * @address 0x56d2c0
 */
void UnitView::clear_selected_equipment()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    if (unit->equipment_object_index != k_datum_index_none) {
        UnitView(unit_index).drop_object_from_hand(unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    return;
}

/**
 * Counts how many of the unit's carried weapons are not marked with the 0x10 'undeployed' flag.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56d990
 */
int16_t UnitView::count_deployed_weapons()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    int16_t count = 0;
    for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
        datum_index weapon_index = unit->weapons[i];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            Weapon *weapon_tag = halo::objects::tag_as<Weapon>(weapon_obj->definition_tag);
            if (!test_flag(weapon_tag->weapon_flags, tags::weapon_tag_flag::doesn_t_count_toward_maximum)) {
                count++;
            }
        }
    }
    return count;
}

/**
 * Engine function unit_current_weapon_has_flag.
 *
 * Original register convention: in_ECX -> unit_index.
 *
 * @address 0x565b60
 */
uint8_t UnitView::current_weapon_has_flag()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    if (unit->zoom_level != -1) {
        int16_t slot = unit->current_weapon_index;
        if (slot != -1 && unit->weapons[slot] != k_datum_index_none) {
            object *weapon = ((object_header *)halo::objects::globals().object_data->data)[unit->weapons[slot] & 0xffff].data;
            if (halo::game::weapon_flag_set(halo::objects::tag_as<Weapon>(weapon->definition_tag), tags::weapon_tag_flag::enables_integrated_night_vision)) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Engine function unit_current_weapon_is_type.
 *
 * Original register convention: see file header.
 *
 * @address 0x561f80
 */
uint8_t UnitView::current_weapon_is_type(datum_index weapon_tag_id)
{
    uint32_t unit_index = datum_handle;
    if (unit_index == (uint32_t)-1 || weapon_tag_id == k_datum_index_none) {
        return 0;
    }

    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    int16_t slot = unit->current_weapon_index;

    if (slot == -1) {
        return 0;
    }

    datum_index weapon_index = unit->weapons[slot];
    if (weapon_index == k_datum_index_none) {
        return 0;
    }

    object *weapon = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
    return weapon->definition_tag == weapon_tag_id;
}

/**
 * Returns whether the unit's currently equipped weapon has type code 2 or 3.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56bd60
 */
uint8_t UnitView::current_weapon_type_is_2_or_3()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    if (unit->current_weapon_index == -1) {
        return 0;
    }
    datum_index weapon_index = unit->weapons[unit->current_weapon_index];
    if (weapon_index == k_datum_index_none) {
        return 0;
    }
    object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
    int8_t weapon_kind = reinterpret_cast<weapon_object *>(weapon_obj)->weapon.triggers[0].effect_state;
    return (weapon_kind == 2) || (weapon_kind == 3);
}

/**
 * Detaches and drops the unit's current weapon object and selects a replacement desired-weapon slot.
 *
 * @address 0x56dec0
 */
uint8_t UnitView::drop_current_weapon(uint8_t force)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int16_t next_slot = UnitView(unit_index).find_next_zone_permitted_weapon_slot((int32_t)(uint16_t)unit->current_weapon_index, 1);

    if ((current_weapon != k_datum_index_none) &&
        ((next_slot != unit->current_weapon_index) || force) &&
        ((((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(current_weapon)].data->flags & 1) == 0)) {
        if ((uint8_t)halo::items::weapon_put_away(current_weapon, (int8_t)force) != 0) {
            halo::interface::weapon_action_notify_for_unit(unit_index, 0xd);
            UnitView(unit_index).drop_object_from_hand(current_weapon);
            unit->weapons[unit->current_weapon_index] = k_datum_index_none;
            unit->current_weapon_index = -1;
            unit->desired_weapon_index = UnitView(unit_index).find_next_zone_permitted_weapon_slot(-1, 0);
            if (((uint8_t)halo::items::weapon_is_out_of_ammo(current_weapon) == 0) && (halo::networking::globals().game_mode == halo::networking::k_game_mode_local)) {
                halo::objects::object_delete(current_weapon);
            }
            return 1;
        }
    }
    return 0;
}

/**
 * Spawns and drops all of the unit's carried grenades of both types into the world (e.g. on death),
 * decrementing each grenade_counts slot to zero. FIXED (register inputs, objdump): the original never reads
 * EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s)
 * read).
 *
 * @address 0x56ef60
 */
void UnitView::drop_grenades()
{
    uint32_t unit_index = datum_handle;
    GlobalsGrenade *grenade_type_table = halo::objects::block_elements<GlobalsGrenade>(global_globals->grenades);
    int grenade_type;

    for (grenade_type = 0; grenade_type < 2; grenade_type++) {
        object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
        unit_data *unit = halo::units::unit_data_of(unit_obj);
        int8_t *count = &unit->grenade_counts[grenade_type];

        while (*count > 0) {
            object_placement_data placement;
            datum_index projectile_tag = halo::objects::tag_handle(grenade_type_table[grenade_type].projectile);
            uint32_t role = 3;

            halo::objects::object_placement_data_initialize(&placement, projectile_tag, unit_index);

            if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
                Object *proj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(placement.definition_tag)].data;
                object_type_definition *type_def = object_type_definitions[proj_tag->object_type];
                if (((struct object_type_definition *)type_def)->network_delta_message_type != -1) {
                    role = 0;
                }
            }

            {
                uint32_t projectile_index = halo::objects::object_new_with_datum_role_control(&placement, role);
                if (projectile_index != k_datum_index_none) {
                    halo::objects::object_unlink_cluster_or_notify_parent(projectile_index);
                    UnitView(unit_index).drop_object_from_hand(projectile_index);
                }
            }

            *count -= 1;
        }
    }
}

/**
 * Drops every weapon currently carried in the unit's inventory except the one currently in hand, clearing
 * each inventory slot as it is dropped and redirecting the desired-weapon index to the current weapon if it
 * pointed at a dropped slot. FIXED (register inputs, objdump): the original never reads EAX as an input (it
 * overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x56f060
 */
void UnitView::drop_inventory_weapons()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    int16_t slot;

    for (slot = 0; slot < 4; slot++) {
        datum_index *weapon = &unit->weapons[slot];

        datum_index dropped = *weapon;

        if (dropped != k_datum_index_none && slot != unit->current_weapon_index) {
            UnitView(unit_index).drop_object_from_hand(dropped);
            if (slot == unit->desired_weapon_index) {
                unit->desired_weapon_index = unit->current_weapon_index;
            }
            *weapon = k_datum_index_none;

            if (halo::items::weapon_is_out_of_ammo(dropped) == 0 && halo::networking::globals().game_mode == halo::networking::k_game_mode_local) {
                halo::objects::object_delete(dropped);
            }
        }
    }
}

/**
 * Releases every weapon in the unit's inventory except the currently equipped one, clearing the corresponding
 * slot and any next/desired-weapon references to it.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56d360
 */
void UnitView::drop_inventory_weapons_except_current()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if ((weapon_index != k_datum_index_none) && (slot != unit->current_weapon_index)) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            if (weapon_obj->network_role == 0) {
                halo::objects::object_delete_unparented(weapon_index);
                halo::objects::object_delete_recursive(weapon_index, 0);
            } else if (weapon_obj->network_role == 3) {
                halo::objects::object_delete_recursive(weapon_index, 0);
            }
            unit->weapons[slot] = k_datum_index_none;
            if (slot == unit->desired_weapon_index) {
                unit->desired_weapon_index = -1;
            }
            if (slot == unit->current_weapon_index) {
                unit->current_weapon_index = -1;
            }
        }
    }
    return;
}

/**
 * Engine function unit_drop_object_from_hand.
 *
 * @address 0x56ed00
 */
void UnitView::drop_object_from_hand(uint32_t object_index)
{
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    unit_object *dropped = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));
    real_vector3d toss;
    real_vector3d root_velocity;
    real_point3d camera;
    real speed;
    int32_t role;

    if (dropped->base.parent_object == k_datum_index_none) {
        uint8_t *object;
        Unit *object_tag;

        halo::objects::object_set_cluster_and_parent(object_index, 0);
        object = halo::objects::object_record_bytes(object_index);
        object_tag = halo::objects::tag_as<Unit>(*(datum_index *)object);
        if ((int32_t)halo::objects::tag_handle(object_tag->base.model) != -1) {
            if ((uint8_t)((struct object *)object)->flags & 1) {
                halo::objects::object_for_each_light_attachment(object_index, 0, 1);
            }
            if ((int32_t)halo::objects::tag_handle(object_tag->base.model) != -1) {
                clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].flags |= 2;
            }
        }
        halo::objects::object_reorient_relative_to_marker(unit_index, s_left_hand_marker, object_index, k_empty_string);
    }
    ((struct unit_object *)halo::objects::object_record_bytes(object_index))->unit.actor_index &= ~3u;
    halo::objects::object_snap_to_parent_marker_and_detach(object_index);
    *(real_vector3d *)&dropped->base.velocity.i = *global_origin3d_pointer;
    *(real_vector3d *)&dropped->base.angular_velocity.i = *global_origin3d_pointer;

    halo::math::vector3d_randomize_direction(*((real_point3d *)&unit->unit.aiming_vector), &toss, halo::math::globals().random_seed_global, 0.0f, 0.39269909f);
    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    speed = (real)(int32_t)((uint32_t)halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale * 0.013333336f + 0.026666667f;
    toss.i *= speed;
    toss.j *= speed;
    toss.k *= speed;
    halo::objects::object_get_root_object_velocities(unit_index, &root_velocity, 0);
    toss.i += root_velocity.i;
    dropped->unit.swarm_previous_unit_index = unit_index;
    toss.j += root_velocity.j;
    toss.k += root_velocity.k;
    halo::items::item_accelerate(object_index, &toss, 0);

    UnitView(unit_index).get_camera_position(&camera);
    if (!halo::objects::object_reposition_to_spawn_location(object_index, &camera, k_datum_index_none) && halo::game::globals().current_engine == 0) {
        halo::objects::object_delete(object_index);
    }
    if ((unit->unit.flags & 0x100000)) {
        role = ((struct object *)halo::objects::object_record_bytes(object_index))->network_role;
        if (role == 0) {
            halo::objects::object_delete_unparented(object_index);
            halo::objects::object_delete_recursive(object_index, 0);
        } else if (role == 3) {
            halo::objects::object_delete_recursive(object_index, 0);
        }
    }
}

/**
 * Returns the index of the first empty weapon inventory slot, or an invalid index if the unit's inventory is
 * full.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56d660
 */
int16_t UnitView::find_empty_weapon_slot()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        if (unit->weapons[slot] == k_datum_index_none) {
            return slot;
        }
    }
    return -1;
}

/**
 * Engine function unit_find_next_grenade_type_with_count.
 *
 * Original register convention: in_EAX, in_ECX, stack.
 *
 * @address 0x5699a0
 */
int32_t UnitView::find_next_grenade_type_with_count(int32_t start_index, int16_t direction)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    int32_t fallback = -1;
    int32_t original = start_index;
    if ((int16_t)start_index == -1) {
        start_index = 0;
        original = start_index;
    }

    for (;;) {
        int16_t index = (int16_t)start_index;
        if (0 < unit->grenade_counts[index]) {
            if (index != (int16_t)original) {
                return start_index;
            }
            fallback = start_index;
            if (direction == 0) {
                return start_index;
            }
        }
        if (direction < 0) {
            start_index = (index == 0) ? 1 : (int32_t)index - 1;
        } else if (index == 1) {
            start_index = 0;
        } else {
            start_index = (int32_t)index + 1;
        }
        if ((int16_t)start_index == (int16_t)original) {
            return fallback;
        }
    }
}

/**
 * Finds the index of the first carried weapon whose tag flags have the given bit set, or 0xffff if none.
 *
 * @address 0x570520
 */
uint16_t UnitView::find_weapon_index_by_flag(uint8_t flag_bit)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    int32_t slot = 0;

    for (;;) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
            if ((weapon_tag->weapon_flags & (1u << (flag_bit & 0x1f))) != 0) {
                return (uint16_t)slot;
            }
        }
        slot++;
        if (slot > 3) {
            return 0xffff;
        }
    }
}

/**
 * Finds the index of the first carried weapon whose tag flags have bit 3 (must_be_readied) set, or 0xffff if
 * none.
 *
 * @address 0x570460
 */
uint16_t UnitView::find_weapon_index_with_fixed_flag()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    int16_t slot = 0;

    for (;;) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            Weapon *weapon_tag = (Weapon *)halo::cache::globals().tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
            if ((weapon_tag->weapon_flags >> 3 & 1) != 0) {
                return slot;
            }
        }
        slot++;
        if (slot > 3) {
            return 0xffff;
        }
    }
}

/**
 * Engine function unit_find_weapon_marker_transform.
 *
 * @address 0x5640a0
 */
uint8_t UnitView::find_weapon_marker_transform(uint32_t vehicle_index, int16_t seat_index, real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint)
{
    uint32_t unit_index = datum_handle;
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)halo::objects::object_record_bytes(unit_index));
    GBXModel *model = halo::objects::tag_as<GBXModel>(halo::objects::tag_handle(unit_tag->base.model));
    ModelAnimations *graph = halo::objects::tag_as<ModelAnimations>(halo::objects::tag_handle(unit_tag->base.animation_graph));
    UnitSeat &seat = halo::objects::block_element<UnitSeat>(halo::objects::tag_as<Unit>(*(datum_index *)halo::objects::object_record_bytes(vehicle_index))->seats, seat_index);
    ModelAnimationsAnimationGraphUnitSeat *block = reinterpret_cast<ModelAnimationsAnimationGraphUnitSeat *>(0);
    int16_t i;
    int16_t enter_animation;
    ModelAnimationsAnimation *animation;
    object_marker seat_marker;
    object_marker hint_marker;
    real_orientation orientations[k_maximum_nodes_per_model];
    real_matrix4x3 root;
    real_matrix4x3 entry;
    char hint_name[0x100];

    for (i = 0; i < (int32_t)graph->units.count; i++) {
        if (_stricmp(halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(graph->units, i).label.string, seat.label.string) == 0) {
            block = reinterpret_cast<ModelAnimationsAnimationGraphUnitSeat *>(&halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(graph->units, i));
            break;
        }
    }
    if (block == 0 || (int32_t)block->animations.count <= 7) {
        return 0;
    }
    enter_animation = halo::objects::block_element<int16_t>(block->animations, 7);
    if (enter_animation == -1) {
        return 0;
    }
    animation = (ModelAnimationsAnimation *)(&halo::objects::block_element<ModelAnimationsAnimation>(graph->animations, enter_animation));
    halo::objects::object_get_node_local_transform(vehicle_index, seat.marker_name.string, &seat_marker, 1);
    halo::models::animation_view(animation).get_frame_orientations((GBXModel *)model, 0, orientations);
    halo::math::matrix4x3_from_quaternion(orientations[0].rotation, root);
    root.position = orientations[0].translation;
    halo::math::matrix4x3_multiply(&seat_marker.node_transform, &root, &entry);
    strcpy(hint_name, seat.marker_name.string);
    strcat(hint_name, " enter-hint");
    halo::objects::object_get_node_local_transform(vehicle_index, hint_name, &hint_marker, 1);
    if (out_seat != 0) {
        *out_seat = seat_marker.node_transform.position;
    }
    if (out_entry != 0) {
        *out_entry = entry.position;
    }
    if (out_hint != 0) {
        *out_hint = hint_marker.node_transform.position;
    }
    return 1;
}

/**
 * FIXED (objdump 0x565ae7): a stack argument (the zoom level) goes on to weapon_get_zoom_magnification in DX
 * with the weapon in EAX (tail jump); the draft called it with nothing and its caller passed the zoom level
 * as the unit.
 *
 * Original register convention: EAX -> unit_index, stack -> zoom_level.
 *
 * @address 0x565ab0
 */
float UnitView::get_active_weapon_scale(int16_t zoom_level)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    int16_t slot = unit->current_weapon_index;

    if (slot != -1 && unit->weapons[slot] != k_datum_index_none) {
        return halo::items::weapon_get_zoom_magnification(unit->weapons[slot], zoom_level);
    }
    return 1.0f;
}

/**
 * Returns the index of the unit's currently selected grenade type.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56e060
 */
int8_t UnitView::get_current_grenade_index()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    return unit->current_grenade_index;
}

/**
 * Returns a label string for the unit's currently held weapon, or 'unarmed' if no weapon is equipped.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56dfd0
 */
const char *UnitView::get_current_weapon_label()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    if (unit->current_weapon_index != -1) {
        datum_index weapon_index = unit->weapons[unit->current_weapon_index];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            return halo::objects::tag_as<Weapon>(weapon_obj->definition_tag)->label.string;
        }
    }
    return "unarmed";
}

/**
 * Returns the ammo count for a given grenade-type slot (index in CX) held by the unit.
 *
 * Original register convention: in_EAX, in_CX.
 *
 * @address 0x56e030
 */
int32_t UnitView::get_grenade_count(int16_t grenade_type)
{
    uint32_t unit_index = datum_handle;
    if (grenade_type == -1) {
        return 0;
    }
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    return unit->grenade_counts[grenade_type];
}

/**
 * Engine function unit_get_weapon_marker_indices.
 *
 * @address 0x5642c0
 */
uint8_t UnitView::get_weapon_marker_indices(uint8_t use_alternate, float *out_dx_to_key_frame, float *out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    Object *obj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    ModelAnimations *graph = halo::objects::tag_as<ModelAnimations>(obj_tag->animation_graph.tag_id.index);
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        &halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(graph->units, unit->animation_definition_index);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        &halo::objects::block_element<ModelAnimationsAnimationGraphWeapon>(unit_seat->weapons, unit->animation_weapon_index);

    int32_t raw_index = (use_alternate ? 3 : 0) + 0x27;
    int16_t animation_index = -1;
    if (raw_index < (int32_t)weapon_anim->animations.count) {
        animation_index = halo::objects::block_element<int16_t>(weapon_anim->animations, raw_index);
    }
    if (animation_index == -1) {
        return 0;
    }

    ModelAnimationsAnimation *anim = &halo::objects::block_element<ModelAnimationsAnimation>(graph->animations, animation_index);

    halo::models::animation_view(anim).get_frame_info_distance(out_dx_to_key_frame, out_dx_total);

    if (out_key_frame_index != 0) {
        *out_key_frame_index = anim->key_frame_index;
    }
    if (out_frame_count != 0) {
        *out_frame_count = anim->frame_count;
    }
    return 1;
}

/**
 * Engine function unit_get_weapon_object_index.
 *
 * Original register convention: in_EAX, in_CX.
 *
 * @address 0x569970
 */
datum_index UnitView::get_weapon_object_index(int16_t slot_index)
{
    uint32_t unit_index = datum_handle;
    if (slot_index == -1) {
        return k_datum_index_none;
    }
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    return unit->weapons[slot_index];
}

/**
 * Returns whether the unit currently carries a weapon of the given type in any inventory slot.
 *
 * Original register convention: in_EAX, unaff_EBX.
 *
 * @address 0x56d610
 */
uint8_t UnitView::has_weapon_of_type(int32_t weapon_group_tag)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            if ((int32_t)weapon_obj->definition_tag == weapon_group_tag) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Not a function of its own: 0x56d070 is the shared epilogue (pop ebx, pop ebp, add esp 0x8c, ret) of the code
 * before it, and nothing calls it. Kept empty.
 *
 * @address 0x56d070
 */
void halo::units::unit_inventory_get_weapon(void)
{
    return;
}

/**
 * Returns whether the unit's inventory does NOT already contain another weapon of the same type as a given
 * reference object.
 *
 * Original register convention: in_EAX, in_ECX.
 *
 * @address 0x56da80
 */
uint8_t halo::units::unit_lacks_weapon_type_of(uint32_t reference_object_index, uint32_t unit_index)
{
    object *reference_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(reference_object_index)].data;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
        datum_index weapon_index = unit->weapons[i];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            if (reference_obj->definition_tag == weapon_obj->definition_tag) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Engine function unit_local_player_weapon_flag_check.
 *
 * @address 0x565b00
 */
uint8_t halo::units::unit_local_player_weapon_flag_check(void)
{
    if (local_player_globals->local_player_count == 1) {
        int32_t slot = -1;
        if (local_player_globals->local_players[0] != k_datum_index_none) {
            slot = 0;
        }
        if (slot != -1 && slot < 1) {
            uint32_t player_handle = local_player_globals->local_players[slot];
            if (player_handle != (uint32_t)-1) {
                datum_index unit_handle = halo::game::player_at(player_handle)->unit;
                if (unit_handle != k_datum_index_none) {
                    return UnitView(unit_handle).current_weapon_has_flag();
                }
            }
        }
    }
    return 0;
}

/**
 * Calls unit_try_set_animation_state when the implicit weapon/object index is valid, used as a small guard before
 * weapon-related teardown.
 *
 * Original register convention: EAX -> object_index.
 *
 * @address 0x56ab10
 */
void UnitView::notify_weapon_removed()
{
    int32_t object_index = datum_handle;
    if (object_index != -1) {
        UnitView((uint32_t)object_index).try_set_animation_state(animation_state_value(unit_animation_state_id::unknown_25));
    }
}

/**
 * FIXED (objdump 0x56ab30..0x56ab40): the state is the constant 0x26 (the seat close), not an argument.
 *
 * Original register convention: EAX -> object_index.
 *
 * @address 0x56ab30
 */
void UnitView::notify_weapon_removed_dup()
{
    int32_t object_index = datum_handle;
    if (object_index != -1) {
        UnitView((uint32_t)object_index).try_set_animation_state(animation_state_value(unit_animation_state_id::unknown_26));
    }
}

/**
 * Recomputes which inventory slot should become the unit's next weapon and triggers the weapon-switch
 * routine.
 *
 * Original register convention: unaff_ESI.
 *
 * @address 0x56d6a0
 */
void UnitView::pick_and_ready_next_weapon()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    unit->desired_weapon_index = UnitView(unit_index).find_next_zone_permitted_weapon_slot((int32_t)(uint16_t)unit->current_weapon_index, 0);
    UnitView(unit_index).ready_desired_weapon(1);
    return;
}

/**
 * Handles a unit picking up a nearby weapon object into a free inventory slot, updating attachment/physics
 * state and optionally switching to it immediately.
 *
 * @address 0x56d400
 */
uint8_t halo::units::unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index)
{
    object *unit_obj = halo::objects::object_try_and_get(unit_index, _object_mask_unit);
    object *weapon_obj = halo::objects::object_try_and_get(weapon_index, _object_mask_weapon);
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client) {
        char *seat_name = UnitView(unit_index).get_seat_or_state_name();
        char *weapon_label = k_empty_string;
        if (weapon_index != k_datum_index_none) {
            object *label_src = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
            weapon_label = halo::objects::tag_as<Weapon>(label_src->definition_tag)->label.string;
        }
        if (UnitView(unit_index).set_or_test_seat_and_weapon_label(seat_name, weapon_label, 0) == 0) {
            object *check = halo::objects::object_try_and_get(unit_index, _object_mask_unit);
            if ((check != nullptr) && ((halo::units::unit_data_of(check))->animation_state == animation_state_value(unit_animation_state_id::seat_exit))) {
                UnitView(unit_index).detach_from_seat(1, 1, 0);
            }
        }
    }

    if ((test_flag(weapon_obj->flags, objects::object_flag::needs_cluster_update)) && (weapon_obj->parent_object == k_datum_index_none)) {
        if (UnitView(unit_index).check_weapon_use_permission(weapon_index) != 0  ) {
            if (halo::game::game_engine_notify_weapon_ready_state_change(unit_index, weapon_index) != 0  ) {
                if (pickup_mode == 2) {
                    UnitView(unit_index).drop_inventory_weapons_except_current();
                }
                int16_t slot = UnitView(unit_index).find_empty_weapon_slot();
                if (slot != -1) {
                    halo::objects::object_unlink_cluster_or_notify_parent(weapon_index);
                    Object *weapon_def = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
                    if ((halo::objects::tag_handle(weapon_def->model) != k_datum_index_none) &&
                        (!test_flag(weapon_obj->flags, objects::object_flag::no_collision))) {
                        halo::objects::object_for_each_light_attachment(weapon_index, 1, 0);
                    }
                    set_flag(weapon_obj->flags, objects::object_flag::no_collision);
                    ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].flags &= 0xfd;
                    halo::items::item_set_holder(weapon_index, unit_index);
                    unit->weapons[slot] = weapon_index;
                    unit->weapon_ready_ticks[slot] = 0;

                    if (pickup_mode != 0) {
                        if (pickup_mode == 1) {
                            if (!test_flag(unit->control_flags, units::unit_control_flag::primary_trigger)) {
                                halo::game::unit_set_local_player_weapon_index(unit_index, slot);
                            }
                        } else if (pickup_mode != 2) {
                            return 1;
                        }
                        unit->desired_weapon_index = slot;
                        return 1;
                    }
                    unit->desired_weapon_index = UnitView(unit_index).find_next_zone_permitted_weapon_slot((int32_t)(uint16_t)unit->current_weapon_index, 0);
                    return 1;
                }
            }
        }
    }
    return 0;
}

/**
 * Engine function unit_ready_desired_weapon.
 *
 * @address 0x56d6e0
 */
void UnitView::ready_desired_weapon(uint8_t force)
{
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_header_of(unit_index).data);
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)(unit));
    datum_index desired_weapon = k_datum_index_none;
    int16_t current = unit->unit.current_weapon_index;

    if (unit->unit.desired_weapon_index != -1) {
        desired_weapon = unit->unit.weapons[unit->unit.desired_weapon_index];
    }
    if (current != -1) {
        datum_index weapon = unit->unit.weapons[current];

        if (weapon != k_datum_index_none && halo::items::weapon_put_away(weapon, (int8_t)force) != 0) {
            object *weapon_obj;

            halo::objects::object_snap_to_parent_marker_and_detach(weapon);
            halo::objects::object_unlink_cluster_or_notify_parent(weapon);
            halo::objects::object_mark_pending_delete(weapon);
            weapon_obj = halo::objects::object_header_of(weapon).data;
            if (halo::objects::tag_handle(halo::objects::tag_as<Object>(weapon_obj->definition_tag)->model) != -1 && !test_flag(weapon_obj->flags, objects::object_flag::no_collision)) {
                halo::objects::object_for_each_light_attachment(weapon, 1, 0);
            }
            set_flag(weapon_obj->flags, objects::object_flag::no_collision);
            halo::objects::object_header_of(weapon).flags &= 0xfd;
            halo::items::item_set_holder(weapon, unit_index);
            unit->unit.current_weapon_index = -1;
        }
    }
    if (unit->unit.current_weapon_index != -1) {
        UnitView(unit_index).validate_and_clear_weapon_switch();
        return;
    }
    if (desired_weapon == k_datum_index_none) {
        UnitView(unit_index).set_or_test_seat_and_weapon_label(UnitView(unit_index).get_seat_or_state_name(), "unarmed", 1);
        unit->unit.current_weapon_index = -1;
        UnitView(unit_index).validate_and_clear_weapon_switch();
        return;
    }
    {
        char *weapon_label = halo::items::weapon_get_label(desired_weapon);
        ModelAnimations *graph;
        ModelAnimationsAnimationGraphWeapon *weapon_anim;
        object *weapon_obj;
        Weapon *weapon_tag;
        int16_t desired;

        UnitView(unit_index).set_or_test_seat_and_weapon_label(UnitView(unit_index).get_seat_or_state_name(), weapon_label, 1);
        graph = halo::objects::tag_as<ModelAnimations>(halo::objects::tag_handle(unit_tag->base.animation_graph));
        weapon_anim = &halo::objects::block_element<ModelAnimationsAnimationGraphWeapon>(
            halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(graph->units, (int8_t)(uint8_t)unit->unit.animation_definition_index).weapons,
            (int8_t)(uint8_t)unit->unit.animation_weapon_index);
        halo::objects::object_set_cluster_and_parent(desired_weapon, 0);
        weapon_obj = halo::objects::object_header_of(desired_weapon).data;
        weapon_tag = halo::objects::tag_as<Weapon>(weapon_obj->definition_tag);
        if (halo::objects::tag_handle(weapon_tag->base.base.model) != -1) {
            if (test_flag(weapon_obj->flags, objects::object_flag::no_collision)) {
                halo::objects::object_for_each_light_attachment(desired_weapon, 0, 1);
            }
            if (halo::objects::tag_handle(weapon_tag->base.base.model) != -1) {
                clear_flag(weapon_obj->flags, objects::object_flag::no_collision);
                halo::objects::object_header_of(desired_weapon).flags |= 2;
            }
        }
        halo::objects::object_reorient_relative_to_marker(unit_index, weapon_anim->hand_marker.string, desired_weapon,
            weapon_anim->grip_marker.string);
        desired = unit->unit.desired_weapon_index;
        unit->unit.current_weapon_index = desired;
        if (desired != -1) {
            unit->unit.weapon_ready_ticks[desired] = halo::game::globals().game_time->game_time;
        }
        halo::items::weapon_ready(desired_weapon);
        UnitView(unit_index).validate_and_clear_weapon_switch();
    }
}
#undef OBJECT_TAG

/**
 * Recomputes a control/targeting flag on the unit based on whether it has any active target or seat
 * references, then refreshes each carried weapon and the unit's occupant tracking.
 *
 * Original register convention: param_1, unaff_CL.
 *
 * @address 0x569bf0
 */
void UnitView::refresh_targeting_flag_and_weapons(uint8_t initial_targeting_flag)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    uint8_t has_reference = initial_targeting_flag;

    if ((unit->actor_index != k_datum_index_none) || (unit->swarm_actor_index != k_datum_index_none) ||
        (unit->controlling_player != k_datum_index_none)) {
        has_reference = 1;
    }

    uint32_t flags = unit->flags;
    if (((unit_obj->vitality_flags & _object_health_frozen_bit) == 0) && has_reference) {
        unit->flags = flags | 1;
        flags |= 0x41;
    } else {
        unit->flags = flags & 0xfffffffe;
        flags &= 0xffffffbe;
    }
    unit->flags = flags;

    for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
        if (unit->weapons[i] != k_datum_index_none) {
            halo::items::item_set_holder(unit->weapons[i], unit_index);
        }
    }
    UnitView(unit_index).recompute_seat_occupants();
    return;
}

/**
 * Releases the unit's currently selected secondary item by dispatching to a type-specific release routine,
 * then clears the selection.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56d300
 */
void UnitView::release_selected_equipment()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    datum_index equipment_index = unit->equipment_object_index;
    if (equipment_index == k_datum_index_none) {
        return;
    }
    object *equipment_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(equipment_index)].data;
    int32_t equipment_role = equipment_obj->network_role;
    if (equipment_role == 0) {
        halo::objects::object_delete_unparented(equipment_index);
    }
    if (equipment_role == 0 || equipment_role == 3) {
        halo::objects::object_delete_recursive(equipment_index, 0);
    }
    unit->equipment_object_index = k_datum_index_none;
    return;
}

/**
 * Engine function unit_release_thrown_grenade.
 *
 * @address 0x56e440
 */
void UnitView::release_thrown_grenade(uint8_t early)
{
    uint32_t object_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)unit);
    real_vector3d *aim = (real_vector3d *)&unit->unit.aiming_vector;
    datum_index grenade;
    real_vector3d velocity;

    if ((uint8_t)unit->unit.throwing_grenade_state != _unit_throwing_grenade_state_in_hand) {
        return;
    }
    grenade = unit->unit.throwing_grenade_projectile;
    if (grenade == k_datum_index_none) {
        unit->unit.throwing_grenade_state = _unit_throwing_grenade_state_released;
        return;
    }
    halo::objects::object_snap_to_parent_marker_and_detach(grenade);
    if (unit->unit.actor_index != k_datum_index_none) {
        real_point3d position;

        halo::objects::object_get_position(&position, unit->unit.throwing_grenade_projectile);
        halo::ai::actor_compute_grenade_throw_vector(unit->unit.actor_index, &position, &velocity);
    } else {
        if (unit->unit.controlling_player != k_datum_index_none) {
            GlobalsPlayerInformation *info = halo::objects::block_elements<GlobalsPlayerInformation>(global_globals->player_information);
            real_vector3d forward = *aim;
            real_vector3d right;
            real_vector3d up;
            real_point3d launch;
            real forward_offset = info->grenade_origin.x;
            real right_offset = info->grenade_origin.y;
            real up_offset = info->grenade_origin.z;

            halo::math::vector3d_cross_product(right, forward, *halo::math::globals().global_up3d_pointer);
            if (halo::math::vector3d_normalize_with_length(right) == 0.0f) {
                right = *halo::math::globals().global_up3d_pointer;
            }
            halo::math::vector3d_cross_product(up, right, forward);
            halo::math::vector3d_normalize_with_length(up);
            UnitView(object_index).get_camera_position(&launch);
            launch.x = launch.x + forward.i * forward_offset + right.i * right_offset + up.i * up_offset;
            launch.y = launch.y + forward.j * forward_offset + right.j * right_offset + up.j * up_offset;
            launch.z = launch.z + forward.k * forward_offset + right.k * right_offset + up.k * up_offset;
            halo::objects::object_set_position_and_relink(&launch, grenade, 0);
        }
        {
            real speed = unit_tag->grenade_velocity * 0.033333335f;

            velocity.i = speed * aim->i;
            velocity.j = speed * aim->j;
            velocity.k = speed * aim->k;
        }
    }

    if (early) {
        real progress = (real)unit->unit.throwing_grenade_counter / (real)unit->unit.throwing_grenade_duration;

        if (progress < 1.0f) {
            real lob = halo::math::random_real_range(0.02f, 0.046666667f);
            real rest = 1.0f - progress;

            velocity.i = lob * aim->i * rest + velocity.i * progress;
            velocity.j = lob * aim->j * rest + velocity.j * progress;
            velocity.k = lob * aim->k * rest + velocity.k * progress;
        }
    }

    {
        uint8_t *object = halo::objects::object_record_bytes(grenade);
        real_vector3d delta;
        real_point3d camera;

        delta.i = velocity.i - ((struct object *)object)->velocity.i;
        delta.j = velocity.j - ((struct object *)object)->velocity.j;
        delta.k = velocity.k - ((struct object *)object)->velocity.k;
        halo::objects::object_apply_impulse_and_spin(grenade, &delta);
        unit->unit.throwing_grenade_projectile = k_datum_index_none;
        unit->unit.throwing_grenade_state = _unit_throwing_grenade_state_released;
        UnitView(object_index).get_camera_position(&camera);
        if (!halo::objects::object_reposition_to_spawn_location(grenade, &camera, k_datum_index_none)) {
            halo::objects::object_delete(grenade);
            return;
        }
    }
    if (unit->base.network_role == 0 && halo::networking::globals().game_mode == halo::networking::k_game_mode_host && !halo::objects::object_is_delete_pending(grenade)) {
        ((struct object *)halo::objects::object_record_bytes(grenade))->network_role = 0;
        halo::objects::object_type_override_call_0x68(grenade);
        int32_t bits = halo::projectiles::projectile_send_creation(grenade);

        if (bits > 0) {
            halo::networking::network_session_broadcast_to_flagged(bits, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
        }
    }
}

namespace unit_scripting_set_or_drop_weapon_local {

typedef struct unit_set_or_drop_weapon_message {
    int32_t unit_key;
    int32_t weapon_key;
    int32_t force;
} unit_set_or_drop_weapon_message;

}

/**
 * Script/console-callable helper that resolves weapon-name arguments to object ids and updates or drops the
 * unit's selected weapon accordingly.
 *
 * @address 0x56ddb0
 */
void halo::units::unit_scripting_set_or_drop_weapon(message_delta_context *context)
{
    using namespace unit_scripting_set_or_drop_weapon_local;
    unit_set_or_drop_weapon_message decoded;
    datum_index *keys;
    uint32_t unit_index;
    unit_object *unit;
    datum_index weapon = k_datum_index_none;
    datum_index current = k_datum_index_none;
    int16_t current_index;
    int32_t i;

    if (context->state->incremental != 0) {
        halo::networking::message_delta_decode_compound_field_staged(halo::networking::raw_context(context));
        return;
    }
    if (halo::networking::message_delta_decode_compound_field(halo::networking::raw_context(context), &decoded) == 0 || decoded.unit_key == 0) {
        return;
    }
    keys = object_network_id_table->handles;
    unit_index = keys[decoded.unit_key];
    if (unit_index == k_datum_index_none) {
        return;
    }
    unit = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(unit_index, 3));
    if (unit == 0) {
        return;
    }
    if (decoded.weapon_key != 0) {
        weapon = keys[decoded.weapon_key];
    }
    if (UnitView(unit_index).get_weapon_object_index(unit->unit.current_weapon_index) != weapon) {
        for (i = 0; i < 4; i++) {
            if (((datum_index *)&unit->unit.weapons)[i] == weapon) {
                unit->unit.desired_weapon_index = (int16_t)i;
                UnitView(unit_index).ready_desired_weapon(1);
                break;
            }
        }
    }
    current_index = unit->unit.current_weapon_index;
    if (current_index != -1) {
        current = ((datum_index *)&unit->unit.weapons)[current_index];
    }
    if (current == weapon) {
        UnitView(unit_index).drop_current_weapon((uint8_t)decoded.force);
    }
}

/**
 * Engine function unit_set_grenade_type_and_count_delta.
 *
 * Original register convention: in_EAX, in_DX, param_1.
 *
 * @address 0x56d160
 */
int32_t UnitView::set_grenade_type_and_count_delta(int16_t grenade_type, int8_t delta)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    unit->grenade_counts[grenade_type] += delta;
    unit->desired_grenade_index = (int8_t)grenade_type;
    unit->current_grenade_index = (int8_t)grenade_type;
    return unit->grenade_counts[grenade_type];
}

/**
 * Records the unit's grenade-throw aim direction (object.forward.x/y from the caller-supplied direction, z
 * zeroed) and a reference up-vector (the world-up constant), unless the unit is currently seated in
 * something.
 *
 * @address 0x5704d0
 */
void UnitView::set_throw_aim_direction(const real_vector2d *direction_xy)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->parent_object == k_datum_index_none) {
        obj->forward.i = direction_xy->i;
        obj->forward.j = direction_xy->j;
        obj->forward.k = 0.0f;
        obj->up = *halo::math::globals().global_up3d_pointer;
    }
}

/**
 * Spawns the grenade projectile object for a throw and attaches it to the unit's left-hand marker, advancing
 * the throw-state machine.
 *
 * @address 0x56e280
 */
void UnitView::throw_grenade_move_to_hand()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    int8_t grenade_type = unit->current_grenade_index;
    GlobalsGrenade *grenade_table = halo::objects::block_elements<GlobalsGrenade>(global_globals->grenades);

    if (((unit->controlling_player == k_datum_index_none) ||
         ((weapon_bottomless_clip == 0) &&
          ((halo::game::globals().current_engine == 0) || ((game_engine_unknown_aa00 & 4) != 0) ||
           ((motion_sensor_override_value >> 2 & 1) == 0)))) &&
        (unit->actor_index == k_datum_index_none) &&
        ((unit_obj->network_role == 3) || (unit_obj->network_role == 0))) {
        unit->grenade_counts[grenade_type] -= 1;
    }

    if ((halo::networking::globals().game_mode != halo::networking::k_game_mode_host) && (halo::networking::globals().game_mode != halo::networking::k_game_mode_local)) {
        unit->throwing_grenade_projectile = k_datum_index_none;
        unit->throwing_grenade_state = _unit_throwing_grenade_state_in_hand;
        return;
    }

    object_marker hand_marker;
    halo::objects::object_get_node_local_transform(unit_index, "left hand", &hand_marker, 1);

    object_placement_data placement;
    halo::objects::object_placement_data_initialize(&placement, halo::objects::tag_handle(grenade_table[grenade_type].projectile),
                                     unit_index);
    placement.flags |= 2;
    placement.forward = ((struct unit_object *)unit_obj)->unit.aiming_vector;
    halo::math::vector3d_build_perpendicular(placement.up, placement.forward);
    halo::math::vector3d_normalize_with_length(placement.up);
    placement.position = hand_marker.node_transform.position;

    uint32_t projectile_index = halo::objects::object_new_with_datum_role_control(&placement, 3);
    if (projectile_index != k_datum_index_none) {
        halo::objects::object_attach_to_object(unit_index, projectile_index, hand_marker.node_index);
        unit->throwing_grenade_projectile = projectile_index;
        unit->throwing_grenade_state = _unit_throwing_grenade_state_in_hand;
        object *projectile_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(projectile_index)].data;
        reinterpret_cast<projectile_object *>(projectile_obj)->projectile.thrown_grenade = 1;
        return;
    }
    unit->throwing_grenade_state = _unit_throwing_grenade_state_released;
    return;
}

/**
 * Not callable: see the header. Kept only so the address stays listed in the symbol tables.
 *
 * @address 0x571b40
 */
void halo::units::unit_throw_grenade_release(void)
{
}

/**
 * Attempts to give the unit one more grenade of its currently selected type, up to the type's maximum,
 * updating related UI/HUD state.
 *
 * Original register convention: param_1, unaff_EBX.
 *
 * @address 0x56d080
 */
uint8_t halo::units::unit_try_give_grenade(uint32_t tag_source_index, uint32_t unit_index)
{
    Equipment *tag_data = halo::objects::tag_as<Equipment>(((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(tag_source_index)].data->definition_tag);
    int16_t grenade_type = tag_data->grenade_type;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    GlobalsGrenade *grenade = &halo::objects::block_element<GlobalsGrenade>(global_globals->grenades, grenade_type);
    if ((grenade != nullptr) && (unit->grenade_counts[grenade_type] < grenade->maximum_count)) {
        unit->grenade_counts[grenade_type] += 1;
        set_flag(unit_obj->flags, objects::object_flag::changed);
        int32_t local_player = halo::game::player_index_from_unit_index(unit_index);
        if (local_player != -1) {
            uint32_t local_player2 = (uint32_t)halo::game::player_index_from_unit_index(unit_index);
            if (halo::game::player_at(local_player2)->local_player_index != -1) {
                halo::items::equipment_pickup_play_sound(tag_source_index);
            }
        }
        halo::objects::object_delete(tag_source_index);
        return 1;
    }
    return 0;
}

namespace unit_try_ready_weapon_local {

static int unit_animation_state_allows_melee(int8_t state)
{
    if (is_scripted_animation_state(animation_state_id(state))) {
        return 0;
    }
    return 1;
}

}

/**
 * Engine function unit_try_ready_weapon.
 *
 * @address 0x569a20
 */
uint8_t UnitView::try_ready_weapon(uint8_t forced, const real_vector2d *direction)
{
    using namespace unit_try_ready_weapon_local;
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)unit);
    int8_t state = (int8_t)(uint8_t)unit->unit.animation_state;
    uint8_t airborne = 0;
    int16_t new_state;

    if (!unit_animation_state_allows_melee(state)) {
        return 0;
    }
    if (unit->base.type == _object_type_biped) {
        airborne = test_flag(halo::units::biped_data_of(unit)->flags, units::biped_flag::airborne);
    }
    if (forced) {
        new_state = animation_state_value(unit_animation_state_id::unknown_20);
    } else if (state == 0x28) {
        new_state = animation_state_value(unit_animation_state_id::unknown_29);
    } else {
        new_state = (int16_t)(0x1e + (airborne != 0));
    }
    if (!UnitView(unit_index).try_set_animation_state(new_state) && !forced) {
        return 0;
    }
    if (test_flag(unit_tag->unit_flags, tags::unit_tag_flag::melee_attack_is_fatal)) {
        unit->unit.animation_state = animation_state_value(unit_animation_state_id::ready_weapon);
    }
    if (direction != 0) {
        UnitView(unit_index).set_throw_aim_direction(direction);
    }
    if (forced) {
        unit->unit.melee_state = _unit_melee_state_lunge;
        unit->unit.melee_damage_countdown = 0;
        return 1;
    }
    unit->unit.melee_state = _unit_melee_state_ready;
    return 1;
}

namespace unit_try_ready_weapon_variant_local {

static int unit_animation_state_allows_melee(int8_t state)
{
    if (is_scripted_animation_state(animation_state_id(state))) {
        return 0;
    }
    return 1;
}

}

/**
 * Engine function unit_try_ready_weapon_variant.
 *
 * @address 0x569b30
 */
uint8_t UnitView::try_ready_weapon_variant(const real_vector2d *direction)
{
    using namespace unit_try_ready_weapon_variant_local;
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));

    if (!unit_animation_state_allows_melee((int8_t)(uint8_t)unit->unit.animation_state)) {
        return 0;
    }
    if (unit->base.type == _object_type_biped && test_flag(halo::units::biped_data_of(unit)->flags, units::biped_flag::airborne)) {
        return 0;
    }
    if (!UnitView(unit_index).try_set_animation_state(animation_state_value(unit_animation_state_id::unknown_27))) {
        return 0;
    }
    if (direction != 0) {
        UnitView(unit_index).set_throw_aim_direction(direction);
    }
    return 1;
}

/**
 * Switches the unit's currently selected secondary item (e.g. grenade type) to param_2 if none is already
 * selected, releasing the previous selection when requested.
 *
 * @address 0x56d1a0
 */
uint8_t UnitView::try_select_equipment(uint32_t new_equipment_object_index, int16_t release_current)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    if ((unit->equipment_object_index != k_datum_index_none) && (release_current == 1)) {
        halo::objects::object_delete(unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    if (unit->equipment_object_index == k_datum_index_none) {
        halo::objects::object_unlink_cluster_or_notify_parent(new_equipment_object_index);
        object *new_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(new_equipment_object_index)].data;
        Object *new_def = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(new_obj->definition_tag)].data;
        if ((halo::objects::tag_handle(new_def->model) != k_datum_index_none) && (!test_flag(new_obj->flags, objects::object_flag::no_collision))) {
            halo::objects::object_for_each_light_attachment(new_equipment_object_index, 1, 0);
        }
        set_flag(new_obj->flags, objects::object_flag::no_collision);
        ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(new_equipment_object_index)].flags &= 0xfd;

        int32_t local_player = halo::game::player_index_from_unit_index(unit_index);
        if (local_player != -1) {
            uint32_t local_player2 = (uint32_t)halo::game::player_index_from_unit_index(unit_index);
            if (halo::game::player_at(local_player2)->local_player_index != -1) {
                halo::items::equipment_pickup_play_sound(new_equipment_object_index);
            }
        }
        halo::items::item_set_holder(new_equipment_object_index, unit_index);
        unit->equipment_object_index = new_equipment_object_index;
        return 1;
    }
    return 0;
}

/**
 * Compares a reference weapon against every weapon the unit carries of the same type, checking whether it is
 * the current weapon and has the lowest score field.
 *
 * Original register convention: in_EAX, in_ECX.
 *
 * @address 0x56dae0
 */
uint8_t halo::units::unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index)
{
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    object *reference_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(reference_weapon_index)].data;

    if (unit->current_weapon_index == -1) {
        return 0;
    }
    datum_index current_weapon = unit->weapons[unit->current_weapon_index];
    if (current_weapon == k_datum_index_none) {
        return 0;
    }

    uint8_t result = 1;
    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index == k_datum_index_none) {
            continue;
        }
        object *slot_weapon = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_index)].data;
        if (reference_obj->definition_tag != slot_weapon->definition_tag) {
            continue;
        }
        if (slot == unit->current_weapon_index) {
            float slot_score = ((struct unit_object *)slot_weapon)->unit.aiming_vector.j;
            if (!(slot_score < 0.0f) && (slot_score != 0.0f)) {
                float ref_score = ((struct unit_object *)reference_obj)->unit.aiming_vector.j;
                if (ref_score < slot_score) {
                    continue;
                }
            }
        }
        result = 0;
    }
    return result;
}

}
