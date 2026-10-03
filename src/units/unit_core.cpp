#include "halo/game/records.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/units/records.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/core/lcg.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "networking.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/devices/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
static constexpr int32_t k_max_tracked_units = 4;

static auto &global_globals = halo::link::ref<uint8_t *>(halo::game::vars().global_globals);
static auto &object_network_id_table = halo::link::ref<network_id_table *>(halo::units::vars().object_network_id_table);
static auto &network_message_scratch = halo::link::ref<uint8_t [halo::k_network_message_scratch_size]>(halo::game::vars().network_message_scratch);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &unit_base_animation_state_names = halo::link::ref<char *[6]>(halo::units::vars().unit_base_animation_state_names);

namespace halo::units {

/**
 * Adds a positional impulse to a unit: halves it first if the unit is attached to a parent (running the
 * ground-adjust dirty-flag clear either way), accumulates it into velocity, marks the biped grounded/jumping
 * flags, applies a small random angular jitter scaled by the impulse magnitude when the unit is unattended or
 * its weapon-mode tag flag is set, and -- for an unattached unit only -- reorients its forward vector toward
 * the impulse direction when it has a nonzero length.
 *
 * @address 0x559fa0
 */
void UnitView::apply_impulse(real_vector3d *impulse)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    void *tag_data = halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);

    if (test_flag(((struct Unit *)tag_data)->unit_flags, tags::unit_tag_flag::special_cinematic_unit)) {
        return;
    }

    if (!test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen)) {
        impulse->i *= 0.5f;
        impulse->j *= 0.5f;
        impulse->k *= 0.5f;
    }
    UnitView(object_index).clear_ground_adjust_dirty();

    obj->velocity.i += impulse->i;
    obj->velocity.j += impulse->j;
    obj->velocity.k += impulse->k;
    clear_flag(obj->flags, objects::object_flag::at_rest);
    set_flag(biped->flags, units::biped_flag::airborne | units::biped_flag::jumping);

    if (test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen) || (*(uint8_t *)((uint8_t *)tag_data + 0x2f4) & 0x44) != 0) {
        real_vector3d jitter_axis;
        float length;
        halo::math::vector3d_cross_product(jitter_axis, *impulse, *(halo::math::globals().global_up3d_pointer));
        length = halo::math::vector3d_normalize_with_length(jitter_axis);
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        {
            float magnitude = (float)halo::libm::sqrt((double)(impulse->i * impulse->i + impulse->j * impulse->j +
                                                     impulse->k * impulse->k));
            float angle = (float)(int32_t)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale * magnitude * 1.5707964f;
            obj->angular_velocity.i += jitter_axis.i * angle;
            obj->angular_velocity.j += jitter_axis.j * angle;
            obj->angular_velocity.k += jitter_axis.k * angle;
        }
    }

    if (obj->parent_object == k_datum_index_none) {
        real_vector3d direction = *impulse;
        float length = halo::math::vector3d_normalize_with_length(direction);
        if (length > 0.0f) {
            obj->forward = direction;
            ::halo::units::unit_update_up_vector((Biped *)(Biped *)tag_data, (::object *)obj);
        }
    }
}

/**
 * Detects when a unit has fallen far below the level (Z < -2000) while marked deleted-pending or outside any
 * BSP cluster, and deletes it.
 *
 * @address 0x55e4a0
 */
void UnitView::check_fell_off_level()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;

    if (halo::game::globals().current_engine == 0 &&
        (test_flag(obj->flags, objects::object_flag::outside_map) || obj->location_cluster_index == -1)) {
        if (obj->position.z < -2000.0f) {
            halo::objects::object_delete(object_index);
        }
    }
}

/**
 * Dispatches a scripted event (id 0x1b) that includes hashed identifiers for both the unit and its currently
 * equipped weapon.
 *
 * Original register convention: param_1, in_ECX.
 *
 * @address 0x56dcd0
 */
void halo::units::unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index)
{
    int32_t unit_hash = 0;
    if (unit_index != k_datum_index_none) {
        unit_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)unit_index);
        if (unit_hash == -1) {
            unit_hash = 0;
        }
    }

    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int32_t weapon_hash = 0;
    if (current_weapon != k_datum_index_none) {
        weapon_hash = halo::objects::hash_table_get(&object_network_id_table->id_to_index, (int32_t)current_weapon);
        if (weapon_hash == -1) {
            weapon_hash = 0;
        }
    }

    struct { int32_t unit_hash; int32_t weapon_hash; uint8_t event_byte; } item;
    void *items[1];
    int32_t encoded_len;
    item.unit_hash = unit_hash;
    item.weapon_hash = weapon_hash;
    item.event_byte = event_byte;
    items[0] = &item;
    encoded_len = halo::networking::message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 0x1b, 0, items, 0, 1, 0);
    if (0 < encoded_len) {
        halo::networking::network_session_broadcast_to_flagged(encoded_len, halo::networking::globals().server, 1, network_message_scratch, 1, 0, 0, 3);
    }
    return;
}

/**
 * object_type_definition "unit" row, +0x3c column. Scrubs every reference this unit holds to
 * `forgotten_object_index` (a thrown grenade, a driver/gunner seat occupant, an inventory weapon, held
 * equipment, or the last damage attacker), and, if the current weapon slot was cleared, picks a new one via
 * unit_find_next_zone_permitted_weapon_slot.
 *
 * @address 0x56f0f0
 */
void UnitView::forget_object_reference(datum_index forgotten_object_index)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    int16_t slot;

    if (unit->throwing_grenade_projectile == forgotten_object_index) {
        unit->throwing_grenade_projectile = k_datum_index_none;
    }
    if (unit->driver_unit_index == forgotten_object_index) {
        unit->driver_unit_index = k_datum_index_none;
    }
    if (unit->gunner_unit_index == forgotten_object_index) {
        unit->gunner_unit_index = k_datum_index_none;
    }

    for (slot = 0; slot < 4; slot++) {
        if (unit->weapons[slot] == forgotten_object_index) {
            unit->weapons[slot] = k_datum_index_none;
            if (slot == unit->desired_weapon_index) {
                unit->desired_weapon_index = -1;
            }
            if (slot == unit->current_weapon_index) {
                unit->current_weapon_index = -1;
            }
        }
    }

    if (unit->current_weapon_index == -1) {
        unit->desired_weapon_index = UnitView(object_index).find_next_zone_permitted_weapon_slot(-1, 0);
    }

    if (unit->equipment_object_index == forgotten_object_index) {
        unit->equipment_object_index = k_datum_index_none;
    }
    if (unit->delayed_damage_responsible_object == forgotten_object_index) {
        unit->delayed_damage_responsible_object = k_datum_index_none;
    }
}

/**
 * Returns whether a biped is eligible for idle behaviors (delegated to biped_is_idle_eligible) only when the
 * unit is a biped; returns 0 for other unit types.
 *
 * @address 0x570ad0
 */
uint32_t UnitView::get_biped_specific_value()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->type == _object_type_biped) {
        return BipedView(object_index).is_idle_eligible();
    }
    return 0;
}

/**
 * Produces the biped's collision pill for this tick and lifts its origin onto the pill's centre: the object's
 * own position gains one collision_radius unless biped_flags bit 3 ("physics_pill_centered_at_origin") says
 * the pill is already centred there. The pill height is the standing / crouching collision height blended by
 * crouch_fraction, less the two hemispherical caps (twice the radius). A spherical biped (bit 4) that is
 * neither player-controlled nor carrying object flag 0x400000 gets a zero-height pill, i.e.
 *
 * @address 0x55a2e0
 */
void halo::units::unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out)
{
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    biped_data *biped = halo::units::biped_data_of(obj);
    unit_data *unit = halo::units::unit_data_of(obj);

    halo::objects::object_get_position(object_position, object_index);

    if (!test_flag(tag->biped_flags, tags::biped_tag_flag::physics_pill_centered_at_origin)) {
        object_position->z += tag->collision_radius;
    }

    if (!test_flag(tag->biped_flags, tags::biped_tag_flag::spherical) &&
        (unit->controlling_player != k_datum_index_none || test_flag(obj->flags, objects::object_flag::unknown_400000))) {
        *pill_height = ((tag->crouching_collision_height - tag->standing_collision_height) * biped->crouch_fraction +
                        tag->standing_collision_height) - (tag->collision_radius + tag->collision_radius);
        *pill_radius_out = tag->collision_radius;
        return;
    }
    *pill_height = 0.0f;
    *pill_radius_out = tag->collision_radius;
}

/**
 * Returns a single flag bit (bit 6) from the unit's 0x204 flags dword.
 *
 * Original register convention: in_EAX.
 *
 * @address 0x569bc0
 */
uint32_t UnitView::get_flag_bit6()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    return (unit->flags >> 6) & 1;
}

/**
 * Engine function unit_get_hud_interface_tag_id.
 *
 * Original register convention: in_EDX -> unit_tag, in_AL -> use_second.
 *
 * @address 0x560c70
 */
TagID halo::units::unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second)
{
    int32_t index = (int32_t)unit_tag->new_hud_interfaces.count - 1;
    if (index > (use_second != 0)) {
        index = use_second != 0;
    }
    if (index < 0) {
        TagID none = {0xffff, 0xffff};
        return none;
    }
    UnitUnitHudInterface *entries = (UnitUnitHudInterface *)unit_tag->new_hud_interfaces.pointer;
    return entries[(int16_t)index].hud.tag_id;
}

/**
 * Tests whether object.flags bit 0x20 (extension_of_parent) is set on the unit; used by vehicle_update
 * alongside a distance check to decide whether to force a position resync.
 *
 * @address 0x570c80
 */
uint8_t UnitView::get_recently_updated_flag()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    return (obj->flags & 0x20) == 0x20;
}

/**
 * Returns bit 7 of the tag word immediately following Unit (Vehicle.vehicle_flags's causes_collision_damage
 * bit, for a vehicle).
 *
 * @address 0x571c70
 */
uint32_t UnitView::get_tag_flag_bit7()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = halo::objects::tag_record_bytes(obj->definition_tag);
    return (*(uint32_t *)(tag + 0x2f0) >> 7) & 1;
}

/**
 * Returns whether the unit has any attached child object of type 5 (projectile), walking the same
 * child-object sibling chain as unit_propagate_position_delta_to_children.
 *
 * @address 0x570d70
 */
uint8_t UnitView::has_child_of_type5()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    datum_index child = obj->first_child_object;

    if (child == k_datum_index_none) {
        return 0;
    }

    for (;;) {
        object *child_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(child)].data;
        if ((1 << (child_obj->type & 0x1f) & 0x20) != 0) {
            return 1;
        }
        child = child_obj->next_object;
        if (child == k_datum_index_none) {
            return 0;
        }
    }
}

/**
 * Resolves a small globally tracked list of parentless unit objects and returns whether no other nearby
 * (within 100 units, per the squared-distance test) attached object with significant velocity exists near any
 * of them.
 *
 * @address 0x575c50
 */
uint8_t halo::units::unit_is_area_clear_of_fast_objects(void)
{
    real_point3d tracked_positions[k_max_tracked_units];
    int32_t tracked_count = 0;
    int16_t slot = -1;
    uint8_t result = 1;

    if (*(int32_t *)halo::game::globals().local_player_globals->local_players != -1) {
        slot = 0;
    }

    while (slot != -1) {
        if (slot >= 0 && slot < 1) {
            uint32_t player_handle = *(uint32_t *)&halo::game::globals().local_player_globals->local_players[slot];
            if (player_handle != k_datum_index_none) {
                uint32_t unit_handle = halo::game::player_at(player_handle)->unit;
                if (unit_handle != k_datum_index_none) {
                    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_handle)].data;
                    if (obj->parent_object == k_datum_index_none && tracked_count < k_max_tracked_units) {
                        tracked_positions[tracked_count] = obj->bounding_center;
                        tracked_count++;
                    }
                }
            }
        }
        slot = (*(int32_t *)halo::game::globals().local_player_globals->local_players != -1 && slot < 0) ? 0 : -1;
    }

    if (tracked_count != 0) {
        object_iterator iter = {0};
        iter.type_mask = 0x86868686;
        iter.handle = k_datum_index_none;
        object *obj;

        while ((obj = halo::objects::object_iterator_next(&iter)) != 0) {
            int32_t i;
            for (i = 0; i < tracked_count; i++) {
                if (obj->parent_object != k_datum_index_none) {
                    float dx = obj->bounding_center.x - tracked_positions[i].x;
                    float dy = obj->bounding_center.y - tracked_positions[i].y;
                    float dz = obj->bounding_center.z - tracked_positions[i].z;
                    if (dx * dx + dy * dy + dz * dz < 100.0f &&
                        obj->velocity.k * obj->velocity.k + obj->velocity.j * obj->velocity.j +
                        obj->velocity.i * obj->velocity.i >= 0.0011111111f) {
                        result = 0;
                        return !result;
                    }
                }
            }
        }
    }
    return !result;
}

/**
 * Engine function unit_point_in_front_and_asleep.
 *
 * @address 0x56bc80
 */
uint8_t halo::units::unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index)
{
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(unit_index, 3));
    float dot;

    if (obj == 0 || obj->base.type != _object_type_biped) {
        return 0;
    }
    if (*(uint32_t *)(halo::objects::tag_record_bytes(*(datum_index *)obj) + 0x17c) & 0x10000) {
        return 0;
    }
    dot = (obj->base.bounding_center.z - world_point->z) * obj->unit.looking_vector.k +
          (obj->base.bounding_center.y - world_point->y) * obj->unit.looking_vector.j +
          (obj->base.bounding_center.x - world_point->x) * obj->unit.looking_vector.i;
    if (!(dot > 0.0f)) {
        const char *name = UnitView(unit_index).get_seat_or_state_name();
        const char *asleep = unit_base_animation_state_names[0];

        for (;;) {
            if (*asleep != *name) return 0;
            if (*asleep == 0) break;
            asleep++;
            name++;
        }
    }
    return 1;
}

/**
 * Tracks how long the unit has been airborne, resetting the counter and bumping a landing-recovery counter
 * whenever any contact-point marker reports full (bit 2) or partial (bit 0x10) ground contact.
 *
 * @address 0x575640
 */
void UnitView::update_ground_contact_counter(uint8_t *contact_points)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    Physics *physics_tag = halo::objects::tag_as<Physics>(halo::objects::tag_handle(((Unit *)tag)->base.physics));
    vehicle_data *vehicle = halo::units::vehicle_data_of(obj);
    int32_t count = physics_tag->mass_points.count;
    int32_t i;

    if (vehicle->airborne_ticks != 0xff) {
        vehicle->airborne_ticks += 1;
    }

    for (i = 0; i < count; i++) {
        uint32_t flags = *(uint32_t *)(contact_points + i * 0x130);
        if ((flags & 2) != 0) {
            vehicle->airborne_ticks = 0;
            if ((int8_t)vehicle->landing_ticks != -1) {
                vehicle->landing_ticks += 1;
            }
            return;
        }
        if ((flags & 0x10) != 0) {
            vehicle->airborne_ticks = 0;
        }
    }
    vehicle->landing_ticks = 0;
}

}
