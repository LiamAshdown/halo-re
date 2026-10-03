#include "halo/objects/record_access.hpp"
#include "halo/units/records.hpp"
#include "halo/units/unit.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "physics.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"
#include "halo/units/api.hpp"

static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);
static auto &ai_marker_name_b = halo::link::ref<char []>(halo::ai::vars().ai_marker_name_b);

namespace halo::units {

/**
 * Engine function unit_add_marker_relative_offset.
 *
 * @address 0x569190
 */
void UnitView::add_marker_relative_offset(uint32_t mode, float *world_point, uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator)
{
    uint32_t unit_index = datum_handle;
    unit_object *unit = reinterpret_cast<unit_object *>(halo::objects::object_record_bytes(unit_index));
    datum_index parent_index = unit->base.parent_object;
    real_point3d reference;
    int have_reference = 0;

    if (parent_index == k_datum_index_none && !test_flag(unit->base.vitality_flags, objects::vitality_flag::health_frozen)) {
        if (unit->base.type == _object_type_biped) {
            UnitView(unit_index).compute_marker_offset_position((real_vector3d *)reference_direction, (int16_t)mode, accumulator, world_point, (float *)offsets);
            return;
        }
    } else if (unit->base.type == _object_type_biped && parent_index != k_datum_index_none) {
        object *parent = reinterpret_cast<object *>(halo::objects::object_record_bytes(parent_index));

        if (parent->type == _object_type_vehicle && UnitView(parent_index).predict_aim_target_position(&reference) != -1) {
            have_reference = 1;
        }
    }
    if (!have_reference) {
        halo::objects::object_get_position(&reference, unit_index);
    }
    UnitView(unit_index).get_camera_position(accumulator);
    accumulator->x = (world_point[0] - reference.x) + accumulator->x;
    accumulator->y = (world_point[1] - reference.y) + accumulator->y;
    accumulator->z = (world_point[2] - reference.z) + accumulator->z;
}

/**
 * Computes and caches the unit's current light/luminosity value from its RGB color state, or inherits both
 * cached values from a parent/attached object when one is present.
 *
 * @address 0x56ec60
 */
void UnitView::calculate_luminosity()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    unit_data *unit = halo::units::unit_data_of(obj);
    object *parent = halo::objects::object_try_and_get(obj->parent_object, 3);

    if (parent == 0) {
        real_vector3d rgb;
        halo::objects::object_sample_total_lighting_at_point(&obj->position, (bsp_leaf_reference *)&obj->location_leaf_index, &rgb);
        unit->illumination = rgb.i * 0.299f + rgb.j * 0.587f + rgb.k * 0.114f;
        unit->attached_light_luminosity = halo::objects::object_sum_attached_light_luminance(object_index);
        return;
    }

    unit_data *parent_unit = halo::units::unit_data_of(parent);
    unit->illumination = parent_unit->illumination;
    unit->attached_light_luminosity = parent_unit->attached_light_luminosity;
}

/**
 * Fills out_position with a blend-mode-dependent offset position for a unit marker/attachment point. Mode 0
 * just copies the object's position. Any other mode seeds out_position from base_position, and mode 3
 * additionally offsets it along reference_direction and a perpendicular built from offsets.
 *
 * @address 0x55a170
 */
void UnitView::compute_marker_offset_position(real_vector3d *reference_direction, int16_t mode, real_point3d *out_position, float *base_position, float *offsets)
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(object_index)].data;
    Biped *tag = (Biped *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    Biped *tag_data = reinterpret_cast<Biped *>(tag);
    biped_data *biped = halo::units::biped_data_of(obj);
    unit_data *unit = halo::units::unit_data_of(obj);
    float fraction;

    if (mode == 0) {
        halo::objects::object_get_position(out_position, object_index);
    } else {
        *out_position = *(real_point3d *)base_position;
        if (mode == 3) {
            out_position->x += offsets[0] * reference_direction->i;
            out_position->y += offsets[0] * reference_direction->j;
            out_position->z += offsets[0] * reference_direction->k;
            out_position->x += offsets[1] * -reference_direction->j;
            out_position->y += offsets[1] * reference_direction->i;
            out_position->z = (out_position->z + offsets[1] * 0.0f) + offsets[2];
            return;
        }
    }

    if (mode == 1) {
        fraction = 0.0f;
    } else if (mode == 2) {
        fraction = 1.0f;
    } else {
        fraction = biped->crouch_fraction;
        if (!test_flag(biped->flags, units::biped_flag::airborne) && fraction > 0.0f && fraction < 1.0f) {
            float step = halo::game::globals().game_time->leftover_time * 29.999998f * tag->crouch_camera_velocity;
            if (unit->base_animation_state == _unit_base_animation_state_crouch) {
                fraction += step;
            } else {
                fraction -= step;
            }
        }
    }
    out_position->z += fraction * tag_data->crouching_camera_height +
                        (1.0f - fraction) * tag_data->standing_camera_height;
}

/**
 * Direction from the unit's position to the world-space average of the mass points selected by
 * vehicle_data.active_marker_mask (+0x520). Fails when no bit is set, when the object has no physics, when no
 * selected mass point exists, or when the direction has zero length.
 *
 * @address 0x575e30
 */
uint8_t UnitView::get_average_active_marker_direction(real_vector3d *out_direction)
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = halo::objects::object_record_bytes(unit_index);
    uint32_t mask = *(uint32_t *)(obj + 0x520);
    object_physics_context ctx;
    Physics *physics;
    int32_t count;
    int16_t active_count = 0;
    int16_t i;
    real_point3d sum;
    real_point3d world;
    real_point3d position;
    float inv;

    if (mask == 0) {
        return 0;
    }
    if (!halo::physics::object_physics_context_build(unit_index, &ctx)) {
        return 0;
    }

    physics = reinterpret_cast<Physics *>(ctx.definition);
    count = physics->mass_points.count;
    sum = *global_zero_vector3d_pointer;
    if (count <= 0) {
        return 0;
    }
    i = 0;
    do {
        if ((mask & (1u << (i & 0x1f))) != 0) {
            real_point3d *p = (real_point3d *)(&halo::objects::block_element<PhysicsMassPoint>(physics->mass_points, (int32_t)i).position.x);
            sum.x += p->x;
            sum.y += p->y;
            sum.z += p->z;
            active_count++;
        }
        i++;
    } while ((int32_t)i < count);

    if (active_count <= 0) {
        return 0;
    }
    inv = 1.0f / (float)(int32_t)active_count;
    sum.x *= inv;
    sum.y *= inv;
    sum.z *= inv;
    halo::math::matrix4x3_transform_point(world, sum, *((real_matrix4x3 *)&ctx.scale));
    halo::objects::object_get_position(&position, unit_index);
    out_direction->i = world.x - position.x;
    out_direction->j = world.y - position.y;
    out_direction->k = world.z - position.z;
    if (halo::math::vector3d_normalize_with_length(*out_direction) == 0.0f) {
        return 0;
    }
    return 1;
}

/**
 * Returns a unit's stored direction/offset vector, transformed into world space through its parent object's
 * skeleton node when attached.
 *
 * Original register convention: in_ECX, in_EAX.
 *
 * @address 0x569720
 */
void UnitView::get_forward_vector_or_marker_normal(real_vector3d *out)
{
    uint32_t unit_index = datum_handle;
    object *unit_obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;

    if (unit_obj->parent_object == k_datum_index_none) {
        if (out != nullptr) {
            *out = unit_obj->forward;
        }
        return;
    }

    object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_obj->parent_object)].data;
    if (out != nullptr) {
        real_matrix4x3 *node = halo::objects::object_block<real_matrix4x3>(*parent, parent->nodes) + unit_obj->parent_marker_index;
        halo::math::matrix4x3_transform_normal(*out, unit_obj->forward, *node);
    }
    return;
}

/**
 * Retrieves the world position of a fixed named marker via object_get_node_local_transform and returns it through the implicit
 * ESI output pointer
 *
 * Original register convention: ECX -> object_index, ESI -> out.
 *
 * @address 0x568f50
 */
void UnitView::get_primary_eye_marker_position(real_point3d *out)
{
    uint32_t object_index = datum_handle;
    object_marker marker;
    halo::objects::object_get_node_local_transform(object_index, ai_marker_name_a, &marker, 1)  ;
    *out = marker.node_transform.position;
    return;
}

/**
 * Retrieves the world position of a second fixed named marker via object_get_node_local_transform and returns it through the
 * implicit ESI output pointer
 *
 * Original register convention: ECX -> object_index, ESI -> out.
 *
 * @address 0x569280
 */
void UnitView::get_secondary_eye_marker_position(real_point3d *out)
{
    uint32_t object_index = datum_handle;
    object_marker marker;
    halo::objects::object_get_node_local_transform(object_index, ai_marker_name_b, &marker, 1)  ;
    *out = marker.node_transform.position;
    return;
}

/**
 * REWRITTEN from objdump. For each contact point (stride 0x130) with flag bit 1 whose velocity (+0x54..+0x5c)
 * is faster than 0.03: intensity = clamp((speed - 0.03) * 4.5454545, 0, 1); position = contact +0x04 + normal
 * (+0x60) * (contact +0x74 - physics reinterpret_cast<uint8_t *>(node) +0x68 + 0.003); offset = velocity * (0.8660254 / speed) + normal *
 * 0.5; then material_effects_play_at_marker(reinterpret_cast<uint8_t *>(tag) +0x3dc, 9 + (reinterpret_cast<uint8_t *>(node) +0x24 & 1), contact +0x70, &reinterpret_cast<uint8_t *>(obj) +0x98,
 * intensity, &position, &offset).
 *
 * @address 0x575460
 */
void UnitView::update_marker_skid_effects(uint8_t *contact_points)
{
    uint32_t unit_index = datum_handle;
    object *obj = reinterpret_cast<object *>(halo::objects::object_record_bytes(unit_index));
    Vehicle *tag = halo::objects::tag_as<Vehicle>(*(datum_index *)obj);
    Physics *physics_tag;
    int32_t count;
    int16_t i;

    if ((int32_t)halo::objects::tag_handle(tag->material_effects) == -1) {
        return;
    }
    physics_tag = halo::objects::tag_as<Physics>(halo::objects::tag_handle(tag->base.base.physics));
    count = physics_tag->mass_points.count;
    for (i = 0; (int32_t)i < count; i++) {
        uint8_t *contact = contact_points + (int32_t)i * 0x130;
        PhysicsMassPoint *node = &halo::objects::block_element<PhysicsMassPoint>(physics_tag->mass_points, (int32_t)i);
        real_vector3d *velocity = (real_vector3d *)(contact + 0x54);
        real_vector3d *normal = (real_vector3d *)(contact + 0x60);
        real_point3d *point = (real_point3d *)(contact + 0x04);
        real_point3d position;
        real_vector3d offset;
        real speed, scaled, depth, k;
        uint32_t intensity_bits;

        if ((contact[0] & 2) == 0) {
            continue;
        }
        speed = (real)halo::libm::sqrt((double)(velocity->k * velocity->k + velocity->j * velocity->j +
            velocity->i * velocity->i));
        if (!(speed > 0.03f)) {
            continue;
        }
        scaled = (speed - 0.03f) * 4.5454545f;
        depth = *(real *)(contact + 0x74) - node->radius + 0.003f;
        position.x = depth * normal->i + point->x;
        position.y = depth * normal->j + point->y;
        position.z = depth * normal->k + point->z;
        k = 0.8660254f / speed;
        offset.i = normal->i * 0.5f + k * velocity->i;
        offset.j = normal->j * 0.5f + k * velocity->j;
        offset.k = normal->k * 0.5f + k * velocity->k;
        if (!(scaled >= 0.0f)) {
            scaled = 0.0f;
        } else if (!(scaled <= 1.0f)) {
            scaled = 1.0f;
        }
        intensity_bits = *(uint32_t *)&scaled;
        halo::effects::material_effects_play_at_marker(halo::objects::tag_handle(tag->material_effects), (int16_t)(9 + (node->flags & 1)),
            *(int16_t *)(contact + 0x70), (uint32_t *)&obj->location_leaf_index, intensity_bits, &position, &offset);
    }
}

}
