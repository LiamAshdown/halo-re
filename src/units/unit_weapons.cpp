#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/core/lcg.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "effects.h"
#include "crt.h"

extern "C" {
extern data_array *object_data;
extern tag_instance *tag_instances;
extern game_engine_definition *current_game_engine;
extern int16_t network_game_mode;
extern object_type_definition *object_type_definitions[k_maximum_object_types];
extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag, datum_index role);
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role);
extern void object_delete(uint32_t object_index);
extern void object_delete_unparented(uint32_t object_index);
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings);
extern Globals *global_globals;
extern real vector2d_normalize_with_length(real_vector2d *v);
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
extern void unit_invalidate_local_player_zoom_level(uint32_t unit_index);
extern void weapon_action_notify_for_unit(datum_index unit_index, int32_t action_code);
extern uint32_t weapon_prevents_grenade_throwing(datum_index item_index);
extern void weapon_reset_triggers(datum_index weapon_index);
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask);
extern char k_empty_string[];
extern int32_t weapon_put_away(datum_index item_index, int8_t force);
extern uint8_t weapon_is_out_of_ammo(datum_index item_index);
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index);
extern real_vector3d *global_origin3d_pointer;
extern random_seed random_seed_global;
extern char s_left_hand_marker[];
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location);
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback);
extern void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name, uint32_t object_index, char *object_marker_name);
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out, random_seed *seed, real lo, real hi);
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity, real_vector3d *out_angular_velocity);
extern void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer);
extern uint8_t object_reposition_to_spawn_location(uint32_t object_index, real_point3d *target_position, uint32_t ignore_object_index);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern void animation_get_frame_orientations(ModelAnimationsAnimation *animation, GBXModel *model, int16_t frame, real_orientation *out_orientations);
extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out);
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);
extern real weapon_get_zoom_magnification(datum_index item_index, int16_t zoom_level);
extern void animation_get_frame_info_distance(ModelAnimationsAnimation *animation, float *dx_to_key_frame, float *dx_total);
extern uint8_t *local_player_globals;
extern data_array *player_data;
extern uint8_t game_engine_notify_weapon_ready_state_change(datum_index unit_index, datum_index weapon_index);
extern void item_set_holder(uint32_t item_index, datum_index holder_index);
extern void unit_set_local_player_weapon_index(datum_index unit, int16_t weapon_index);
extern game_time_globals *game_time;
extern char *weapon_get_label(datum_index item_index);
extern void weapon_ready(datum_index item_index);
extern void object_mark_pending_delete(uint32_t object_index);
extern real_vector3d *global_up3d_pointer;
extern uint8_t network_message_scratch[0x7ff8];
extern network_server_globals *network_server;
extern real vector3d_normalize_with_length(real_vector3d *v);
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
extern void object_get_position(real_point3d *out, uint32_t object_index);
extern void object_set_position_and_relink(real_point3d *position, uint32_t object_index, bsp_leaf_reference *location);
extern uint32_t actor_compute_grenade_throw_vector(datum_index actor_index, real_point3d *grenade_position, real_vector3d *out_vector);
extern real random_real_range(real min, real max);
extern void object_apply_impulse_and_spin(uint32_t object_index, real_vector3d *delta_velocity);
extern uint8_t object_is_delete_pending(uint32_t object_index);
extern void object_type_override_call_0x68(uint32_t object_index);
extern int32_t projectile_send_creation(uint32_t projectile_index);
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused);
extern uint8_t *object_network_id_table;
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context);
extern uint8_t weapon_bottomless_clip;
extern uint32_t game_engine_unknown_aa00;
extern uint32_t motion_sensor_override_value;
extern void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir);
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index, int16_t marker_index);
extern int32_t player_index_from_unit_index(uint32_t unit_index);
extern void equipment_pickup_play_sound(uint32_t object_index);
}

namespace halo::units {

namespace unit_add_initial_weapons_local {

static object *object_from_index(uint32_t object_index)
{
    return ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
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
    Unit *tag = (Unit *)tag_instances[halo::datum_slot(object_from_index(unit_index)->definition_tag)].data;
    int32_t i;

    for (i = 0; (int32_t)(int16_t)i < (int32_t)tag->weapons.count; i++) {
        uint32_t weapon_tag = *(uint32_t *)&((UnitWeapon *)tag->weapons.pointer)[i].weapon.tag_id;
        object_placement_data placement;
        uint32_t role;
        datum_index weapon_index;
        int32_t network_role;

        if (weapon_tag == k_datum_index_none) {
            continue;
        }
        object_placement_data_initialize(&placement, weapon_tag, unit_index);
        role = 3;
        if (network_game_mode == 2 &&
            object_type_definitions[((Object *)tag_instances[halo::datum_slot(placement.definition_tag)].data)->object_type]
                ->network_delta_message_type != -1) {
            role = 0;
        }
        weapon_index = object_new_with_datum_role_control(&placement, role);
        if (weapon_index == k_datum_index_none) {
            continue;
        }
        if (current_game_engine != 0 &&
            UnitView(unit_index).has_weapon_of_type((int32_t)object_from_index(weapon_index)->definition_tag)) {
            object_delete(weapon_index);
            continue;
        }
        if (::halo::units::unit_pickup_weapon(0, weapon_index, unit_index)) {
            continue;
        }
        network_role = object_from_index(weapon_index)->network_role;
        if (network_role == 0) {
            object_delete_unparented(weapon_index);
        } else if (network_role != 3) {
            continue;
        }
        object_delete_recursive(weapon_index, 0);
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    Unit *unit_tag = (Unit *)tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int16_t grenade_type = unit->current_grenade_index;
    if ((grenade_type == -1) || (unit->grenade_counts[grenade_type] <= 0)) {
        return 0;
    }

    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 0;
    default:
        if (weapon_prevents_grenade_throwing(current_weapon)   != 0) {
            return 0;
        }
        if (current_weapon != k_datum_index_none) {
            weapon_reset_triggers(current_weapon);
        }
        object *biped_check = object_try_and_get(unit_index, _object_mask_biped);
        if (biped_check != (object *)0) {
            *((uint8_t *)biped_check + 0x505) = 0;
        }
        unit->replacement_animation_state = 0;
        unit->overlays[0].animation_index = -1;

        if (UnitView(unit_index).try_set_animation_state(0x21) == 0) {
            return 0;
        }

        unit->throwing_grenade_state = 1;
        unit->throwing_grenade_counter = 0;
        uint8_t *graph = (uint8_t *)tag_instances[halo::datum_slot(unit_tag->base.animation_graph.tag_id.index)].data;
        ModelAnimationsAnimation *animations = (ModelAnimationsAnimation *)(*(uint8_t **)&((ModelAnimations *)graph)->animations.pointer);
        unit->throwing_grenade_duration = (animations[unit_obj->animation_index].key_frame_index - unit_obj->animation_frame) + 1;

        if (direction != 0) {
            UnitView(unit_index).set_throw_aim_direction(direction);
        } else {
            real_vector2d aim;

            aim.i = ((struct unit_object *)unit_obj)->unit.aiming_vector.i;
            aim.j = ((struct unit_object *)unit_obj)->unit.aiming_vector.j;
            if (0.0f < vector2d_normalize_with_length(&aim)) {
                UnitView(unit_index).set_throw_aim_direction(&aim);
            }
        }
        weapon_action_notify_for_unit(unit_index, 0x11);
        unit_invalidate_local_player_zoom_level(unit_index);
        uint8_t *grenade_table_entry = ((uint8_t *)global_globals->grenades.pointer) + (int8_t)grenade_type * 0x44;
        if (*(int32_t *)(grenade_table_entry + 0x10) != -1) {
            effect_new_on_object(unit_index, *(datum_index *)(grenade_table_entry + 0x10), unit_index, -1,
                0.0f, 0.0f, 0, 0);
        }
        return 1;
    }
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
        object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
        weapon_label = (char *)(tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data) + 0x30c;
    }

    if (UnitView(unit_index).set_or_test_seat_and_weapon_label(seat_name, weapon_label, 0) == 0) {
        return 0;
    }
    if (current_game_engine != 0 && *(void **)((uint8_t *)current_game_engine + 0x60) != (void *)0) {
        uint8_t (*permission)(uint32_t, uint32_t) =
            *(uint8_t (**)(uint32_t, uint32_t))((uint8_t *)current_game_engine + 0x60);

        return permission(unit_index, weapon_index);
    }
    return 1;
}

/**
 * Clears the unit's currently selected secondary item field, releasing it first via FUN_0056ed00.
 *
 * Original register convention: in_ECX.
 *
 * @address 0x56d2c0
 */
void UnitView::clear_selected_equipment()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    int16_t count = 0;
    for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
        datum_index weapon_index = unit->weapons[i];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
            uint8_t *weapon_tag = (uint8_t *)tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
            if ((*(uint8_t *)(weapon_tag + 0x308) & 0x10) == 0) {
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if (unit->zoom_level != -1) {
        int16_t slot = unit->current_weapon_index;
        if (slot != -1 && unit->weapons[slot] != k_datum_index_none) {
            object *weapon = ((object_header *)object_data->data)[unit->weapons[slot] & 0xffff].data;
            void *weapon_tag = tag_instances[halo::datum_slot(weapon->definition_tag)].data;
            if ((*(uint32_t *)((uint8_t *)weapon_tag + 0x308) & 0x4000) != 0) {
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

    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot = unit->current_weapon_index;

    if (slot == -1) {
        return 0;
    }

    datum_index weapon_index = unit->weapons[slot];
    if (weapon_index == k_datum_index_none) {
        return 0;
    }

    object *weapon = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (unit->current_weapon_index == -1) {
        return 0;
    }
    datum_index weapon_index = unit->weapons[unit->current_weapon_index];
    if (weapon_index == k_datum_index_none) {
        return 0;
    }
    object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
    int8_t weapon_kind = *(int8_t *)((uint8_t *)weapon_obj + 0x261);
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int16_t next_slot = UnitView(unit_index).find_next_zone_permitted_weapon_slot((int32_t)(uint16_t)unit->current_weapon_index, 1);

    if ((current_weapon != k_datum_index_none) &&
        ((next_slot != unit->current_weapon_index) || force) &&
        ((((object_header *)object_data->data)[halo::datum_slot(current_weapon)].data->flags & 1) == 0)) {
        if ((uint8_t)weapon_put_away(current_weapon, (int8_t)force) != 0) {
            weapon_action_notify_for_unit(unit_index, 0xd);
            UnitView(unit_index).drop_object_from_hand(current_weapon);
            unit->weapons[unit->current_weapon_index] = k_datum_index_none;
            unit->current_weapon_index = -1;
            unit->desired_weapon_index = UnitView(unit_index).find_next_zone_permitted_weapon_slot(-1, 0);
            if (((uint8_t)weapon_is_out_of_ammo(current_weapon) == 0) && (network_game_mode == 0)) {
                object_delete(current_weapon);
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
    uint8_t *grenade_type_table = (uint8_t *)global_globals->grenades.pointer;
    int32_t table_offset = 0;
    int grenade_type;

    for (grenade_type = 0; grenade_type < 2; grenade_type++) {
        object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        int8_t *count = &unit->grenade_counts[grenade_type];

        while (*count > 0) {
            object_placement_data placement;
            datum_index projectile_tag = *(datum_index *)(grenade_type_table + table_offset + 0x30);
            uint32_t role = 3;

            object_placement_data_initialize(&placement, projectile_tag, unit_index);

            if (network_game_mode == 2) {
                Object *proj_tag = (Object *)tag_instances[halo::datum_slot(placement.definition_tag)].data;
                object_type_definition *type_def = object_type_definitions[proj_tag->object_type];
                if (((struct object_type_definition *)type_def)->network_delta_message_type != -1) {
                    role = 0;
                }
            }

            {
                uint32_t projectile_index = object_new_with_datum_role_control(&placement, role);
                if (projectile_index != k_datum_index_none) {
                    object_unlink_cluster_or_notify_parent(projectile_index);
                    UnitView(unit_index).drop_object_from_hand(projectile_index);
                }
            }

            *count -= 1;
        }
        table_offset += 0x44;
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
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

            if (weapon_is_out_of_ammo(dropped) == 0 && network_game_mode == 0) {
                object_delete(dropped);
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if ((weapon_index != k_datum_index_none) && (slot != unit->current_weapon_index)) {
            object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
            if (weapon_obj->network_role == 0) {
                object_delete_unparented(weapon_index);
                object_delete_recursive(weapon_index, 0);
            } else if (weapon_obj->network_role == 3) {
                object_delete_recursive(weapon_index, 0);
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

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[halo::datum_slot((h))].data)
/**
 * Engine function unit_drop_object_from_hand.
 *
 * @address 0x56ed00
 */
void UnitView::drop_object_from_hand(uint32_t object_index)
{
    uint32_t unit_index = datum_handle;
    uint8_t *unit = OBJECT_DATA(unit_index);
    uint8_t *dropped = OBJECT_DATA(object_index);
    real_vector3d toss;
    real_vector3d root_velocity;
    real_point3d camera;
    real speed;
    int32_t role;

    if (((struct object *)dropped)->parent_object == k_datum_index_none) {
        uint8_t *object;
        uint8_t *object_tag;

        object_set_cluster_and_parent(object_index, 0);
        object = OBJECT_DATA(object_index);
        object_tag = (uint8_t *)tag_instances[halo::datum_slot(*(datum_index *)object)].data;
        if (*(int32_t *)&((struct Unit *)object_tag)->base.model.tag_id != -1) {
            if ((uint8_t)((struct object *)object)->flags & 1) {
                object_for_each_light_attachment(object_index, 0, 1);
            }
            if (*(int32_t *)&((struct Unit *)object_tag)->base.model.tag_id != -1) {
                clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                ((object_header *)object_data->data)[halo::datum_slot(object_index)].flags |= 2;
            }
        }
        object_reorient_relative_to_marker(unit_index, s_left_hand_marker, object_index, k_empty_string);
    }
    ((struct unit_object *)OBJECT_DATA(object_index))->unit.actor_index &= ~3u;
    object_snap_to_parent_marker_and_detach(object_index);
    *(real_vector3d *)&((struct object *)dropped)->velocity.i = *global_origin3d_pointer;
    *(real_vector3d *)&((struct object *)dropped)->angular_velocity.i = *global_origin3d_pointer;

    vector3d_randomize_direction((real_point3d *)&((struct unit_object *)unit)->unit.aiming_vector, &toss, &random_seed_global, 0.0f, 0.39269909f);
    random_seed_global = halo::advance_random_seed(random_seed_global);
    speed = (real)(int32_t)((uint32_t)random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale * 0.013333336f + 0.026666667f;
    toss.i *= speed;
    toss.j *= speed;
    toss.k *= speed;
    object_get_root_object_velocities(unit_index, &root_velocity, 0);
    toss.i += root_velocity.i;
    ((struct unit_object *)dropped)->unit.swarm_previous_unit_index = unit_index;
    toss.j += root_velocity.j;
    toss.k += root_velocity.k;
    item_accelerate(object_index, &toss, 0);

    UnitView(unit_index).get_camera_position(&camera);
    if (!object_reposition_to_spawn_location(object_index, &camera, k_datum_index_none) && current_game_engine == 0) {
        object_delete(object_index);
    }
    if (((unit_object *)unit)->unit.flags & 0x100000) {
        role = ((struct object *)OBJECT_DATA(object_index))->network_role;
        if (role == 0) {
            object_delete_unparented(object_index);
            object_delete_recursive(object_index, 0);
        } else if (role == 3) {
            object_delete_recursive(object_index, 0);
        }
    }
}
#undef OBJECT_DATA

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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int32_t slot = 0;

    for (;;) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
            Weapon *weapon_tag = (Weapon *)tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot = 0;

    for (;;) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
            Weapon *weapon_tag = (Weapon *)tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
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

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[halo::datum_slot((h))].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[halo::datum_slot((t))].data)
/**
 * Engine function unit_find_weapon_marker_transform.
 *
 * @address 0x5640a0
 */
uint8_t UnitView::find_weapon_marker_transform(uint32_t vehicle_index, int16_t seat_index, real_point3d *out_entry, real_point3d *out_seat, real_point3d *out_hint)
{
    uint32_t unit_index = datum_handle;
    uint8_t *unit_tag = TAG_DATA(*(datum_index *)OBJECT_DATA(unit_index));
    uint8_t *model = TAG_DATA(*(datum_index *)&((struct Unit *)unit_tag)->base.model.tag_id);
    uint8_t *graph = TAG_DATA(*(datum_index *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id);
    uint8_t *seat = (uint8_t *)((struct Unit *)TAG_DATA(*(datum_index *)OBJECT_DATA(vehicle_index)))->seats.pointer + seat_index * 0x11c;
    uint8_t *block = 0;
    int16_t i;
    int16_t enter_animation;
    ModelAnimationsAnimation *animation;
    object_marker seat_marker;
    object_marker hint_marker;
    real_orientation orientations[k_maximum_nodes_per_model];
    real_matrix4x3 root;
    real_matrix4x3 entry;
    char hint_name[0x100];

    for (i = 0; i < *(int32_t *)&((ModelAnimations *)graph)->units.count; i++) {
        if (_stricmp((char *)(*(uint8_t **)&((ModelAnimations *)graph)->units.pointer + i * 0x64), (char *)(seat + 0x4)) == 0) {
            block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer + i * 0x64;
            break;
        }
    }
    if (block == 0 || *(int32_t *)(block + 0x40) <= 7) {
        return 0;
    }
    enter_animation = (*(int16_t **)(block + 0x44))[7];
    if (enter_animation == -1) {
        return 0;
    }
    animation = (ModelAnimationsAnimation *)(*(uint8_t **)&((ModelAnimations *)graph)->animations.pointer + enter_animation * 0xb4);
    object_get_node_local_transform(vehicle_index, (char *)(seat + 0x24), &seat_marker, 1);
    animation_get_frame_orientations(animation, (GBXModel *)model, 0, orientations);
    matrix4x3_from_quaternion(&orientations[0].rotation, &root);
    root.position = orientations[0].translation;
    matrix4x3_multiply(&seat_marker.node_transform, &root, &entry);
    strcpy(hint_name, (char *)(seat + 0x24));
    strcat(hint_name, " enter-hint");
    object_get_node_local_transform(vehicle_index, hint_name, &hint_marker, 1);
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
#undef OBJECT_DATA
#undef TAG_DATA

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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    int16_t slot = unit->current_weapon_index;

    if (slot != -1 && unit->weapons[slot] != k_datum_index_none) {
        return weapon_get_zoom_magnification(unit->weapons[slot], zoom_level);
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    return unit->current_grenade_index;
}

/**
 * Returns a label string for the unit's currently held weapon, or 'unarmed' if no weapon is equipped.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56dfd0
 */
char * UnitView::get_current_weapon_label()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (unit->current_weapon_index != -1) {
        datum_index weapon_index = unit->weapons[unit->current_weapon_index];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
            return (char *)(tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data) + 0x30c;
        }
    }
    return (char *)"unarmed";
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    return unit->grenade_counts[grenade_type];
}

/**
 * Engine function unit_get_weapon_marker_indices.
 *
 * @address 0x5642c0
 */
uint8_t UnitView::get_weapon_marker_indices(uint8_t use_alternate, uint32_t out_dx_to_key_frame, uint32_t out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[halo::datum_slot(obj->definition_tag)].data;
    void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)&((ModelAnimations *)graph)->units.pointer;
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                 unit->animation_weapon_index * 0xbc);

    int32_t raw_index = (use_alternate ? 3 : 0) + 0x27;
    int16_t animation_index = -1;
    if (raw_index < (int32_t)weapon_anim->animations.count) {
        animation_index = *(int16_t *)((uint8_t *)weapon_anim->animations.pointer + raw_index * 2);
    }
    if (animation_index == -1) {
        return 0;
    }

    uint8_t *animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
    ModelAnimationsAnimation *anim = (ModelAnimationsAnimation *)(animations + animation_index * 0xb4);

    animation_get_frame_info_distance(anim, (float *)out_dx_to_key_frame, (float *)out_dx_total);

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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    for (int16_t slot = 0; slot < k_maximum_weapons_per_unit; slot++) {
        datum_index weapon_index = unit->weapons[slot];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
            if ((int32_t)weapon_obj->definition_tag == weapon_group_tag) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Empty stub; performs no operation in this build.
 *
 * @address 0x56d070
 */
void unit_inventory_get_weapon(void)
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
uint8_t unit_lacks_weapon_type_of(uint32_t reference_object_index, uint32_t unit_index)
{
    object *reference_obj = ((object_header *)object_data->data)[halo::datum_slot(reference_object_index)].data;
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    for (int32_t i = 0; i < k_maximum_weapons_per_unit; i++) {
        datum_index weapon_index = unit->weapons[i];
        if (weapon_index != k_datum_index_none) {
            object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
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
uint8_t unit_local_player_weapon_flag_check(void)
{
    if (*(int16_t *)(local_player_globals + 0xc) == 1) {
        int32_t slot = -1;
        if (*(int32_t *)(local_player_globals + 4) != -1) {
            slot = 0;
        }
        if (slot != -1 && slot < 1) {
            uint32_t player_handle = *(uint32_t *)(local_player_globals + 4 + slot * 4);
            if (player_handle != (uint32_t)-1) {
                datum_index unit_handle = *(datum_index *)((uint8_t *)player_data->data +
                                                             halo::datum_slot(player_handle) * 0x200 + 0x34);
                if (unit_handle != k_datum_index_none) {
                    return UnitView(unit_handle).current_weapon_has_flag();
                }
            }
        }
    }
    return 0;
}

/**
 * Calls FUN_00565f90 when the implicit weapon/object index is valid, used as a small guard before
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
        UnitView((uint32_t)object_index).try_set_animation_state(0x25);
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
        UnitView((uint32_t)object_index).try_set_animation_state(0x26);
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

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
uint8_t unit_pickup_weapon(int16_t pickup_mode, uint32_t weapon_index, uint32_t unit_index)
{
    object *unit_obj = object_try_and_get(unit_index, _object_mask_unit);
    object *weapon_obj = object_try_and_get(weapon_index, _object_mask_weapon);
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if (network_game_mode == 1) {
        char *seat_name = UnitView(unit_index).get_seat_or_state_name();
        char *weapon_label = k_empty_string;
        if (weapon_index != k_datum_index_none) {
            object *label_src = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
            weapon_label = (char *)(tag_instances[halo::datum_slot(label_src->definition_tag)].data) + 0x30c;
        }
        if (UnitView(unit_index).set_or_test_seat_and_weapon_label(seat_name, weapon_label, 0) == 0) {
            object *check = object_try_and_get(unit_index, _object_mask_unit);
            if ((check != (object *)0) && (((unit_data *)((uint8_t *)check + k_unit_data_offset))->animation_state == 0x1b)) {
                UnitView(unit_index).detach_from_seat(1, 1, 0);
            }
        }
    }

    if ((test_flag(weapon_obj->flags, objects::object_flag::needs_cluster_update)) && (weapon_obj->parent_object == k_datum_index_none)) {
        if (UnitView(unit_index).check_weapon_use_permission(weapon_index) != 0  ) {
            if (game_engine_notify_weapon_ready_state_change(unit_index, weapon_index) != 0  ) {
                if (pickup_mode == 2) {
                    UnitView(unit_index).drop_inventory_weapons_except_current();
                }
                int16_t slot = UnitView(unit_index).find_empty_weapon_slot();
                if (slot != -1) {
                    object_unlink_cluster_or_notify_parent(weapon_index);
                    Object *weapon_def = (Object *)tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
                    if ((*(uint32_t *)&weapon_def->model.tag_id != k_datum_index_none) &&
                        (!test_flag(weapon_obj->flags, objects::object_flag::no_collision))) {
                        object_for_each_light_attachment(weapon_index, 1, 0);
                    }
                    set_flag(weapon_obj->flags, objects::object_flag::no_collision);
                    ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].flags &= 0xfd;
                    item_set_holder(weapon_index, unit_index);
                    unit->weapons[slot] = weapon_index;
                    unit->weapon_ready_ticks[slot] = 0;

                    if (pickup_mode != 0) {
                        if (pickup_mode == 1) {
                            if (!test_flag(unit->control_flags, units::unit_control_flag::primary_trigger)) {
                                unit_set_local_player_weapon_index(unit_index, slot);
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

#define OBJECT_HEADER(h) (((object_header *)object_data->data)[halo::datum_slot((h))])
#define OBJECT_TAG(o) ((uint8_t *)tag_instances[halo::datum_slot(*(datum_index *)(o))].data)
/**
 * Engine function unit_ready_desired_weapon.
 *
 * @address 0x56d6e0
 */
void UnitView::ready_desired_weapon(uint8_t force)
{
    uint32_t unit_index = datum_handle;
    uint8_t *unit = (uint8_t *)OBJECT_HEADER(unit_index).data;
    uint8_t *unit_tag = OBJECT_TAG(unit);
    datum_index desired_weapon = k_datum_index_none;
    int16_t current = ((unit_object *)unit)->unit.current_weapon_index;

    if (((unit_object *)unit)->unit.desired_weapon_index != -1) {
        desired_weapon = *(datum_index *)(unit + 0x2f8 + ((unit_object *)unit)->unit.desired_weapon_index * 4);
    }
    if (current != -1) {
        datum_index weapon = *(datum_index *)(unit + 0x2f8 + current * 4);

        if (weapon != k_datum_index_none && weapon_put_away(weapon, (int8_t)force) != 0) {
            uint8_t *weapon_obj;

            object_snap_to_parent_marker_and_detach(weapon);
            object_unlink_cluster_or_notify_parent(weapon);
            object_mark_pending_delete(weapon);
            weapon_obj = (uint8_t *)OBJECT_HEADER(weapon).data;
            if (*(int32_t *)(OBJECT_TAG(weapon_obj) + 0x34) != -1 && (weapon_obj[0x10] & 1) == 0) {
                object_for_each_light_attachment(weapon, 1, 0);
            }
            *(uint32_t *)(weapon_obj + 0x10) |= 1;
            OBJECT_HEADER(weapon).flags &= 0xfd;
            item_set_holder(weapon, unit_index);
            ((unit_object *)unit)->unit.current_weapon_index = -1;
        }
    }
    if (((unit_object *)unit)->unit.current_weapon_index != -1) {
        UnitView(unit_index).validate_and_clear_weapon_switch();
        return;
    }
    if (desired_weapon == k_datum_index_none) {
        UnitView(unit_index).set_or_test_seat_and_weapon_label(UnitView(unit_index).get_seat_or_state_name(), (char *)"unarmed", 1);
        ((unit_object *)unit)->unit.current_weapon_index = -1;
        UnitView(unit_index).validate_and_clear_weapon_switch();
        return;
    }
    {
        char *weapon_label = weapon_get_label(desired_weapon);
        uint8_t *graph;
        uint8_t *weapon_anim;
        uint8_t *weapon_obj;
        uint8_t *weapon_tag;
        int16_t desired;

        UnitView(unit_index).set_or_test_seat_and_weapon_label(UnitView(unit_index).get_seat_or_state_name(), weapon_label, 1);
        graph = (uint8_t *)tag_instances[*(datum_index *)&((struct Unit *)unit_tag)->base.animation_graph.tag_id & 0xffff].data;
        weapon_anim = *(uint8_t **)(*(uint8_t **)&((ModelAnimations *)graph)->units.pointer + (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_definition_index * 0x64 + 0x5c) +
            (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_weapon_index * 0xbc;
        object_set_cluster_and_parent(desired_weapon, 0);
        weapon_obj = (uint8_t *)OBJECT_HEADER(desired_weapon).data;
        weapon_tag = OBJECT_TAG(weapon_obj);
        if (*(int32_t *)(weapon_tag + 0x34) != -1) {
            if ((weapon_obj[0x10] & 1) != 0) {
                object_for_each_light_attachment(desired_weapon, 0, 1);
            }
            if (*(int32_t *)(weapon_tag + 0x34) != -1) {
                *(uint32_t *)(weapon_obj + 0x10) &= ~1u;
                OBJECT_HEADER(desired_weapon).flags |= 2;
            }
        }
        object_reorient_relative_to_marker(unit_index, (char *)(weapon_anim + 0x40), desired_weapon,
            (char *)(weapon_anim + 0x20));
        desired = ((unit_object *)unit)->unit.desired_weapon_index;
        ((unit_object *)unit)->unit.current_weapon_index = desired;
        if (desired != -1) {
            *(int32_t *)(unit + 0x308 + desired * 4) = game_time->game_time;
        }
        weapon_ready(desired_weapon);
        UnitView(unit_index).validate_and_clear_weapon_switch();
    }
}
#undef OBJECT_HEADER
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
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
            item_set_holder(unit->weapons[i], unit_index);
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    datum_index equipment_index = unit->equipment_object_index;
    if (equipment_index == k_datum_index_none) {
        return;
    }
    object *equipment_obj = ((object_header *)object_data->data)[halo::datum_slot(equipment_index)].data;
    if (equipment_obj->network_role == 0) {
        object_delete_unparented(equipment_index);
    } else if (equipment_obj->network_role != 3) {
        goto clear;
    }
    object_delete_recursive(equipment_index, 0);
clear:
    unit->equipment_object_index = k_datum_index_none;
    return;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[halo::datum_slot((h))].data)
/**
 * Engine function unit_release_thrown_grenade.
 *
 * @address 0x56e440
 */
void UnitView::release_thrown_grenade(uint8_t early)
{
    uint32_t object_index = datum_handle;
    uint8_t *unit = OBJECT_DATA(object_index);
    uint8_t *unit_tag = (uint8_t *)tag_instances[halo::datum_slot(*(datum_index *)unit)].data;
    real_vector3d *aim = (real_vector3d *)&((struct unit_object *)unit)->unit.aiming_vector;
    datum_index grenade;
    real_vector3d velocity;

    if ((uint8_t)((struct unit_object *)unit)->unit.throwing_grenade_state != 2) {
        return;
    }
    grenade = ((unit_object *)unit)->unit.throwing_grenade_projectile;
    if (grenade == k_datum_index_none) {
        unit[0x28d] = 3;
        return;
    }
    object_snap_to_parent_marker_and_detach(grenade);
    if (((unit_object *)unit)->unit.actor_index != k_datum_index_none) {
        real_point3d position;

        object_get_position(&position, ((unit_object *)unit)->unit.throwing_grenade_projectile);
        actor_compute_grenade_throw_vector(((unit_object *)unit)->unit.actor_index, &position, &velocity);
    } else {
        if (((unit_object *)unit)->unit.controlling_player != k_datum_index_none) {
            uint8_t *info = (uint8_t *)global_globals->player_information.pointer;
            real_vector3d forward = *aim;
            real_vector3d right;
            real_vector3d up;
            real_point3d launch;
            real forward_offset = *(float *)(info + 0x68);
            real right_offset = *(float *)(info + 0x6c);
            real up_offset = *(float *)(info + 0x70);

            vector3d_cross_product(&right, &forward, global_up3d_pointer);
            if (vector3d_normalize_with_length(&right) == 0.0f) {
                right = *global_up3d_pointer;
            }
            vector3d_cross_product(&up, &right, &forward);
            vector3d_normalize_with_length(&up);
            UnitView(object_index).get_camera_position(&launch);
            launch.x = launch.x + forward.i * forward_offset + right.i * right_offset + up.i * up_offset;
            launch.y = launch.y + forward.j * forward_offset + right.j * right_offset + up.j * up_offset;
            launch.z = launch.z + forward.k * forward_offset + right.k * right_offset + up.k * up_offset;
            object_set_position_and_relink(&launch, grenade, 0);
        }
        {
            real speed = ((struct Unit *)unit_tag)->grenade_velocity * 0.033333335f;

            velocity.i = speed * aim->i;
            velocity.j = speed * aim->j;
            velocity.k = speed * aim->k;
        }
    }

    if (early) {
        real progress = (real)((unit_object *)unit)->unit.throwing_grenade_counter / (real)((unit_object *)unit)->unit.throwing_grenade_duration;

        if (progress < 1.0f) {
            real lob = random_real_range(0.02f, 0.046666667f);
            real rest = 1.0f - progress;

            velocity.i = lob * aim->i * rest + velocity.i * progress;
            velocity.j = lob * aim->j * rest + velocity.j * progress;
            velocity.k = lob * aim->k * rest + velocity.k * progress;
        }
    }

    {
        uint8_t *object = OBJECT_DATA(grenade);
        real_vector3d delta;
        real_point3d camera;

        delta.i = velocity.i - ((struct object *)object)->velocity.i;
        delta.j = velocity.j - ((struct object *)object)->velocity.j;
        delta.k = velocity.k - ((struct object *)object)->velocity.k;
        object_apply_impulse_and_spin(grenade, &delta);
        ((unit_object *)unit)->unit.throwing_grenade_projectile = k_datum_index_none;
        unit[0x28d] = 3;
        UnitView(object_index).get_camera_position(&camera);
        if (!object_reposition_to_spawn_location(grenade, &camera, k_datum_index_none)) {
            object_delete(grenade);
            return;
        }
    }
    if (((unit_object *)unit)->base.network_role == 0 && network_game_mode == 2 && !object_is_delete_pending(grenade)) {
        ((struct object *)OBJECT_DATA(grenade))->network_role = 0;
        object_type_override_call_0x68(grenade);
        int32_t bits = projectile_send_creation(grenade);

        if (bits > 0) {
            network_session_broadcast_to_flagged(bits, network_server, 1, network_message_scratch, 1, 0, 0, 3);
        }
    }
}
#undef OBJECT_DATA

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
void unit_scripting_set_or_drop_weapon(int32_t *message)
{
    using namespace unit_scripting_set_or_drop_weapon_local;
    unit_set_or_drop_weapon_message decoded;
    int32_t *keys;
    uint32_t unit_index;
    uint8_t *unit;
    datum_index weapon = k_datum_index_none;
    datum_index current = k_datum_index_none;
    int16_t current_index;
    int32_t i;

    if (*(int32_t *)*message != 0) {
        message_delta_decode_compound_field_staged(message);
        return;
    }
    if (message_delta_decode_compound_field(message, &decoded) == 0 || decoded.unit_key == 0) {
        return;
    }
    keys = *(int32_t **)(object_network_id_table + 0x28);
    unit_index = (uint32_t)keys[decoded.unit_key];
    if (unit_index == k_datum_index_none) {
        return;
    }
    unit = (uint8_t *)object_try_and_get(unit_index, 3);
    if (unit == 0) {
        return;
    }
    if (decoded.weapon_key != 0) {
        weapon = (datum_index)keys[decoded.weapon_key];
    }
    if (UnitView(unit_index).get_weapon_object_index(((unit_object *)unit)->unit.current_weapon_index) != weapon) {
        for (i = 0; i < 4; i++) {
            if (((datum_index *)&((struct unit_object *)unit)->unit.weapons)[i] == weapon) {
                ((unit_object *)unit)->unit.desired_weapon_index = (int16_t)i;
                UnitView(unit_index).ready_desired_weapon(1);
                break;
            }
        }
    }
    current_index = ((unit_object *)unit)->unit.current_weapon_index;
    if (current_index != -1) {
        current = ((datum_index *)&((struct unit_object *)unit)->unit.weapons)[current_index];
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->parent_object == k_datum_index_none) {
        obj->forward.i = direction_xy->i;
        obj->forward.j = direction_xy->j;
        obj->forward.k = 0.0f;
        obj->up = *global_up3d_pointer;
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    int8_t grenade_type = unit->current_grenade_index;
    uint8_t *grenade_table = (uint8_t *)global_globals->grenades.pointer;

    if (((unit->controlling_player == k_datum_index_none) ||
         ((weapon_bottomless_clip == 0) &&
          ((current_game_engine == 0) || ((game_engine_unknown_aa00 & 4) != 0) ||
           ((motion_sensor_override_value >> 2 & 1) == 0)))) &&
        (unit->actor_index == k_datum_index_none) &&
        ((unit_obj->network_role == 3) || (unit_obj->network_role == 0))) {
        unit->grenade_counts[grenade_type] -= 1;
    }

    if ((network_game_mode != 2) && (network_game_mode != 0)) {
        unit->throwing_grenade_projectile = k_datum_index_none;
        unit->throwing_grenade_state = 2;
        return;
    }

    object_marker hand_marker;
    object_get_node_local_transform(unit_index, (char *)"left hand", &hand_marker, 1);

    object_placement_data placement;
    object_placement_data_initialize(&placement, *(datum_index *)(grenade_table + grenade_type * 0x44 + 0x40),
                                     unit_index);
    placement.flags |= 2;
    placement.forward = ((struct unit_object *)unit_obj)->unit.aiming_vector;
    vector3d_build_perpendicular(&placement.up, &placement.forward);
    vector3d_normalize_with_length(&placement.up);
    placement.position = *(real_point3d *)((uint8_t *)&hand_marker + 0x60);

    uint32_t projectile_index = object_new_with_datum_role_control(&placement, 3);
    if (projectile_index != k_datum_index_none) {
        object_attach_to_object(unit_index, projectile_index, hand_marker.node_index);
        unit->throwing_grenade_projectile = projectile_index;
        unit->throwing_grenade_state = 2;
        object *projectile_obj = ((object_header *)object_data->data)[halo::datum_slot(projectile_index)].data;
        *((uint8_t *)projectile_obj + 0x278) = 1;
        return;
    }
    unit->throwing_grenade_state = 3;
    return;
}

/**
 * Not callable: see the header. Kept only so the address stays listed in the symbol tables.
 *
 * @address 0x571b40
 */
void unit_throw_grenade_release(void)
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
uint8_t unit_try_give_grenade(uint32_t tag_source_index, uint32_t unit_index)
{
    uint8_t *tag_data = (uint8_t *)tag_instances[
        ((object_header *)object_data->data)[halo::datum_slot(tag_source_index)].data->definition_tag & 0xffff].data;
    int16_t grenade_type = *(int16_t *)(tag_data + 0x30a);
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    int16_t *max_count_ptr = (int16_t *)((uint8_t *)global_globals->grenades.pointer + grenade_type * 0x44);
    if ((max_count_ptr != (int16_t *)0) && (unit->grenade_counts[grenade_type] < *max_count_ptr)) {
        unit->grenade_counts[grenade_type] += 1;
        set_flag(unit_obj->flags, objects::object_flag::changed);
        int32_t local_player = player_index_from_unit_index(unit_index);
        if (local_player != -1) {
            uint32_t local_player2 = (uint32_t)player_index_from_unit_index(unit_index);
            if (*(int16_t *)((uint8_t *)player_data->data + halo::datum_slot(local_player2) * 0x200 + 2) != -1) {
                equipment_pickup_play_sound(tag_source_index);
            }
        }
        object_delete(tag_source_index);
        return 1;
    }
    return 0;
}

namespace unit_try_ready_weapon_local {

static int unit_animation_state_allows_melee(int8_t state)
{
    switch (state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 0;
    default:
        return 1;
    }
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
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *unit_tag = (uint8_t *)tag_instances[halo::datum_slot(*(datum_index *)unit)].data;
    int8_t state = (int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state;
    uint8_t airborne = 0;
    int16_t new_state;

    if (!unit_animation_state_allows_melee(state)) {
        return 0;
    }
    if (((unit_object *)unit)->base.type == 0) {
        airborne = unit[0x4cc] & 1;
    }
    if (forced) {
        new_state = 0x20;
    } else if (state == 0x28) {
        new_state = 0x29;
    } else {
        new_state = (int16_t)(0x1e + (airborne != 0));
    }
    if (!UnitView(unit_index).try_set_animation_state(new_state) && !forced) {
        return 0;
    }
    if (*(uint32_t *)&((struct Unit *)unit_tag)->unit_flags & 0x100) {
        unit[0x2a3] = 0x19;
    }
    if (direction != 0) {
        UnitView(unit_index).set_throw_aim_direction(direction);
    }
    if (forced) {
        unit[0x289] = 4;
        unit[0x28a] = 0;
        return 1;
    }
    unit[0x289] = 1;
    return 1;
}

namespace unit_try_ready_weapon_variant_local {

static int unit_animation_state_allows_melee(int8_t state)
{
    switch (state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1d: case 0x1e: case 0x1f:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 0;
    default:
        return 1;
    }
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
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;

    if (!unit_animation_state_allows_melee((int8_t)(uint8_t)((struct unit_object *)unit)->unit.animation_state)) {
        return 0;
    }
    if (((unit_object *)unit)->base.type == 0 && (unit[0x4cc] & 1) != 0) {
        return 0;
    }
    if (!UnitView(unit_index).try_set_animation_state(0x27)) {
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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    if ((unit->equipment_object_index != k_datum_index_none) && (release_current == 1)) {
        object_delete(unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    if (unit->equipment_object_index == k_datum_index_none) {
        object_unlink_cluster_or_notify_parent(new_equipment_object_index);
        object *new_obj = ((object_header *)object_data->data)[halo::datum_slot(new_equipment_object_index)].data;
        Object *new_def = (Object *)tag_instances[halo::datum_slot(new_obj->definition_tag)].data;
        if ((*(uint32_t *)&new_def->model.tag_id != k_datum_index_none) && (!test_flag(new_obj->flags, objects::object_flag::no_collision))) {
            object_for_each_light_attachment(new_equipment_object_index, 1, 0);
        }
        set_flag(new_obj->flags, objects::object_flag::no_collision);
        ((object_header *)object_data->data)[halo::datum_slot(new_equipment_object_index)].flags &= 0xfd;

        int32_t local_player = player_index_from_unit_index(unit_index);
        if (local_player != -1) {
            uint32_t local_player2 = (uint32_t)player_index_from_unit_index(unit_index);
            if (*(int16_t *)((uint8_t *)player_data->data + halo::datum_slot(local_player2) * 0x200 + 2) != -1) {
                equipment_pickup_play_sound(new_equipment_object_index);
            }
        }
        item_set_holder(new_equipment_object_index, unit_index);
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
uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index)
{
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    object *reference_obj = ((object_header *)object_data->data)[halo::datum_slot(reference_weapon_index)].data;

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
        object *slot_weapon = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
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
