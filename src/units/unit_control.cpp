#include "halo/units/animation_states.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/units/records.hpp"
#include "halo/units/unit.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/core/lcg.hpp"
#include "halo/tags/flags.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/models/api.hpp"
#include "game.h"
#include "hs.h"
#include "physics.h"
#include "projectiles.h"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/items/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include <string.h>
#include "halo/hs/api.hpp"

static auto &global_zero_vector3d_pointer = halo::link::ref<const real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);
static auto &global_structure_collision_bsp = halo::link::ref<void *>(halo::physics::vars().global_structure_collision_bsp);
static auto &global_down3d_pointer = halo::link::ref<const real_vector3d *>(halo::ai::vars().global_down3d_pointer);
static auto &global_scenario = halo::link::ref<uint8_t *>(halo::hs::vars().global_scenario);
static auto &control_binding_device_type = halo::link::ref<int32_t>(halo::units::vars().control_binding_device_type);
static auto &object_type_definitions_ex = halo::link::ref<uint8_t *>(halo::units::vars().object_type_definitions_ex);
static auto &s_stand = halo::link::ref<char *>(halo::units::vars().s_stand);

namespace halo::units {

/**
 * Applies a rate-limited (max 0.3 change per call) update to unit_data.animation_blend_weight.
 *
 * @address 0x570400
 */
void UnitView::accumulate_clamped_offset(float new_value)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    float delta = new_value - unit->mouth_aperture;

    if (delta < -0.3f) {
        unit->mouth_aperture -= 0.3f;
        return;
    }
    if (delta > 0.3f) {
        delta = 0.3f;
    }
    unit->mouth_aperture += delta;
}

/**
 * Engine function unit_apply_control_block.
 *
 * Original register convention: see file header.
 *
 * @address 0x5639f0
 */
void UnitView::apply_control_block(const unit_control_data *control, int32_t source_id)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    if (halo::networking::globals().game_mode == 2) {
        unit->network_update_forced = test_flag(control->control_flags, units::unit_control_flag::primary_trigger | units::unit_control_flag::grenade);
        unit->saved_control = *control;
    }

    unit->throttle = control->throttle;
    unit->primary_trigger = control->primary_trigger;
    unit->aiming_speed = control->aiming_speed;
    if (control->weapon_index != -1) {
        unit->desired_weapon_index = control->weapon_index;
    }
    if (control->grenade_index != -1) {
        unit->desired_grenade_index = (int8_t)control->grenade_index;
    }
    unit->desired_zoom_level = (int8_t)control->zoom_level;
    unit->control_flags = control->control_flags;
    unit->desired_looking_vector = control->looking_vector;
    unit->desired_aiming_vector = control->aiming_vector;
    unit->desired_facing_vector = control->facing_vector;
    unit->seat_command = control->animation_state;

    if (source_id != -1) {
        unit->control_update_id = source_id;
        unit->control_update_id_valid = 1;
    } else {
        unit->control_update_id_valid = 0;
    }
}

/**
 * FIXED (objdump 0x5697a0): the original reads exactly two stack arguments ([ebp+8] the direction, [ebp+0xc]
 * the flag) and writes the clamped direction back through the first (EAX = ECX = [ebp+8] at the final
 * matrix4x3_transform_normal call). The draft's third `out` parameter read the caller's stack junk. REWRITTEN
 * from objdump 0x5697a0..0x569962. The unit basis is forward F / up U from object_get_orientation and left =
 * U x F (at the zero position, scale 1). The direction goes into that frame as (F.d, L.d, U.d);
 *
 * @address 0x5697a0
 */
uint8_t UnitView::clamp_direction_to_aim_or_look_bounds(real_vector3d *world_direction, uint8_t use_aiming_bounds)
{
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    uint8_t clamped = 0;
    uint8_t valid;
    float *bounds;
    real_matrix4x3 m;
    real_vector3d local;
    float x, y, z, yaw, pitch;

    if (use_aiming_bounds) {
        valid = (uint8_t)unit->unit.aiming_bounds_valid;
        bounds = (float *)&unit->unit.aiming_bounds;
    } else {
        valid = (uint8_t)unit->unit.looking_bounds_valid;
        bounds = (float *)&unit->unit.looking_bounds;
    }
    if (!valid) {
        return 0;
    }

    m.scale = 1.0f;
    halo::objects::object_get_orientation(&m.forward, unit_index, &m.up);
    m.left.i = m.forward.k * m.up.j - m.forward.j * m.up.k;
    m.left.j = m.up.k * m.forward.i - m.forward.k * m.up.i;
    m.left.k = m.forward.j * m.up.i - m.up.j * m.forward.i;
    m.position = *global_zero_vector3d_pointer;

    x = m.forward.j * world_direction->j + m.forward.k * world_direction->k + m.forward.i * world_direction->i;
    y = m.left.i * world_direction->i + m.left.j * world_direction->j + m.left.k * world_direction->k;
    z = m.up.j * world_direction->j + m.up.k * world_direction->k + m.up.i * world_direction->i;
    yaw = (float)halo::libm::atan2((double)y, (double)x);
    pitch = (float)halo::libm::atan2((double)z, halo::libm::sqrt((double)(y * y + x * x)));

    if (!(yaw >= bounds[0])) {
        yaw = bounds[0];
        clamped = 1;
    } else if (!(yaw <= bounds[1])) {
        yaw = bounds[1];
        clamped = 1;
    }
    if (!(pitch >= bounds[2])) {
        pitch = bounds[2];
        clamped = 1;
    } else if (!(pitch <= bounds[3])) {
        pitch = bounds[3];
        clamped = 1;
    } else if (!clamped) {
        return 0;
    }

    local.i = (float)halo::x87::fcos((double)yaw) * (float)halo::x87::fcos((double)pitch);
    local.j = (float)halo::x87::fsin((double)yaw) * (float)halo::x87::fcos((double)pitch);
    local.k = (float)halo::x87::fsin((double)pitch);
    halo::math::matrix4x3_transform_normal(*world_direction, local, m);
    return clamped;
}

/**
 * Engine function unit_get_aiming_vector.
 *
 * Original register convention: in_ECX, in_EAX.
 *
 * @address 0x5696f0
 */
void UnitView::get_aiming_vector(real_vector3d *out)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    *out = unit->aiming_vector;
    return;
}

/**
 * Engine function unit_get_camera_position.
 *
 * Original register convention: in_ECX -> unit_index, unaff_EDI -> out.
 *
 * @address 0x568f80
 */
void UnitView::get_camera_position(real_point3d *out)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);
    object_marker marker;

    if (unit_obj->parent_object == k_datum_index_none) {
        if (((unit_obj->vitality_flags & _object_health_frozen_bit) == 0) && (unit_obj->type == _object_type_biped)) {
            Biped *biped_tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;
            halo::objects::object_get_position(out, unit_index);
            biped_data *biped = halo::units::biped_data_of(unit_obj);
            float height = biped->crouch_fraction;
            if ((!test_flag(biped->flags, units::biped_flag::airborne)) && (0.0f < height) && (height < 1.0f)) {
                float rate = halo::game::globals().game_time->leftover_time * 29.999998f * biped_tag->crouch_camera_velocity;
                if (unit->base_animation_state == _unit_base_animation_state_crouch) {
                    height = height + rate;
                } else {
                    height = height - rate;
                }
            }
            out->z = height * biped_tag->crouching_camera_height + (1.0f - height) * biped_tag->standing_camera_height + out->z;
            return;
        }
        if (unit->gunner_unit_index == k_datum_index_none) {
            halo::objects::object_get_node_local_transform(unit_index, (char *)"head", &marker, 1);
        } else {
            object *gunner = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit->gunner_unit_index)].data;
            Unit *unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_obj->definition_tag)].data;
            UnitSeat *seat = (UnitSeat *)unit_tag->seats.pointer +
                (halo::units::unit_data_of(gunner))->vehicle_seat_index;
            halo::objects::object_get_node_local_transform(unit_index, seat->marker_name.string, &marker, 1);
        }
        *out = marker.node_transform.position;
        return;
    } else {
        object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_obj->parent_object)].data;
        Unit *parent_tag;
        UnitSeat *seat;
        *out = parent->position;
        if ((_object_mask_unit & (1 << (parent->type & 0x1f))) == 0) {
            return;
        }
        if (unit->vehicle_seat_index == -1) {
            return;
        }
        parent_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(parent->definition_tag)].data;
        seat = (UnitSeat *)parent_tag->seats.pointer + unit->vehicle_seat_index;
        if (parent->type == _object_type_vehicle && seat->camera_marker_name.string[0] == '\0') {
            return;
        }
        halo::objects::object_get_node_local_transform(unit_obj->parent_object, seat->camera_marker_name.string, &marker, 1);
        *out = marker.node_transform.position;
        return;
    }
}

/**
 * Computes a look/aim origin and direction for a unit, preferring its pelvis and head model nodes when the
 * Biped tag names both: with the "average pelvis/head" flag (biped_flags bit 0x10) set, origin is their
 * midpoint and direction is the zero vector (UNSURE -- see file header); otherwise origin is the pelvis
 * position and direction is head minus pelvis (unnormalized).
 *
 * @address 0x55a390
 */
void UnitView::get_look_origin_and_direction(uint32_t *out_autoaim_width, real_vector3d *out_direction, real_point3d *out_origin)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    real_matrix4x3 *nodes = (real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);

    if (tag->pelvis_model_node_index != halo::k_word_none && tag->head_model_node_index != halo::k_word_none) {
        real_matrix4x3 *pelvis = &nodes[tag->pelvis_model_node_index];
        real_matrix4x3 *head = &nodes[tag->head_model_node_index];

        if (test_flag(tag->biped_flags, tags::biped_tag_flag::spherical)) {
            out_origin->x = (pelvis->position.x + head->position.x) * 0.5f;
            out_origin->y = (pelvis->position.y + head->position.y) * 0.5f;
            out_origin->z = (pelvis->position.z + head->position.z) * 0.5f;
            out_direction->i = global_origin3d_pointer->x;
            out_direction->j = global_origin3d_pointer->y;
            out_direction->k = global_origin3d_pointer->z;
            *out_autoaim_width = *(uint32_t *)&tag->autoaim_width;
            return;
        }
        *out_origin = pelvis->position;
        out_direction->i = head->position.x - pelvis->position.x;
        out_direction->j = head->position.y - pelvis->position.y;
        out_direction->k = head->position.z - pelvis->position.z;
        *out_autoaim_width = *(uint32_t *)&tag->autoaim_width;
        return;
    }

    {
        float pill_height, pill_radius;
        ::halo::units::unit_get_crouch_height_offset(out_origin, object_index, &pill_height, &pill_radius);
        pill_height *= 0.5f;
        out_origin->z += pill_height;
        out_direction->i = pill_height * halo::math::globals().global_up3d_pointer->i;
        out_direction->j = pill_height * halo::math::globals().global_up3d_pointer->j;
        out_direction->k = pill_height * halo::math::globals().global_up3d_pointer->k;
    }
    *out_autoaim_width = *(uint32_t *)&tag->autoaim_width;
}

/**
 * One-time initialization of the unit's random idle-turn target angle: seeded from its current heading (or
 * zero, for an AI-controlled unit where actor_resolve_wander_or_look_direction applies) plus a small random
 * offset, only run once per activation of the idle_turn_seeded flag.
 *
 * @address 0x570650
 */
void UnitView::initialize_random_turn_angle()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    float half_range;

    if ((unit->flags & _unit_flag_idle_turn_seeded) != 0) {
        return;
    }
    unit->flags |= _unit_flag_idle_turn_seeded;

    real_vector3d direction;
    if (unit->actor_index == k_datum_index_none || halo::ai::actor_resolve_wander_or_look_direction(unit->actor_index, &direction) == 0) {
        float angle = (float)halo::libm::atan2((double)obj->forward.j, (double)obj->forward.i);
        if (angle > 3.1415927f) {
            angle -= 6.2831855f;
        }
        unit->idle_turn_angle = angle;
        half_range = 1.7453293f;
    } else {
        unit->idle_turn_angle = 0.0f;
        half_range = 0.43633232f;
    }

    halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
    unit->idle_turn_angle = (half_range - -half_range) * (float)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) *
                            halo::k_unit_word_scale + -half_range + unit->idle_turn_angle;
}

/**
 * Engine function unit_is_look_target_valid.
 *
 * Original register convention: in_ECX -> unit_index.
 *
 * @address 0x562570
 */
uint8_t UnitView::is_look_target_valid()
{
    uint32_t unit_index = datum_handle;
    if (unit_index == (uint32_t)-1) {
        return 1;
    }

    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    if (unit->controlling_player != k_datum_index_none &&
        (obj->parent_object == k_datum_index_none || unit->vehicle_seat_index == -1)) {
        return 0;
    }
    return 1;
}

/**
 * Returns whether a given world point lies within a cone of half-angle param_1 around the unit's forward
 * direction.
 *
 * @address 0x56c100
 */
uint8_t halo::units::unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point)
{
    if (unit_index == k_datum_index_none) {
        return 0;
    }
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(unit_obj);

    object_marker marker;
    halo::objects::object_get_node_local_transform(unit_index, ai_marker_name_a, &marker, 1)  ;

    real_vector3d to_point;
    float cos_angle;
    float dot;

    to_point.i = world_point->x - marker.node_transform.position.x;
    to_point.j = world_point->y - marker.node_transform.position.y;
    to_point.k = world_point->z - marker.node_transform.position.z;
    halo::math::vector3d_normalize_with_length(to_point);
    dot = to_point.k * unit->looking_vector.k + to_point.j * unit->looking_vector.j +
          to_point.i * unit->looking_vector.i;
    cos_angle = (float)halo::x87::fcos((double)cone_angle);
    return cos_angle < dot;
}

/**
 * Attempts to compute a projected/predicted aim position in front of the unit for certain vehicle sub-types
 * (0, 1, 4, 6), validating line-of-clearance via a collision test. Writes the result through out_position and
 * returns a non-negative value on success, or -1 on failure or for unsupported sub-types.
 *
 * @address 0x571de0
 */
int32_t UnitView::predict_aim_target_position(real_point3d *out_position)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    real_point3d base_position;
    real_vector3d delta;
    char hit;
    float hit_fraction = 0.0f;
    int32_t hit_result = 0;
    collision_bsp_segment_result segment_result;

    halo::objects::object_get_position(&base_position, unit_index);

    switch (tag->vehicle_type) {
    case 0: case 1: case 4: case 6:
        halo::objects::object_get_position(&base_position, unit_index);
        base_position.x += halo::math::globals().global_up3d_pointer->i * 0.4f;
        base_position.y += halo::math::globals().global_up3d_pointer->j * 0.4f;
        base_position.z += halo::math::globals().global_up3d_pointer->k * 0.4f;
        delta.i = global_down3d_pointer->i * 2.0f;
        delta.j = global_down3d_pointer->j * 2.0f;
        delta.k = global_down3d_pointer->k * 2.0f;

        {
            uint32_t flt_max_bits = 0x7f7fffff;
            hit = halo::physics::collision_bsp_query_segment_init(1, &segment_result, (ModelCollisionGeometryBSP *)global_structure_collision_bsp, 0,
                (uint32_t *)0, &base_position, &delta, *(float *)&flt_max_bits);
        }
        hit_fraction = segment_result.t;
        hit_result = *(int32_t *)((uint8_t *)&segment_result + 0x8);
        if (hit != 0) {
            out_position->x = delta.i * hit_fraction + base_position.x;
            out_position->y = delta.j * hit_fraction + base_position.y;
            out_position->z = delta.k * hit_fraction + base_position.z;
            return hit_result;
        }
        break;
    default:
        break;
    }
    return -1;
}

/**
 * Predicts a network-simulated unit's movement over a fraction of a tick without touching the live object:
 * finds the local player's controlled unit, runs the same pre-solve bookkeeping biped_update itself does on a
 * scratch copy of it, drives biped_integrate_movement against that copy, and returns the (time-scaled)
 * position/forward/up deltas between the copy's result and the live object's current state.
 *
 * @address 0x55cca0
 */
uint32_t halo::units::unit_predict_movement_delta(real_vector3d *out_position_delta, real_vector3d *out_forward_delta, real_vector3d *out_up_delta, float time_fraction)
{
    if (halo::game::globals().game_time->paused != 0) {
        return 0;
    }

    {
        data_iterator iterator;
        void *entry;

        iterator.data = halo::game::globals().player_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

        entry = halo::memory::data_iterator_next(&iterator);
        if (entry == 0) {
            return 0;
        }
        while (*(int16_t *)((uint8_t *)entry + 2) == -1) {
            entry = halo::memory::data_iterator_next(&iterator);
            if (entry == 0) {
                return 0;
            }
        }

        {
            datum_index unit_index = *(datum_index *)((uint8_t *)entry + 0x34);
            if (unit_index == k_datum_index_none) {
                return 0;
            }

            {
                object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
                void *tag_data = halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
                uint8_t working_copy[0x550];
                object *copy = (object *)working_copy;
                unit_data *copy_unit = (unit_data *)(working_copy + 0x1f4);
                biped_data *copy_biped = (biped_data *)(working_copy + 0x4cc);
                uint8_t output_flags[2] = {0, 0};
                float t;

                memcpy(working_copy, obj, sizeof(working_copy));

                if (copy->parent_object != k_datum_index_none) {
                    return 0;
                }

                if (test_flag(copy->vitality_flags, objects::vitality_flag::health_frozen) || (*(uint8_t *)((uint8_t *)tag_data + 0x2f4) & 0x44) == 0) {
                    copy_unit->desired_facing_vector.k = 0.0f;
                    if (halo::math::vector3d_normalize_with_length(copy_unit->desired_facing_vector) == 0.0f) {
                        copy_unit->desired_facing_vector = *halo::math::globals().global_forward3d_pointer;
                    }
                }

                switch (animation_state_id(copy_unit->animation_state)) {
                case unit_animation_state_id::idle:
                case unit_animation_state_id::turn_in_place_a:
                case unit_animation_state_id::turn_in_place_b:
                    copy_biped->movement_state = 0;
                    break;
                case unit_animation_state_id::move_front:
                case unit_animation_state_id::move_back:
                case unit_animation_state_id::move_left:
                case unit_animation_state_id::move_right:
                    copy_biped->movement_state = 1;
                    break;
                default:
                    copy_biped->movement_state = 2;
                    break;
                }

                if (copy_unit->throttle.i * copy_unit->throttle.i + copy_unit->throttle.j * copy_unit->throttle.j +
                    copy_unit->throttle.k * copy_unit->throttle.k < 0.010000001f) {
                    copy_unit->throttle.i = global_origin3d_pointer->x;
                    copy_unit->throttle.j = global_origin3d_pointer->y;
                    copy_unit->throttle.k = global_origin3d_pointer->z;
                }

                copy_biped->airborne_ticks = (copy_biped->flags & 1) ?
                    ((copy_biped->airborne_ticks < 0x7f) ? copy_biped->airborne_ticks + 1 : copy_biped->airborne_ticks) : 0;
                copy_biped->slipping_ticks = (copy_biped->flags & 2) ?
                    ((copy_biped->slipping_ticks < 0x7f) ? copy_biped->slipping_ticks + 1 : copy_biped->slipping_ticks) : 0;

                output_flags[1] = (uint8_t)(copy_unit->control_flags & 1);
                output_flags[0] = 0;

                BipedView(unit_index).integrate_movement((::object *)working_copy, (int8_t *)output_flags);

                t = time_fraction * 29.999998f;
                if (t > 1.0f) t = 1.0f;
                else if (t < 0.0f) t = 0.0f;

                out_position_delta->i = (copy->position.x - obj->position.x) * t;
                out_position_delta->j = (copy->position.y - obj->position.y) * t;
                out_position_delta->k = (copy->position.z - obj->position.z) * t;
                out_forward_delta->i = (copy->forward.i - obj->forward.i) * t;
                out_forward_delta->j = (copy->forward.j - obj->forward.j) * t;
                out_forward_delta->k = (copy->forward.k - obj->forward.k) * t;
                out_up_delta->i = (copy->up.i - obj->up.i) * t;
                out_up_delta->j = (copy->up.j - obj->up.j) * t;
                out_up_delta->k = (copy->up.k - obj->up.k) * t;
                return 1;
            }
        }
    }
}

/**
 * Optionally replaces `axis` with the unit's current aiming vector, optionally projects `point` onto the line
 * through the unit's camera along `axis`, and returns in *out_speed the component of the unit's root-object
 * velocity along `axis`.
 *
 * @address 0x5658f0
 */
void UnitView::project_onto_aiming_axis(real *out_speed, uint8_t project_point, uint8_t use_unit_aiming_vector, real_point3d *point, real_vector3d *axis)
{
    datum_index unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    real_point3d camera;
    real_vector3d velocity;
    real distance;

    if (use_unit_aiming_vector) {
        *axis = (halo::units::unit_data_of(unit_obj))->aiming_vector;
    }

    if (project_point) {
        UnitView(unit_index).get_camera_position(&camera);
        distance = (point->x - camera.x) * axis->i + (point->z - camera.z) * axis->k +
            (point->y - camera.y) * axis->j;
        point->x = distance * axis->i + camera.x;
        point->y = distance * axis->j + camera.y;
        point->z = distance * axis->k + camera.z;
    }

    halo::objects::object_get_root_object_velocities(unit_index, &velocity, (real_vector3d *)0);
    *out_speed = velocity.j * axis->j + velocity.k * axis->k + velocity.i * axis->i;
}

/**
 * REWRITTEN from objdump 0x55add0..0x55aec3. Stack: unit; EDI: the vehicle (or other object) it is leaving --
 * every caller loads EDI with it. Flattens the unit's forward (world forward when degenerate), sets up to
 * world up, flags +0x4cc bit 0, then places it around the vehicle: first a 27-point grid at 2x the pill
 * radius from the unit, then around the vehicle's bounding centre (+0xa0) at its bounding radius (+0xac). The
 * draft had no vehicle and passed zeros (no radius, no grid) to both placement calls.
 *
 * @address 0x55add0
 */
void UnitView::reset_orientation_and_find_position(uint32_t vehicle_index)
{
    uint32_t object_index = datum_handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(object_index));
    real_vector3d *forward = (real_vector3d *)&obj->base.forward;

    forward->k = 0.0f;
    if (halo::math::vector3d_normalize_with_length(*forward) == 0.0f) {
        *forward = *halo::math::globals().global_forward3d_pointer;
    }
    *(real_vector3d *)&obj->base.up.i = *halo::math::globals().global_up3d_pointer;
    set_flag(halo::units::biped_data_of(obj)->flags, units::biped_flag::airborne);
    if (!(uint8_t)::halo::units::unit_find_placement_position(object_index, vehicle_index, 0, 2.0f, 1, 0, 1, 0, 0)) {
        unit_object *vehicle = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(vehicle_index));
        real_point3d center = *(real_point3d *)&vehicle->base.bounding_center.x;

        ::halo::units::unit_find_placement_position(object_index, vehicle_index, 0, vehicle->base.bounding_radius, 1, 0, 0, 0, (real_vector3d *)&center);
    }
}

/**
 * UNSURE (see file header): rotates the object's forward and up vectors by an angle derived from normalizing
 * rotation_axis, then re-derives up as forward crossed with a rotated copy of itself to keep the basis
 * orthonormal, falling back to global_forward3d/global_up3d if the result degenerates.
 *
 * @address 0x55e6b0
 */
void UnitView::rotate_basis_about_axis()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    real_vector3d axis = obj->angular_velocity;
    float angle = halo::math::vector3d_normalize_with_length(axis);
    float angle_cos = (float)halo::x87::fcos(angle);
    float angle_sin = (float)halo::x87::fsin(angle);
    real_vector3d up;
    real_vector3d right;

    halo::math::vector3d_rotate_about_axis(obj->forward, axis, angle_sin, angle_cos);
    halo::math::vector3d_normalize_with_length(obj->forward);
    up = obj->up;
    halo::math::vector3d_rotate_about_axis(up, axis, angle_sin, angle_cos);
    right.i = up.k * obj->forward.j - up.j * obj->forward.k;
    right.j = up.i * obj->forward.k - up.k * obj->forward.i;
    right.k = up.j * obj->forward.i - up.i * obj->forward.j;
    obj->up.i = right.j * obj->forward.k - right.k * obj->forward.j;
    obj->up.j = right.k * obj->forward.i - right.i * obj->forward.k;
    obj->up.k = right.i * obj->forward.j - right.j * obj->forward.i;
    if (halo::math::vector3d_normalize_with_length(obj->up) == 0.0f) {
        obj->forward = *halo::math::globals().global_forward3d_pointer;
        obj->up = *halo::math::globals().global_up3d_pointer;
    }
}

/**
 * REWRITTEN from objdump 0x56bfc0..0x56c067: casts 25 along the aim from the camera (mask 0x22, ignoring the
 * unit); on a floor (normal.k > 0.95) the unit is moved 0.25 above the hit point. A debug "teleport to aim"
 * reached from game_engine_update_local_player_control; the draft relinked a NULL position.
 *
 * Original register convention: EBX -> unit_index.
 *
 * @address 0x56bfc0
 */
void UnitView::sample_camera_shake_from_velocity()
{
    uint32_t unit_index = datum_handle;
    unit_object *obj = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    real_point3d camera_position;
    real_vector3d delta;
    collision_result hit;

    UnitView(unit_index).get_camera_position(&camera_position);
    delta.i = obj->unit.aiming_vector.i * 25.0f;
    delta.j = obj->unit.aiming_vector.j * 25.0f;
    delta.k = obj->unit.aiming_vector.k * 25.0f;
    if (halo::physics::collision_test_movement_segment(halo::to_bits(halo::collision_test_flag::back_face | halo::collision_test_flag::structure_bsp), &camera_position, &delta, unit_index, &hit) &&
        hit.plane.normal.k > 0.95f) {
        real_point3d position = hit.point;

        position.z += 0.25f;
        halo::objects::object_set_position_and_relink(&position, unit_index, 0);
    }
}

/**
 * Engine function unit_set_control_countdown.
 *
 * Original register convention: see file header.
 *
 * @address 0x563b20
 */
void UnitView::set_control_countdown(int32_t countdown, uint32_t extra_control_flags)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);

    unit->persistent_control_ticks = countdown;
    unit->persistent_control_flags = extra_control_flags;
}

/**
 * When a global cinematic/lookup mode is active (global_scenario != 0), orients the unit to face a direction
 * taken from an indexed table (selected by the vehicle's cinematic_facing_index and the current game-mode
 * selector), and adjusts its extension_of_parent flag and height depending on whether its tag defines a
 * physics reference. FIXED (register inputs, objdump): the original never reads EAX as an input (it
 * overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x570de0
 */
void UnitView::set_facing_from_index_table()
{
    uint32_t object_index = datum_handle;
    object *obj;
    Object *tag;
    int16_t facing_index;
    float angle;
    real_vector3d forward;

    if (global_scenario == 0) {
        return;
    }

    obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    tag = (Object *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    facing_index = *(int16_t *)((uint8_t *)obj + 0x5b0);

    halo::objects::object_reset_velocity_and_wake(object_index);

    {
        real_point3d *spawn_position;

        if (control_binding_device_type == 5) {
            uint8_t *entry = *(uint8_t **)(global_scenario + 0x37c) + facing_index * 0x94;

            spawn_position = (real_point3d *)entry;
            angle = *(float *)(entry + 0xc);
        } else {
            int16_t stride = *(int16_t *)(object_type_definitions_ex + 0xe);
            int16_t base_field_offset = *(int16_t *)(object_type_definitions_ex + 10);
            uint8_t *base = *(uint8_t **)(global_scenario + 4 + base_field_offset);

            spawn_position = (real_point3d *)(base + facing_index * stride + 0x8);
            angle = *(float *)((uint8_t *)spawn_position + 0xc);
        }

        forward.i = (float)halo::libm::cos((double)angle);
        forward.j = (float)halo::libm::sin((double)angle);
        forward.k = 0.0f;
        halo::objects::object_set_position_and_orientation(object_index, &forward, halo::math::globals().global_up3d_pointer, spawn_position);
    }

    if ((int32_t)halo::objects::tag_handle(((struct Object *)tag)->physics) == -1) {
        set_flag(obj->flags, objects::object_flag::at_rest);
    } else {
        clear_flag(obj->flags, objects::object_flag::at_rest);
        obj->position.z += tag->bounding_radius * 0.5f;
    }
}

/**
 * Engine function unit_state_allows_control.
 *
 * Original register convention: ECX -> animation_block.
 *
 * @address 0x565ca0
 */
uint8_t halo::units::unit_state_allows_control(const uint8_t *animation_block)
{
    switch (animation_state_id((int8_t)animation_block[0xb])) {
    case unit_animation_state_id::unknown_01:
    case unit_animation_state_id::turn_in_place_a:
    case unit_animation_state_id::turn_in_place_b:
    case unit_animation_state_id::unknown_17:
    case unit_animation_state_id::seat_enter:
    case unit_animation_state_id::seat_exit:
    case unit_animation_state_id::custom_animation:
    case unit_animation_state_id::scripted_action:
    case unit_animation_state_id::unknown_1e:
    case unit_animation_state_id::unknown_1f:
    case unit_animation_state_id::throwing_grenade:
    case unit_animation_state_id::unknown_22:
    case unit_animation_state_id::unknown_23:
    case unit_animation_state_id::unknown_27:
    case unit_animation_state_id::unknown_29:
        return 0;
    default:
        return 1;
    }
}

/**
 * While a biped is airborne and has no resolved target-lock comparison, counts ticks (saturating at 0x7f)
 * since the last target lock; once past 5 ticks while the jump control is held, snaps it to its minimum
 * ground height.
 *
 * @address 0x55ec90
 */
void UnitView::track_target_lock_timeout()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    biped_data *biped = halo::units::biped_data_of(obj);

    if (!test_flag(biped->flags, units::biped_flag::airborne) && biped->landing_type != 1) {
        if ((int8_t)biped->jump_ticks < 0x7f) {
            biped->jump_ticks = biped->jump_ticks + 1;
        }
        if (test_flag(unit->control_flags, units::unit_control_flag::jump) && (int8_t)biped->jump_ticks > 5) {
            UnitView(object_index).snap_to_min_ground_height();
        }
    }
}

namespace unit_update_look_delta_controls_local {

static float clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

}

/**
 * Updates unit_data.animation_controls[0..2] from the frame-to-frame change of a reference point, projected
 * onto a forward/cross/up basis and scaled per-axis by a tag-defined acceleration_scale. When the unit is
 * properly seated (valid parent and seat index), the reference point is the parent object's position and the
 * basis comes from the seat's marker transform on the parent; otherwise the unit's own position and
 * forward/up vectors are used.
 *
 * @address 0x56e820
 */
void UnitView::update_look_delta_controls()
{
    using namespace unit_update_look_delta_controls_local;
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    real_point3d sample;
    real_vector3d forward, up;
    real_vector3d *scale;
    real_vector3d delta;

    if (unit->vehicle_seat_index == -1 || obj->parent_object == k_datum_index_none) {
        sample = obj->position;
        forward = obj->forward;
        up = obj->up;
        {
            Unit *tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
            scale = (real_vector3d *)&((struct Unit *)tag)->seat_acceleration_scale;
        }
    } else {
        object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(obj->parent_object)].data;
        Unit *parent_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(parent->definition_tag)].data;
        UnitSeat *seat = (UnitSeat *)((uint8_t *)*(uint8_t **)&((struct Unit *)parent_tag)->seats.pointer +
                                       unit->vehicle_seat_index * sizeof(UnitSeat));
        object_marker marker;
        int16_t found = halo::objects::object_get_node_local_transform(obj->parent_object, seat->marker_name.string,
                                                          &marker, 1);
        if (found == 0) {
            unit->animation_controls[0] = 0.5f;
            unit->animation_controls[1] = 0.5f;
            unit->animation_controls[2] = 0.5f;
            return;
        }
        halo::objects::object_get_position(&sample, obj->parent_object);
        forward = marker.node_transform.forward;
        up = marker.node_transform.up;
        scale = (real_vector3d *)&seat->acceleration_scale;
    }

    delta.i = sample.x - unit->seat_acceleration_last_position.x;
    delta.j = sample.y - unit->seat_acceleration_last_position.y;
    delta.k = sample.z - unit->seat_acceleration_last_position.z;
    delta.i -= unit->seat_acceleration_last_velocity.i;
    delta.j -= unit->seat_acceleration_last_velocity.j;
    delta.k -= unit->seat_acceleration_last_velocity.k;

    unit->animation_controls[0] = clamp01((forward.i * delta.i + forward.j * delta.j + forward.k * delta.k) * scale->i + 0.5f);
    unit->animation_controls[1] = clamp01(((forward.k * up.j - forward.j * up.k) * delta.i +
                                            (up.k * forward.i - forward.k * up.i) * delta.j +
                                            (forward.j * up.i - forward.i * up.j) * delta.k) * scale->j + 0.5f);
    unit->animation_controls[2] = clamp01((up.j * delta.j + up.i * delta.i + up.k * delta.k) * scale->k + 0.5f);

    {
        real_vector3d raw_movement;
        raw_movement.i = delta.i + unit->seat_acceleration_last_velocity.i;
        raw_movement.j = delta.j + unit->seat_acceleration_last_velocity.j;
        raw_movement.k = delta.k + unit->seat_acceleration_last_velocity.k;
        unit->seat_acceleration_last_position = sample;
        unit->seat_acceleration_last_velocity = raw_movement;
    }
}

/**
 * Randomly wanders the unit's idle look/turn angle (idle_turn_angle) each tick within clamped bounds around
 * idle_turn_offset, then applies the resulting rotation.
 *
 * @address 0x570840
 */
void UnitView::update_random_turn_angle(real_vector3d *out_axis)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    uint8_t is_actor_controlled = 0;
    float yaw_low, yaw_high;
    float pitch_low, pitch_high;
    float delta;

    if (unit->actor_index == k_datum_index_none || halo::ai::actor_resolve_wander_or_look_direction(unit->actor_index, out_axis) == 0) {
        *out_axis = *halo::math::globals().global_forward3d_pointer;
    } else {
        is_actor_controlled = 1;
    }

    yaw_low = 1.0f;
    yaw_high = 1.0f;
    if (is_actor_controlled) {
        float a = (0.7853982f - unit->idle_turn_angle) * 4.2441316f;
        yaw_low = (a < 1.0f) ? a : 1.0f;
        float b = (unit->idle_turn_angle + 0.7853982f) * 4.2441316f;
        yaw_high = (b < 1.0f) ? b : 1.0f;
    }

    {
        float a = (0.20943952f - unit->idle_turn_offset) * 15.915494f;
        if (a < yaw_low) yaw_low = a;
        float b = (unit->idle_turn_offset + 0.20943952f) * 15.915494f;
        if (b < yaw_high) yaw_high = b;
    }
    pitch_low = yaw_low;
    pitch_high = yaw_high;

    if (pitch_high <= pitch_low) {
        if (pitch_high >= -1.0f) {
            float clamped = (pitch_high < 1.0f) ? pitch_high : 1.0f;
            halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
            delta = (0.02094395f - clamped * -0.02094395f) *
                    (float)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale + clamped * -0.02094395f;
        } else {
            delta = 0.02094395f;
        }
    } else if (pitch_low >= -1.0f) {
        float clamped = (pitch_low < 1.0f) ? pitch_low : 1.0f;
        halo::math::globals().random_seed_global = halo::advance_random_seed(halo::math::globals().random_seed_global);
        delta = (float)(halo::math::globals().random_seed_global >> halo::k_random_high_shift) * halo::k_unit_word_scale *
                (clamped * 0.02094395f - -0.02094395f) - 0.02094395f;
    } else {
        delta = -0.02094395f;
    }

    delta += unit->idle_turn_offset;
    unit->idle_turn_offset = delta;
    delta += unit->idle_turn_angle;
    unit->idle_turn_angle = delta;

    if (delta < -3.1415927f) {
        unit->idle_turn_angle = delta + 6.2831855f;
    } else if (delta > 3.1415927f) {
        unit->idle_turn_angle = delta - 6.2831855f;
    }

    {
        float c = (float)halo::libm::cos((double)unit->idle_turn_angle);
        float s = (float)halo::libm::sin((double)unit->idle_turn_angle);
        halo::math::vector3d_rotate_about_axis(*out_axis, *halo::math::globals().global_up3d_pointer, s, c);
    }
}

namespace unit_update_stance_and_jump_local {

static int16_t animation_table_lookup(ModelAnimations *graph, int32_t index)
{
    if (index < 0 || index >= (int32_t)graph->unit_damage.count) {
        return -1;
    }
    return (*(int16_t **)&graph->unit_damage.pointer)[index];
}

}

/**
 * Engine function unit_update_stance_and_jump.
 *
 * @address 0x566de0
 */
void UnitView::update_stance_and_jump(uint8_t force_ready, uint8_t allow_death_reaction, uint8_t suppress_shield_check, uint8_t ignore_disoriented, uint8_t force_reaction, float turn_angle, int16_t weapon_class_index, const real_vector2d *throttle, uint8_t require_still)
{
    using namespace unit_update_stance_and_jump_local;
    uint32_t unit_index = datum_handle;
    unit_object *obj = reinterpret_cast<unit_object *>(*(uint8_t **)((uint8_t *)halo::objects::globals().object_data->data + halo::datum_slot(unit_index) * 0xc + 8));
    Unit *unit_tag = halo::objects::tag_as<Unit>(*(datum_index *)obj);
    uint8_t forced = force_ready;
    uint8_t soft_ping;
    uint8_t hard_ping;
    int32_t weapon_class = weapon_class_index;
    int32_t facing;
    int32_t stance_class;
    datum_index graph_tag;
    ModelAnimations *graph;
    int16_t new_state;
    int16_t animation;
    bool animation_started = false;
    uint8_t allowed;
    double turn;

    if (forced) {
        allow_death_reaction = 0;
        soft_ping = 1;
        hard_ping = unit_tag->hard_death_threshold > 0.0f && obj->base.current_body_damage > unit_tag->hard_death_threshold;
    } else if (allow_death_reaction) {
        forced = 1;
        soft_ping = 1;
        hard_ping = 0;
    } else {
        soft_ping = obj->base.current_body_damage > unit_tag->soft_ping_threshold ||
            obj->base.current_shield_damage > unit_tag->soft_ping_threshold;
        hard_ping = obj->base.current_body_damage > unit_tag->hard_ping_threshold;
        if (ignore_disoriented || (int8_t)(uint8_t)obj->unit.flags < 0) {
            hard_ping = 0;
        }
    }
    if (force_reaction) {
        hard_ping = 1;
        soft_ping = 1;
    }
    if (weapon_class_index == -1) {
        weapon_class = 0;
    }
    turn = halo::libm::fabs(turn_angle);
    if (turn < 0.78539818525314331) {
        facing = 3;
    } else if (turn > 2.1598450094461441) {
        facing = 0;
    } else {
        facing = turn_angle > 0.0f ? 1 : 2;
    }
    if (halo::game::globals().current_engine != 0 && (int16_t)weapon_class == 2 && hard_ping && forced) {
        facing = 1;
    }
    if (require_still && !soft_ping && !forced) {
        return;
    }
    graph_tag = halo::objects::tag_handle(unit_tag->base.animation_graph);
    graph = halo::objects::tag_as<ModelAnimations>(graph_tag);

    if (!hard_ping && !forced) {
        if (obj->unit.overlays[2].animation_index != -1 && obj->unit.overlays[2].frame <= unit_tag->soft_ping_interrupt_ticks) {
            return;
        }
        animation = halo::models::animation_choose_random_permutation(graph_tag,
            animation_table_lookup(graph, (int16_t)(facing * 0xb + weapon_class)), static_cast<animation_random_stream>(1));
        if (animation == -1) {
            return;
        }
        obj->unit.overlays[2].animation_index = animation;
        obj->unit.overlays[2].frame = 0;
        return;
    }

    new_state = forced ? 0x19 : 0x17;
    if (forced) {
        stance_class = hard_ping + 2;
        allowed = 1;
    } else {
        stance_class = 1;
        allowed = ::halo::units::unit_animation_state_is_compatible(reinterpret_cast<uint8_t *>(obj) + 0x298, new_state) ? 1 : 0;
    }
    if ((uint8_t)obj->unit.animation_state == animation_state_value(unit_animation_state_id::unknown_17) && obj->base.animation_frame > unit_tag->hard_ping_interrupt_ticks) {
        allowed = 1;
    }
    if (!forced) {
        if ((uint8_t)obj->base.vitality_flags & 4) {
            allowed = 0;
        }
        if ((int32_t)obj->base.parent_object != -1) {
            return;
        }
    }
    if (!allowed) {
        return;
    }
    if (forced) {
        UnitView(unit_index).set_or_test_seat_and_weapon_label(s_stand, UnitView(unit_index).get_current_weapon_label(), 1);
    }
    if (new_state == animation_state_value(unit_animation_state_id::ready_weapon) && obj->base.type == 0 && test_flag(halo::units::biped_data_of(obj)->flags, units::biped_flag::airborne) &&
        !test_flag(reinterpret_cast<const Biped *>(unit_tag)->biped_flags, tags::biped_tag_flag::has_no_dying_airborne)) {
        new_state = animation_state_value(unit_animation_state_id::unknown_18);
        if (UnitView(unit_index).try_set_animation_state(animation_state_value(unit_animation_state_id::unknown_18))) {
            animation_started = true;
        }
    }

    if (!animation_started) {
        animation = halo::models::animation_choose_random_permutation(graph_tag,
            animation_table_lookup(graph, (int16_t)((facing + stance_class * 4) * 0xb + weapon_class)), static_cast<animation_random_stream>(1));
        if (animation == -1) {
            if (forced) {
                obj->unit.animation_state_flags = (uint16_t)((obj->unit.animation_state_flags & 0xfff7) | 4);
                if ((uint8_t)unit_tag->unit_flags & 2) {
                    halo::objects::object_delete_teardown(unit_index);
                    UnitView(unit_index).pick_random_spawned_actor_count();
                }
            }
        } else {
            ModelAnimationsAnimation *animation_data = reinterpret_cast<ModelAnimationsAnimation *>(&halo::objects::block_element<ModelAnimationsAnimation>(graph->animations, animation));

            if ((uint8_t)obj->unit.animation_state == animation_state_value(unit_animation_state_id::throwing_grenade)) {
                UnitView(unit_index).release_thrown_grenade(1);
            }
            halo::objects::object_copy_default_node_transforms(unit_index, 3);
            obj->unit.animation_state = (uint8_t)new_state;
            UnitView(unit_index).set_custom_animation(graph_tag, animation);
            set_flag(obj->unit.animation_state_flags, units::unit_animation_state_flag::action_active);
            if (forced) {
                uint8_t keep_still = suppress_shield_check || allow_death_reaction;

                if (!keep_still && halo::networking::globals().game_mode != 0) {
                    datum_index weapon = UnitView(unit_index).get_weapon_object_index(obj->unit.current_weapon_index);

                    if (halo::objects::object_try_and_get(weapon, 4) != 0 && halo::items::weapon_must_be_readied(weapon) == 1) {
                        keep_still = 1;
                    }
                }
                if (keep_still) {
                    obj->unit.delayed_weapon_drop_ticks = 0;
                } else {
                    int16_t frames = (int16_t)animation_data->frame_count;
                    int8_t ticks = (int8_t)halo::math::random_int_range((int16_t)(frames >> 2),
                        (int16_t)((frames >> 1) + (frames >> 2)));

                    obj->unit.delayed_weapon_drop_ticks = (uint8_t)(ticks > 1 ? ticks : 1);
                }
            }
            if ((int16_t)facing != 0 &&
                (int16_t)animation_data->main_animation_index ==
                    animation_table_lookup(graph, (int16_t)stance_class * 0x2c + (int16_t)weapon_class)) {
                facing = 0;
            }
            if (forced) {
                if ((int16_t)facing == 3) {
                    set_flag(obj->unit.animation_state_flags, units::unit_animation_state_flag::unknown_8);
                } else {
                    clear_flag(obj->unit.animation_state_flags, units::unit_animation_state_flag::unknown_8);
                }
            }
        }
    }
    if (throttle == 0 || (test_flag(unit_tag->unit_flags, tags::unit_tag_flag::don_t_reface_during_pings)) || obj->base.type != 0 ||
        (int32_t)obj->base.parent_object != -1 || (!hard_ping && !forced)) {
        return;
    }
    {
        real_vector2d direction;

        switch ((int16_t)facing) {
        case 0:
            direction.i = -throttle->i;
            direction.j = -throttle->j;
            break;
        case 1:
            direction.i = -throttle->j;
            direction.j = throttle->i;
            break;
        case 2:
            direction.i = throttle->j;
            direction.j = -throttle->i;
            break;
        default:
            direction = *throttle;
            break;
        }
        UnitView(unit_index).set_throw_aim_direction(&direction);
    }
}

namespace unit_update_up_vector_local {

static void level_to_world_up(object *obj)
{
    obj->forward.k = 0.0f;
    if (halo::math::vector3d_normalize_with_length(obj->forward) == 0.0f) {
        obj->forward = *halo::math::globals().global_forward3d_pointer;
    }
    obj->up = *halo::math::globals().global_up3d_pointer;
}

}

/**
 * Engine function unit_update_up_vector.
 *
 * @address 0x560800
 */
void halo::units::unit_update_up_vector(Biped *biped_tag, object *obj)
{
    using namespace unit_update_up_vector_local;
    biped_data *biped = halo::units::biped_data_of(obj);
    uint32_t tag_flags = biped_tag->biped_flags;
    uint8_t frozen = (obj->vitality_flags & _object_health_frozen_bit) != 0;

    if (test_flag(tag_flags, tags::biped_tag_flag::flying) && !frozen) {
        real_vector3d up0;
        real_vector3d side;
        float c, s;

        halo::math::vector3d_cross_product(side, *halo::math::globals().global_up3d_pointer, obj->forward);
        halo::math::vector3d_cross_product(up0, obj->forward, side);
        if (halo::math::vector3d_normalize_with_length(up0) == 0.0f) {
            up0 = *halo::math::globals().global_forward3d_pointer;
            side = *halo::math::globals().global_left3d_pointer;
        }
        c = (float)halo::libm::cos((double)biped->bank_angle);
        s = (float)halo::libm::sin((double)biped->bank_angle);
        up0.i *= c;
        up0.j *= c;
        up0.k *= c;
        halo::math::vector3d_normalize_with_length(side);
        obj->up.i = side.i * s + up0.i;
        obj->up.j = side.j * s + up0.j;
        obj->up.k = side.k * s + up0.k;
        return;
    }

    if (test_flag(tag_flags, tags::biped_tag_flag::can_climb_any_surface) && !frozen) {
        real_vector3d target;
        real_vector3d cross1;
        real_vector3d frame;

        if (biped->ground_surface_index == k_datum_index_none) {
            target = obj->up;
        } else {
            real_vector3d axis;
            real_vector3d turned;
            real_vector3d check;
            uint8_t use_target = 0;

            target = biped->ground_normal;
            halo::math::vector3d_cross_product(axis, target, obj->up);
            if (halo::math::vector3d_normalize_with_length(axis) == 0.0f) {
                if (target.j * obj->up.j + target.k * obj->up.k + target.i * obj->up.i > 0.0f) {
                    use_target = 1;
                } else {
                    axis = obj->forward;
                }
            }
            if (!use_target) {
                float c = (float)halo::libm::cos(0.1745329201221466);
                float s = (float)halo::libm::sin(0.1745329201221466);
                turned = obj->up;
                halo::math::vector3d_rotate_about_axis(turned, axis, s, c);
                halo::math::vector3d_cross_product(check, target, turned);
                if (check.k * axis.k + check.j * axis.j + check.i * axis.i > 0.0f) {
                    target = turned;
                }
            }
        }

        halo::math::vector3d_cross_product(cross1, target, obj->forward);
        halo::math::vector3d_cross_product(frame, cross1, target);
        if (halo::math::vector3d_normalize_with_length(frame) == 0.0f) {
            halo::math::vector3d_cross_product(cross1, obj->up, target);
            halo::math::vector3d_cross_product(frame, cross1, target);
            if (halo::math::vector3d_normalize_with_length(frame) == 0.0f) {
                target = *halo::math::globals().global_up3d_pointer;
                frame = *halo::math::globals().global_forward3d_pointer;
            }
        }
        obj->up = target;
        obj->forward = frame;
        return;
    }

    if (!frozen) {
        level_to_world_up(obj);
        return;
    }

    if (test_flag(biped->flags, units::biped_flag::airborne)) {
        level_to_world_up(obj);
        return;
    }
    {
        real_vector3d *normal = &biped->ground_normal;
        float dot = obj->up.k * normal->k + obj->up.j * normal->j + obj->up.i * normal->i;
        float angle;
        real_vector3d axis;
        float c, s;

        if ((float)halo::libm::fabs((double)(dot - 1.0f)) < 0.0001f) {
            return;
        }
        angle = (float)halo::libm::acos((double)dot);
        if (angle == 0.0f) {
            return;
        }
        halo::math::vector3d_cross_product(axis, *normal, obj->up);
        if (halo::math::vector3d_normalize_with_length(axis) == 0.0f) {
            return;
        }
        c = (float)halo::libm::cos((double)angle);
        s = (float)halo::libm::sin((double)angle);
        halo::math::vector3d_rotate_about_axis(obj->up, axis, s, c);
        halo::math::vector3d_rotate_about_axis(obj->forward, axis, s, c);
        halo::math::vector3d_normalize_with_length(obj->up);
        halo::math::vector3d_normalize_with_length(obj->forward);
    }
}

}
