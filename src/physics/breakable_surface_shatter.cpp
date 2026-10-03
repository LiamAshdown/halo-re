/**
 * Damage, reset and shattering of breakable collision surfaces.
 */

#include "tags.h"
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
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"

extern "C" { extern uint8_t breakable_surfaces_enabled; }
extern "C" { extern ModelCollisionGeometryBSP *global_structure_collision_bsp; }
extern "C" { extern uint8_t *global_structure_bsp; }
extern "C" { extern Globals *global_globals; }
extern "C" { extern tag_instance *tag_instances; }
extern "C" { extern const projection_axis_pair k_projection_axes[6]; }
extern "C" { extern const real_vector3d *global_forward3d_pointer; }
extern "C" { extern const real_point3d *global_origin3d_pointer; }
extern "C" { extern real_point3d *sphere_point_table; }
extern "C" { extern int16_t sphere_point_table_count; }
extern "C" { extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t); }
extern "C" { extern uint8_t polygon2d_point_inside_margin(real_point2d *vertices, int16_t count, real_point2d *point, real margin); }
extern "C" { extern datum_index halo::sound::sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index, sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint); }
extern "C" { extern double sqrt(double x); }
extern "C" { extern double fabs(double x); }
extern "C" { extern double floor(double x); }
extern "C" { extern double ceil(double x); }
extern "C" { extern double pow(double base, double exponent); }
#define F(p, o) (*(float *)((uint8_t *)(p) + (o)))
#define I32(p, o) (*(int32_t *)((uint8_t *)(p) + (o)))
#define I16(p, o) (*(int16_t *)((uint8_t *)(p) + (o)))
#define k_breakable_queue_size 1023
#define k_breakable_polygon_size 64
static uint32_t effect_random_next(void)
{
    halo::effects::globals().effect_random_seed = halo::effects::globals().effect_random_seed * 0x19660d + 0x3c6ef35f;
    return halo::effects::globals().effect_random_seed >> 16;
}

static float shatter_random_fraction(void)
{
    return (float)(int32_t)effect_random_next() * 1.5259022e-05f;
}

static int16_t shatter_grid_bound(float value, int round_up)
{
    double rounded;

    if (value < -1000.0f) {
        value = -1000.0f;
    } else if (!(value <= 1000.0f)) {
        value = 1000.0f;
    }
    rounded = round_up ? ceil((double)value) : floor((double)value);
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
        t = (float)pow((double)t, (double)exponent);
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
    uint8_t *first_surface;
    uint8_t *shatter;
    int32_t queue[k_breakable_queue_size];
    int16_t queue_read = 0;
    int16_t queue_count = 1;
    uint8_t have_bounds = 0;
    real_point3d bounds_min;
    real_point3d bounds_max;
    uint8_t *damage_raw = (uint8_t *)damage;

    if (!breakable_surfaces_enabled) {
        return;
    }
    first_surface = (uint8_t *)bsp->surfaces.pointer + collision_surface_index * 0xc;
    {
        uint8_t *collision_materials = *(uint8_t **)(global_structure_bsp + 0xa8);
        int16_t global_material = I16(collision_materials, I16(first_surface, 0xa) * 0x14 + 0x12);

        shatter = (uint8_t *)global_globals->materials.pointer + global_material * 0x374 + 0x2d4;
    }
    queue[0] = collision_surface_index;

    do {
        int32_t surface_index = queue[queue_read++];
        uint8_t *surfaces = (uint8_t *)bsp->surfaces.pointer;
        uint8_t *edges = (uint8_t *)bsp->edges.pointer;
        uint8_t *vertices = (uint8_t *)bsp->vertices.pointer;
        int32_t first_edge = I32(surfaces, surface_index * 0xc + 4);
        int32_t plane_index = I32(surfaces, surface_index * 0xc);
        float *stored_plane = (float *)((uint8_t *)bsp->planes.pointer + (plane_index & 0x7fffffff) * 0x10);
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
            float ax = (float)fabs((double)plane[0]);
            float ay = (float)fabs((double)plane[1]);
            float az = (float)fabs((double)plane[2]);

            if (az < ay || az < ax) {
                axis = (int16_t)(ay < ax ? 0 : 1);
            } else {
                axis = 2;
            }
        }
        axes = &k_projection_axes[axis * 2 + (plane[axis] > 0.0f ? 1 : 0)];
        u_axis = axes->i;
        v_axis = axes->j;

        do {
            uint8_t *edge = edges + edge_index * 0x18;
            uint8_t on_right = (uint8_t)(I32(edge, 0x14) == surface_index);
            float *a = (float *)(vertices + I32(edge, (on_right ? 0 : 1) * 4) * 0x10);
            int32_t neighbour = I32(edge, 0x10 + (on_right ? 0 : 1) * 4);

            if (vertex_count == 0) {
                float *b = (float *)(vertices + I32(edge, (on_right ? 1 : 0) * 4) * 0x10);
                float length;

                if (surface_index == collision_surface_index) {
                    float *position = (float *)(damage_raw + 0x28);
                    float pu = position[u_axis];
                    float pv = position[v_axis];

                    ((float *)&origin)[u_axis] = pu;
                    ((float *)&origin)[v_axis] = pv;
                    if (!(fabs((double)plane[axis]) < 0.0001)) {
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
                length = (float)sqrt((double)(e_axis.i * e_axis.i + e_axis.k * e_axis.k + e_axis.j * e_axis.j));
                if (!(fabs((double)length) < 0.0001)) {
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
                    uint8_t *other = surfaces + neighbour * 0xc;

                    if ((uint16_t)other[0x9] == breakable_surface_index && I16(other, 0xa) == I16(first_surface, 0xa) &&
                        queue_count < k_breakable_queue_size) {
                        queue[queue_count++] = neighbour;
                    }
                }
            }
            edge_index = I32(edge, 0x8 + (on_right ? 1 : 0) * 4);
            vertex_count++;
        } while (edge_index != first_edge);

        for (entry = 0; entry < I32(shatter, 0x48); entry++) {
            uint8_t *particles = *(uint8_t **)(shatter + 0x4c) + entry * 0x80;
            float spacing = F(particles, 0x14);
            int16_t i_min, i_max, j_min, j_max;
            int16_t j;

            if (I32(particles, 0xc) == -1) {
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
                    if (!polygon2d_point_inside_margin(polygon, vertex_count, &projected, 0.0f)) {
                        continue;
                    }

                    {
                        uint8_t *damage_effect = (uint8_t *)tag_instances[*(uint32_t *)damage_raw & 0xffff].data + 0x194;
                        real_vector3d velocity;
                        real_vector3d away;
                        float distance;
                        particle_creation_data creation;
                        float alpha;

                        velocity.i = global_origin3d_pointer->x;
                        velocity.j = global_origin3d_pointer->y;
                        velocity.k = global_origin3d_pointer->z;
                        away.i = point.x - F(damage_raw, 0x28);
                        away.j = point.y - F(damage_raw, 0x2c);
                        away.k = point.z - F(damage_raw, 0x30);
                        distance = (float)sqrt((double)(away.k * away.k + away.j * away.j + away.i * away.i));
                        if (!(fabs((double)distance) < 0.0001)) {
                            float inverse = 1.0f / distance;

                            away.i *= inverse;
                            away.j *= inverse;
                            away.k *= inverse;
                        } else {
                            distance = 0.0f;
                        }
                        if (F(damage_effect, 0x1c) > 0.0f) {
                            float t = shatter_falloff(distance, F(damage_effect, 0x1c), F(damage_effect, 0x20)) *
                                F(damage_effect, 0x18);

                            velocity.i = away.i * t + velocity.i;
                            velocity.j = away.j * t + velocity.j;
                            velocity.k = away.k * t + velocity.k;
                        }
                        if (F(damage_effect, 0x4) > 0.0f) {
                            float t = shatter_falloff(distance, F(damage_effect, 0x4), F(damage_effect, 0x8)) *
                                F(damage_effect, 0x0);

                            velocity.i = t * F(damage_raw, 0x34) + velocity.i;
                            velocity.j = t * F(damage_raw, 0x38) + velocity.j;
                            velocity.k = t * F(damage_raw, 0x3c) + velocity.k;
                        }
                        if (F(particles, 0x1c) > 0.0f) {
                            float scale = (F(particles, 0x1c) - F(particles, 0x18)) * shatter_random_fraction() +
                                F(particles, 0x18);

                            velocity.i *= scale;
                            velocity.j *= scale;
                            velocity.k *= scale;
                        }

                        memset(&creation, 0, sizeof creation);
                        creation.definition_index = I32(particles, 0xc);
                        creation.object_index = k_datum_index_none;
                        creation.marker_index = -1;
                        *(int16_t *)((uint8_t *)&creation + 0xa) = -1;
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
                        creation.angular_velocity = (F(particles, 0x28) - F(particles, 0x24)) * shatter_random_fraction() +
                            F(particles, 0x24);
                        creation.scale = (F(particles, 0x38) - F(particles, 0x34)) * shatter_random_fraction() +
                            F(particles, 0x34);
                        color_interpolate((ColorRGB *)(particles + 0x58), (ColorRGB *)(particles + 0x48),
                            (ColorRGB *)((uint8_t *)&creation + 0x50),
                            (color_interpolation_flags)(I32(particles, 0x10) & 3), shatter_random_fraction());
                        alpha = (F(particles, 0x54) - F(particles, 0x44)) * shatter_random_fraction() + F(particles, 0x44);
                        if (alpha < 0.0f) {
                            F(&creation, 0x4c) = 0.0f;
                        } else {
                            alpha = (F(particles, 0x54) - F(particles, 0x44)) * shatter_random_fraction() +
                                F(particles, 0x44);
                            if (!(alpha <= 1.0f)) {
                                F(&creation, 0x4c) = 1.0f;
                            } else {
                                F(&creation, 0x4c) = (F(particles, 0x54) - F(particles, 0x44)) * shatter_random_fraction() +
                                    F(particles, 0x44);
                            }
                        }

                        {
                            real_vector3d *direction = (real_vector3d *)&creation.direction;
                            float length = (float)sqrt((double)(direction->k * direction->k +
                                direction->j * direction->j + direction->i * direction->i));
                            uint8_t random_direction = 1;

                            if (!(fabs((double)length) < 0.0001)) {
                                float inverse = 1.0f / length;

                                direction->i *= inverse;
                                direction->j *= inverse;
                                direction->k *= inverse;
                                random_direction = (uint8_t)(length == 0.0f);
                            }
                            if (random_direction) {
                                int16_t index = (int16_t)((effect_random_next() * (uint32_t)(int32_t)sphere_point_table_count) >> 16);

                                *direction = *(real_vector3d *)&sphere_point_table[index];
                            }
                        }
                        halo::effects::particle_new(&creation);
                    }
                }
            }
        }
    } while (queue_read < queue_count);

    if (I32(shatter, 0x2c) != -1 && have_bounds) {
        sound_location location;

        memset(&location, 0, sizeof location);
        location.type = 1;
        location.scale = 1.0f;
        location.gain = 1.0f;
        location.position.x = (bounds_max.x + bounds_min.x) * 0.5f;
        location.position.y = (bounds_max.y + bounds_min.y) * 0.5f;
        location.position.z = (bounds_max.z + bounds_min.z) * 0.5f;
        location.forward.i = global_forward3d_pointer->i;
        location.forward.j = global_forward3d_pointer->j;
        location.forward.k = global_forward3d_pointer->k;
        location.velocity.i = global_origin3d_pointer->x;
        location.velocity.j = global_origin3d_pointer->y;
        location.velocity.k = global_origin3d_pointer->z;
        *(int32_t *)((uint8_t *)&location + 0x30) = I32(damage_raw, 0x14);
        *(int32_t *)((uint8_t *)&location + 0x34) = I32(damage_raw, 0x18);
        halo::sound::sound_play_new(I32(shatter, 0x2c), &location, k_datum_index_none, 0, 0, 0, 0);
    }
}

}

#undef F
#undef I32
#undef I16
#undef k_breakable_queue_size
#undef k_breakable_polygon_size
