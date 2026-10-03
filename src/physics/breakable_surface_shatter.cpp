/**
 * Damage, reset and shattering of breakable collision surfaces.
 */

#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "tags.h"
#include "halo/bitmaps/api.hpp"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "game.h"
#include "sound.h"
#include "bitmaps.h"
#include "physics.h"
#include <string.h>

#include "halo/physics/breakable_surface.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/objects/record_access.hpp"
#include "halo/objects/api.hpp"

static auto &breakable_surfaces_enabled = halo::link::ref<uint8_t>(halo::physics::vars().breakable_surfaces_enabled);
static auto &global_structure_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_structure_collision_bsp);
static auto &global_structure_bsp = halo::link::ref<ScenarioStructureBSP *>(halo::ai::vars().global_structure_bsp);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
static auto &global_origin3d_pointer = halo::link::ref<const real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static_assert(offsetof(ScenarioStructureBSP, collision_materials.pointer) == 0xa8);
static_assert(offsetof(ScenarioStructureBSPCollisionMaterial, material) == 0x12);
static_assert(offsetof(GlobalsMaterial, particle_effects.count) == 0x2d4 + 0x48);
static_assert(offsetof(GlobalsMaterial, sound.tag_id) == 0x2d4 + 0x2c);
static_assert(sizeof(GlobalsMaterial) == 0x374);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, particle_type.tag_id) == 0xc);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, flags) == 0x10);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, density) == 0x14);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, velocity_scale) == 0x18);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, angular_velocity) == 0x24);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, radius) == 0x34);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, tint_lower_bound) == 0x44);
static_assert(offsetof(GlobalsBreakableSurfaceParticleEffect, tint_upper_bound) == 0x54);
static_assert(offsetof(DamageEffect, breaking_effect_forward_velocity) == 0x194);
static_assert(offsetof(DamageEffect, breaking_effect_outward_velocity) == 0x194 + 0x18);
static_assert(offsetof(damage_data, origin) == 0x28);
static_assert(offsetof(damage_data, direction) == 0x34);
static_assert(offsetof(particle_creation_data, color) == 0x4c);
static constexpr int k_breakable_queue_size = 1023;
static constexpr int k_breakable_polygon_size = 64;
static uint32_t effect_random_next(void)
{
    halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
    return halo::math::globals().effect_random_seed >> 16;
}

static float shatter_random_fraction(void)
{
    return (float)(int32_t)effect_random_next() * halo::k_unit_word_scale;
}

static int16_t shatter_grid_bound(float value, int round_up)
{
    double rounded;

    if (value < -1000.0f) {
        value = -1000.0f;
    } else if (!(value <= 1000.0f)) {
        value = 1000.0f;
    }
    rounded = round_up ? halo::libm::ceil((double)value) : halo::libm::floor((double)value);
    return (int16_t)(int32_t)(float)rounded;
}

static float shatter_falloff(float distance, float radius, float exponent)
{
    float t = 1.0f - distance / radius;

    if (t < 0.0f) {
        t = 0.0f;
    } else if (!(t <= 1.0f)) {
        t = 1.0f;
    }
    if (exponent != 0.0f) {
        t = (float)halo::libm::pow((double)t, (double)exponent);
    }
    return t;
}

namespace halo::physics {

/**
 * Implements breakable surface shatter.
 *
 * @address 0x500090
 */
void BreakableSurfaces::breakable_surface_shatter(uint16_t breakable_surface_index, damage_data *damage, int32_t collision_surface_index)
{
    ModelCollisionGeometryBSP *bsp = global_structure_collision_bsp;
    ModelCollisionGeometryBSPSurface *first_surface;
    GlobalsMaterial *shatter;
    int32_t queue[k_breakable_queue_size];
    int16_t queue_read = 0;
    int16_t queue_count = 1;
    uint8_t have_bounds = 0;
    real_point3d bounds_min;
    real_point3d bounds_max;

    if (!breakable_surfaces_enabled) {
        return;
    }
    first_surface = (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer + collision_surface_index;
    {
        ScenarioStructureBSPCollisionMaterial *collision_materials =
            (ScenarioStructureBSPCollisionMaterial *)global_structure_bsp->collision_materials.pointer;
        int16_t global_material = (int16_t)collision_materials[first_surface->material].material;

        shatter = (GlobalsMaterial *)global_globals->materials.pointer + global_material;
    }
    queue[0] = collision_surface_index;

    do {
        int32_t surface_index = queue[queue_read++];
        ModelCollisionGeometryBSPSurface *surfaces = (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
        ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
        ModelCollisionGeometryBSPVertex *vertices = (ModelCollisionGeometryBSPVertex *)bsp->vertices.pointer;
        int32_t first_edge = (int32_t)surfaces[surface_index].first_edge;
        int32_t plane_index = (int32_t)surfaces[surface_index].plane;
        float *stored_plane = (float *)&((ModelCollisionGeometryBSPPlane *)bsp->planes.pointer)[plane_index & halo::k_leaf_index_mask];
        float plane[4];
        int16_t axis;
        const projection_axis_pair *axes;
        int16_t u_axis;
        int16_t v_axis;
        int16_t vertex_count = 0;
        real_point2d polygon[k_breakable_polygon_size];
        real_point3d origin;
        real_vector3d e_axis;
        real_vector3d f_axis;
        float e_origin = 0.0f, f_origin = 0.0f;
        float e_min = 0.0f, e_max = 0.0f, f_min = 0.0f, f_max = 0.0f;
        int32_t edge_index = first_edge;
        int16_t entry;

        if (plane_index < 0) {
            plane[0] = -stored_plane[0];
            plane[1] = -stored_plane[1];
            plane[2] = -stored_plane[2];
            plane[3] = -stored_plane[3];
        } else {
            plane[0] = stored_plane[0];
            plane[1] = stored_plane[1];
            plane[2] = stored_plane[2];
            plane[3] = stored_plane[3];
        }

        {
            float ax = (float)halo::libm::fabs((double)plane[0]);
            float ay = (float)halo::libm::fabs((double)plane[1]);
            float az = (float)halo::libm::fabs((double)plane[2]);

            if (az < ay || az < ax) {
                axis = (int16_t)(ay < ax ? 0 : 1);
            } else {
                axis = 2;
            }
        }
        axes = &halo::math::globals().k_projection_axes[axis * 2 + (plane[axis] > 0.0f ? 1 : 0)];
        u_axis = axes->i;
        v_axis = axes->j;

        do {
            ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
            uint8_t on_right = (uint8_t)((int32_t)edge->right_surface == surface_index);
            float *a = (float *)&vertices[on_right ? edge->start_vertex : edge->end_vertex];
            int32_t neighbour = (int32_t)(on_right ? edge->left_surface : edge->right_surface);

            if (vertex_count == 0) {
                float *b = (float *)&vertices[on_right ? edge->end_vertex : edge->start_vertex];
                float length;

                if (surface_index == collision_surface_index) {
                    float *position = (float *)&damage->origin;
                    float pu = position[u_axis];
                    float pv = position[v_axis];

                    ((float *)&origin)[u_axis] = pu;
                    ((float *)&origin)[v_axis] = pv;
                    if (!(halo::libm::fabs((double)plane[axis]) < 0.0001)) {
                        ((float *)&origin)[axis] = ((plane[3] - pu * plane[u_axis]) - pv * plane[v_axis]) / plane[axis];
                    } else {
                        ((float *)&origin)[axis] = 0.0f;
                    }
                } else {
                    origin.x = a[0];
                    origin.y = a[1];
                    origin.z = a[2];
                }
                e_axis.i = b[0] - a[0];
                e_axis.j = b[1] - a[1];
                e_axis.k = b[2] - a[2];
                length = (float)halo::libm::sqrt((double)(e_axis.i * e_axis.i + e_axis.k * e_axis.k + e_axis.j * e_axis.j));
                if (!(halo::libm::fabs((double)length) < 0.0001)) {
                    float inverse = 1.0f / length;

                    e_axis.i *= inverse;
                    e_axis.j *= inverse;
                    e_axis.k *= inverse;
                }
                f_axis.i = e_axis.j * plane[2] - e_axis.k * plane[1];
                f_axis.j = e_axis.k * plane[0] - plane[2] * e_axis.i;
                f_axis.k = plane[1] * e_axis.i - e_axis.j * plane[0];
                e_origin = e_axis.k * origin.z + e_axis.j * origin.y + e_axis.i * origin.x;
                f_origin = f_axis.k * origin.z + f_axis.j * origin.y + f_axis.i * origin.x;
                e_max = e_min = (e_axis.k * a[2] + e_axis.i * a[0] + e_axis.j * a[1]) - e_origin;
                f_max = f_min = (f_axis.k * a[2] + f_axis.i * a[0] + f_axis.j * a[1]) - f_origin;
            } else {
                float e = (e_axis.k * a[2] + e_axis.i * a[0] + e_axis.j * a[1]) - e_origin;
                float f = (f_axis.k * a[2] + f_axis.i * a[0] + f_axis.j * a[1]) - f_origin;

                if (!(e > e_min)) {
                    e_min = e;
                }
                if (!(f > f_min)) {
                    f_min = f;
                }
                if (e > e_max) {
                    e_max = e;
                }
                if (f > f_max) {
                    f_max = f;
                }
            }
            if (vertex_count < k_breakable_polygon_size) {
                polygon[vertex_count].x = a[u_axis];
                polygon[vertex_count].y = a[v_axis];
            }
            if (have_bounds) {
                if (!(a[0] > bounds_min.x)) bounds_min.x = a[0];
                if (!(a[1] > bounds_min.y)) bounds_min.y = a[1];
                if (!(a[2] > bounds_min.z)) bounds_min.z = a[2];
                if (a[0] > bounds_max.x) bounds_max.x = a[0];
                if (a[1] > bounds_max.y) bounds_max.y = a[1];
                if (a[2] > bounds_max.z) bounds_max.z = a[2];
            } else {
                bounds_min.x = bounds_max.x = a[0];
                bounds_min.y = bounds_max.y = a[1];
                bounds_min.z = bounds_max.z = a[2];
                have_bounds = 1;
            }

            if (neighbour != -1) {
                int16_t i;

                for (i = 0; i < queue_count; i++) {
                    if (queue[i] == neighbour) {
                        neighbour = -1;
                        break;
                    }
                }
                if (neighbour != -1) {
                    ModelCollisionGeometryBSPSurface *other = &surfaces[neighbour];

                    if ((uint16_t)(uint8_t)other->breakable_surface == breakable_surface_index && other->material == first_surface->material &&
                        queue_count < k_breakable_queue_size) {
                        queue[queue_count++] = neighbour;
                    }
                }
            }
            edge_index = (int32_t)(on_right ? edge->reverse_edge : edge->forward_edge);
            vertex_count++;
        } while (edge_index != first_edge);

        for (entry = 0; entry < (int32_t)shatter->particle_effects.count; entry++) {
            GlobalsBreakableSurfaceParticleEffect *particles = (GlobalsBreakableSurfaceParticleEffect *)shatter->particle_effects.pointer + entry;
            float spacing = particles->density;
            int16_t i_min, i_max, j_min, j_max;
            int16_t j;

            if (halo::objects::tag_handle(particles->particle_type) == -1) {
                continue;
            }
            if (spacing == 0.0f) {
                if (surface_index != collision_surface_index) {
                    continue;
                }
                i_min = i_max = j_min = j_max = 0;
            } else {
                i_min = shatter_grid_bound(e_min / spacing, 1);
                j_min = shatter_grid_bound(f_min / spacing, 1);
                i_max = shatter_grid_bound(e_max / spacing, 0);
                j_max = shatter_grid_bound(f_max / spacing, 0);
                if (j_min > j_max) {
                    continue;
                }
            }

            for (j = j_min; j <= j_max; j++) {
                float j_real = (float)(int32_t)j;
                int16_t i;

                for (i = i_min; i <= i_max; i++) {
                    real_point3d point;
                    real_point2d projected;
                    float a = shatter_random_fraction() * 1.5f - 0.75f;
                    float b = shatter_random_fraction() * 1.5f - 0.75f;
                    float along;

                    point = origin;
                    along = ((float)(int32_t)i + a) * spacing;
                    point.x = e_axis.i * along + point.x;
                    point.y = e_axis.j * along + point.y;
                    point.z = e_axis.k * along + point.z;
                    along = (j_real + b) * spacing;
                    point.x = f_axis.i * along + point.x;
                    point.y = f_axis.j * along + point.y;
                    point.z = f_axis.k * along + point.z;
                    projected.x = ((float *)&point)[u_axis];
                    projected.y = ((float *)&point)[v_axis];
                    if (!halo::math::polygon2d_point_inside_margin(polygon, vertex_count, projected, 0.0f)) {
                        continue;
                    }

                    {
                        DamageEffect *damage_effect = (DamageEffect *)halo::cache::globals().tag_instances[damage->damage_effect_tag & halo::k_slot_mask].data;
                        real_vector3d velocity;
                        real_vector3d away;
                        float distance;
                        particle_creation_data creation;
                        float alpha;

                        velocity.i = global_origin3d_pointer->x;
                        velocity.j = global_origin3d_pointer->y;
                        velocity.k = global_origin3d_pointer->z;
                        away.i = point.x - damage->origin.x;
                        away.j = point.y - damage->origin.y;
                        away.k = point.z - damage->origin.z;
                        distance = (float)halo::libm::sqrt((double)(away.k * away.k + away.j * away.j + away.i * away.i));
                        if (!(halo::libm::fabs((double)distance) < 0.0001)) {
                            float inverse = 1.0f / distance;

                            away.i *= inverse;
                            away.j *= inverse;
                            away.k *= inverse;
                        } else {
                            distance = 0.0f;
                        }
                        if (damage_effect->breaking_effect_outward_radius > 0.0f) {
                            float t = shatter_falloff(distance, damage_effect->breaking_effect_outward_radius, damage_effect->breaking_effect_outward_exponent) *
                                damage_effect->breaking_effect_outward_velocity;

                            velocity.i = away.i * t + velocity.i;
                            velocity.j = away.j * t + velocity.j;
                            velocity.k = away.k * t + velocity.k;
                        }
                        if (damage_effect->breaking_effect_forward_radius > 0.0f) {
                            float t = shatter_falloff(distance, damage_effect->breaking_effect_forward_radius, damage_effect->breaking_effect_forward_exponent) *
                                damage_effect->breaking_effect_forward_velocity;

                            velocity.i = t * damage->direction.i + velocity.i;
                            velocity.j = t * damage->direction.j + velocity.j;
                            velocity.k = t * damage->direction.k + velocity.k;
                        }
                        if (particles->velocity_scale[1] > 0.0f) {
                            float scale = (particles->velocity_scale[1] - particles->velocity_scale[0]) * shatter_random_fraction() +
                                particles->velocity_scale[0];

                            velocity.i *= scale;
                            velocity.j *= scale;
                            velocity.k *= scale;
                        }

                        memset(&creation, 0, sizeof creation);
                        creation.definition_index = halo::objects::tag_handle(particles->particle_type);
                        creation.object_index = k_datum_index_none;
                        creation.marker_index = -1;
                        creation.first_person_weapon_index = 0xff;
                        creation.unknown_0b = 0xff;
                        creation.first_person = 0;
                        creation.third_person_only = 0;
                        creation.first_person_only = 0;
                        creation.position = point;
                        *(real_vector3d *)&creation.direction = velocity;
                        creation.velocity = velocity;
                        creation.gravity.i = global_origin3d_pointer->x;
                        creation.gravity.j = global_origin3d_pointer->y;
                        creation.gravity.k = global_origin3d_pointer->z;
                        creation.rotation = shatter_random_fraction() * 6.2831855f;
                        creation.angular_velocity = (particles->angular_velocity[1] - particles->angular_velocity[0]) * shatter_random_fraction() +
                            particles->angular_velocity[0];
                        creation.scale = (particles->radius[1] - particles->radius[0]) * shatter_random_fraction() +
                            particles->radius[0];
                        halo::bitmaps::color_interpolate((ColorRGB *)&particles->tint_upper_bound.red, (ColorRGB *)&particles->tint_lower_bound.red,
                            (ColorRGB *)&creation.color.red,
                            (color_interpolation_flags)(particles->flags & 3), shatter_random_fraction());
                        alpha = (particles->tint_upper_bound.alpha - particles->tint_lower_bound.alpha) * shatter_random_fraction() + particles->tint_lower_bound.alpha;
                        if (alpha < 0.0f) {
                            creation.color.alpha = 0.0f;
                        } else {
                            alpha = (particles->tint_upper_bound.alpha - particles->tint_lower_bound.alpha) * shatter_random_fraction() +
                                particles->tint_lower_bound.alpha;
                            if (!(alpha <= 1.0f)) {
                                creation.color.alpha = 1.0f;
                            } else {
                                creation.color.alpha = (particles->tint_upper_bound.alpha - particles->tint_lower_bound.alpha) * shatter_random_fraction() +
                                    particles->tint_lower_bound.alpha;
                            }
                        }

                        {
                            real_vector3d *direction = (real_vector3d *)&creation.direction;
                            float length = (float)halo::libm::sqrt((double)(direction->k * direction->k +
                                direction->j * direction->j + direction->i * direction->i));
                            uint8_t random_direction = 1;

                            if (!(halo::libm::fabs((double)length) < 0.0001)) {
                                float inverse = 1.0f / length;

                                direction->i *= inverse;
                                direction->j *= inverse;
                                direction->k *= inverse;
                                random_direction = (uint8_t)(length == 0.0f);
                            }
                            if (random_direction) {
                                int16_t index = (int16_t)((effect_random_next() * (uint32_t)(int32_t)halo::math::globals().sphere_point_table_count) >> 16);

                                *direction = *(real_vector3d *)&halo::math::globals().sphere_point_table[index];
                            }
                        }
                        halo::effects::particle_new(&creation);
                    }
                }
            }
        }
    } while (queue_read < queue_count);

    if (halo::objects::tag_handle(shatter->sound) != -1 && have_bounds) {
        sound_location location;

        memset(&location, 0, sizeof location);
        location.type = 1;
        location.scale = 1.0f;
        location.gain = 1.0f;
        location.position.x = (bounds_max.x + bounds_min.x) * 0.5f;
        location.position.y = (bounds_max.y + bounds_min.y) * 0.5f;
        location.position.z = (bounds_max.z + bounds_min.z) * 0.5f;
        location.forward.i = halo::math::globals().global_forward3d_pointer->i;
        location.forward.j = halo::math::globals().global_forward3d_pointer->j;
        location.forward.k = halo::math::globals().global_forward3d_pointer->k;
        location.velocity.i = global_origin3d_pointer->x;
        location.velocity.j = global_origin3d_pointer->y;
        location.velocity.k = global_origin3d_pointer->z;
        location.leaf_index = damage->location_leaf_index;
        location.cluster_index = damage->location_cluster_index;
        location.unknown_36 = damage->unknown_1a;
        halo::sound::sound_play_new(halo::objects::tag_handle(shatter->sound), &location, k_datum_index_none, 0, 0, 0, 0);
    }
}

}


