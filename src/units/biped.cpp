#include "halo/networking/game_mode.hpp"
#include "halo/units/seat_detach.hpp"
#include "halo/units/animation_states.hpp"
#include "halo/units/records.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/hs/script_globals.hpp"
#include "halo/units/unit.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/models/api.hpp"
#include "game.h"
#include "hs.h"
#include "networking.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"

static auto &cinematic_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().cinematic_globals_ptr);
static auto &unit_updates_suppressed = halo::link::ref<uint8_t>(halo::units::vars().unit_updates_suppressed);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &global_down3d_pointer = halo::link::ref<real_vector3d *>(halo::ai::vars().global_down3d_pointer);
static auto &k_default_resting_plane = halo::link::ref<uint32_t [4]>(halo::units::vars().k_default_resting_plane);
static auto &k_biped_minimum_age_ticks = halo::link::ref<int32_t>(halo::units::vars().k_biped_minimum_age_ticks);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &biped_detach_from_flipped_vehicle = halo::link::ref<uint8_t>(halo::units::vars().biped_detach_from_flipped_vehicle);
static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);

namespace halo::units {

/**
 * Advances the biped's animation frame counter (unknown_4d0); once it reaches the loaded threshold
 * (byte 0x4d1), invalidates the cached comparison (unknown_508). Then, unless updates are globally
 * suppressed, fires paired trigger events (ids 5) once the counter reaches exactly 2, or if the comparison is
 * unresolved and the threshold is small. Reports state 0x15 or 0x16 depending on whether the comparison flag
 * reads 1.
 *
 * @address 0x55eb90
 */
void BipedView::advance_frame_counter_trigger(char *state_out)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    biped_data *biped = halo::units::biped_data_of(obj);
    int8_t frame_count = biped->landing_ticks + 1;

    biped->landing_ticks = frame_count;
    if (biped->landing_duration_ticks <= frame_count) {
        biped->landing_type = -1;
    }

    if (cinematic_globals_ptr[9] == 0 && unit_updates_suppressed == 0 &&
        (frame_count == 2 || (biped->landing_type == -1 && biped->landing_duration_ticks < 2))) {
        UnitView(object_index).fire_animation_sound_trigger(5, 0);
        UnitView(object_index).fire_animation_sound_trigger(5, 1);
    }

    *state_out = (biped->landing_type == 1) + 0x15;
}

/**
 * Nudges an idle-eligible unit with a small randomized angular impulse (perpendicular to its up-vector when
 * reasonably upright, otherwise a random direction in the horizontal plane) to produce idle fidget motion,
 * unless it's already in one of the special "greeting" animation states, then refreshes its orientation basis
 * and reports a follow-up animation-state code.
 *
 * @address 0x55e940
 */
void BipedView::apply_idle_fidget(uint8_t *state_out)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    uint32_t already_idle = 0;

    if (BipedView(object_index).is_idle_eligible()) {
        already_idle = 1;
        if (test_flag(tag->biped_flags, tags::biped_tag_flag::rotate_while_airborne)) {
            if (unit->animation_state != animation_state_value(unit_animation_state_id::melee_airborne) && unit->animation_state != animation_state_value(unit_animation_state_id::leap_melee)) {
                float magnitude = (float)halo::math::random_real_range(0.05235988, 0.08726646);
                real_vector3d impulse_dir;

                if (!(obj->up.k <= 0.8f)) {
                    double angle = halo::math::random_real_range(0.0, 6.2831855);
                    impulse_dir.i = (float)halo::x87::fcos(angle);
                    impulse_dir.j = (float)halo::x87::fsin(angle);
                    impulse_dir.k = 0.0f;
                } else {
                    halo::math::vector3d_cross_product(impulse_dir, *halo::math::globals().global_up3d_pointer, obj->up);
                    if (!(halo::math::vector3d_normalize_with_length(impulse_dir) > 0.0f)) {
                        double angle = halo::math::random_real_range(0.0, 6.2831855);
                        impulse_dir.i = (float)halo::x87::fcos(angle);
                        impulse_dir.j = (float)halo::x87::fsin(angle);
                        impulse_dir.k = 0.0f;
                    }
                }
                obj->angular_velocity.i += impulse_dir.i * magnitude;
                obj->angular_velocity.j += impulse_dir.j * magnitude;
                obj->angular_velocity.k += impulse_dir.k * magnitude;
            }
            UnitView(object_index).rotate_basis_about_axis();
        }
    }

    {
        int8_t state = unit->animation_state;
        if (state == 0x27 || state == 0x28) {
            state_out[0] = 0x28;
        } else if (state == 0x14 || already_idle) {
            state_out[0] = 0x14;
        }
    }
}

/**
 * Rate-limited (every 15 ticks) evasion check: if the unit is unattached, not a special weapon type (Biped
 * tag flags 0x84 clear), unattended (flags bit 0x1000 clear), has an actor and isn't mid scripted-action, and
 * has been grounded (unknown_501) more than 30 ticks, probes for a nearby open position via
 * unit_test_placement_candidate; if none is found, or a clearance/height test against the grenade-table
 * radius fails, dispatches an evade reaction (code 0).
 *
 * @address 0x55e190
 */
void BipedView::check_evade_reaction()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);

    if (!test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen) && !test_flag(tag->biped_flags, tags::biped_tag_flag::flying | tags::biped_tag_flag::immune_to_falling_damage) &&
        !test_flag(unit->flags, units::unit_flag::no_falling_damage) && unit->actor_index != k_datum_index_none &&
        unit->animation_state != animation_state_value(unit_animation_state_id::scripted_action) && (int8_t)biped->airborne_ticks > 0x1e &&
        (biped->last_falling_reaction_tick == -1 ||
         (int32_t)(biped->last_falling_reaction_tick + 0xf) < halo::game::globals().game_time->game_time)) {
        void *table = halo::objects::block_elements<GlobalsFallingDamage>(global_globals->falling_damage);
        real_point3d ground;
        real_point3d position;

        biped->last_falling_reaction_tick = halo::game::globals().game_time->game_time;
        if (UnitView(object_index).test_placement_candidate(global_down3d_pointer, 0, 6.0f, &ground) == -1) {
            UnitView((int32_t)object_index).dispatch_reaction_animation(0);
        } else {
            float radius = *(float *)((uint8_t *)table + 0x94);
            float v = obj->velocity.k;

            halo::objects::object_get_position(&position, object_index);
            if (v <= 0.0f && !(radius * radius > (position.z - ground.z) * halo::physics::globals().gravity * 2.0f + v * v)) {
                UnitView((int32_t)object_index).dispatch_reaction_animation(0);
            }
        }
    }
}

/**
 * object_type_definition "biped" row, +0x54 column. Invalidates the cached ground-surface and look-at datum
 * indices without touching the rest of biped_data.
 *
 * @address 0x559f70
 */
void BipedView::clear_ground_surface_references()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    biped_data *biped = halo::units::biped_data_of(obj);

    biped->ground_surface_index = k_datum_index_none;
    biped->cached_ground_surface_index = k_datum_index_none;
    biped->last_ground_surface_index = k_datum_index_none;
}

namespace biped_create_local {

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(object_index) * 0xc + 8);
}

static uint8_t *object_definition(uint8_t *object)
{
    return halo::objects::tag_record_bytes(*(datum_index *)object);
}

}

/**
 * Engine function biped_create.
 *
 * @address 0x558dc0
 */
uint8_t BipedView::create()
{
    using namespace biped_create_local;
    datum_index object_index = datum_handle;
    uint8_t *object = object_get(object_index);
    uint8_t *definition = object_definition(object);
    int32_t i;

    for (i = 0; i < 4; i++) {
        ((uint32_t *)&((struct biped_object *)object)->biped.ground_normal)[i] = k_default_resting_plane[i];
    }
    ((struct biped_object *)object)->biped.jump_ticks = 0x7f;
    ((struct biped_object *)object)->biped.ground_surface_index = -1;
    ((struct biped_object *)object)->biped.cached_ground_surface_index = -1;
    halo::objects::object_get_position((real_point3d *)&((struct biped_object *)object)->biped.cached_ground_point, object_index);
    ((struct biped_object *)object)->biped.last_ground_surface_index = -1;
    ((struct biped_object *)object)->biped.cached_ground_point_tick = -1;
    ((struct biped_object *)object)->biped.melee_target_index = -1;
    if ((definition[0x2f4] & 0x40) != 0) {
        UnitView(object_index).find_nearest_valid_surface_plane();
    }
    ::halo::units::unit_update_up_vector((Biped *)definition, (::object *)object);
    ((struct biped_object *)object)->biped.last_ground_object_ticks = 0;
    ((struct biped_object *)object)->biped.last_ground_object_index = -1;
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client || halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        ((struct biped_object *)object)->biped.unknown_526 = 0;
        ((struct biped_object *)object)->biped.network_update_sequence = 0;
        ((struct biped_object *)object)->biped.network_delta_sequence = 0;
        ((struct object *)object)->network_state_009 = 0;
    }
    ((struct object *)object)->shield_update_pending = 0;
    return 1;
}

/**
 * Periodically refreshes and returns the biped's cached look-at surface (biped_data.unknown_4dc) and writes
 * the cached point (unknown_4e0) through out_position. objdump 0x55ab30..0x55acff (orphan pass 4 review
 * rewrite;
 *
 * @address 0x55ab30
 */
datum_index BipedView::get_cached_look_at_position(real_point3d *out_position)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    biped_data *biped = halo::units::biped_data_of(obj);

    if (test_flag(tag->biped_flags, tags::biped_tag_flag::flying) && !test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) {
        biped->cached_ground_surface_index = k_datum_index_none;
        halo::objects::object_get_position(out_position, object_index);
    } else if (biped->cached_ground_surface_index == k_datum_index_none && halo::game::globals().game_time->game_time > (int32_t)biped->cached_ground_point_tick) {
        ModelCollisionGeometryBSP *bsp = halo::physics::globals().structure_collision_bsp;
        int32_t surface = (int32_t)biped->ground_surface_index;
        real_point3d point = biped->cached_ground_point;
        real_point2d closest;

        biped->cached_ground_point_tick = halo::game::globals().game_time->game_time;
        if (surface != -1) {
            ModelCollisionGeometryBSPSurface *surfaces =
                (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
            const real_plane3d *plane = (const real_plane3d *)((uint8_t *)bsp->planes.pointer +
                (surfaces[surface].plane & halo::k_leaf_index_mask) * 0x10);

            halo::physics::collision_bsp_surface_closest_edge_point_2d(bsp, surface, 2, 1,
                (real_point2d *)&biped->cached_ground_point, &closest);
            halo::math::decal_plane_solve_third_axis(&point, 1, 2, plane, closest);
            biped->cached_ground_surface_index = biped->ground_surface_index;
        } else {
            int32_t previous = (int32_t)biped->last_ground_surface_index;
            if (previous != -1 &&
                halo::physics::collision_bsp_surface_test_point_side_2d(bsp, (real_point2d *)&biped->cached_ground_point,
                    previous, 2, 1)) {
                biped->cached_ground_surface_index = (datum_index)previous;
                halo::physics::collision_bsp_surface_solve_third_axis(bsp, previous, 1, &point, 2,
                    (const real_point2d *)&biped->cached_ground_point);
                biped->cached_ground_surface_index = (datum_index)previous;
            }
        }

        if (biped->cached_ground_surface_index == k_datum_index_none) {
            biped->cached_ground_surface_index = (datum_index)UnitView(object_index).test_placement_candidate(global_down3d_pointer, 0, 2.0f, &point);
        }
        if (biped->cached_ground_surface_index != k_datum_index_none) {
            biped->cached_ground_point = point;
            biped->last_ground_surface_index = biped->cached_ground_surface_index;
        }
    }

    *out_position = biped->cached_ground_point;
    return biped->cached_ground_surface_index;
}

/**
 * Returns whether a biped has been in its current grounded state long enough (more than 3 ticks), and is
 * either unattached to a parent or the Biped tag's bit 4 is clear, to be eligible for idle behaviors.
 *
 * @address 0x55e8e0
 */
uint32_t BipedView::is_idle_eligible()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    biped_data *biped = halo::units::biped_data_of(obj);

    return (int8_t)biped->airborne_ticks > 3 &&
           (!test_flag(tag->biped_flags, tags::biped_tag_flag::flying) || test_flag(obj->vitality_flags, objects::vitality_flag::health_frozen));
}

/**
 * Engine function biped_is_old_enough.
 *
 * @address 0x55b780
 */
uint8_t BipedView::is_old_enough()
{
    uint32_t object_index = datum_handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));
    int32_t stamp = obj->base.network_update_tick;

    if (stamp == -1) {
        return 1;
    }
    return (uint8_t)(halo::game::globals().game_time->game_time >= stamp + k_biped_minimum_age_ticks);
}

/**
 * Engine function biped_placement_offset_centered_pill.
 *
 * @address 0x558d50
 */
void BipedView::placement_offset_centered_pill(object_placement_data *placement)
{
    datum_index object_index = datum_handle;
    uint8_t *object = *(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(object_index) * 0xc + 8);
    Biped *biped_tag = halo::objects::tag_as<Biped>(*(datum_index *)object);
    uint32_t flags = biped_tag->biped_flags;
    float radius;

    if (!test_flag(flags, tags::biped_tag_flag::physics_pill_centered_at_origin) || test_flag(flags, tags::biped_tag_flag::flying)) {
        return;
    }
    radius = biped_tag->collision_radius;
    placement->position.x = radius * placement->up.i + placement->position.x;
    placement->position.y = radius * placement->up.j + placement->position.y;
    placement->position.z = radius * placement->up.k + placement->position.z;
}

/**
 * Fires a paired trigger event (id 2) once a unit exceeds a small velocity threshold (~0.033 units/tick)
 * after having been still for more than 3 ticks (biped_data.unknown_502).
 *
 * @address 0x55ec20
 */
void BipedView::trigger_on_velocity_threshold()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    biped_data *biped = halo::units::biped_data_of(obj);

    if ((int8_t)biped->slipping_ticks > 3 &&
        obj->velocity.i * obj->velocity.i + obj->velocity.j * obj->velocity.j +
                obj->velocity.k * obj->velocity.k > 0.0011111111f &&
        unit_updates_suppressed == 0) {
        UnitView(object_index).fire_animation_sound_trigger(2, 0);
        UnitView(object_index).fire_animation_sound_trigger(2, 1);
    }
}

namespace biped_update_local {

}

/**
 * Engine function biped_update.
 *
 * @address 0x5590a0
 */
uint8_t BipedView::update()
{
    using namespace biped_update_local;
    uint32_t object_index = datum_handle;
    uint8_t *obj = halo::objects::object_record_bytes(object_index);
    Biped *tag = halo::objects::tag_as<Biped>(*(datum_index *)obj);
    int8_t state[2];

    if (((unit_object *)obj)->base.network_role == 1 && ((struct object *)obj)->network_position_valid == 1 && ((unit_object *)obj)->base.parent_object == k_datum_index_none) {
        UnitView(object_index).recalculate_position();
    }
    state[0] = 0;
    state[1] = 0;

    auto finish = [&]() -> uint8_t {
        if (UnitView(object_index).update_animation_state_machine(state) == 1) {
            UnitView(object_index).snap_to_min_ground_height();
        }
        if (test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen) && test_flag(((struct object *)obj)->flags, objects::object_flag::at_rest)) {
            ((struct biped_object *)obj)->base.dead_at_rest_ticks++;
        } else {
            ((struct biped_object *)obj)->base.dead_at_rest_ticks = 0;
        }
        return 1;
    };

    if (((unit_object *)obj)->base.parent_object != k_datum_index_none) {
        object *parent = reinterpret_cast<object *>(halo::objects::object_record_bytes(((unit_object *)obj)->base.parent_object));

        if (parent->type != _object_type_vehicle) {
            if (parent->type == _object_type_biped) {
                state[0] = (int8_t)(((uint8_t)parent->vitality_flags & 4) | 0x20);
            }
            return finish();
        }
        UnitView(object_index).evaluate_flee_reaction();
        if (test_flag(((struct unit_object *)obj)->unit.control_flags, units::unit_control_flag::action) && halo::networking::globals().game_mode != halo::networking::k_game_mode_client) {
            unit_object *self = reinterpret_cast<unit_object *>(halo::objects::object_try_and_get(object_index, 3));
            datum_index vehicle_index;

            if (self != 0 && (vehicle_index = self->base.parent_object) != k_datum_index_none &&
                self->unit.vehicle_seat_index != -1) {
                if (self->base.type == _object_type_vehicle) {
                    biped_detach_from_seat(object_index, vehicle_index);
                    biped_free_local_player_history(self);
                } else if (!::halo::units::unit_state_is_scripted_animation(halo::units::unit_data_of(self))) {
                    Unit *self_tag = halo::objects::tag_as<Unit>(*(datum_index *)self);
                    datum_index graph = halo::objects::tag_handle(self_tag->base.animation_graph);
                    uint8_t *seat_block = *(uint8_t **)(halo::objects::tag_record_bytes(graph) + 0x10) + (int8_t)(uint8_t)self->unit.animation_definition_index * 0x64;

                    if (*(int32_t *)(seat_block + 0x40) > 8 && (*(int16_t **)(seat_block + 0x44))[8] != -1) {
                        int16_t exit_animation = (*(int16_t **)(seat_block + 0x44))[8];
                        uint8_t *object;
                        Object *object_tag;

                        if (((struct unit_object *)halo::objects::object_record_bytes(vehicle_index))->unit.driver_unit_index == object_index) {
                            UnitView((int32_t)vehicle_index).notify_weapon_removed();
                        }
                        UnitView(object_index).set_custom_animation(halo::objects::tag_handle(self_tag->base.animation_graph), halo::models::animation_choose_random_permutation(graph, exit_animation, (animation_random_stream)1));
                        object = halo::objects::object_record_bytes(object_index);
                        object_tag = halo::objects::tag_as<Object>(*(datum_index *)object);
                        if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
                            if (test_flag(((struct object *)object)->flags, objects::object_flag::no_collision)) {
                                halo::objects::object_for_each_light_attachment(object_index, 0, 1);
                            }
                            if ((int32_t)halo::objects::tag_handle(object_tag->model) != -1) {
                                clear_flag(((struct object *)object)->flags, objects::object_flag::no_collision);
                                halo::objects::object_header_of(object_index).flags |= 2;
                            }
                        }
                        self->unit.animation_state = animation_state_value(unit_animation_state_id::seat_exit);
                        halo::ai::actor_notify_weapon_pickup_once(object_index);
                        if (self->base.network_role == 0) {
                            ::halo::units::unit_dispatch_scripted_event_9(0, (int32_t)object_index);
                        }
                    }
                }
            }
        }
        if (biped_detach_from_flipped_vehicle && parent->up.k < 0.0f && test_flag(parent->flags, objects::object_flag::on_ground) &&
            halo::networking::globals().game_mode != halo::networking::k_game_mode_client) {
            unit_object *self = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));
            datum_index vehicle_index = self->base.parent_object;

            if (vehicle_index != k_datum_index_none && self->unit.vehicle_seat_index != -1) {
                biped_detach_from_seat(object_index, vehicle_index);
            }
            if (self->base.network_role == 0) {
                ::halo::units::unit_dispatch_scripted_event_9(1, (int32_t)object_index);
            }
            biped_free_local_player_history(self);
        }
        return finish();
    }

    ::halo::units::unit_update_up_vector((Biped *)(Biped *)tag, (::object *)(object *)obj);
    if (test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen) || !test_flag(tag->biped_flags, tags::biped_tag_flag::flying | tags::biped_tag_flag::can_climb_any_surface)) {
        ((unit_object *)obj)->unit.desired_facing_vector.k = 0.0f;
        if (halo::math::vector3d_normalize_with_length(*((real_vector3d *)&((struct unit_object *)obj)->unit.desired_facing_vector)) == 0.0f) {
            *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i = *halo::math::globals().global_forward3d_pointer;
        }
    }
    switch (animation_state_id((uint8_t)((struct unit_object *)obj)->unit.animation_state)) {
    case unit_animation_state_id::idle:
    case unit_animation_state_id::turn_in_place_a:
    case unit_animation_state_id::turn_in_place_b:
        ((struct biped_object *)obj)->biped.movement_state = 0;
        break;
    case unit_animation_state_id::move_front:
    case unit_animation_state_id::move_back:
    case unit_animation_state_id::move_left:
    case unit_animation_state_id::move_right:
        ((struct biped_object *)obj)->biped.movement_state = 1;
        break;
    default:
        ((struct biped_object *)obj)->biped.movement_state = 2;
        break;
    }
    {
        float *v = (float *)&((struct unit_object *)obj)->unit.throttle;

        if (v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < 0.01f) {
            *(real_point3d *)&((unit_object *)obj)->unit.throttle.i = *global_origin3d_pointer;
        }
    }
    if (test_flag(((struct biped_object *)obj)->biped.flags, units::biped_flag::airborne)) {
        if ((int8_t)(uint8_t)((struct biped_object *)obj)->biped.airborne_ticks < 0x7f) {
            ((struct biped_object *)obj)->biped.airborne_ticks++;
        }
    } else {
        ((struct biped_object *)obj)->biped.airborne_ticks = 0;
    }
    if (test_flag(((struct biped_object *)obj)->biped.flags, units::biped_flag::jumping)) {
        if ((int8_t)(uint8_t)((struct biped_object *)obj)->biped.slipping_ticks < 0x7f) {
            ((struct biped_object *)obj)->biped.slipping_ticks++;
        }
    } else {
        ((struct biped_object *)obj)->biped.slipping_ticks = 0;
    }
    state[1] = (int8_t)((uint8_t)((struct unit_object *)obj)->unit.control_flags & 1);
    state[0] = 0;
    if (!test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) {
        BipedView(object_index).update_facing(state);
    }
    BipedView(object_index).integrate_movement_with_collision(state);
    if (test_flag(((struct object *)obj)->vitality_flags, objects::vitality_flag::health_frozen)) {
        BipedView(object_index).update_idle_basis((uint8_t *)state);
    } else if (test_flag(((struct biped_object *)obj)->biped.flags, units::biped_flag::airborne)) {
        BipedView(object_index).apply_idle_fidget((uint8_t *)state);
    } else if (((struct biped_object *)obj)->biped.landing_type != -1) {
        BipedView(object_index).advance_frame_counter_trigger((char *)state);
    } else if (test_flag(((struct biped_object *)obj)->biped.flags, units::biped_flag::jumping)) {
        BipedView(object_index).trigger_on_velocity_threshold();
    }
    if (unit_updates_suppressed) {
        return finish();
    }
    if ((uint8_t)((struct biped_object *)obj)->biped.melee_ticks == 0) {
        if (((unit_object *)obj)->unit.controlling_player != k_datum_index_none && (int8_t)(uint8_t)((struct unit_object *)obj)->unit.control_flags < 0) {
            datum_index weapon = UnitView(object_index).get_weapon_object_index(((struct unit_object *)halo::objects::object_record_bytes(object_index))->unit.current_weapon_index);

            if (!halo::items::weapon_prevents_melee_attack(weapon) && (uint8_t)((struct unit_object *)obj)->unit.zoom_level == 0xff) {
                int8_t total;
                int8_t quarter;
                int8_t tail_time;

                UnitView(object_index).start_seat_overlay_animation_a(7);
                halo::items::weapon_reset_triggers(weapon);
                halo::interface::weapon_action_notify_for_unit(object_index, 4);
                total = (int8_t)halo::items::weapon_get_first_person_animation_time(weapon, 0xd, 0, -1);
                quarter = (int8_t)(total >> 2);
                ((struct biped_object *)obj)->biped.melee_ticks = (uint8_t)(total - quarter);
                tail_time = (int8_t)halo::items::weapon_get_first_person_animation_time(weapon, 0xd, 1, -1);
                ((struct biped_object *)obj)->biped.melee_inflict_tick = (uint8_t)(total - quarter - tail_time);
                if (unit_updates_suppressed) {
                    return finish();
                }
            }
        }
    } else {
        if ((uint8_t)((struct biped_object *)obj)->biped.melee_ticks == (uint8_t)((struct biped_object *)obj)->biped.melee_inflict_tick) {
            UnitView(object_index).melee_attack_scan();
        }
        ((struct biped_object *)obj)->biped.melee_ticks--;
        if (unit_updates_suppressed) {
            return finish();
        }
    }
    UnitView(object_index).update_footstep_and_idle_triggers();
    if (!unit_updates_suppressed) {
        BipedView(object_index).check_evade_reaction();
        UnitView(object_index).check_fell_off_level();
    }

    return finish();
}

/**
 * REWRITTEN from objdump 0x55eaa0..0x55eb87. Stack: threshold (seconds); ECX: the Biped tag; ESI: the biped
 * object. With t0 / t1 / t2 = tag +0x3dc / +0x3e0 / +0x3e4 in ticks / 30: nothing before t0. Before t1 the
 * fraction is (threshold - t0) / (t1 - t0) of tag +0x3d4 (phase 0); from t1 on it is threshold / (t2 - t1) of
 * tag +0x3d8 (phase 1 -- the binary does not subtract t1 there). With a positive span: +0x508 = phase, +0x4d0
 * = 0 and +0x4d1 = trunc(value * 30 * clamp(fraction, 0, 1)).
 *
 * @address 0x55eaa0
 */
void halo::units::biped_update_animation_frame_trigger(float threshold, const Biped *timing_table, object *object_base)
{
    biped_object *biped = reinterpret_cast<biped_object *>(object_base);
    float t0 = timing_table->minimum_soft_landing_velocity * 0.033333335f;
    float t1 = timing_table->minimum_hard_landing_velocity * 0.033333335f;
    float numerator;
    float span;
    float value;
    float fraction;
    int16_t phase;

    if (threshold < t0) {
        return;
    }
    if (!(threshold >= t1)) {
        numerator = threshold - t0;
        span = t1 - t0;
        value = timing_table->maximum_soft_landing_time;
        phase = 0;
    } else {
        numerator = threshold;
        span = timing_table->maximum_hard_landing_velocity * 0.033333335f - t1;
        value = timing_table->maximum_hard_landing_time;
        phase = 1;
    }
    value = value * 30.0f;
    if (!(span > 0.0f)) {
        return;
    }
    fraction = numerator / span;
    if (!(fraction >= 0.0f)) {
        fraction = 0.0f;
    } else if (!(fraction <= 1.0f)) {
        fraction = 1.0f;
    }
    biped->biped.landing_type = phase;
    biped->biped.landing_ticks = 0;
    biped->biped.landing_duration_ticks = (uint8_t)(int32_t)(value * fraction);
}

/**
 * Selects between three per-tick basis states for an idle biped: while a ground-adjust solve is still in
 * progress, keeps stepping it (unit_rotate_basis_about_axis is not reached here); once grounded for more than
 * 2 ticks (and the tag doesn't request otherwise), holds the "idle basis refresh" state, re-running
 * unit_rotate_basis_about_axis once on entry; otherwise resets the idle refresh state and levels the
 * up-vector via unit_update_up_vector.
 *
 * @address 0x55e840
 */
void BipedView::update_idle_basis(uint8_t *state_out)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);

    if (test_flag(biped->flags, units::biped_flag::ground_adjust_dirty) && biped->ground_adjust_iteration < biped->ground_adjust_iteration_limit) {
        BipedView(object_index).ground_adjust_step();
        state_out[1] = 0;
        return;
    }

    if ((int8_t)biped->airborne_ticks > 2 && !test_flag(tag->biped_flags, tags::biped_tag_flag::has_no_dying_airborne)) {
        if (unit->animation_state == animation_state_value(unit_animation_state_id::dying_airborne)) {
            UnitView(object_index).rotate_basis_about_axis();
        }
        state_out[0] = 0x18;
        state_out[1] = 0;
        return;
    }

    if (unit->animation_state == animation_state_value(unit_animation_state_id::dying_airborne)) {
        biped->bank_angle = 0.0f;
        ::halo::units::unit_update_up_vector((Biped *)tag, (::object *)obj);
    }
    state_out[0] = 0x19;
    state_out[1] = 0;
}

/**
 * object_type_definition "biped" row, +0x38 column. Refreshes object.function_in_values[0..3] from the biped
 * tag's four scale-function selectors; the only implemented selector (1) drives the slot with the object's
 * current ground speed as a fraction of the tag's max_velocity.
 *
 * @address 0x559e40
 */
void BipedView::update_scale_function_inputs()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    const int16_t *selector = &tag->biped_a_in;
    float *function_in = obj->function_in_values;
    int32_t i;

    for (i = 4; i != 0; i--) {
        if (*selector != 0) {
            float value = 0.0f;
            if (*selector == 1) {
                value = (float)halo::libm::sqrt((double)(obj->velocity.i * obj->velocity.i +
                                              obj->velocity.j * obj->velocity.j +
                                              obj->velocity.k * obj->velocity.k)) /
                        (tag->max_velocity * 0.033333335f);
                if (value < 0.0f) {
                    value = 0.0f;
                } else if (value > 1.0f) {
                    value = 1.0f;
                }
            }
            *function_in = value;
        }
        selector++;
        function_in++;
    }
}

/**
 * FIXED (step 1, objdump -d 0x55e0a0..0x55e18c): target arrives in EAX and the biped in ECX, and every callee
 * takes register arguments the draft left out. The one pushed stack slot at the call site is never read.
 *
 * @address 0x55e0a0
 */
void halo::units::biped_update_target_lock_timer(datum_index target, uint32_t object_index)
{
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    biped_data *biped = halo::units::biped_data_of(obj);
    unit_data *unit = halo::units::unit_data_of(obj);
    object *target_obj;

    if ((int8_t)biped->bump_ticks < 0) {
        if (target != k_datum_index_none) {
            biped->bump_ticks = 0xf1;
        } else {
            biped->bump_ticks = biped->bump_ticks + 1;
        }
        return;
    }
    if (target == k_datum_index_none) {
        return;
    }
    target_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(target)].data;
    halo::ai::actor_squad_react_to_grenade_for_vehicle_occupants(target, object_index);
    if (unit->controlling_player == k_datum_index_none && !halo::cutscene::recorded_animation_object_is_playing(object_index)) {
        return;
    }
    if (biped->bump_object_index != target) {
        biped->bump_object_index = target;
        biped->bump_ticks = 0;
        return;
    }
    biped->bump_ticks = biped->bump_ticks + 1;
    if ((int8_t)biped->bump_ticks <= 3) {
        return;
    }
    if (target_obj->type == _object_type_biped && halo::hs::fields::bump_possession != 0) {
        int32_t local_player = halo::game::unit_get_local_player_weapon_index(object_index);
        if ((int16_t)local_player != -1) {
            biped_data *target_biped = halo::units::biped_data_of(target_obj);
            target_biped->bump_ticks = 0xf1;
            halo::game::local_player_set_controlled_unit(target, (int16_t)local_player);
        }
    }
    biped->bump_ticks = 0xf1;
}

}
