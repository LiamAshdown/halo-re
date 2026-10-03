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

extern "C" {
extern game_time_globals *game_time;
extern real_point3d *global_zero_vector3d_pointer;
extern uint8_t object_physics_context_build(uint32_t object_index, object_physics_context *out_context);
extern char ai_marker_name_a[];
extern char ai_marker_name_b[];
extern double sqrt(double x);
}

namespace halo::units {

/**
 * Engine function unit_add_marker_relative_offset.
 *
 * @address 0x569190
 */
void UnitView::add_marker_relative_offset(uint32_t mode, float *world_point, uint32_t reference_direction, uint32_t offsets, real_point3d *accumulator)
{
    uint32_t unit_index = datum_handle;
    uint8_t *unit = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    datum_index parent_index = ((unit_object *)unit)->base.parent_object;
    real_point3d reference;
    int have_reference = 0;

    if (parent_index == k_datum_index_none && !test_flag(((struct object *)unit)->vitality_flags, objects::vitality_flag::health_frozen)) {
        if (((unit_object *)unit)->base.type == 0) {
            UnitView(unit_index).compute_marker_offset_position((real_vector3d *)reference_direction, (int16_t)mode, accumulator, world_point, (float *)offsets);
            return;
        }
    } else if (((unit_object *)unit)->base.type == 0 && parent_index != k_datum_index_none) {
        uint8_t *parent = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(parent_index)].data;

        if (((object *)parent)->type == 1 && UnitView(parent_index).predict_aim_target_position(&reference) != -1) {
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
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    object *parent = halo::objects::object_try_and_get(obj->parent_object, 3);

    if (parent == 0) {
        real_vector3d rgb;
        halo::objects::object_sample_total_lighting_at_point(&obj->position, (bsp_leaf_reference *)&obj->location_leaf_index, &rgb);
        unit->illumination = rgb.i * 0.299f + rgb.j * 0.587f + rgb.k * 0.114f;
        unit->attached_light_luminosity = halo::objects::object_sum_attached_light_luminance(object_index);
        return;
    }

    unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
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
    uint8_t *tag_data = (uint8_t *)tag;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
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
            float step = game_time->leftover_time * 29.999998f * tag->crouch_camera_velocity;
            if (unit->base_animation_state == 3) {
                fraction += step;
            } else {
                fraction -= step;
            }
        }
    }
    out_position->z += fraction * ((struct Biped *)tag_data)->crouching_camera_height +
                        (1.0f - fraction) * ((struct Biped *)tag_data)->standing_camera_height;
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
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    uint32_t mask = *(uint32_t *)(obj + 0x520);
    object_physics_context ctx;
    uint8_t *physics;
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

    physics = (uint8_t *)ctx.definition;
    count = *(int32_t *)(physics + 0x74);
    sum = *global_zero_vector3d_pointer;
    if (count <= 0) {
        return 0;
    }
    i = 0;
    do {
        if ((mask & (1u << (i & 0x1f))) != 0) {
            real_point3d *p = (real_point3d *)(*(uint8_t **)(physics + 0x78) + (int32_t)i * 0x80 + 0x38);
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
        if (out != (real_vector3d *)0) {
            *out = unit_obj->forward;
        }
        return;
    }

    object *parent = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_obj->parent_object)].data;
    if (out != (real_vector3d *)0) {
        real_matrix4x3 *node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset) + unit_obj->parent_marker_index;
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
 * (+0x60) * (contact +0x74 - physics node +0x68 + 0.003); offset = velocity * (0.8660254 / speed) + normal *
 * 0.5; then material_effects_play_at_marker(tag +0x3dc, 9 + (node +0x24 & 1), contact +0x70, &obj +0x98,
 * intensity, &position, &offset).
 *
 * @address 0x575460
 */
void UnitView::update_marker_skid_effects(uint8_t *contact_points)
{
    uint32_t unit_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)obj)].data;
    uint8_t *physics_tag;
    int32_t count;
    int16_t i;

    if (*(int32_t *)(tag + 0x3dc) == -1) {
        return;
    }
    physics_tag = (uint8_t *)halo::cache::globals().tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    count = *(int32_t *)(physics_tag + 0x74);
    for (i = 0; (int32_t)i < count; i++) {
        uint8_t *contact = contact_points + (int32_t)i * 0x130;
        uint8_t *node = *(uint8_t **)(physics_tag + 0x78) + (int32_t)i * 0x80;
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
        speed = (real)sqrt((double)(velocity->k * velocity->k + velocity->j * velocity->j +
            velocity->i * velocity->i));
        if (!(speed > 0.03f)) {
            continue;
        }
        scaled = (speed - 0.03f) * 4.5454545f;
        depth = *(real *)(contact + 0x74) - *(real *)(node + 0x68) + 0.003f;
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
        halo::effects::material_effects_play_at_marker(*(uint32_t *)(tag + 0x3dc), (int16_t)(9 + (*(uint32_t *)(node + 0x24) & 1)),
            *(int16_t *)(contact + 0x70), (uint32_t *)&((struct object *)obj)->location_leaf_index, intensity_bits, &position, &offset);
    }
}

}
