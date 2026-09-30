// breakable_surface_shatter  (Ghidra: FUN_00500090; was declared as physics_point_spawn_contact_effect by its two
//   callers with the address on a continuation line, so it never bound and breaking glass trapped)
// address 0x500090, size 4778 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x500090..0x501339 (DecompileAt skeleton checked instruction by instruction).
//   When the breakable_surfaces global (0x689470) is set: flood-fills the structure collision surfaces sharing the
//   hit surface's breakable index (+0x09) and collision material (+0x0a), starting at collision_surface_index. For
//   each one, walks its edge ring (vertex on this surface's side, neighbour queued once), projects the polygon on
//   the plane's two minor axes (k_projection_axes), and measures it along the first edge E and F = plane normal x E
//   from an origin (the damage point projected on the plane for the first surface, else the first vertex). For each
//   particle entry of the material's breakable-surface block (Globals material +0x2d4: +0x48 count, +0x4c 0x80-byte
//   entries) it lays a grid of `spacing` (+0x14) over that range (ceil/floor, clamped to +-1000; a zero spacing
//   makes one cell on the first surface only), jitters each cell by +-0.75, and spawns a particle for every point
//   inside the polygon: velocity from the damage effect's radial (+0x194 +0x18/+0x1c/+0x20) and directional
//   (+0x194 +0x0/+0x4/+0x8) terms scaled by a random [+0x18, +0x1c], random angle, +0x24/+0x28 and +0x34/+0x38
//   ranges, colour between +0x48 and +0x58 and alpha between +0x44 and +0x54. Finally plays the block's sound
//   (+0x2c) at the centre of the shattered area. All jitter uses the effect random seed (0x719cd4).
// blam-cc: stack -> breakable_surface_index, damage, collision_surface_index (cdecl)

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
#include "fn_effects.h"
#include "fn_physics.h"
#include <string.h>

extern uint8_t breakable_surfaces_enabled;                          // 0x00689470, the breakable_surfaces global
extern ModelCollisionGeometryBSP *global_structure_collision_bsp;   // 0x00746f98
extern uint8_t *global_structure_bsp;                           // 0x00746f9c
extern Globals *global_globals;
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern random_seed effect_random_seed;                              // 0x00719cd4
extern const projection_axis_pair k_projection_axes[6];             // 0x0065c29c
extern const real_vector3d *global_forward3d_pointer;               // 0x00696718
extern const real_point3d *global_origin3d_pointer;                 // 0x00696714
extern real_point3d *sphere_point_table;                            // 0x006b7af4
extern int16_t sphere_point_table_count;                            // 0x006b7af8


extern ColorRGB *color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest,
    color_interpolation_flags flags, float t); // 0x43f6a0, EAX, ECX, stack
extern uint8_t polygon2d_point_inside_margin(real_point2d *vertices, int16_t count, real_point2d *point,
    real margin); // 0x4cae60, ECX, stack
extern datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size,
    uint32_t first_person_hint); // 0x549af0
extern double sqrt(double x);
extern double fabs(double x);
extern double floor(double x); // 0x623e40
extern double ceil(double x);  // 0x6267f0
extern double pow(double base, double exponent); // 0x6283c0 _CIpow

#define F(p, o) (*(float *)((uint8_t *)(p) + (o)))
#define I32(p, o) (*(int32_t *)((uint8_t *)(p) + (o)))
#define I16(p, o) (*(int16_t *)((uint8_t *)(p) + (o)))

#define k_breakable_queue_size 1023
#define k_breakable_polygon_size 64 // the original kept 8 on its stack (no bound check)

static uint32_t effect_random_next(void)
{
    effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
    return effect_random_seed >> 16;
}

static float shatter_random_fraction(void)
{
    return (float)(int32_t)effect_random_next() * 1.5259022e-05f;
}

// 0x500862: clamp to [-1000, 1000] (NaN passes the upper test), then ceil or floor
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

// 0x500c9f: 1 - distance / radius clamped to [0, 1], raised to the exponent when it is not 0
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

void breakable_surface_shatter(uint16_t breakable_surface_index, damage_data *damage, int32_t collision_surface_index)
{
    ModelCollisionGeometryBSP *bsp = global_structure_collision_bsp; // esp+0xe8
    uint8_t *first_surface;                                            // esp+0xfc
    uint8_t *shatter;                                                  // esp+0x88
    int32_t queue[k_breakable_queue_size];                             // esp+0x258
    int16_t queue_read = 0;                                            // esp+0xe4
    int16_t queue_count = 1;                                           // esp+0x84
    uint8_t have_bounds = 0;                                           // esp+0x13
    real_point3d bounds_min;                                           // esp+0xac/0xb4/0xbc
    real_point3d bounds_max;                                           // esp+0xb0/0xb8/0xc0
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
        int32_t surface_index = queue[queue_read++];                 // esp+0xf0
        uint8_t *surfaces = (uint8_t *)bsp->surfaces.pointer;         // esp+0x11c
        uint8_t *edges = (uint8_t *)bsp->edges.pointer;               // esp+0x7c
        uint8_t *vertices = (uint8_t *)bsp->vertices.pointer;         // esp+0x80
        int32_t first_edge = I32(surfaces, surface_index * 0xc + 4);  // esp+0xc
        int32_t plane_index = I32(surfaces, surface_index * 0xc);
        float *stored_plane = (float *)((uint8_t *)bsp->planes.pointer + (plane_index & 0x7fffffff) * 0x10);
        float plane[4];                                                // esp+0x6c
        int16_t axis;                                                  // esp+0x44
        const projection_axis_pair *axes;                              // esp+0xf4 (byte offset)
        int16_t u_axis;                                                // esp+0x98
        int16_t v_axis;                                                // esp+0x4c
        int16_t vertex_count = 0;                                      // esp+0x5c
        real_point2d polygon[k_breakable_polygon_size];                // esp+0x1b8
        real_point3d origin;                                           // esp+0x8c
        real_vector3d e_axis;                                          // esp+0x9c
        real_vector3d f_axis;                                          // esp+0xc4
        float e_origin = 0.0f, f_origin = 0.0f;                        // esp+0xa8 / 0xd0
        float e_min = 0.0f, e_max = 0.0f, f_min = 0.0f, f_max = 0.0f;  // esp+0xd4/0xd8/0xdc/0xe0
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
        // 0x5001c3: the dominant axis (ties go to the later axis, as the fcom/C0 tests do)
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
            uint8_t *edge = edges + edge_index * 0x18;                 // esp+0xec
            uint8_t on_right = (uint8_t)(I32(edge, 0x14) == surface_index); // esp+0x1b
            float *a = (float *)(vertices + I32(edge, (on_right ? 0 : 1) * 4) * 0x10);
            int32_t neighbour = I32(edge, 0x10 + (on_right ? 0 : 1) * 4); // esp+0x118

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

            // 0x500765: queue the neighbour once when it is part of the same breakable surface and material
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
            int16_t i_min, i_max, j_min, j_max; // esp+0x52/0x56/0x50/0x54
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
                float j_real = (float)(int32_t)j; // esp+0xec
                int16_t i;

                for (i = i_min; i <= i_max; i++) {
                    real_point3d point;   // esp+0x38
                    real_point2d projected; // esp+0x184
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
                        real_vector3d velocity;  // esp+0x20
                        real_vector3d away;      // esp+0x60
                        float distance;          // esp+0x14
                        particle_creation_data creation; // esp+0x120
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
                        creation.unknown_0d = 0;
                        creation.unknown_0e = 0;
                        creation.position = point;
                        *(real_vector3d *)&creation.unknown_1c = velocity;
                        creation.velocity = velocity;
                        creation.gravity.i = global_origin3d_pointer->x;
                        creation.gravity.j = global_origin3d_pointer->y;
                        creation.gravity.k = global_origin3d_pointer->z;
                        creation.unknown_40 = shatter_random_fraction() * 6.2831855f;
                        creation.unknown_44 = (F(particles, 0x28) - F(particles, 0x24)) * shatter_random_fraction() +
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

                        // 0x5010aa: the stored direction (+0x1c) is normalized; a zero one takes a random table vector
                        {
                            real_vector3d *direction = (real_vector3d *)&creation.unknown_1c;
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
                        particle_new(&creation);
                    }
                }
            }
        }
    } while (queue_read < queue_count);

    // 0x501216: the shatter sound at the centre of the broken area
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
        sound_play_new(I32(shatter, 0x2c), &location, k_datum_index_none, 0, 0, 0, 0);
    }
}
