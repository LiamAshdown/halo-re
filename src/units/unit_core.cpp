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

extern "C" {
extern data_array *object_data;
extern double sqrt(double x);
extern uint8_t *global_globals;
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index, int16_t region_index, int16_t material_index, uint32_t plane);
extern game_engine_definition *current_game_engine;
extern void object_delete(uint32_t object_index);
extern uint8_t event9_target;
extern int32_t hash_table_get(hash_table *table, int32_t key);
extern network_id_table *object_network_id_table;
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern network_server_globals *network_server;
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, int32_t force, int32_t unused);
extern void object_get_position(real_point3d *out_position, uint32_t object_index);
extern player_globals *local_player_globals;
extern data_array *player_data;
extern object * object_iterator_next(object_iterator *iterator);
extern char *unit_base_animation_state_names[6];
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask);
}

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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    void *tag_data = tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

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
        halo::math::vector3d_cross_product(&jitter_axis, impulse, global_up3d_pointer);
        length = halo::math::vector3d_normalize_with_length(&jitter_axis);
        random_seed_global = halo::advance_random_seed(random_seed_global);
        {
            float magnitude = (float)sqrt((double)(impulse->i * impulse->i + impulse->j * impulse->j +
                                                     impulse->k * impulse->k));
            float angle = (float)(int32_t)(random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale * magnitude * 1.5707964f;
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
 * Tests line-of-sight/visibility from the unit toward a target point along a caller-supplied grid basis,
 * applying melee-response damage and decal/light effects along the trace. UNSURE: see file header -- the
 * scan's own basis vectors could not be recovered and are parameters here rather than constants this function
 * derives itself.
 *
 * @address 0x56f800
 */
void UnitView::can_see_point(real_vector3d *target_direction, real_vector3d *perp, real_vector3d *up)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    Unit *tag = (Unit *)tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    uint32_t best_object = k_datum_index_none;
    int32_t best_object_distance = -1;
    int32_t best_decal = -1;
    int32_t best_decal_extra = 0;
    int16_t best_object_type = 0;
    float best_object_fraction = 0.0f;

    for (int32_t row = -2; row <= 2; row++) {
        for (int32_t col = -2; col <= 2; col++) {
            real_vector3d delta;
            uint8_t scratch[0x50];
            int16_t *hit_kind = (int16_t *)scratch;

            delta.i = target_direction->i * 0.8f + ((float)col * perp->i + (float)row * up->i) * 0.1f;
            delta.j = target_direction->j * 0.8f + ((float)col * perp->j + (float)row * up->j) * 0.1f;
            delta.k = target_direction->k * 0.8f + ((float)col * perp->k + (float)row * up->k) * 0.1f;

            if (halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::front_face | halo::collision_test_flag::ignore_invisible | halo::collision_test_flag::structure_bsp | halo::collision_test_flag::water_surface | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::unstick), (real_point3d *)target_direction, &delta, unit_index, scratch) == 0) {
                continue;
            }

            if (*hit_kind == 3) {
                datum_index candidate = *(datum_index *)(scratch + 0x38);
                object *cand_obj = ((object_header *)object_data->data)[halo::datum_slot(candidate)].data;
                uint32_t candidate_index = candidate;

                if (cand_obj->type != 2 && cand_obj->parent_object != k_datum_index_none) {
                    candidate_index = cand_obj->parent_object;
                    cand_obj = ((object_header *)object_data->data)[halo::datum_slot(candidate_index)].data;
                }

                if (best_object == k_datum_index_none ||
                    (cand_obj->type == 0 &&
                     ((best_object_type == 0 && *(float *)(scratch + 0x14) < best_object_fraction) ||
                      best_object_type != 0))) {
                    best_object_type = cand_obj->type;
                    best_object_distance = *(int32_t *)(scratch + 0x34);
                    best_object_fraction = *(float *)(scratch + 0x14);
                    best_object = candidate_index;
                }
            }
        }
    }

    {
        int32_t weapon_response_tag = -1;
        int32_t secondary_damage_effect = -1;

        if (unit->current_weapon_index != -1) {
            datum_index weapon_index = unit->weapons[unit->current_weapon_index];
            if (weapon_index != k_datum_index_none) {
                object *weapon_obj = ((object_header *)object_data->data)[halo::datum_slot(weapon_index)].data;
                Weapon *weapon_tag = (Weapon *)tag_instances[halo::datum_slot(weapon_obj->definition_tag)].data;
                weapon_response_tag = *(int32_t *)&weapon_tag->player_melee_response.tag_id;
                secondary_damage_effect = *(int32_t *)&((struct Weapon *)weapon_tag)->player_melee_response.tag_id;
            }
        }
        int32_t damage_effect_tag = (weapon_response_tag != -1) ? weapon_response_tag
                                                                  : *(int32_t *)&tag->melee_damage.tag_id;

        if (best_object != k_datum_index_none) {
            object *best_obj = ((object_header *)object_data->data)[halo::datum_slot(best_object)].data;
            if (best_obj->type == 1 && best_obj->network_role != 1) {
                UnitView(best_object).apply_impulse_to_seat(target_direction);
            }
        }

        if (damage_effect_tag != -1) {
            damage_data dd = {0};

            dd.damage_effect_tag = damage_effect_tag;
            dd.flags = 1;
            dd.responsible_player = obj->owner_team;
            dd.responsible_object = unit_index;
            dd.team_index = (int16_t)obj->owner_team;
            dd.location_leaf_index = obj->location_leaf_index;
            *(int32_t *)&dd.location_cluster_index = *(int32_t *)&obj->location_cluster_index;
            dd.epicentre = *(real_point3d *)target_direction;
            dd.origin = obj->bounding_center;
            dd.direction = *target_direction;
            dd.random_blend = 1.0f;
            dd.multiplier = 1.0f;
            dd.material_type = (int16_t)best_object_distance;

            if (best_object == k_datum_index_none) {
                if ((int16_t)best_decal != -1) {
                    halo::physics::breakable_surface_apply_damage(&dd, best_decal, best_decal_extra);
                }
            } else {
                object *best_obj = ((object_header *)object_data->data)[halo::datum_slot(best_object)].data;
                if (best_obj->type == 7) {
                    halo::devices::device_machine_melee_attacked(best_object);
                }
                if (*(float *)(global_globals + 0x174 + 0x34) > 0.0f) {
                    float f = (obj->forward.i * obj->velocity.i + obj->forward.j * obj->velocity.j +
                               obj->forward.k * obj->velocity.k) * 30.0f /
                              *(float *)(global_globals + 0x174 + 0x34);
                    dd.random_blend = (f < 0.0f) ? 0.0f : (f > 1.0f ? 1.0f : f);
                }
                if (obj->type == 0 && *(int8_t *)((uint8_t *)obj + 0x501) > 0x0f) {
                    dd.random_blend = 1.5f;
                }
                if (best_obj->type == 0) {
                    object_apply_damage(&dd, best_object, -1, -1, -1, 0);
                }
            }
        }

        if ((int16_t)best_object_distance != -1) {
            ::halo::units::unit_trigger_material_hit_effect((int16_t)best_object_distance, k_datum_index_none, k_datum_index_none);
            if (secondary_damage_effect != -1) {
                damage_data dd2 = {0};
                dd2.damage_effect_tag = secondary_damage_effect;
                dd2.team_index = -1;
                dd2.responsible_player = k_datum_index_none;
                dd2.responsible_object = k_datum_index_none;
                dd2.location_leaf_index = 0;
                *(int16_t *)&dd2.location_cluster_index = -1;
                dd2.epicentre = obj->bounding_center;
                dd2.origin = obj->bounding_center;
                dd2.direction.i = -target_direction->i;
                dd2.direction.j = -target_direction->j;
                dd2.direction.k = -target_direction->k;
                dd2.random_blend = 1.0f;
                dd2.multiplier = 1.0f;
                dd2.material_type = -1;
                object_apply_damage(&dd2, unit_index, -1, -1, -1, 0);
            }
        }
        unit->melee_state = 0;
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (current_game_engine == 0 &&
        (test_flag(obj->flags, objects::object_flag::outside_map) || obj->location_cluster_index == -1)) {
        if (obj->position.z < -2000.0f) {
            object_delete(object_index);
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
void unit_dispatch_scripted_event_1b(uint8_t event_byte, uint32_t unit_index)
{
    int32_t unit_hash = 0;
    if (unit_index != k_datum_index_none) {
        unit_hash = hash_table_get(&object_network_id_table->id_to_index, (int32_t)unit_index);
        if (unit_hash == -1) {
            unit_hash = 0;
        }
    }

    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    datum_index current_weapon = k_datum_index_none;
    if (unit->current_weapon_index != -1) {
        current_weapon = unit->weapons[unit->current_weapon_index];
    }

    int32_t weapon_hash = 0;
    if (current_weapon != k_datum_index_none) {
        weapon_hash = hash_table_get(&object_network_id_table->id_to_index, (int32_t)current_weapon);
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
    encoded_len = message_delta_encode_message((int32_t)network_message_scratch, halo::k_network_message_scratch_size, 0, 0x1b, 0, items, 0, 1, 0);
    if (0 < encoded_len) {
        network_session_broadcast_to_flagged(encoded_len, network_server, 1, network_message_scratch, 1, 0, 0, 3);
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

    if (obj->type == 0) {
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
void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height, float *pill_radius_out)
{
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)tag_instances[halo::datum_slot(obj->definition_tag)].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    object_get_position(object_position, object_index);

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
    object *unit_obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    return (unit->flags >> 6) & 1;
}

/**
 * Engine function unit_get_hud_interface_tag_id.
 *
 * Original register convention: in_EDX -> unit_tag, in_AL -> use_second.
 *
 * @address 0x560c70
 */
TagID unit_get_hud_interface_tag_id(Unit *unit_tag, uint8_t use_second)
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)tag_instances[halo::datum_slot(obj->definition_tag)].data;
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    datum_index child = obj->first_child_object;

    if (child == k_datum_index_none) {
        return 0;
    }

    for (;;) {
        object *child_obj = ((object_header *)object_data->data)[halo::datum_slot(child)].data;
        if ((1 << (child_obj->type & 0x1f) & 0x20) != 0) {
            return 1;
        }
        child = child_obj->next_object;
        if (child == k_datum_index_none) {
            return 0;
        }
    }
}

#define K_MAX_TRACKED_UNITS 4
/**
 * Resolves a small globally tracked list of parentless unit objects and returns whether no other nearby
 * (within 100 units, per the squared-distance test) attached object with significant velocity exists near any
 * of them.
 *
 * @address 0x575c50
 */
uint8_t unit_is_area_clear_of_fast_objects(void)
{
    real_point3d tracked_positions[K_MAX_TRACKED_UNITS];
    int32_t tracked_count = 0;
    int16_t slot = -1;
    uint8_t result = 1;

    if (*(int32_t *)local_player_globals->local_players != -1) {
        slot = 0;
    }

    while (slot != -1) {
        if (slot >= 0 && slot < 1) {
            uint32_t player_handle = *(uint32_t *)&local_player_globals->local_players[slot];
            if (player_handle != k_datum_index_none) {
                uint32_t unit_handle = *(uint32_t *)((uint8_t *)player_data->data +
                                                      halo::datum_slot(player_handle) * 0x200 + 0x34);
                if (unit_handle != k_datum_index_none) {
                    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_handle)].data;
                    if (obj->parent_object == k_datum_index_none && tracked_count < K_MAX_TRACKED_UNITS) {
                        tracked_positions[tracked_count] = obj->bounding_center;
                        tracked_count++;
                    }
                }
            }
        }
        slot = (*(int32_t *)local_player_globals->local_players != -1 && slot < 0) ? 0 : -1;
    }

    if (tracked_count != 0) {
        object_iterator iter = {0};
        iter.type_mask = 0x86868686;
        iter.handle = k_datum_index_none;
        object *obj;

        while ((obj = object_iterator_next(&iter)) != 0) {
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
#undef K_MAX_TRACKED_UNITS

/**
 * Engine function unit_point_in_front_and_asleep.
 *
 * @address 0x56bc80
 */
uint8_t unit_point_in_front_and_asleep(real_point3d *world_point, uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)object_try_and_get(unit_index, 3);
    float dot;

    if (obj == 0 || ((unit_object *)obj)->base.type != 0) {
        return 0;
    }
    if (*(uint32_t *)((uint8_t *)tag_instances[halo::datum_slot(*(datum_index *)obj)].data + 0x17c) & 0x10000) {
        return 0;
    }
    dot = (((unit_object *)obj)->base.bounding_center.z - world_point->z) * ((unit_object *)obj)->unit.looking_vector.k +
          (((unit_object *)obj)->base.bounding_center.y - world_point->y) * ((unit_object *)obj)->unit.looking_vector.j +
          (((unit_object *)obj)->base.bounding_center.x - world_point->x) * ((unit_object *)obj)->unit.looking_vector.i;
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
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)tag_instances[halo::datum_slot(obj->definition_tag)].data;
    uint8_t *physics_tag = (uint8_t *)tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    int32_t count = *(int32_t *)(physics_tag + 0x74);
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
