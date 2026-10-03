#include "halo/networking/game_mode.hpp"
#include "halo/units/seat_detach.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/units/records.hpp"
#include "halo/game/records.hpp"
#include "halo/ai/records.hpp"
#include "halo/objects/record_access.hpp"
#include <string.h>
#include "halo/models/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/lcg.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "ai.h"
#include "crt.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"

static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &object_network_id_table = halo::link::ref<uint8_t *>(halo::units::vars().object_network_id_table);
static auto &network_object_index_cache = halo::link::ref<uint8_t []>(halo::units::vars().network_object_index_cache);
static auto &unit_base_animation_state_names = halo::link::ref<char *[6]>(halo::units::vars().unit_base_animation_state_names);

namespace halo::units {

/**
 * Engine function unit_all_seats_unoccupied.
 *
 * Original register convention: in_EAX -> unit_index.
 *
 * @address 0x566910
 */
uint8_t UnitView::all_seats_unoccupied()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    int32_t count = (int32_t)unit_tag->seats.count;

    for (int16_t i = 0; i < count; i++) {
        if (::halo::units::unit_is_seat_occupied(unit_index, i) == 1) {
            return 0;
        }
    }
    return 1;
}

/**
 * Scans all units for one that is dead (with a specific sub-state) or currently entering/exiting a seat,
 * returning true if any match.
 *
 * @address 0x56c070
 */
uint8_t halo::units::unit_any_dying_or_seat_transition(void)
{
    object_iterator iter = { _object_mask_unit, 1, 0, 0, 0xffffffff };
    object *obj = halo::objects::object_iterator_next(&iter);
    while (obj != nullptr) {
        unit_data *unit = halo::units::unit_data_of(obj);
        if (((unit->animation_state == animation_state_value(unit_animation_state_id::throwing_grenade)) && (unit->throwing_grenade_state != _unit_throwing_grenade_state_released)) ||
            (((unit->animation_state == animation_state_value(unit_animation_state_id::ready_weapon)) || (unit->animation_state == animation_state_value(unit_animation_state_id::unknown_18))) &&
             (!test_flag(unit->animation_state_flags, units::unit_animation_state_flag::unknown_4)))) {
            return 1;
        }
        obj = halo::objects::object_iterator_next(&iter);
    }
    return 0;
}

/**
 * Returns whether any of the unit's flagged seats currently has an occupant.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56cc80
 */
uint8_t UnitView::any_flagged_seat_occupied()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    int32_t count = (int32_t)unit_tag->seats.count;
    if (count < 1) {
        return 0;
    }
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
    for (int16_t i = 0; i < count; i++) {
        if ((seats[i].flags & 4) != 0) {
            if (::halo::units::unit_is_seat_occupied((int32_t)unit_index, i) == 1) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * Applies a linear impulse to the unit's velocity and, if the resulting up-cross-impulse axis is
 * non-degenerate, an angular impulse (scaled by pi times its length) to its angular velocity -- but only when
 * the unit's own tag defines a physics reference. Also clears the extension_of_parent flag and marks
 * object+0x524 dirty.
 *
 * @address 0x571cb0
 */
void UnitView::apply_impulse_to_seat(real_vector3d *impulse)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *tag = halo::objects::tag_as<Unit>(obj->definition_tag);

    if ((int32_t)halo::objects::tag_handle(tag->base.physics) == -1) {
        return;
    }

    obj->velocity.i += impulse->i;
    obj->velocity.j += impulse->j;
    obj->velocity.k += impulse->k;

    {
        real_vector3d axis;
        real length;

        axis.i = halo::math::globals().global_up3d_pointer->j * impulse->k - impulse->j * halo::math::globals().global_up3d_pointer->k;
        axis.j = impulse->i * halo::math::globals().global_up3d_pointer->k - impulse->k * halo::math::globals().global_up3d_pointer->i;
        axis.k = impulse->j * halo::math::globals().global_up3d_pointer->i - impulse->i * halo::math::globals().global_up3d_pointer->j;
        length = halo::math::vector3d_normalize_with_length(axis);

        if (length > 0.0f) {
            float scale = length * 3.1415927f;
            obj->angular_velocity.i += axis.i * scale;
            obj->angular_velocity.j += axis.j * scale;
            obj->angular_velocity.k += axis.k * scale;
        }
    }

    clear_flag(obj->flags, objects::object_flag::at_rest);
    *((uint8_t *)obj + 0x524) = 1;
}

/**
 * Allocates a new zone/list datum and populates it with references to every seated occupant of the unit.
 *
 * Original register convention: in_ECX.
 *
 * @address 0x56bbd0
 */
datum_index UnitView::build_seat_occupant_zone_list()
{
    uint32_t unit_index = datum_handle;
    datum_index result = k_datum_index_none;
    if (unit_index == k_datum_index_none) {
        return result;
    }

    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    result = halo::memory::datum_new(halo::objects::globals().object_list_header_data);
    if (result != k_datum_index_none) {
        object_list_header *node = halo::objects::object_list_header_at(result);
        node->count = 0;
        node->first_reference = k_datum_index_none;

        datum_index child = unit_obj->first_child_object;
        while (child != k_datum_index_none) {
            object *child_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(child)].data;
            if (((_object_mask_unit & (1 << (child_obj->type & 0x1f))) != 0) &&
                ((halo::units::unit_data_of(child_obj))->vehicle_seat_index != -1)) {
                halo::hs::object_list_reference_add(result, child);
            }
            child = child_obj->next_object;
        }
    }
    return result;
}

namespace unit_detach_and_enter_named_seat_local {

}

/**
 * Engine function unit_detach_and_enter_named_seat.
 *
 * @address 0x569d40
 */
void UnitView::detach_and_enter_named_seat(uint32_t target_parent_index, char *seat_marker_name)
{
    using namespace unit_detach_and_enter_named_seat_local;
    uint32_t unit_index = datum_handle;
    unit_object *obj;
    Unit *vehicle_tag;
    int16_t i;

    if (unit_index == k_datum_index_none || target_parent_index == k_datum_index_none || seat_marker_name[0] == 0) {
        return;
    }
    obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    if ((uint8_t)obj->base.vitality_flags & 4) {
        return;
    }
    if (obj->base.parent_object != k_datum_index_none && obj->unit.vehicle_seat_index != -1 &&
        halo::networking::globals().game_mode != halo::networking::k_game_mode_client) {
        if (obj->base.parent_object != k_datum_index_none && obj->unit.vehicle_seat_index != -1) {
            biped_detach_from_seat(unit_index, obj->base.parent_object);
        }
        if (obj->base.network_role == 0) {
            ::halo::units::unit_dispatch_scripted_event_9(1, (int32_t)unit_index);
        }
        biped_free_local_player_history(obj);
    }
    if (obj->base.parent_object != k_datum_index_none) {
        return;
    }
    vehicle_tag = halo::objects::tag_as<Unit>(*(datum_index *)halo::objects::object_record_bytes(target_parent_index));
    for (i = 0; i < (int32_t)vehicle_tag->seats.count; i++) {
        char *seat_label = halo::objects::block_element<UnitSeat>(vehicle_tag->seats, i).label.string;

        if (_stricmp(seat_marker_name, seat_label) != 0) {
            continue;
        }
        if (::halo::units::unit_is_seat_occupied((int32_t)target_parent_index, i)) {
            continue;
        }
        if (obj->base.type == _object_type_vehicle || UnitView(unit_index).set_or_test_seat_and_weapon_label(seat_label, 0, 0)) {
            ::halo::units::unit_enter_vehicle_seat(target_parent_index, i, unit_index);
            return;
        }
    }
}

/**
 * Detaches the unit from its current parent/attachment object.
 *
 * @address 0x570140
 */
void halo::units::unit_detach_from_parent(object *obj, uint32_t unit_index, real_vector3d *cross_out, real_vector3d *cross_ecx_operand, real_vector3d *cross_stack_operand, real_point3d *reposition_target)
{
    halo::math::vector3d_cross_product(*cross_out, *cross_ecx_operand, *cross_stack_operand);
    halo::objects::object_set_position_and_relink(reposition_target, unit_index, 0);
    halo::objects::object_attach_to_object(unit_index, unit_index, 0);

    set_flag(obj->flags, objects::object_flag::at_rest);
    {
        unit_data *unit = halo::units::unit_data_of(obj);
        set_flag(unit->flags, units::unit_flag::detached);
    }
    UnitView(unit_index).try_ready_weapon(1, 0);
}

namespace unit_detach_from_seat_local {

}

/**
 * Engine function unit_detach_from_seat.
 *
 * @address 0x56c640
 */
void UnitView::detach_from_seat(uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event)
{
    using namespace unit_detach_from_seat_local;
    uint32_t unit_index = datum_handle;
    unit_object *obj;

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client && require_client_flag != 1) {
        return;
    }
    obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    if (obj->base.parent_object != k_datum_index_none && obj->unit.vehicle_seat_index != -1) {
        biped_detach_from_seat(unit_index, obj->base.parent_object);
    }
    if (!suppress_trigger && fire_trigger_event == 1 && obj->base.network_role == 0) {
        ::halo::units::unit_dispatch_scripted_event_9(1, (int32_t)unit_index);
    }
    biped_free_local_player_history(obj);
}

/**
 * Engine function unit_detach_if_flag_clear.
 *
 * @address 0x56c440
 */
void halo::units::unit_detach_if_flag_clear(uint8_t skip_flag, uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event)
{
    if (!skip_flag) {
        UnitView(unit_index).detach_from_seat(suppress_trigger, require_client_flag, fire_trigger_event);
    }
    return;
}

/**
 * REWRITTEN from objdump 0x56ca40..0x56cc0a. EDI: unit. A unit with a parent is pushed away from it: the
 * direction from the parent's position to the unit's (else the unit's forward) times 0.02 is added to its
 * velocity after it is detached (0x4f6610), its position nudged up onto the structure (0x53e870 on a copy)
 * and relinked; it drops unit +0x204 bit 15 and object bit 5, sets +0x474, and its lights are reattached.
 *
 * Original register convention: EDI -> unit_index.
 *
 * @address 0x56ca40
 */
void UnitView::detach_reposition_and_nudge()
{
    uint32_t unit_index = datum_handle;
    unit_object *self = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    real_point3d parent_position;
    real_point3d position;
    real_vector3d push;
    uint8_t *object;
    Unit *tag;

    if (self->base.parent_object == k_datum_index_none) {
        return;
    }
    halo::objects::object_get_position(&parent_position, self->base.parent_object);
    halo::objects::object_get_position(&position, unit_index);
    push.i = position.x - parent_position.x;
    push.j = position.y - parent_position.y;
    push.k = position.z - parent_position.z;
    if (halo::math::vector3d_normalize_with_length(push) == 0.0f) {
        push = *(real_vector3d *)&self->base.forward.i;
    }
    push.i = push.i * 0.02f;
    push.j = push.j * 0.02f;
    push.k = push.k * 0.02f;
    halo::objects::object_snap_to_parent_marker_and_detach(unit_index);
    position = *(real_point3d *)&self->base.position.x;
    halo::scenario::structure_bsp_switcher::locate_point_nudge_up(&position);
    object = halo::objects::object_record_bytes(unit_index);
    halo::objects::object_unlink_cluster_or_notify_parent(unit_index);
    ((struct object *)object)->position = position;
    halo::objects::object_recalculate_bounding_radius(unit_index);
    halo::objects::object_set_cluster_and_parent(unit_index, 0);
    clear_flag(self->unit.flags, units::unit_flag::detached);
    clear_flag(self->base.flags, objects::object_flag::at_rest);
    self->unit.network_update_forced = 1;
    self->base.velocity.i = push.i + self->base.velocity.i;
    self->base.velocity.j = push.j + self->base.velocity.j;
    self->base.velocity.k = push.k + self->base.velocity.k;
    object = halo::objects::object_record_bytes(unit_index);
    tag = halo::objects::tag_as<Unit>(*(datum_index *)object);
    if ((int32_t)halo::objects::tag_handle(tag->base.model) != -1 && (test_flag(((struct object *)object)->flags, objects::object_flag::no_collision))) {
        halo::objects::object_for_each_light_attachment(unit_index, 0, 1);
    }
    if ((int32_t)halo::objects::tag_handle(tag->base.model) != -1) {
        clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
        ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].flags |= 0x02;
    }
    halo::objects::object_recalculate_bounding_radius(unit_index);
}

namespace unit_dispatch_seat_exit_message_local {

typedef struct unit_seat_exit_message {
    int32_t unit_key;
    uint8_t skip_animation;
    uint8_t pad_05[11];
} unit_seat_exit_message;

}

/**
 * Dispatches to either a local seat-exit handler or a network handler depending on a flag read from the
 * object pointed to by in_EAX.
 *
 * @address 0x56c400
 */
void halo::units::unit_dispatch_seat_exit_message(int32_t *message)
{
    using namespace unit_dispatch_seat_exit_message_local;
    unit_seat_exit_message decoded;
    int32_t unit_index;

    if (*(int32_t *)*message != 0) {
        halo::networking::message_delta_decode_compound_field_staged((void **)message);
        return;
    }
    if (halo::networking::message_delta_decode_compound_field((void **)message, &decoded) == 0 || decoded.unit_key == 0) {
        return;
    }
    unit_index = (*(int32_t **)(object_network_id_table + 0x28))[decoded.unit_key];
    if (unit_index == -1) {
        return;
    }
    if (decoded.skip_animation == 1 || ::halo::units::unit_try_start_seat_exit_animation(1, (uint32_t)unit_index) == 0) {
        UnitView((uint32_t)unit_index).detach_from_seat(0, 1, 0);
    }
}

/**
 * Engine function unit_dispatch_seat_overlay_command.
 *
 * Original register convention: in_EAX -> unit_index, in_CX -> command.
 *
 * @address 0x567400
 */
void UnitView::dispatch_seat_overlay_command(int16_t command)
{
    uint32_t unit_index = datum_handle;
    switch (command) {
    case 1:
        UnitView(unit_index).start_seat_overlay_animation_b(command);
        return;
    case 2:
        UnitView(unit_index).start_seat_overlay_animation_b(command);
        return;
    case 3:
        UnitView(unit_index).start_seat_overlay_animation_b(command);
        break;
    case 4:
        UnitView(unit_index).start_seat_overlay_animation_b(command);
        return;
    case 5:
        UnitView(unit_index).start_seat_overlay_animation_a(command);
        return;
    case 6:
        UnitView(unit_index).start_seat_overlay_animation_a(command);
        return;
    case 7:
        UnitView(unit_index).start_seat_overlay_animation_b(command);
        return;
    case 8:
        UnitView(unit_index).start_seat_overlay_animation_b(command);
        return;
    }
    return;
}

/**
 * Engine function unit_enter_vehicle_seat.
 *
 * @address 0x566970
 */
uint32_t halo::units::unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index)
{
    unit_object *unit;
    UnitSeat *seat;
    Unit *unit_tag;
    ModelAnimationsAnimationGraphUnitSeat *unit_seat;
    char *marker_name;
    real_point3d position;
    real_vector3d delta;
    object_marker marker;

    if (::halo::units::unit_seat_is_occupied_by_other(unit_index, seat_index, vehicle_index, 0) == 0) {
        return 0;
    }
    seat = &halo::objects::block_element<UnitSeat>(halo::objects::tag_as<Unit>(*(datum_index *)halo::objects::object_record_bytes(vehicle_index))->seats, seat_index);
    halo::objects::object_get_position(&position, unit_index);
    marker_name = seat->marker_name.string;
    halo::objects::object_get_node_local_transform(vehicle_index, marker_name, &marker, 1);
    delta.i = position.x - marker.node_transform.position.x;
    delta.j = position.y - marker.node_transform.position.y;
    delta.k = position.z - marker.node_transform.position.z;
    halo::math::matrix4x3_inverse_transform_vector(delta, delta, marker.node_transform);
    halo::objects::object_reorient_relative_to_marker(vehicle_index, marker_name, unit_index, (char *)"");

    unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    unit->unit.vehicle_seat_index = seat_index;
    unit->base.parent_object = vehicle_index;
    UnitView(vehicle_index).recompute_seat_occupants();

    unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    unit->unit.desired_weapon_index =
        UnitView(unit_index).find_next_zone_permitted_weapon_slot(*(uint16_t *)&unit->unit.current_weapon_index, 0);
    UnitView(unit_index).ready_desired_weapon(1);
    if (UnitView(unit_index).set_or_test_seat_and_weapon_label(seat->label.string, UnitView(unit_index).get_current_weapon_label(), 1) == 0) {
        UnitView(unit_index).set_or_test_seat_and_weapon_label(seat->label.string, 0, 1);
    }

    unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)unit);
    unit_seat = &halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(
        halo::objects::tag_as<ModelAnimations>(halo::objects::tag_handle(unit_tag->base.animation_graph))->units, (int8_t)(uint8_t)unit->unit.animation_definition_index);
    int16_t enter_animation = (int32_t)unit_seat->animations.count > 7 ? halo::objects::block_element<int16_t>(unit_seat->animations, 7) : int16_t(-1);
    if (enter_animation != -1) {
        object *reloaded;
        int16_t animation;

        halo::objects::object_copy_default_node_transforms(unit_index, 6);
        animation = halo::models::animation_choose_random_permutation(halo::objects::tag_handle(unit_tag->base.animation_graph), enter_animation, (animation_random_stream)1);
        reloaded = reinterpret_cast<object *>(halo::objects::object_record_bytes(unit_index));
        reloaded->animation_graph = halo::objects::tag_handle(unit_tag->base.animation_graph);
        reloaded->animation_index = animation;
        reloaded->animation_frame = 0;
        unit->unit.animation_state = animation_state_value(unit_animation_state_id::seat_enter);
        halo::objects::object_offset_node_translation(unit_index, &delta);
        halo::objects::object_recalculate_bounding_radius_recursive(unit_index);
    }

    if (((struct unit_object *)halo::objects::object_record_bytes(unit_index))->unit.actor_index != k_datum_index_none) {
        halo::ai::ai_communication_broadcast(0x24, unit_index, k_datum_index_none, -1, k_datum_index_none, k_datum_index_none, 0);
    }
    UnitView(unit_index).validate_and_clear_weapon_switch();
    {
        vehicle_object *vehicle = reinterpret_cast<vehicle_object *>(halo::objects::object_try_and_get(vehicle_index, 2));

        if (vehicle != 0) {
            vehicle->vehicle.network_update_tick = -1;
        }
    }
    return 1;
}

/**
 * Not callable: see the header. Kept only so the address stays listed in the symbol tables.
 *
 * @address 0x56fd40
 */
void halo::units::unit_exit_seat_end(void)
{
}

/**
 * Engine function unit_exit_vehicle_seat.
 *
 * Original register convention: in_EAX -> player_index.
 *
 * @address 0x568120
 */
void halo::units::unit_exit_vehicle_seat(uint32_t player_index)
{
    if ((player_index != k_datum_index_none) && (0 <= (int16_t)player_index) &&
        ((int16_t)player_index < halo::game::globals().player_data->maximum_count)) {
        player *player_record = halo::game::player_at(player_index);
        int16_t salt = player_record->identifier;
        int16_t requested_salt = (int16_t)(player_index >> 16);
        if ((salt != 0) && ((requested_salt == 0) || (salt == requested_salt))) {
            uint32_t unit_index = player_record->unit;

            UnitView(unit_index).update_stance_and_jump(1, 0, 0, 0, 0, 0.0f, -1, 0, 0);
            UnitView(unit_index).release_transient_state(0);

            unit_state_change_record record = {};
            record.unit = unit_index;
            record.valid = 1;
            record.killed = 1;
            record.no_direction = 1;
            record.pad_0b = 0xff;
            record.region_index = -1;
            record.player_value = static_cast<uint32_t>(player_record->respawn_timer);

            ::halo::units::unit_broadcast_state_change_event(record);

            object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
            obj->network_role = 3;
            if ((((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].flags & 8) == 0) {
                halo::networking::network_index_cache_remove(network_object_index_cache, (int32_t)unit_index);
            }
        }
    }
    return;
}

/**
 * Engine function unit_find_best_seat_to_enter.
 *
 * Original register convention: see file header.
 *
 * @address 0x566560
 */
uint16_t UnitView::find_best_seat_to_enter(uint32_t vehicle_index, int16_t *out_seat)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    object *vehicle_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(vehicle_index)].data;
    Unit *vehicle_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(vehicle_obj->definition_tag)].data;

    unit_data *vehicle_unit = halo::units::unit_data_of(vehicle_obj);
    uint16_t best_rank = 0;
    int16_t best_seat = -1;

    if ((vehicle_obj->vitality_flags & _object_health_frozen_bit) != 0) {
        *out_seat = -1;
        return 0;
    }
    if (test_flag(((struct unit_object *)vehicle_obj)->unit.flags, units::unit_flag::unknown_10000)) {
        *out_seat = -1;
        return 0;
    }
    if ((int32_t)vehicle_tag->seats.count < 1) {
        *out_seat = -1;
        return 0;
    }

    float best_distance = 3.4028235e+38f;
    uint8_t best_is_third_person = 0;

    for (int16_t seat_i = 0; seat_i < (int32_t)vehicle_tag->seats.count; seat_i++) {
        UnitSeat *seat = (UnitSeat *)(&halo::objects::block_element<UnitSeat>(vehicle_tag->seats, seat_i));

        real_vector3d marker_a, marker_b;
        if (UnitView(unit_index).find_weapon_marker_transform(vehicle_index, seat_i, (real_point3d *)&marker_a, (real_point3d *)&marker_b, 0) != 0) {
            float dx = unit_obj->bounding_center.x - marker_a.i;
            float dy = unit_obj->bounding_center.y - marker_a.j;
            float dz = unit_obj->bounding_center.z - marker_a.k;
            float dist_a = (float)halo::libm::sqrt((double)(dx * dx + dy * dy + dz * dz));

            dx = unit_obj->bounding_center.x - marker_b.i;
            dy = unit_obj->bounding_center.y - marker_b.j;
            dz = unit_obj->bounding_center.z - marker_b.k;
            float dist_b = (float)halo::libm::sqrt((double)(dx * dx + dy * dy + dz * dz));

            float dist = (dist_b < dist_a) ? dist_b : dist_a;

            if (dist < 1.0f &&
                (!test_flag(seat->flags, tags::unit_seat_tag_flag::not_valid_without_driver) || vehicle_unit->driver_unit_index != k_datum_index_none) &&
                seat->label.string[0] != '\0' &&
                UnitView(unit_index).set_or_test_seat_and_weapon_label(seat->label.string, 0, 0) != 0) {

                uint32_t occupant = (uint32_t)-1;
                uint16_t rank;
                if (::halo::units::unit_seat_is_occupied_by_other(unit_index, seat_i, vehicle_index, &occupant) == 0) {
                    if (occupant == (uint32_t)-1) {
                        continue;
                    }
                    object *occupant_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(occupant)].data;
                    unit_data *occupant_unit = halo::units::unit_data_of(occupant_obj);
                    if (occupant_unit->actor_index == k_datum_index_none || halo::ai::actor_check_vehicle_target_available(unit_index, occupant_unit->actor_index, 0) == 0) {
                        continue;
                    }
                    rank = 1;
                } else {
                    rank = 2;
                }

                float weight = 1.0f;
                if (best_is_third_person && ((seat->flags >> 2 & 1) == 0)) {
                    weight = 1.5f;
                }

                if (best_seat == -1 || best_rank < rank || dist * weight < best_distance) {
                    best_is_third_person = (uint8_t)(seat->flags >> 2) & 1;
                    best_distance = dist;
                    best_rank = rank;
                    best_seat = seat_i;
                }
            }
        }
    }

    *out_seat = best_seat;
    return best_rank;
}

/**
 * Finds the next valid, zone-permitted weapon inventory slot starting from param_1, searching forward or
 * backward depending on param_2.
 *
 * @address 0x56dba0
 */
int16_t UnitView::find_next_zone_permitted_weapon_slot(int32_t start_slot, int16_t direction)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    int16_t best_slot = -1;
    int32_t original = start_slot;
    if ((int16_t)start_slot == -1) {
        start_slot = 0;
        original = start_slot;
    }

    int32_t cursor = start_slot;
    do {
        int16_t slot = (int16_t)cursor;
        if ((unit->weapons[slot] != k_datum_index_none) && (UnitView(unit_index).check_weapon_use_permission(unit->weapons[slot]) != 0)  ) {
            if ((direction != 0) || (best_slot == -1) ||
                (unit->weapon_ready_ticks[best_slot] < unit->weapon_ready_ticks[slot])) {
                best_slot = slot;
            }
            object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[unit->weapons[slot] & 0xffff].data;
            Weapon *weapon_tag = halo::objects::tag_as<Weapon>(weapon_obj->definition_tag);
            if ((weapon_tag->weapon_flags >> 3 & 1) != 0) {
                return best_slot;
            }
            if (slot != (int16_t)original) {
                return best_slot;
            }
        }
        if (direction < 0) {
            cursor = (slot == 0) ? 3 : (int32_t)slot - 1;
        } else if (slot == 3) {
            cursor = 0;
        } else {
            cursor = (int32_t)slot + 1;
        }
    } while ((int16_t)cursor != (int16_t)original);
    return best_slot;
}

/**
 * Engine function unit_get_seat_hud_interface_tag_id.
 *
 * Original register convention: see file header.
 *
 * @address 0x560cb0
 */
TagID halo::units::unit_get_seat_hud_interface_tag_id(Unit *unit_tag, int16_t seat_index, uint8_t use_second)
{
    UnitSeat *seat = (UnitSeat *)(&halo::objects::block_element<UnitSeat>(unit_tag->seats, seat_index));
    int32_t index = (int32_t)seat->unit_hud_interface.count - 1;

    if (index > (use_second != 0)) {
        index = use_second != 0;
    }
    if (index < 0) {
        TagID none = {0xffff, 0xffff};
        return none;
    }
    UnitUnitHudInterface *entries = (UnitUnitHudInterface *)seat->unit_hud_interface.pointer;
    return entries[(int16_t)index].hud.tag_id;
}

/**
 * Returns the name of the seat/marker the unit currently occupies, or a default state-name string if it has
 * no parent.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56c2f0
 */
char * UnitView::get_seat_or_state_name()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    if ((unit_obj->parent_object != k_datum_index_none) && (unit->vehicle_seat_index != -1)) {
        object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_obj->parent_object)].data;
        Unit *parent_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(parent->definition_tag)].data;
        UnitSeat *seat = (UnitSeat *)parent_tag->seats.pointer + unit->vehicle_seat_index;
        return seat->label.string;
    }
    return unit_base_animation_state_names[unit->base_animation_state];
}

/**
 * Returns whether a given child object is seated at the named marker on this unit.
 *
 * @address 0x56b520
 */
uint8_t UnitView::is_child_seated_at_named_marker(char *seat_label, uint32_t child_object_index)
{
    uint32_t unit_index = datum_handle;
    if ((unit_index == k_datum_index_none) || (child_object_index == k_datum_index_none)) {
        return 0;
    }
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    if ((int32_t)unit_tag->seats.count < 1) {
        return 0;
    }
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
    object *child = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(child_object_index)].data;

    for (int16_t seat_index = 0; seat_index < (int32_t)unit_tag->seats.count; seat_index++) {
        if ((_stricmp(seat_label, seats[seat_index].label.string) == 0) &&
            (child->parent_object == unit_index) &&
            ((halo::units::unit_data_of(child))->vehicle_seat_index == seat_index)) {
            return 1;
        }
    }
    return 0;
}

/**
 * Reports whether a specific seat control is currently available to the unit, depending on its animation
 * state and seating.
 *
 * Original register convention: in_EAX, unaff_DI.
 *
 * @address 0x5693a0
 */
uint8_t UnitView::is_seat_control_available(int16_t command)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    if (is_scripted_animation_state(animation_state_id(unit->animation_state))) {
        return 0;
    }
    if (unit_obj->parent_object != k_datum_index_none) {
        int16_t seat_index = unit->vehicle_seat_index;
        if ((seat_index != -1) && (halo::objects::object_try_and_get(unit_obj->parent_object, _object_mask_unit) != nullptr) &&
            (11 < command) && (command < 14)) {
            object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_obj->parent_object)].data;
            Unit *parent_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(parent->definition_tag)].data;
            UnitSeat *seat = (UnitSeat *)parent_tag->seats.pointer + seat_index;
            return (seat->flags >> 8) & 1;
        }
        return 0;
    }
    if ((command < 12) || (13 < command)) {
        return 1;
    }
    return 0;
}

/**
 * Returns whether any object is currently seated at the given parent/seat-index pair.
 *
 * Original register convention: unaff_EDI, unaff_SI.
 *
 * @address 0x56cc10
 */
uint8_t halo::units::unit_is_seat_occupied(int32_t parent_index, int16_t seat_index)
{
    object_iterator iter = { _object_mask_unit, 0, 0, 0, 0xffffffff };
    object *obj = halo::objects::object_iterator_next(&iter);
    while (obj != nullptr) {
        if ((obj->parent_object == (uint32_t)parent_index) &&
            ((halo::units::unit_data_of(obj))->vehicle_seat_index == seat_index)) {
            return 1;
        }
        obj = halo::objects::object_iterator_next(&iter);
    }
    return 0;
}

/**
 * Iterates a zone/player-indexed list of seat markers and sets one of two alternate flag bits on each seated
 * unit depending on param_1.
 *
 * Original register convention: in_EAX, param_1.
 *
 * @address 0x56c1d0
 */
void halo::units::unit_mark_zone_list_alt_flag(uint32_t zone_list_index, uint8_t use_second_bit)
{
    uint32_t object_index = k_datum_index_none;
    uint32_t next_link = k_datum_index_none;

    if (zone_list_index != k_datum_index_none) {
        uint32_t link = halo::objects::object_list_header_at(zone_list_index)->first_reference;
        if (link == k_datum_index_none) {
            object_index = k_datum_index_none;
            next_link = k_datum_index_none;
        } else {
            object_list_reference *node = reinterpret_cast<object_list_reference *>(halo::objects::object_list_reference_at(link));
            next_link = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != k_datum_index_none) {
        object_header *found = nullptr;
        if ((-1 < (int16_t)object_index) && ((int16_t)object_index < halo::objects::globals().object_data->maximum_count)) {
            object_header *hdr = (object_header *)halo::objects::globals().object_data->data + (int16_t)object_index;
            if ((hdr->identifier != 0) &&
                (((int16_t)(object_index >> 16) == 0) || (hdr->identifier == (int16_t)(object_index >> 16)))) {
                found = hdr;
            }
        }
        if ((found != nullptr) && ((_object_mask_unit & (1 << (found->type & 0x1f))) != 0) &&
            (found->data != nullptr)) {
            unit_data *unit = halo::units::unit_data_of(found->data);
            if (!use_second_bit) {
                set_flag(unit->flags, units::unit_flag::unknown_20000000);
            } else {
                set_flag(unit->flags, units::unit_flag::unknown_10000000);
            }
        }

        if (next_link == k_datum_index_none) {
            object_index = k_datum_index_none;
            next_link = k_datum_index_none;
        } else {
            object_list_reference *node = reinterpret_cast<object_list_reference *>(halo::objects::object_list_reference_at(next_link));
            next_link = node->next;
            object_index = node->object_index;
        }
    }
    return;
}

/**
 * Iterates a player/zone-indexed list of seat markers and marks each seated unit's flags field with bit
 * 0x100000.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56b290
 */
void halo::units::unit_mark_zone_occupants_flag(uint32_t zone_list_index)
{
    uint32_t object_index = k_datum_index_none;
    uint32_t next_link = k_datum_index_none;

    if (zone_list_index != k_datum_index_none) {
        uint32_t link = halo::objects::object_list_header_at(zone_list_index)->first_reference;
        if (link == k_datum_index_none) {
            object_index = k_datum_index_none;
            next_link = k_datum_index_none;
        } else {
            object_list_reference *node = reinterpret_cast<object_list_reference *>(halo::objects::object_list_reference_at(link));
            next_link = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != k_datum_index_none) {
        object_header *found = nullptr;
        if ((-1 < (int16_t)object_index) && ((int16_t)object_index < halo::objects::globals().object_data->maximum_count)) {
            object_header *hdr = (object_header *)halo::objects::globals().object_data->data + (int16_t)object_index;
            if ((hdr->identifier != 0) &&
                (((int16_t)(object_index >> 16) == 0) || (hdr->identifier == (int16_t)(object_index >> 16)))) {
                found = hdr;
            }
        }
        if ((found != nullptr) && ((_object_mask_unit & (1 << (found->type & 0x1f))) != 0) &&
            (found->data != nullptr)) {
            unit_data *unit = halo::units::unit_data_of(found->data);
            set_flag(unit->flags, units::unit_flag::delete_when_dropped);
        }

        if (next_link == k_datum_index_none) {
            object_index = k_datum_index_none;
            next_link = k_datum_index_none;
        } else {
            object_list_reference *node = reinterpret_cast<object_list_reference *>(halo::objects::object_list_reference_at(next_link));
            next_link = node->next;
            object_index = node->object_index;
        }
    }
    return;
}

/**
 * Looks up a named seat on the unit and reports whether the object currently occupying it also appears in the
 * zone/list referenced by param_3.
 *
 * @address 0x56b380
 */
uint8_t UnitView::named_seat_occupant_in_zone(char *seat_label, uint32_t zone_list_index)
{
    uint32_t unit_index = datum_handle;
    if (unit_index == k_datum_index_none) {
        return 0;
    }
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    if ((int32_t)unit_tag->seats.count < 1) {
        return 0;
    }
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;

    for (int16_t seat_index = 0; seat_index < (int32_t)unit_tag->seats.count; seat_index++) {
        if (_stricmp(seat_label, seats[seat_index].label.string) != 0) {
            continue;
        }

        object_iterator iter = { _object_mask_unit, 0, 0, 0, 0xffffffff };
        object *occupant = halo::objects::object_iterator_next(&iter);
        uint32_t occupant_index = k_datum_index_none;
        while (occupant != nullptr) {
            if ((occupant->parent_object == unit_index) &&
                ((halo::units::unit_data_of(occupant))->vehicle_seat_index == seat_index)) {
                occupant_index = iter.handle;
                break;
            }
            occupant = halo::objects::object_iterator_next(&iter);
        }
        if (occupant_index == k_datum_index_none) {
            continue;
        }

        uint32_t zone_object;
        uint32_t next_link;
        if (zone_list_index == k_datum_index_none) {
            zone_object = k_datum_index_none;
            next_link = k_datum_index_none;
        } else {
            uint32_t first_link = halo::objects::object_list_header_at(zone_list_index)->first_reference;
            if (first_link == k_datum_index_none) {
                zone_object = k_datum_index_none;
                next_link = k_datum_index_none;
            } else {
                uint32_t link_slot = first_link & 0xffff;
                object_list_reference *node = halo::objects::object_list_reference_at(link_slot);
                next_link = node->next;
                zone_object = node->object_index;
            }
        }

        while (zone_object != occupant_index) {
            if (zone_object == k_datum_index_none) {
                break;
            }
            if (next_link == k_datum_index_none) {
                zone_object = k_datum_index_none;
                next_link = k_datum_index_none;
            } else {
                object_list_reference *node = reinterpret_cast<object_list_reference *>(halo::objects::object_list_reference_at(next_link));
                next_link = node->next;
                zone_object = node->object_index;
            }
        }
        return zone_object == occupant_index;
    }
    return 0;
}

/**
 * Recomputes which child object occupies the unit's primary and secondary tracked seats (fields 0xc9/0xca) by
 * scanning its list of attached children.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x56ce30
 */
void UnitView::recompute_seat_occupants()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;
    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;

    datum_index child_index = unit_obj->first_child_object;
    while (child_index != k_datum_index_none) {
        object *child = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(child_index)].data;

        if (((_object_mask_unit & (1 << (child->type & 0x1f))) != 0) &&
            ((halo::units::unit_data_of(child))->vehicle_seat_index != -1)) {
            int16_t seat_index = (halo::units::unit_data_of(child))->vehicle_seat_index;
            uint32_t seat_flags = seats[seat_index].flags;

            if (((seat_flags & 4) == 0) || (test_flag(unit->flags, units::unit_flag::unattended)) || (unit->driver_unit_index != k_datum_index_none)) {
                if ((seat_flags & 8) != 0) {
                    if (unit->gunner_unit_index != k_datum_index_none) {
                        if (unit->gunner_unit_index == unit->driver_unit_index) {
                            unit->gunner_unit_index = child_index;
                        }
                    } else {
                        unit->gunner_unit_index = child_index;
                    }
                }
            } else {
                unit->driver_unit_index = child_index;
                if ((seat_flags & 8) != 0) {
                    if (unit->gunner_unit_index == k_datum_index_none) {
                        unit->gunner_unit_index = child_index;
                    }
                }
            }
        }
        child_index = child->next_object;
    }
    return;
}

/**
 * Performs the transient-state cleanup used when a unit fully leaves its seat, also clearing its
 * overlay-animation and weapon-switch fields
 *
 * Original register convention: param_1, param_2.
 *
 * @address 0x568cb0
 */
void UnitView::release_transient_state_and_detach(uint8_t is_light_reset)
{
    uint32_t unit_index = datum_handle;
    object *self_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(self_obj);

    if (!is_light_reset) {
        unit->feign_death_ticks = 0;
        if (unit->controlling_player != k_datum_index_none) {
            halo::game::player_reset_after_unit_change(unit->controlling_player);
            unit->controlling_player = k_datum_index_none;
        }
        if (unit->actor_index != k_datum_index_none) {
            actor *actor_rec = halo::ai::actor_at(unit->actor_index);
            ((struct unit_object *)self_obj)->unit.encounter_index = static_cast<int16_t>(actor_rec->encounter_index);
            ((struct unit_object *)self_obj)->unit.squad_index = actor_rec->squad_index;
            halo::ai::actor_attempt_grenade_throw(unit->actor_index);
            unit->actor_index = k_datum_index_none;
        }
        if (unit->swarm_actor_index != k_datum_index_none) {
            actor *actor_rec = halo::ai::actor_at(unit->swarm_actor_index);
            ((struct unit_object *)self_obj)->unit.encounter_index = static_cast<int16_t>(actor_rec->encounter_index);
            ((struct unit_object *)self_obj)->unit.squad_index = actor_rec->squad_index;
            halo::ai::actor_release_from_cluster_or_delete(unit->swarm_actor_index, unit_index);
            unit->swarm_actor_index = k_datum_index_none;
        }
        unit->death_time = halo::game::globals().game_time->game_time;
    } else {
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(self_obj->definition_tag)].data;
        if (unit_tag->feign_repeat_chance <= (float)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale) {
            clear_flag(unit->flags, units::unit_flag::unknown_2000);
        } else {
            set_flag(unit->flags, units::unit_flag::unknown_2000);
        }
    }

    clear_flag(unit->flags, units::unit_flag::unattended | units::unit_flag::unknown_10);
    unit->control_flags = 0;
    if (unit->current_weapon_index != -1) {
        int16_t slot = ((struct unit_object *)self_obj)->unit.current_weapon_index;
        uint32_t weapon_object_index = k_datum_index_none;
        if (slot != -1) {
            weapon_object_index = unit->weapons[slot];
        }
        object *weapon_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(weapon_object_index)].data;
        reinterpret_cast<weapon_object *>(weapon_obj)->weapon.control_flags = 0;
        ((struct unit_object *)weapon_obj)->unit.desired_aiming_vector.j = halo::math::transition_function_evaluate((transition_function_t)4, 0.0f);
    }
    clear_flag(unit->flags, units::unit_flag::idle_turn_seeded);

    if (self_obj->parent_object != k_datum_index_none) {
        if (unit->vehicle_seat_index == -1) {
            UnitView(unit_index).detach_reposition_and_nudge();
        } else {
            UnitView(unit_index).detach_from_seat(0, 1, 0);
        }
    }

    unit->pending_speech.priority = 0;
    UnitView(unit_index).drop_inventory_weapons();
    if (unit->equipment_object_index != k_datum_index_none) {
        UnitView(unit_index).drop_object_from_hand(unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    if (unit->delayed_weapon_drop_ticks == 0) {
        UnitView(unit_index).drop_current_weapon(1);
    }
    unit->overlays[1].animation_index = -1;
    unit->overlays[0].animation_index = -1;
    unit->melee_state = _unit_melee_state_none;
    if (unit->throwing_grenade_state == _unit_throwing_grenade_state_begin) {
        unit->throwing_grenade_state = _unit_throwing_grenade_state_none;
    }
    return;
}

namespace unit_seat_candidates_from_zone_and_enter_local {

static datum_index object_list_next(datum_index *reference)
{
    object_list_reference *link;

    if (*reference == k_datum_index_none) {
        return k_datum_index_none;
    }
    link = halo::objects::object_list_reference_at(*reference);
    *reference = link->next;
    return link->object_index;
}

}

/**
 * Engine function unit_seat_candidates_from_zone_and_enter.
 *
 * @address 0x56a4c0
 */
int16_t halo::units::unit_seat_candidates_from_zone_and_enter(datum_index vehicle_index, char *seat_name, datum_index object_list)
{
    using namespace unit_seat_candidates_from_zone_and_enter_local;
    int16_t seated = 0;
    object *vehicle;
    Unit *vehicle_tag;
    int16_t seats[16];
    int16_t seat_count;
    datum_index reference = k_datum_index_none;
    datum_index candidate_index;

    if (vehicle_index == k_datum_index_none) {
        return 0;
    }
    vehicle = reinterpret_cast<object *>(halo::objects::object_record_bytes(vehicle_index));
    vehicle_tag = halo::objects::tag_as<Unit>(*(datum_index *)vehicle);
    seat_count = UnitView(vehicle_index).find_seats_matching_name_and_flags(seat_name, 0xffff, seats, 0x10);
    if (object_list != k_datum_index_none) {
        reference = halo::objects::object_list_header_at(object_list)->first_reference;
    }
    for (candidate_index = object_list_next(&reference); candidate_index != k_datum_index_none;
         candidate_index = object_list_next(&reference)) {
        object *candidate = reinterpret_cast<object *>(halo::objects::object_record_bytes(candidate_index));
        int16_t i;

        if (!((1u << ((uint8_t)candidate->type & 0x1f)) & 3) || (test_flag(vehicle->vitality_flags, objects::vitality_flag::health_frozen))) {
            continue;
        }
        for (i = 0; i < seat_count; i++) {
            int16_t seat = seats[i];

            if (seat == -1) {
                continue;
            }
            if (candidate->type != _object_type_vehicle &&
                !UnitView(candidate_index).set_or_test_seat_and_weapon_label(halo::objects::block_element<UnitSeat>(vehicle_tag->seats, seat).label.string, 0, 0)) {
                continue;
            }
            if (candidate->parent_object != k_datum_index_none) {
                if (halo::units::unit_data_of(candidate)->vehicle_seat_index != -1 && halo::networking::globals().game_mode != halo::networking::k_game_mode_client) {
                    unit_object *self = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(candidate_index));

                    if (self->base.parent_object != k_datum_index_none && self->unit.vehicle_seat_index != -1) {
                        biped_detach_from_seat(candidate_index, self->base.parent_object);
                    }
                    if (self->base.network_role == 0) {
                        ::halo::units::unit_dispatch_scripted_event_9(1, (int32_t)candidate_index);
                    }
                    biped_free_local_player_history(self);
                }
                if (candidate->parent_object != k_datum_index_none) {
                    continue;
                }
            }
            if (::halo::units::unit_enter_vehicle_seat(vehicle_index, seat, candidate_index) & 0xff) {
                seats[i] = -1;
                seated++;
                break;
            }
        }
    }
    return seated;
}

/**
 * Returns flag bit 10 from the unit's seat definition at index in_CX.
 *
 * Original register convention: in_EAX, in_CX.
 *
 * @address 0x56cdd0
 */
uint8_t UnitView::seat_flag_bit10(int16_t seat_index)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    if ((-1 < seat_index) && (seat_index < (int32_t)unit_tag->seats.count)) {
        UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
        return (seats[seat_index].flags >> 10) & 1;
    }
    return 0;
}

/**
 * Returns a specific flag bit (bit 2) from the definition of the unit's seat at index in_CX.
 *
 * Original register convention: in_EAX, in_CX.
 *
 * @address 0x56cd10
 */
uint8_t UnitView::seat_flag_bit2(int16_t seat_index)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    if ((-1 < seat_index) && (seat_index < (int32_t)unit_tag->seats.count)) {
        UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
        return (seats[seat_index].flags >> 2) & 1;
    }
    return 0;
}

/**
 * Returns flag bit 3 from the unit's seat definition at index in_CX.
 *
 * Original register convention: in_EAX, in_CX.
 *
 * @address 0x56cd70
 */
uint8_t UnitView::seat_flag_bit3(int16_t seat_index)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    if ((-1 < seat_index) && (seat_index < (int32_t)unit_tag->seats.count)) {
        UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
        return (seats[seat_index].flags >> 3) & 1;
    }
    return 0;
}

/**
 * Engine function unit_seat_index_is_valid.
 *
 * Original register convention: see file header.
 *
 * @address 0x565150
 */
uint8_t halo::units::unit_seat_index_is_valid(uint32_t other_object_index, uint32_t unit_index, int16_t seat_index)
{
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;

    if (-1 < seat_index && seat_index < (int32_t)unit_tag->seats.count) {
        object *other_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(other_object_index)].data;
        if (other_obj->type == _object_type_vehicle) {
            return 1;
        }
        UnitSeat *seat = (UnitSeat *)(&halo::objects::block_element<UnitSeat>(unit_tag->seats, seat_index));
        if (UnitView(other_object_index).set_or_test_seat_and_weapon_label(seat->label.string, 0, 0) != 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * Engine function unit_seat_is_occupied_by_other.
 *
 * @address 0x566840
 */
uint8_t halo::units::unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index, uint32_t *out_occupant_index)
{
    object *self_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(self_index)].data;
    unit_data *self_unit = halo::units::unit_data_of(self_obj);

    uint32_t found = (uint32_t)-1;
    uint8_t not_found = (self_index != vehicle_index);

    object *vehicle_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(vehicle_index)].data;
    datum_index child = vehicle_obj->first_child_object;

    while (child != k_datum_index_none) {
        object *child_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(child)].data;

        if (((1 << (child_obj->type & 0x1f)) & 3) != 0) {
            unit_data *child_unit = halo::units::unit_data_of(child_obj);
            uint8_t match = child_unit->vehicle_seat_index == seat_index;
            uint32_t reassigned = child;
            if (!match && self_unit->controlling_player != k_datum_index_none) {
                reassigned = found;
                match = halo::game::teams_are_enemies(((struct object *)child_obj)->owner_team, ((struct object *)self_obj)->owner_team) != 0;
            }
            if (match) {
                not_found = 0;
                found = reassigned;
            }
        }
        child = child_obj->next_object;
    }

    if (out_occupant_index != 0) {
        *out_occupant_index = found;
    }
    return not_found;
}

/**
 * Engine function unit_set_or_test_seat_and_weapon_label.
 *
 * @address 0x5651e0
 */
uint8_t UnitView::set_or_test_seat_and_weapon_label(const char *seat_label, const char *weapon_label, uint8_t apply)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    Object *obj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    ModelAnimations *graph = halo::objects::tag_as<ModelAnimations>(obj_tag->animation_graph.tag_id.index);
    uint8_t found = 0;
    int16_t seat_i;

    for (seat_i = 0; seat_i < (int32_t)graph->units.count; seat_i++) {
        ModelAnimationsAnimationGraphUnitSeat *seat =
            (ModelAnimationsAnimationGraphUnitSeat *)(&halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(graph->units, seat_i));
        int16_t weapon_slot;

        if (seat_label != 0 && _stricmp(seat_label, seat->label.string) != 0) {
            continue;
        }
        for (weapon_slot = 0; weapon_slot < (int32_t)seat->weapons.count; weapon_slot++) {
            ModelAnimationsAnimationGraphWeapon *weapon_anim =
                &halo::objects::block_element<ModelAnimationsAnimationGraphWeapon>(seat->weapons, weapon_slot);
            int16_t weapon_type_i;

            for (weapon_type_i = 0; weapon_type_i < (int32_t)weapon_anim->weapon_types.count; weapon_type_i++) {
                ModelAnimationsAnimationGraphWeaponType *weapon_type =
                    (ModelAnimationsAnimationGraphWeaponType *)(&halo::objects::block_element<ModelAnimationsAnimationGraphWeaponType>(weapon_anim->weapon_types, weapon_type_i));

                if (weapon_label == 0) {
                    break;
                }
                if (strcmp(weapon_label, "unarmed") == 0 && weapon_type->label.string[0] == '\0') {
                    break;
                }
                if (_stricmp(weapon_label, weapon_type->label.string) == 0) {
                    break;
                }
            }
            if (weapon_type_i >= (int32_t)weapon_anim->weapon_types.count) {
                continue;
            }
            if (apply) {
                int32_t animation_count = (int32_t)seat->animations.count;
                int16_t *seat_animations = halo::objects::block_elements<int16_t>(seat->animations);
                uint8_t aiming = (uint8_t)((animation_count > 2 && seat_animations[2] != -1) ||
                                           (animation_count > 3 && seat_animations[3] != -1) ||
                                           (animation_count > 4 && seat_animations[4] != -1));
                int8_t base_state = -1;
                int16_t i;

                if ((uint8_t)unit->animation_state != animation_state_value(unit_animation_state_id::custom_animation)) {
                    unit->animation_state = -1;
                }
                unit->animation_definition_index = (int8_t)seat_i;
                for (i = 0; i < 6; i++) {
                    if (_stricmp(seat_label, unit_base_animation_state_names[i]) == 0) {
                        base_state = (int8_t)i;
                        break;
                    }
                }
                unit->animation_weapon_type_index = (int8_t)weapon_type_i;
                unit->base_animation_state = base_state;
                unit->animation_weapon_index = (int8_t)weapon_slot;
                if (aiming) {
                    set_flag(unit->animation_state_flags, units::unit_animation_state_flag::aiming_enabled);
                } else {
                    clear_flag(unit->animation_state_flags, units::unit_animation_state_flag::aiming_enabled);
                }
            }
            found = 1;
        }
    }
    return found;
}

namespace unit_try_exit_controlled_seat_local {

}

/**
 * Engine function unit_try_exit_controlled_seat.
 *
 * @address 0x56b5f0
 */
void UnitView::try_exit_controlled_seat()
{
    using namespace unit_try_exit_controlled_seat_local;
    uint32_t unit_index = datum_handle;
    unit_object *obj;
    unit_object *self;
    datum_index vehicle_index;

    if (unit_index == k_datum_index_none) {
        return;
    }
    obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    if (obj->base.parent_object == k_datum_index_none || obj->unit.vehicle_seat_index == -1) {
        return;
    }
    self = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(unit_index, 3));
    if (self == 0 || halo::networking::globals().game_mode == halo::networking::k_game_mode_client || self->base.parent_object == k_datum_index_none ||
        self->unit.vehicle_seat_index == -1) {
        return;
    }
    if (self->base.type == _object_type_vehicle) {
        vehicle_index = obj->base.parent_object;
        if (vehicle_index != k_datum_index_none && obj->unit.vehicle_seat_index != -1) {
            biped_detach_from_seat(unit_index, vehicle_index);
        }
        biped_free_local_player_history(halo::objects::object_as<unit_object>(unit_index));
        return;
    }
    if (!::halo::units::unit_state_is_scripted_animation(halo::units::unit_data_of(self))) {
        Unit *self_tag = halo::objects::tag_as<Unit>(*(datum_index *)self);
        datum_index graph = halo::objects::tag_handle(self_tag->base.animation_graph);
        const ModelAnimationsAnimationGraphUnitSeat &seat_block = halo::objects::block_element<ModelAnimationsAnimationGraphUnitSeat>(
            halo::objects::tag_as<ModelAnimations>(graph)->units, (int8_t)(uint8_t)self->unit.animation_definition_index);
        int16_t exit_animation = (int32_t)seat_block.animations.count > 8 ? halo::objects::block_element<int16_t>(seat_block.animations, 8) : int16_t(-1);

        if (exit_animation != -1) {
            uint8_t *object;
            Object *object_tag;

            vehicle_index = self->base.parent_object;
            if (((struct unit_object *)halo::objects::object_record_bytes(vehicle_index))->unit.driver_unit_index == unit_index) {
                UnitView((int32_t)vehicle_index).notify_weapon_removed();
            }
            UnitView(unit_index).set_custom_animation(halo::objects::tag_handle(self_tag->base.animation_graph), halo::models::animation_choose_random_permutation(graph, exit_animation, (animation_random_stream)1));
            object = halo::objects::object_record_bytes(unit_index);
            object_tag = halo::objects::tag_as<Object>(*(datum_index *)object);
            if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
                if (test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
                    halo::objects::object_for_each_light_attachment(unit_index, 0, 1);
                }
                if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
                    clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                    halo::objects::object_header_of(unit_index).flags |= 2;
                }
            }
            self->unit.animation_state = animation_state_value(unit_animation_state_id::seat_exit);
            halo::ai::actor_notify_weapon_pickup_once(unit_index);
            if (self->base.network_role == 0) {
                ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)unit_index);
            }
        }
    }
}

}
