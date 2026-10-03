#include "halo/hs/script_globals.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/lcg.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/scenario/api.hpp"
#include "game.h"
#include "hs.h"
#include "physics.h"
#include "projectiles.h"
#include "ai.h"
#include "crt.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center, float radius);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern uint8_t *global_structure_bsp;
extern real_vector3d placement_offset_table[27];
extern uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context);
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
extern uint8_t physics_point_find_clear_position(uint32_t flags, real_point3d *current_position, float sample_radius, float x_margin, float y_margin, uint32_t exclude_object_index, real_point3d *out_position);
extern uint8_t collision_test_movement_pill(uint32_t flags, real_point3d *origin, float radius, real_vector3d *delta, collision_result *result);
extern uint8_t object_collision_context_test_pill(object_collision_context *context, real_point3d *origin, real_vector3d *delta, float radius_scale, object_node_collision_result *out_result);
extern uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags, uint32_t exclude_object_index, collision_result *result);
extern int16_t network_game_mode;
extern ai_globals *ai_globals_ptr;
extern char *s_stand;
extern int16_t actor_spawn_additional_units(datum_index actor_variant_tag, int16_t spawn_count, datum_index source_actor_index, float health_scale);
extern double sqrt(double x);
extern real_point3d *global_origin3d_pointer;
extern Globals *global_globals;
extern uint8_t unit_updates_suppressed;
extern uint8_t actor_get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity, uint32_t object_index, float speed_limit);
}

namespace halo::units {

/**
 * Engine function unit_apply_scale_change.
 *
 * Original register convention: see file header.
 *
 * @address 0x562030
 */
void UnitView::apply_scale_change(unit_scale_request *request)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;

    if (request->scale > 0.0f) {
        obj->body_vitality = request->scale;
    }
    if ((request->flags & 1) != 0) {
        UnitView(unit_index).update_stance_and_jump(1, 0, 0, 0, 0, 0.0f, -1, 0, 1);
        if (unit->animation_state == 0x19) {
            UnitView(unit_index).drop_inventory_weapons_except_current();
            unit->grenade_counts[0] = 0;
            unit->grenade_counts[1] = 0;
            if (unit->equipment_object_index != k_datum_index_none) {
                halo::objects::object_delete(unit->equipment_object_index);
                unit->equipment_object_index = k_datum_index_none;
            }
            void *graph = halo::cache::globals().tag_instances[obj_tag->animation_graph.tag_id.index].data;
            uint8_t *animations = *(uint8_t **)&((ModelAnimations *)graph)->animations.pointer;
            ModelAnimationsAnimation *anim =
                (ModelAnimationsAnimation *)(animations + obj->animation_index * 0xb4);
            int32_t remaining = (int32_t)anim->frame_count - 4;
            obj->vitality_flags = obj->vitality_flags | _object_health_frozen_bit;
            unit->flags = unit->flags | _unit_flag_unknown_200;
            obj->animation_frame = (int16_t)((remaining < 0) ? 0 : remaining);
            obj->flags = obj->flags | _object_unknown_20000_bit;
            unit->death_time = halo::game::globals().game_time->game_time;
            obj->body_vitality = 0.0f;
            obj->shield_vitality = 0.0f;
            halo::objects::object_set_shield_depleted_flag(unit_index);
            halo::objects::object_recalculate_bounding_radius_recursive(unit_index);
        }
    }
}

/**
 * REWRITTEN from objdump 0x560630..0x5607ef. ECX: unit (biped_create calls it for bipeds with tag +0x2f4 bit
 * 6). Gathers the structure surfaces within the unit's pill radius + 0.05 of its position (0x501980, up to
 * 0x100), picks the one whose plane (negated for a sign-bit plane index) has the smallest signed distance to
 * the position, and stores the surface (+0x4d8), its plane (+0x514) and the normal as the unit's up (+0x80).
 * The draft called the pill query and the sphere query with the wrong operands.
 *
 * Original register convention: ECX -> unit_index.
 *
 * @address 0x560630
 */
void UnitView::find_nearest_valid_surface_plane()
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    ModelCollisionGeometryBSP *bsp = halo::physics::globals().structure_collision_bsp;
    real_point3d position;
    float pill_height;
    float pill_radius;
    collision_bsp_sphere_result result;
    real_plane3d best_plane;
    float best_distance = 3.4028235e+38f;
    int32_t best_surface = -1;
    uint8_t *surfaces;
    uint8_t *planes;
    int16_t i;

    ::halo::units::unit_get_crouch_height_offset(&position, unit_index, &pill_height, &pill_radius);
    if (!(uint8_t)halo::physics::collision_bsp_query_sphere_init(bsp, 0x100, &result,
            halo::physics::globals().breakable_surface_state->active[halo::scenario::globals().structure_bsp_index], &position, pill_radius + 0.05f)) {
        return;
    }
    if (result.surface_count <= 0) {
        return;
    }
    surfaces = *(uint8_t **)&((struct ModelCollisionGeometryBSP *)bsp)->surfaces.pointer;
    planes = *(uint8_t **)&((struct ModelCollisionGeometryBSP *)bsp)->planes.pointer;
    for (i = 0; (int32_t)i < result.surface_count; i++) {
        int32_t surface = result.surfaces[i];
        int32_t plane_reference = *(int32_t *)(surfaces + surface * 0xc);
        float *plane = (float *)(planes + (plane_reference & halo::k_leaf_index_mask) * 0x10);
        real_plane3d candidate;
        float distance;

        if (plane_reference < 0) {
            candidate.normal.i = -plane[0];
            candidate.normal.j = -plane[1];
            candidate.normal.k = -plane[2];
            candidate.d = -plane[3];
        } else {
            candidate.normal.i = plane[0];
            candidate.normal.j = plane[1];
            candidate.normal.k = plane[2];
            candidate.d = plane[3];
        }
        distance = position.x * candidate.normal.i + position.z * candidate.normal.k + position.y * candidate.normal.j - candidate.d;
        if (distance < best_distance) {
            best_plane = candidate;
            best_surface = surface;
            best_distance = distance;
        }
    }
    if (best_surface == -1) {
        return;
    }
    *(int32_t *)(obj + 0x4d8) = best_surface;
    *(real_plane3d *)(obj + 0x514) = best_plane;
    *(real_vector3d *)&((unit_object *)obj)->base.up.i = best_plane.normal;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot((h))].data)
/**
 * Engine function unit_find_placement_position.
 *
 * @address 0x55a500
 */
uint32_t halo::units::unit_find_placement_position(uint32_t anchor_object, uint32_t orientation_object, real_point3d *out_position, float radius, char grid_mode, char skip_reposition, char scale_radius, uint32_t object_index_a, real_vector3d *reference_direction)
{
    uint8_t found = 0;
    uint8_t borrowed_anchor = 0;
    real_point3d center;
    real_point3d base;
    real_point3d scratch;
    float pill_height;
    float pill_radius;
    uint32_t flags;
    int16_t count;
    real_vector3d side;
    real_vector3d vertical;
    object_collision_context context;
    collision_result segment_result;
    collision_result pill_result;
    object_node_collision_result context_result;
    bsp_leaf_reference location;
    uint8_t *unit;
    uint8_t *tag;
    int16_t i;

    (void)object_index_a;
    if (anchor_object == k_datum_index_none) {
        if (orientation_object == k_datum_index_none) {
            return 0;
        }
    }
    if (orientation_object != k_datum_index_none) {
        center = ((struct object *)OBJECT_DATA(orientation_object))->bounding_center;
    }
    if (anchor_object == k_datum_index_none) {
        anchor_object = orientation_object;
        borrowed_anchor = 1;
    }
    unit = OBJECT_DATA(anchor_object);
    tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)unit)].data;
    flags = (*(uint32_t *)(tag + 0x2f4) & 0x20) ? 0xc2a0 : 0x20c3a0;
    if (reference_direction != 0) {
        base = *(real_point3d *)reference_direction;
        ::halo::units::unit_get_crouch_height_offset(&scratch, anchor_object, &pill_height, &pill_radius);
    } else {
        ::halo::units::unit_get_crouch_height_offset(&base, anchor_object, &pill_height, &pill_radius);
    }
    if (borrowed_anchor) {
        anchor_object = k_datum_index_none;
    }
    count = grid_mode ? 27 : 18;
    if (orientation_object != k_datum_index_none) {
        halo::physics::object_collision_context_build(orientation_object, &context);
    }
    {
        real_vector3d *f = (real_vector3d *)&((struct object *)unit)->forward;
        real_vector3d *u = (real_vector3d *)&((struct object *)unit)->up;

        side.i = u->k * f->j - f->k * u->j;
        side.j = f->k * u->i - u->k * f->i;
        side.k = u->j * f->i - u->i * f->j;
        halo::math::vector3d_normalize_with_length(side);
    }
    vertical.i = pill_height * halo::math::globals().global_up3d_pointer->i;
    vertical.j = pill_height * halo::math::globals().global_up3d_pointer->j;
    vertical.k = pill_height * halo::math::globals().global_up3d_pointer->k;
    if (scale_radius) {
        radius = pill_radius * radius;
    }
    for (i = 0; i < count && !found; i++) {
        real_vector3d *offset = &placement_offset_table[i];
        real_point3d point;
        int32_t leaf;

        if (grid_mode) {
            real_vector3d *f = (real_vector3d *)&((struct object *)unit)->forward;
            real_vector3d *u = (real_vector3d *)&((struct object *)unit)->up;
            float a = radius * offset->i;
            float b = radius * offset->j;
            float c = radius * offset->k;

            point.x = a * f->i + base.x + side.i * b + c * u->i;
            point.y = a * f->j + base.y + side.j * b + c * u->j;
            point.z = a * f->k + base.z + side.k * b + c * u->k;
        } else {
            point.x = radius * offset->i + base.x;
            point.y = radius * offset->j + base.y;
            point.z = radius * offset->k + base.z;
        }
        leaf = (int32_t)halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, &point);
        if (leaf == -1) {
            continue;
        }
        if (*(int16_t *)(*(uint8_t **)(global_structure_bsp + 0xe4) + (leaf & halo::k_leaf_index_mask) * 0x10 + 0x8) == -1) {
            continue;
        }
        if (!halo::physics::physics_point_find_clear_position(flags, &point, pill_radius + pill_radius, pill_height, pill_radius,
                anchor_object, &point)) {
            continue;
        }
        if (halo::physics::collision_test_movement_pill(flags, &point, pill_radius, &vertical, &pill_result)) {
            continue;
        }
        if (orientation_object != k_datum_index_none) {
            if (halo::physics::object_collision_context_test_pill(&context, &point, &vertical, pill_radius, &context_result)) {
                continue;
            }
            if (halo::physics::collision_test_movement_segment_between_points(&point, &center, flags, anchor_object, &segment_result) &&
                segment_result.object_index != orientation_object) {
                continue;
            }
            if (halo::physics::collision_test_movement_segment_between_points(&center, &point, flags, orientation_object, &segment_result) &&
                segment_result.object_index != anchor_object) {
                continue;
            }
        }
        halo::scenario::scenario_location_from_point(&location, &point);
        if (!(*(uint32_t *)(tag + 0x2f4) & 0x8)) {
            point.z = point.z - *(float *)(tag + 0x42c);
        }
        if (anchor_object != k_datum_index_none && !skip_reposition) {
            *(real_point3d *)&((unit_object *)unit)->base.position.x = point;
            halo::objects::object_recalculate_bounding_radius_recursive(anchor_object);
            halo::objects::object_set_position_and_relink(&point, anchor_object, &location);
        }
        if (out_position != 0) {
            *out_position = point;
        }
        found = 1;
    }
    return found;
}
#undef OBJECT_DATA

/**
 * object_type_definition "unit" row, +0x28 column: one-time per-object spawn initializer for bipeds and
 * vehicles alike. Resets the seat/weapon/grenade/animation/vehicle-link/AI-dialogue fields to their empty
 * defaults, seeds the aim/look/facing vectors from the object's current forward direction, rolls the
 * feign-death eligibility for this spawn, applies the tag's default team and initial grenade count, labels
 * the default "stand" seat, gives non-client units their starting weapons, and queues mounted-weapon
 * creation...
 *
 * @address 0x562180
 */
uint8_t UnitView::new_()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Unit *tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint32_t *field;
    int32_t i;

    if (*(int32_t *)&tag->base.animation_graph.tag_id == -1) {
        return 0;
    }

    unit->network_update_applied = 0;
    unit->control_update_id = -1;
    unit->equipment_object_index = k_datum_index_none;
    unit->weapons[0] = k_datum_index_none;
    unit->weapons[1] = k_datum_index_none;
    unit->weapons[2] = k_datum_index_none;
    unit->weapons[3] = k_datum_index_none;
    unit->current_weapon_index = -1;
    unit->desired_weapon_index = -1;
    unit->current_grenade_index = -1;
    unit->desired_grenade_index = -1;
    unit->zoom_level = -1;
    unit->desired_zoom_level = -1;
    unit->controlling_player = k_datum_index_none;
    unit->actor_index = k_datum_index_none;
    unit->swarm_actor_index = k_datum_index_none;
    unit->swarm_next_unit_index = k_datum_index_none;
    unit->swarm_previous_unit_index = (uint32_t)-1;
    unit->vehicle_seat_index = -1;
    unit->driver_unit_index = k_datum_index_none;
    unit->gunner_unit_index = k_datum_index_none;
    unit->animation_state_flags = 0;
    unit->animation_definition_index = -1;
    unit->animation_weapon_index = -1;
    unit->animation_weapon_type_index = -1;
    unit->animation_state = -1;
    unit->replacement_animation_state = 0;
    unit->overlay_animation_state = 0;
    unit->aiming_animation_index = -1;
    unit->looking_animation_index = -1;
    unit->overlays[0].animation_index = -1;
    unit->overlays[1].animation_index = -1;
    unit->overlays[2].animation_index = -1;
    unit->base_animation_state = 2;
    unit->overlay_animation_index = -1;
    unit->emotion_animation_frame = -1;
    unit->emotion_animation_index = -1;
    unit->scripted_base_animation_state = -1;
    unit->aiming_bounds_valid = 0;
    unit->aiming_bounds[0] = 0.0f;
    unit->aiming_bounds[1] = 0.0f;
    unit->aiming_bounds[2] = 0.0f;
    unit->aiming_bounds[3] = 0.0f;
    unit->looking_bounds_valid = 0;
    unit->looking_bounds[0] = 0.0f;
    unit->looking_bounds[1] = 0.0f;
    unit->looking_bounds[2] = 0.0f;
    unit->looking_bounds[3] = 0.0f;

    unit->looking_vector = obj->forward;
    unit->desired_looking_vector = obj->forward;
    unit->aiming_vector = obj->forward;
    unit->desired_aiming_vector = obj->forward;
    unit->desired_facing_vector = obj->forward;

    unit->persistent_control_ticks = 0;
    unit->dialogue_tag_index = k_datum_index_none;
    set_flag(unit->flags, units::unit_flag::permutation_dirty);

    field = (uint32_t *)&unit->current_speech;
    for (i = 0x1f; i != 0; i--) {
        *field++ = 0;
    }
    unit->communication_hold_tick = (uint32_t)-1;

    UnitView(object_index).dialogue_determine_variant();

    field = (uint32_t *)unit->recent_damage;
    for (i = 0x10; i != 0; i--) {
        *field++ = (uint32_t)-1;
    }

    unit->delayed_damage_category = 0;
    unit->delayed_damage_ticks = 0;
    unit->delayed_damage_amount = 0.0f;
    unit->delayed_damage_responsible_object = k_datum_index_none;
    unit->death_time = -1;
    unit->encounter_index = -1;
    unit->squad_index = -1;
    unit->integrated_light_energy = 1.0f;
    unit->flaming_ticks = 0;
    unit->flaming_responsible_object = -1;
    unit->ai_communication_count = 0;
    unit->ai_communication_tick = -1;

    if ((uint16_t)tag->grenade_type <= 1 && tag->grenade_count >= 0) {
        unit->grenade_counts[tag->grenade_type] = (int8_t)tag->grenade_count;
    }

    set_flag(obj->flags, objects::object_flag::unknown_2000 | objects::object_flag::unknown_4000);

    if (tag->feign_death_threshold > 0.0f && tag->feign_death_time > 0.0f && tag->feign_death_chance > 0.0f) {
        float roll = halo::math::random_real();
        if (roll < tag->feign_death_chance) {
            set_flag(unit->flags, units::unit_flag::unknown_2000);
        } else {
            clear_flag(unit->flags, units::unit_flag::unknown_2000);
        }
    }

    if (halo::game::globals().current_engine == 0 && (obj->owner_team == 0 || obj->owner_team == -1)) {
        obj->owner_team = tag->default_team;
    }

    UnitView(object_index).set_or_test_seat_and_weapon_label(s_stand, (const char *)0, 1);

    if (network_game_mode != 1) {
        UnitView(object_index).add_initial_weapons();
    }

    if (tag->seats.count > 0) {
        int32_t seat_index = 0;
        UnitSeat *seats = (UnitSeat *)tag->seats.pointer;
        while (*(int32_t *)&seats[seat_index].built_in_gunner.tag_id == -1) {
            seat_index = seat_index + 1;
            if (seat_index >= (int32_t)tag->seats.count) {
                goto done_seat_scan;
            }
        }
        if (ai_globals_ptr->actors_valid != 0 && ai_globals_ptr->vehicle_entry_count < 8) {
            ai_globals_ptr->vehicle_entry_queue[ai_globals_ptr->vehicle_entry_count] = (datum_index)object_index;
            ai_globals_ptr->vehicle_entry_count = ai_globals_ptr->vehicle_entry_count + 1;
        }
    }
done_seat_scan:
    return 1;
}

/**
 * Returns the object the camera and aiming code should treat as the unit: the vehicle the unit is seated in when
 * the seat is invisible or a gunner seat (seat flags 0x9), otherwise the unit itself.
 *
 * @address 0x569670
 */
uint32_t UnitView::resolve_camera_object()
{
    uint32_t object_index = datum_handle;
    uint32_t result = object_index;

    if (object_index != k_datum_index_none) {
        object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;

        if (obj->parent_object != k_datum_index_none) {
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

            if (unit->vehicle_seat_index != -1) {
                object *parent = ((object_header *)halo::objects::globals().object_data->data)[(uint16_t)obj->parent_object].data;
                Unit *parent_tag = (Unit *)halo::cache::globals().tag_instances[(uint16_t)parent->definition_tag].data;
                UnitSeat *seat = (UnitSeat *)((uint8_t *)parent_tag->seats.pointer +
                                               (uint32_t)unit->vehicle_seat_index * 0x11c);

                if (test_flag(seat->flags, tags::unit_seat_tag_flag::invisible | tags::unit_seat_tag_flag::gunner)) {
                    result = (uint32_t)obj->parent_object;
                }
            }
        }
    }
    return result;
}

/**
 * Lazily computes and caches a random variant/permutation index for a unit, seeded from the global PRNG
 *
 * Original register convention: unaff_EDI -> unit_index.
 *
 * @address 0x568540
 */
int32_t UnitView::pick_random_spawned_actor_count()
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    int32_t result = 0;

    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    if ((unit->flags & _unit_flag_permutation_chosen) == 0) {
        Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;
        if (*(int32_t *)&unit_tag->spawned_actor.tag_id != -1) {
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            int32_t range = (int32_t)(int16_t)(unit_tag->spawned_actor_count[1] + 1) - (int32_t)unit_tag->spawned_actor_count[0];
            result = (int32_t)(((uint32_t)range * (halo::math::globals().random_seed_global >> halo::k_random_high_shift)) >> 0x10) +
                     (int32_t)((((uint32_t)halo::cache::globals().tag_instances >> 16) << 16) | (uint16_t)unit_tag->spawned_actor_count[0]);
            if (0 < (int16_t)result) {
                result = actor_spawn_additional_units(*(datum_index *)&((struct Unit *)unit_tag)->spawned_actor.tag_id, (int16_t)result,
                    unit_index, ((struct Unit *)unit_tag)->spawned_velocity * 0.033333335f);
            }
            unit->flags |= _unit_flag_permutation_chosen;
        }
    }
    return result;
}

namespace unit_place_local {

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(object_index) * 0xc + 8);
}

}

/**
 * Engine function unit_place.
 *
 * @address 0x558d40
 */
void UnitView::place(uint8_t *placement)
{
    using namespace unit_place_local;
    datum_index object_index = datum_handle;
    UnitView(object_index).apply_scale_change((unit_scale_request *)(placement + 0x48));
}

/**
 * Propagates the unit's positional movement delta (new_position - its current position) to any attached child
 * bipeds/vehicles, updating their cached relative offsets (unknown_34c), then recalculates the unit's own
 * position/cluster.
 *
 * @address 0x570cb0
 */
void halo::units::unit_propagate_position_delta_to_children(real_point3d *new_position, uint32_t unit_index)
{
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    real_vector3d delta;
    datum_index child;

    delta.i = new_position->x - obj->position.x;
    delta.j = new_position->y - obj->position.y;
    delta.k = new_position->z - obj->position.z;

    child = obj->first_child_object;
    while (child != k_datum_index_none) {
        object *child_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(child)].data;
        if ((1 << (child_obj->type & 0x1f) & 3) != 0) {
            unit_data *child_unit = (unit_data *)((uint8_t *)child_obj + k_unit_data_offset);
            child_unit->seat_acceleration_last_position.x += delta.i;
            child_unit->seat_acceleration_last_position.y += delta.j;
            child_unit->seat_acceleration_last_position.z += delta.k;
        }
        child = child_obj->next_object;
    }

    halo::objects::object_set_position_and_recalculate(new_position, unit_index);
}

namespace unit_recalculate_position_local {

static uint8_t coordinate_in_range(float value)
{
    return (uint8_t)(!(value < -5000.0f) && value <= 5000.0f);
}

}

/**
 * Engine function unit_recalculate_position.
 *
 * @address 0x558eb0
 */
void UnitView::recalculate_position()
{
    using namespace unit_recalculate_position_local;
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    real_point3d *current = (real_point3d *)&((struct object *)obj)->position;
    real_point3d anchor = *(real_point3d *)&((unit_object *)obj)->base.network_position.x;
    real_point3d previous = *current;
    real_point3d *target = &anchor;
    real_point3d midpoint;
    real_point3d nudged;

    if (!(sqrt((anchor.z - previous.z) * (anchor.z - previous.z) + (anchor.y - previous.y) * (anchor.y - previous.y) +
               (anchor.x - previous.x) * (anchor.x - previous.x)) > 5.0) && halo::hs::fields::object_prediction) {
        if (!halo::objects::object_nudge_position_by_velocity(object_index, &nudged)) {
            nudged = anchor;
        }
        midpoint.x = (previous.x + nudged.x) * 0.5f;
        midpoint.y = (previous.y + nudged.y) * 0.5f;
        midpoint.z = (previous.z + nudged.z) * 0.5f;
        if (!_isnan((double)midpoint.x) && coordinate_in_range(midpoint.x) &&
            halo::camera::real_is_valid(midpoint.y) && coordinate_in_range(midpoint.y) &&
            halo::camera::real_is_valid(midpoint.z) && coordinate_in_range(midpoint.z)) {
            target = &midpoint;
        }
    }
    halo::objects::object_set_position_and_recalculate(target, object_index);
    if (sqrt((current->z - previous.z) * (current->z - previous.z) + (current->y - previous.y) * (current->y - previous.y) +
             (current->x - previous.x) * (current->x - previous.x)) > 2.0) {
        halo::objects::object_set_position_and_recalculate(&anchor, object_index);
    }
}

/**
 * Sets or clears a per-unit boolean flag, resets a cached 3D vector field from a shared default, and clears
 * an additional state bit for biped-type units.
 *
 * Original register convention: in_EAX, param_1.
 *
 * @address 0x56a290
 */
void UnitView::reset_velocity_and_ground_flag(uint8_t enable)
{
    uint32_t unit_index = datum_handle;
    if (unit_index != k_datum_index_none) {
        object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

        if (!enable) {
            clear_flag(unit->flags, units::unit_flag::unknown_1000000);
        } else {
            set_flag(unit->flags, units::unit_flag::unknown_1000000);
        }
        unit_obj->velocity.i = global_origin3d_pointer->x;
        unit_obj->velocity.j = global_origin3d_pointer->y;
        unit_obj->velocity.k = global_origin3d_pointer->z;
        if (unit_obj->type == _object_type_biped) {
            biped_data *biped = (biped_data *)((uint8_t *)unit_obj + k_unit_object_size);
            clear_flag(biped->flags, units::biped_flag::airborne);
        }
    }
    return;
}

/**
 * REWRITTEN from objdump 0x55ecf0..0x55eec4 (really the biped jump launch). Unless already airborne (+0x4cc
 * bit 0) or +0x508 == 1: the Biped tag's jump speed (+0x3b4; a player's scaled by 1 - globals player info
 * +0x84 * stun +0x424, times 4 with the super-jump cheat) is the least speed along the unit's up; an AI unit
 * (+0x1f8, else +0x1f4) then takes its actor's requested velocity capped at that speed (0x417fa0, unclamped
 * in states 0x27 / 0x28) and may refuse.
 *
 * @address 0x55ecf0
 */
uint32_t UnitView::snap_to_min_ground_height()
{
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    float jump_speed;
    real_vector3d velocity;
    real_vector3d *up = (real_vector3d *)&((struct object *)obj)->up;
    float up_speed;
    datum_index actor_index;
    uint8_t result = 1;

    if ((obj[0x4cc] & 1) || *(int16_t *)(obj + 0x508) == 1) {
        return 0;
    }
    jump_speed = *(float *)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data + 0x3b4);
    if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
        jump_speed = (1.0f - *(float *)((uint8_t *)global_globals->player_information.pointer + 0x84) * ((struct unit_object *)obj)->unit.stun) *
            jump_speed;
    }
    if (halo::hs::fields::super_jump && ((unit_object *)obj)->unit.controlling_player != k_datum_index_none) {
        jump_speed = jump_speed * 4.0f;
    }
    velocity = *(real_vector3d *)&((unit_object *)obj)->base.velocity.i;
    up_speed = velocity.j * up->j + velocity.k * up->k + velocity.i * up->i;
    if (!(up_speed >= jump_speed)) {
        float delta = jump_speed - up_speed;

        velocity.i = delta * up->i + velocity.i;
        velocity.j = delta * up->j + velocity.j;
        velocity.k = delta * up->k + velocity.k;
    }
    actor_index = ((unit_object *)obj)->unit.swarm_actor_index;
    if (actor_index == k_datum_index_none) {
        actor_index = ((unit_object *)obj)->unit.actor_index;
    }
    if (actor_index != k_datum_index_none) {
        uint8_t skip_clamp = ((uint8_t)((struct unit_object *)obj)->unit.animation_state == 0x27 || (uint8_t)((struct unit_object *)obj)->unit.animation_state == 0x28) ? 1 : 0;

        result = actor_get_requested_velocity(skip_clamp, actor_index, &velocity, object_index, jump_speed);
        if (!result) {
            return 0;
        }
    }
    *(real_vector3d *)&((unit_object *)obj)->base.velocity.i = velocity;
    *(uint32_t *)(obj + 0x4cc) |= 1;
    obj[0x504] = 0;
    *(int32_t *)(obj + 0x4d8) = -1;
    if (!unit_updates_suppressed) {
        UnitView(object_index).fire_animation_sound_trigger(4, 0);
        UnitView(object_index).fire_animation_sound_trigger(4, 1);
    }
    return result;
}

/**
 * Engine function unit_test_placement_candidate.
 *
 * @address 0x55aa20
 */
int32_t UnitView::test_placement_candidate(const real_vector3d *direction, real_vector3d *out_normal, float distance, real_point3d *out_position)
{
    uint32_t unit_index = datum_handle;
    static collision_bsp_segment_result result;
    real_point3d origin;
    real_vector3d delta;

    halo::objects::object_get_position(&origin, unit_index);
    origin.x += halo::math::globals().global_up3d_pointer->i * 0.4f;
    origin.y += halo::math::globals().global_up3d_pointer->j * 0.4f;
    origin.z += halo::math::globals().global_up3d_pointer->k * 0.4f;
    delta.i = distance * direction->i;
    delta.j = distance * direction->j;
    delta.k = distance * direction->k;
    if (!halo::physics::collision_bsp_query_segment_init(1, &result, halo::physics::globals().structure_collision_bsp, 0, 0, &origin, &delta,
                                          3.4028235e+38f)) {
        return -1;
    }
    if (out_position != 0) {
        out_position->x = delta.i * result.t + origin.x;
        out_position->y = delta.j * result.t + origin.y;
        out_position->z = delta.k * result.t + origin.z;
    }
    if (out_normal != 0) {
        *out_normal = *(real_vector3d *)result.plane;
    }
    return result.surface_index;
}

/**
 * object_type_definition "unit" row, +0x38 column. Refreshes object.function_in_values[0..3] from the unit
 * tag's four scale-function selectors.
 *
 * @address 0x563860
 */
void UnitView::update_scale_function_inputs()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Unit *tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    const int16_t *selector = &tag->unit_a_in;
    float *function_in = obj->function_in_values;
    int32_t i;

    for (i = 4; i != 0; i--) {
        if (*selector != 0) {
            float value = 0.0f;
            switch (*selector) {
            case 1:
                value = unit->driver_seat_power;
                break;
            case 2:
                value = unit->gunner_seat_power;
                break;
            case 3:
                value = (float)(int32_t)(uint8_t)unit->aiming_change * 0.003921569f;
                break;
            case 4:
                value = unit->mouth_aperture;
                break;
            case 5:
                value = unit->integrated_light_power;
                break;
            case 6:
                if (!test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen) && !test_flag(unit->flags, units::unit_flag::unknown_400000)) {
                    value = 1.0f;
                } else {
                    value = 0.0f;
                }
                break;
            case 7:
            {
                tag_instance *graph = &halo::cache::globals().tag_instances[halo::datum_slot(obj->animation_graph)];
                uint8_t *animations_pointer = *(uint8_t **)((uint8_t *)graph->data + 0x78);
                int16_t frame_count = *(int16_t *)(animations_pointer + (int32_t)obj->animation_index * 0xb4 + 0x2e);
                if (obj->animation_index < frame_count) {
                    value = (float)(int32_t)obj->animation_index / (float)(int32_t)frame_count;
                } else {
                    value = 1.0f - (float)(int32_t)unit->shield_sapping * 0.011111111f;
                }
                break;
            }
            }
            *function_in = value;
        }
        selector++;
        function_in++;
    }
}

}
