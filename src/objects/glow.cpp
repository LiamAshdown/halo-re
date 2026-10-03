#include "halo/objects/glow.hpp"
#include "halo/tags/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/core/lcg.hpp"
#include "halo/bitmaps/api.hpp"
#include "game.h"
#include "rasterizer.h"
#include "render.h"
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/render/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern int32_t __ftol(double);
extern void build_sprite(build_sprite_data *data, int16_t sequence_index, int16_t sprite_index, int16_t mode, real_point3d *origin, real_vector3d *direction, float rotation, float scale, ColorARGB *color, float fade, uint32_t flags);
extern void build_sprites_end(build_sprite_data *data);
extern double cos(double x);
extern game_time_globals *game_time;
extern real_point3d *global_zero_vector3d_pointer;
extern data_array *glow_data;
extern data_array *glow_particle_data;
extern uint8_t glow_sprite_shader[];
extern data_array *object_data;
extern int8_t object_function_get_value(void);
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern double sin(double x);
extern double sqrt(double x);
}

/**
 * Calls halo::math::vector3d_cubic_interpolate with the seven arguments the glow code was reversed with (output,
 * control points, four knots, parameter); the function takes ten, so p1..p3 are whatever the caller left behind.
 * Unresolved: the glow particle code still has to be reversed to name the real control points.
 */
static void vector3d_cubic_interpolate_unresolved(real_point3d *out, real_point3d *control_points, float t0, float t1, float t2, float t3, float t)
{
    using call_t = void (*)(real_point3d *, real_point3d *, float, float, float, float, float);
    reinterpret_cast<call_t>(&halo::math::vector3d_cubic_interpolate)(out, control_points, t0, t1, t2, t3, t);
}

/**
 * Creates the glow and glow particle data arrays.
 *
 * Original register convention: none.
 *
 * @address 0x004fcbb0
 */
void halo::objects::GlowSystem::initialize()
{
    if (glow_data != 0) {
        return;
    }
    glow_data = halo::saved_games::game_state_new((char *)"glow", 8, 0x25c);
    if (glow_data != 0 && glow_particle_data == 0) {
        glow_particle_data = halo::saved_games::game_state_new((char *)"glow particles", 0x200, 0x64);
    }
}

/**
 * Disposes the glow data arrays.
 *
 * Original register convention: none.
 *
 * @address 0x004fcc00
 */
void halo::objects::GlowSystem::dispose()
{
    if (glow_data != 0) {
        glow_data->valid = 1;
        halo::memory::data_delete_all(glow_data);
    }
    if (glow_particle_data != 0) {
        glow_particle_data->valid = 1;
        halo::memory::data_delete_all(glow_particle_data);
    }
}

/**
 * Clears the disposing flag of the glow data arrays after a dispose pass.
 *
 * Original register convention: none.
 *
 * @address 0x004fcc30
 */
void halo::objects::GlowSystem::clear_disposing_flag()
{
    if (glow_data != 0) {
        glow_data->valid = 0;
    }
    if (glow_particle_data != 0) {
        glow_particle_data->valid = 0;
    }
}

namespace {
static void *datum_try_get(data_array *array, datum_index index)
{
    int16_t absolute = (int16_t)index;
    int16_t salt;

    if (absolute < 0 || absolute >= array->last_index) {
        return 0;
    }
    salt = *(int16_t *)((uint8_t *)array->data + absolute * array->size);
    if (salt == 0 || ((int16_t)(index >> 16) != 0 && (int16_t)(index >> 16) != salt)) {
        return 0;
    }
    return (uint8_t *)array->data + absolute * array->size;
}
}

/**
 * Allocates a glow datum for a Glow tag, spawns its particles and returns its handle.
 *
 * Original register convention: stack -> glow_tag (cdecl).
 *
 * @address 0x004fcc50
 */
datum_index halo::objects::GlowSystem::create(datum_index glow_tag)
{
    datum_index index;
    uint8_t *self;
    uint8_t *tag;
    datum_index bitmap_tag;
    uint8_t *bitmap;

    if (glow_tag == k_datum_index_none) {
        return k_datum_index_none;
    }
    index = halo::memory::datum_new(glow_data);
    if (index == k_datum_index_none) {
        return index;
    }
    self = (uint8_t *)datum_try_get(glow_data, index);
    tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(glow_tag)].data;
    bitmap_tag = *(datum_index *)(tag + 0x150);
    bitmap = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(bitmap_tag)].data;
    if (*(int16_t *)bitmap == 3) {
        uint8_t *sequence = *(uint8_t **)(bitmap + 0x58);
        uint8_t *sprite = *(uint8_t **)(sequence + 0x38);
        uint8_t *bitmap_data = reinterpret_cast<uint8_t *>(halo::bitmaps::bitmap_group_sequence_get_bitmap_data(bitmap_tag, *(int16_t *)sprite, 0));

        *(datum_index *)(self + 0x224) = glow_tag;
        *(int16_t *)(self + 0x24c) = *(int16_t *)(tag + 0x20);
        *(int16_t *)(self + 0x228) = (int16_t)(int32_t)(((double)*(float *)(sprite + 0xc) - *(float *)(sprite + 8)) *
            (double)*(int16_t *)(bitmap_data + 4));
    }
    return index;
}

/**
 * Frees a glow datum and its particles.
 *
 * Original register convention: stack -> glow_index (cdecl).
 *
 * @address 0x004fcd40
 */
void halo::objects::GlowSystem::destroy(datum_index glow_index)
{
    glow *self = (glow *)datum_try_get(glow_data, glow_index);
    uint8_t *particle = (uint8_t *)self->first_particle;

    while (particle != 0) {
        uint8_t *next = *(uint8_t **)(particle + 0x5c);

        halo::memory::datum_delete(glow_particle_data, *(datum_index *)(particle + 4));
        particle = next;
    }
    halo::memory::datum_delete(glow_data, glow_index);
}

/**
 * Widget render hook for a glow: resolves the owning object and forwards to the glow renderer.
 *
 * @address 0x004fcdb0
 */
void halo::objects::GlowSystem::render_dispatch(uint32_t object_index, datum_index glow_handle)
{
    glow *entry;

    if (object_index == k_datum_index_none || glow_handle == k_datum_index_none) {
        return;
    }

    {
        int16_t index = (int16_t)glow_handle;
        int16_t salt = (int16_t)(glow_handle >> 16);

        entry = 0;
        if (index >= 0 && index < glow_data->last_index) {
            entry = (glow *)((uint8_t *)glow_data->data + glow_data->size * index);
            if (entry->identifier == 0 || (salt != 0 && salt != entry->identifier)) {
                entry = 0;
            }
        }
    }

    {
        void *glow_tag_data = halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;

        object_marker marker;

        halo::objects::glow_update(object_index, entry);
        halo::objects::object_get_node_local_transform(object_index, (char *)glow_tag_data, &marker, 1);
        halo::objects::glow_render(glow_handle);
    }
}

/**
 * Per-tick update of a glow instance attached to an object.
 *
 * Original register convention: object_index is this function's one Ghidra-recognized parameter; the glow instance
 * pointer arrives in EDI, confirmed by disassembling glow_render_dispatch.c's call site (`mov edi,eax; call
 * 0x4fce80`).
 *
 * @address 0x004fce80
 */
void halo::objects::GlowView::update(uint32_t object_index)
{
    glow *entry = self;
    void *glow_tag_data = halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;

    if (glow_tag_data == 0) {
        return;
    }

    {
        int16_t marker_count = (int16_t)halo::objects::object_get_node_local_transform(
            object_index, (char *)glow_tag_data, (object_marker *)((uint8_t *)entry + 8), 5);
        entry->marker_count = marker_count;

        if (entry->disabled == 0) {
            if (marker_count > 1) {

                int16_t nearest[5];
                int16_t i;

                for (i = 0; i < marker_count; i++) {
                    object_marker *mi = (object_marker *)((uint8_t *)entry + 8 + i * 0x6c);
                    int16_t best = -1;
                    float best_score = 0.0f;
                    int16_t j;

                    for (j = 0; j < marker_count; j++) {
                        if (i != j) {
                            object_marker *mj = (object_marker *)((uint8_t *)entry + 8 + j * 0x6c);
                            real_vector3d d;
                            float score;

                            d.i = mj->node_transform.position.x - mi->node_transform.position.x;
                            d.j = mj->node_transform.position.y - mi->node_transform.position.y;
                            d.k = mj->node_transform.position.z - mi->node_transform.position.z;
                            halo::math::vector3d_normalize_with_length(d);

                            score = d.i + d.j + d.k;
                            if (best_score < score) {
                                best_score = score;
                                best = j;
                            }
                        }
                    }
                    nearest[i] = best;
                }

                {
                    int16_t start = marker_count - 1;
                    if (start >= 0) {
                        int16_t out = start;
                        int16_t cursor = -1;
                        uint16_t remaining = (uint16_t)marker_count;

                        do {
                            int16_t k = marker_count;
                            do {
                                k = k - 1;
                                if (k < 0) break;
                            } while (nearest[k] != cursor);
                            entry->marker_order[out] = k;
                            out = out - 1;
                            remaining = remaining - 1;
                            cursor = k;
                        } while (remaining != 0);
                    }
                }

                entry->total_length = 0.0f;
                entry->cumulative_length[0] = 0.0f;

                if (marker_count != 1 && marker_count - 1 >= 0) {
                    int32_t idx = 0;
                    do {
                        object_marker *a = (object_marker *)((uint8_t *)entry + 8 +
                            entry->marker_order[idx] * 0x6c);
                        object_marker *b = (object_marker *)((uint8_t *)entry + 8 +
                            entry->marker_order[idx + 1] * 0x6c);
                        float dx = b->node_transform.position.x - a->node_transform.position.x;
                        float dy = b->node_transform.position.y - a->node_transform.position.y;
                        float dz = b->node_transform.position.z - a->node_transform.position.z;
                        float length = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) + entry->total_length;

                        entry->total_length = length;
                        entry->cumulative_length[idx] = length;
                        idx = idx + 1;
                    } while (idx < marker_count - 1);
                }

                halo::objects::glow_chain_build(entry);
                *(int16_t *)((uint8_t *)entry + 600) = 0;

                entry->disabled = 1;
                return;
            }
        } else if (marker_count > 1) {

            float rotation_rate = *(float *)((uint8_t *)glow_tag_data + 100);
            float translation_rate;
            float driver = 0.0f;

            if (((struct Glow *)glow_tag_data)->attachment_1 != -1) {
                int8_t ok = halo::objects::object_function_get_value(object_index, (int16_t)((struct Glow *)glow_tag_data)->attachment_1, &driver);
                driver = ok ? driver : 0.0f;
                rotation_rate = ((((struct Glow *)glow_tag_data)->effect_rot_vel_mul_high -
                                  ((struct Glow *)glow_tag_data)->effect_rot_vel_mul_low) * driver +
                                 ((struct Glow *)glow_tag_data)->effect_rot_vel_mul_low);
            }
            translation_rate = ((struct Glow *)glow_tag_data)->effect_translational_velocity;
            if (((struct Glow *)glow_tag_data)->attachment_2 != -1) {
                int8_t ok = halo::objects::object_function_get_value(object_index, (int16_t)((struct Glow *)glow_tag_data)->attachment_2, &driver);
                if (!ok) driver = 0.0f;
                translation_rate = ((((struct Glow *)glow_tag_data)->effect_trans_vel_mul_high -
                                     ((struct Glow *)glow_tag_data)->effect_trans_vel_mul_low) * driver +
                                    ((struct Glow *)glow_tag_data)->effect_trans_vel_mul_low) * translation_rate;
            }
            driver = rotation_rate / translation_rate;
            (void)driver;
        }
    }

    *(int16_t *)((uint8_t *)entry + 600) = (int16_t)(*(int16_t *)((uint8_t *)entry + 600) +
        game_time->ticks_this_frame);

    if (entry->marker_count > 1) {
        glow_particle *p;

        for (p = entry->first_particle; p != 0; p = *(glow_particle **)&((struct glow_particle *)p)->next) {
            if (((uint8_t)p->flags & 2) == 0) {

                halo::objects::glow_particle_advance_time(object_index, entry, (uint8_t *)p,
                                            halo::render::globals().time_since_frame * *(float *)((uint8_t *)p));
                halo::objects::glow_particle_compute_position(object_index, entry, p);
                *(uint32_t *)((uint8_t *)p + 0x24) = *(uint32_t *)((uint8_t *)p + 0x20);
            }
        }

        for (p = entry->first_particle; p != 0; ) {
            glow_particle *next = *(glow_particle **)&((struct glow_particle *)p)->next;

            if (((uint8_t)p->flags & 2) != 0) {
                int16_t *age = (int16_t *)&p->age;
                int16_t *lifetime = (int16_t *)&p->lifetime;

                *age = (int16_t)(*age + game_time->ticks_this_frame);
                halo::objects::glow_particle_compute_fade(entry, p);

                if (test_flag(((struct Glow *)glow_tag_data)->glow_flags, tags::glow_tag_flag::trailing_particles_shrink_over_time)) {
                    float fraction = 1.0f - (float)*age / (float)*lifetime;
                    if (fraction < 0.0f) fraction = 0.0f;
                    *(float *)((uint8_t *)p + 0x24) = fraction * *(float *)((uint8_t *)p + 0x20);
                }
                halo::objects::glow_particle_compute_color(entry, p);

                *(float *)((uint8_t *)p + 0x2c) += halo::render::globals().time_since_frame * p->render_color[0];
                *(float *)((uint8_t *)p + 0x30) += halo::render::globals().time_since_frame * p->render_color[1];
                *(float *)((uint8_t *)p + 0x34) += halo::render::globals().time_since_frame * p->render_color[2];

                if (*lifetime < *age) {
                    glow_particle *prev = *(glow_particle **)&((struct glow_particle *)p)->previous;
                    if (next == 0) {
                        entry->last_particle = prev;
                    } else {
                        *(glow_particle **)&((struct glow_particle *)next)->previous = prev;
                    }
                    if (prev == 0) {
                        entry->first_particle = next;
                    } else {
                        *(glow_particle **)&((struct glow_particle *)prev)->next = next;
                    }
                    halo::memory::datum_delete(glow_particle_data, ((struct glow_particle *)p)->handle);
                    entry->spawn_count = entry->spawn_count - 1;
                }
            }
            p = next;
        }
    }

    if (((struct Glow *)glow_tag_data)->particle_generation_freq > 0.01f &&
        game_time->ticks_this_frame != 0) {
        float threshold = 30.0f / ((struct Glow *)glow_tag_data)->particle_generation_freq;
        int16_t timer;

        if (threshold < 1.0f) threshold = 1.0f;
        timer = *(int16_t *)((uint8_t *)entry + 600);

        while (threshold < (float)timer) {
            glow_particle *spawned = halo::objects::glow_particle_spawn(entry);
            if (spawned == 0) break;

            entry->spawn_count = entry->spawn_count + 1;
            if (entry->last_particle == 0) {
                entry->first_particle = spawned;
            } else {
                *(glow_particle **)((uint8_t *)entry->last_particle + 0x5c) = spawned;
                *(glow_particle **)&((struct glow_particle *)spawned)->previous = entry->last_particle;
            }
            entry->last_particle = spawned;

            timer = (int16_t)(timer - __ftol((double)threshold));
            *(int16_t *)((uint8_t *)entry + 600) = timer;
        }
    }
}

/**
 * Computes the fade factor of a glow particle from its phase.
 *
 * @address 0x004fd3a0
 */
void halo::objects::GlowParticleView::compute_fade(glow *entry)
{
    glow_particle *particle = self;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;

    if (!test_flag(((struct Glow *)tag)->glow_flags, tags::glow_tag_flag::trailing_particles_fade_over_time)) {
        particle->fade = 1.0f;
        return;
    }

    {
        float fade = 1.0f - (float)particle->age / (float)particle->lifetime;
        if (fade < 0.0f) {
            particle->fade = 0.0f;
            return;
        }
        if (fade > 1.0f) {
            fade = 1.0f;
        }
        particle->fade = fade;
    }
}

/**
 * Computes the colour of a glow particle from the glow's tag settings and its current phase.
 *
 * Original register convention: same EAX=entry/ECX=particle shape as glow_particle_compute_fade.c (0x4fd3a0), which
 * this function is the direct color-fade companion of.
 *
 * @address 0x004fd420
 */
void halo::objects::GlowParticleView::compute_color(glow *entry)
{
    glow_particle *particle = self;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;

    if (test_flag(((struct Glow *)tag)->glow_flags, tags::glow_tag_flag::trailing_particles_slow_over_time)) {
        float fade = 1.0f - (float)particle->age / (float)particle->lifetime;
        if (fade < 0.0f) {
            fade = 0.0f;
        }
        particle->render_color[0] = fade * particle->base_color[0];
        particle->render_color[1] = fade * particle->base_color[1];
        particle->render_color[2] = fade * particle->base_color[2];
        return;
    }
    particle->render_color[0] = particle->base_color[0];
    particle->render_color[1] = particle->base_color[1];
    particle->render_color[2] = particle->base_color[2];
}

/**
 * Computes the world position of a glow particle from the owning object's marker and the particle's offset.
 *
 * @address 0x004fd4a0
 */
void halo::objects::GlowParticleView::compute_position(uint32_t object_index, glow *entry)
{
    glow_particle *particle = self;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;
    int16_t attachment = ((struct Glow *)tag)->attachment_5;
    uint8_t *p = (uint8_t *)particle;

    if (attachment != -1) {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        float driver = *(float *)((uint8_t *)obj + 0x134 + attachment * 4);

        if (((1 << (attachment & 0x1f)) & ((struct object *)obj)->function_valid_flags) == 0) {
            driver = 0.0f;
        }

        *(float *)(p + 0x10) = (((struct Glow *)tag)->color_bound_1.red - ((struct Glow *)tag)->color_bound_0.red) * driver + ((struct Glow *)tag)->color_bound_0.red;
        *(float *)(p + 0x14) = (((struct Glow *)tag)->color_bound_1.green - ((struct Glow *)tag)->color_bound_0.green) * driver + ((struct Glow *)tag)->color_bound_0.green;
        *(uint32_t *)(p + 0xc) = 0x3f800000;
        *(float *)(p + 0x18) = (((struct Glow *)tag)->color_bound_1.blue - ((struct Glow *)tag)->color_bound_0.blue) * driver + ((struct Glow *)tag)->color_bound_0.blue;
    }

    if (test_flag(((struct Glow *)tag)->glow_flags, tags::glow_tag_flag::modify_particle_color_in_range)) {
        float t = ((struct glow_particle *)p)->t;
        float rate = ((struct Glow *)tag)->color_rate_of_change;

        *(float *)(p + 0x10) = (((struct Glow *)tag)->color_bound_1.red - ((struct Glow *)tag)->color_bound_0.red) * rate * t + ((struct Glow *)tag)->color_bound_0.red;
        *(float *)(p + 0x14) = (((struct Glow *)tag)->color_bound_1.green - ((struct Glow *)tag)->color_bound_0.green) * rate * t + ((struct Glow *)tag)->color_bound_0.green;
        *(uint32_t *)(p + 0xc) = 0x3f800000;
        *(float *)(p + 0x18) = (((struct Glow *)tag)->color_bound_1.blue - ((struct Glow *)tag)->color_bound_0.blue) * rate * t + ((struct Glow *)tag)->color_bound_0.blue;
    }

    {
        float t = ((struct glow_particle *)p)->t / entry->total_length;
        float half_span = ((struct Glow *)tag)->fading_percentage_of_glow * 0.5f;
        float fade;

        if (t >= half_span) {
            if (t <= 1.0f - half_span) {
                fade = 1.0f;
            } else {
                fade = (1.0f - t) / half_span;
            }
        } else {
            fade = t / half_span;
        }

        if (fade < 0.0f) {
            fade = 0.0f;
        } else if (fade > 1.0f) {
            fade = 1.0f;
        }
        ((struct glow_particle *)p)->fade = fade;
    }
}

/**
 * Advances one particle's animation phase by the given rate and respawns or repositions it as needed.
 *
 * @address 0x004fd650
 */
void halo::objects::GlowView::particle_advance_time(uint32_t object_index, uint8_t *particle, float rate)
{
    glow *entry = self;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;
    int16_t attachment = ((struct Glow *)tag)->attachment_3;
    int16_t loop_mode = ((struct Glow *)tag)->boundary_effect;
    uint32_t flags = *(uint32_t *)(particle + 0x54);
    float *t = (float *)(particle + 0x28);

    if (attachment != -1) {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        float driver = *(float *)((uint8_t *)obj + 0x134 + attachment * 4);

        if (((1 << (attachment & 0x1f)) & ((struct object *)obj)->function_valid_flags) == 0) {
            driver = 0.0f;
        }
        *(float *)(particle + 0x1c) =
            (((struct Glow *)tag)->max_distance_particle_to_object - ((struct Glow *)tag)->min_distance_particle_to_object) *
            ((((struct Glow *)tag)->distance_to_object_mul_high - ((struct Glow *)tag)->distance_to_object_mul_low) * driver + ((struct Glow *)tag)->distance_to_object_mul_low) +
            ((struct Glow *)tag)->min_distance_particle_to_object;
    }

    if ((flags & 1) == 0) {

        rate = rate + *t;
        *t = rate;

        if (loop_mode == 0) {
            if (entry->total_length < rate) {
                do {
                    *t = *t - entry->total_length;
                } while (entry->total_length < *t);
                *(uint32_t *)(particle + 0x54) = flags | 1;
                *t = entry->total_length - *t;
                halo::objects::glow_particle_reposition(entry, particle, rate);
                return;
            }
        } else if (loop_mode == 1 && entry->total_length < rate) {
            do {
                *t = *t - entry->total_length;
            } while (entry->total_length < *t);
            halo::objects::glow_particle_reposition(entry, particle, rate);
            return;
        }
    } else {

        rate = *t - rate;
        *t = rate;

        if (loop_mode == 0) {
            if (rate < 0.0f) {
                float wrapped;
                do {
                    wrapped = entry->total_length + *t;
                    *t = wrapped;
                } while (*t < 0.0f);
                *(uint32_t *)(particle + 0x54) = flags & ~1U;
                *t = entry->total_length - wrapped;
            }
        } else if (loop_mode == 1 && rate < 0.0f) {
            do {
                *t = entry->total_length + *t;
            } while (*t < 0.0f);
            halo::objects::glow_particle_reposition(entry, particle, rate);
            return;
        }
    }

    halo::objects::glow_particle_reposition(entry, particle, rate);
}

/**
 * Links the particles of a glow instance into its render chain.
 *
 * Original register convention: Ghidra shows a single `int in_EAX` with no other implicit inputs; by analogy with
 * glow_particle_compute_fade.c's EAX=entry convention, EAX is the glow entry.
 *
 * @address 0x004fd830
 */
void halo::objects::GlowView::chain_build()
{
    glow *entry = self;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;
    int32_t i = 0;
    int alternate = 1;
    glow_particle *prev = 0;

    for (i = 0; i < entry->spawn_count; i++) {
        glow_particle *p = halo::objects::glow_particle_new(entry, (int16_t)i, entry->spawn_count);
        if (p == 0) {
            return;
        }

        if (test_flag(((struct Glow *)tag)->glow_flags, tags::glow_tag_flag::particles_move_backwards)) {
            ((struct glow_particle *)p)->flags |= 1;
        }
        if (test_flag(((struct Glow *)tag)->glow_flags, tags::glow_tag_flag::partices_move_in_both_directions)) {
            uint32_t flags = ((struct glow_particle *)p)->flags;
            if (alternate) {
                flags &= ~1U;
            } else {
                flags |= 1;
            }
            alternate = !alternate;
            ((struct glow_particle *)p)->flags = flags;
        }

        if (entry->first_particle == 0) {
            entry->first_particle = p;
        }
        if (prev != 0) {
            *(glow_particle **)&((struct glow_particle *)prev)->next = p;
        }
        *(glow_particle **)&((struct glow_particle *)p)->previous = prev;
        entry->last_particle = p;
        prev = p;
    }
}

namespace {
static float glow_next_random_unit(void)
{
    halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
    return (float)(halo::math::globals().effect_random_seed >> halo::k_random_high_shift) * halo::k_unit_word_scale;
}
}

/**
 * Creates the particle at the given index of a glow with randomised parameters.
 *
 * Original register convention: Ghidra shows a clean (short param_1, short param_2) plus one unresolved `unaff_EDI`;
 * by the same reasoning as every other function in this file group, EDI is the glow entry.
 *
 * @address 0x004fd8e0
 */
glow_particle * halo::objects::GlowView::particle_new(int16_t index, int16_t count)
{
    glow *entry = self;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;
    glow_particle *p = halo::objects::glow_particle_datum_new();

    if (p != 0) {
        uint8_t *pb = (uint8_t *)p;

        if (((struct Glow *)tag)->attachment_3 == -1) {
            *(float *)(pb + 0x1c) = (((struct Glow *)tag)->max_distance_particle_to_object - ((struct Glow *)tag)->min_distance_particle_to_object) *
                                     glow_next_random_unit() + ((struct Glow *)tag)->min_distance_particle_to_object;
        }
        if (((struct Glow *)tag)->attachment_4 == -1) {
            float v = ((struct Glow *)tag)->particle_size_bounds[0] +
                      (((struct Glow *)tag)->particle_size_bounds[1] - ((struct Glow *)tag)->particle_size_bounds[0]) * glow_next_random_unit();
            *(float *)(pb + 0x20) = v / (float)entry->particle_count;
        }
        if (((struct Glow *)tag)->attachment_5 == -1 && !test_flag(((struct Glow *)tag)->glow_flags, tags::glow_tag_flag::modify_particle_color_in_range)) {
            float t = glow_next_random_unit();
            *(uint32_t *)(pb + 0xc) = 0x3f800000;
            *(float *)(pb + 0x10) = (((struct Glow *)tag)->color_bound_1.red - ((struct Glow *)tag)->color_bound_0.red) * t + ((struct Glow *)tag)->color_bound_0.red;
            *(float *)(pb + 0x14) = (((struct Glow *)tag)->color_bound_1.green - ((struct Glow *)tag)->color_bound_0.green) * t + ((struct Glow *)tag)->color_bound_0.green;
            *(float *)(pb + 0x18) = (((struct Glow *)tag)->color_bound_1.blue - ((struct Glow *)tag)->color_bound_0.blue) * t + ((struct Glow *)tag)->color_bound_0.blue;
        }
        if (((struct Glow *)tag)->normal_particle_distribution == 0) {
            ((struct glow_particle *)pb)->t = glow_next_random_unit() * entry->total_length;
            *(float *)(pb + 8) = glow_next_random_unit() * 6.2831855f;
        } else if (((struct Glow *)tag)->normal_particle_distribution == 1) {
            ((struct glow_particle *)pb)->t = ((float)index / (float)count) * entry->total_length;
            *(float *)(pb + 8) = glow_next_random_unit() * 6.2831855f;
        }
    }
    return p;
}

/**
 * Allocates and initialises a fresh particle for a glow instance.
 *
 * Original register convention: Ghidra shows a single unresolved `unaff_EBX`; by the same +0x224 anchor as every
 * sibling function, EBX is the glow entry.
 *
 * @address 0x004fdb20
 */
glow_particle * halo::objects::GlowView::particle_spawn()
{
    glow *entry = self;
    uint8_t *tag = (uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(entry->definition_tag)].data;
    glow_particle *p = halo::objects::glow_particle_datum_new();

    if (p != 0) {
        uint8_t *pb = (uint8_t *)p;
        float size;

        if (entry->marker_count < 2) {
            *(float *)(pb + 0x2c) = entry->markers[0].node_transform.position.x;
            *(float *)(pb + 0x30) = entry->markers[0].node_transform.position.y;
            *(float *)(pb + 0x34) = entry->markers[0].node_transform.position.z;
        } else {
            float lo = ((struct Glow *)tag)->trailing_particle_minimum_t * entry->total_length;
            float hi = ((struct Glow *)tag)->trailing_particle_maximum_t * entry->total_length;
            ((struct glow_particle *)pb)->t = (hi - lo) * glow_next_random_unit() + lo;
            halo::objects::glow_particle_reposition(entry, pb, 0.0f);
        }

        {
            int16_t velocity_mode = ((struct Glow *)tag)->trailing_particle_distribution;
            if (velocity_mode == 0) {
                ((struct glow_particle *)pb)->base_color[0] = 0.0f;
                ((struct glow_particle *)pb)->base_color[1] = 0.0f;
                ((struct glow_particle *)pb)->base_color[2] = 1.0f;
            } else if (velocity_mode == 1) {
                uint8_t *marker = (uint8_t *)entry + ((struct glow_particle *)pb)->unknown_02 * 0x6c + 0x5c;
                ((struct glow_particle *)pb)->base_color[0] = *(float *)marker;
                ((struct glow_particle *)pb)->base_color[1] = *(float *)(marker + 4);
                ((struct glow_particle *)pb)->base_color[2] = *(float *)(marker + 8);
            } else if (velocity_mode == 2) {
                real_vector3d v;
                v.i = glow_next_random_unit() * 2.0f - 1.0f;
                v.j = glow_next_random_unit() * 2.0f - 1.0f;
                v.k = glow_next_random_unit() * 2.0f - 1.0f;
                halo::math::vector3d_normalize_with_length(v);
                ((struct glow_particle *)pb)->base_color[0] = v.i;
                ((struct glow_particle *)pb)->base_color[1] = v.j;
                ((struct glow_particle *)pb)->base_color[2] = v.k;
            }
        }

        {
            float speed = ((struct Glow *)tag)->velocity_of_trailing_particles * 0.033333335f;
            ((struct glow_particle *)pb)->base_color[0] *= speed;
            ((struct glow_particle *)pb)->base_color[1] *= speed;
            ((struct glow_particle *)pb)->base_color[2] *= speed;
        }

        size = ((struct Glow *)tag)->particle_size_bounds[0] +
               (((struct Glow *)tag)->particle_size_bounds[1] - ((struct Glow *)tag)->particle_size_bounds[0]) * glow_next_random_unit();
        *(float *)(pb + 0x20) = size / (float)entry->particle_count;

        ((struct glow_particle *)pb)->lifetime = (int16_t)__ftol((double)*(float *)(pb + 0x20));

        {
            float t = glow_next_random_unit();
            *(uint32_t *)(pb + 0xc) = 0x3f800000;
            *(float *)(pb + 0x10) = (((struct Glow *)tag)->color_bound_1.red - ((struct Glow *)tag)->color_bound_0.red) * t + ((struct Glow *)tag)->color_bound_0.red;
            *(float *)(pb + 0x14) = (((struct Glow *)tag)->color_bound_1.green - ((struct Glow *)tag)->color_bound_0.green) * t + ((struct Glow *)tag)->color_bound_0.green;
            ((struct glow_particle *)pb)->flags = ((struct glow_particle *)pb)->flags | 2;
            *(float *)(pb + 0x18) = (((struct Glow *)tag)->color_bound_1.blue - ((struct Glow *)tag)->color_bound_0.blue) * t + ((struct Glow *)tag)->color_bound_0.blue;
        }
    }
    return p;
}

/**
 * Allocates a glow particle datum from the particle data array.
 *
 * Original register convention: none (no parameters); return value only.
 *
 * @address 0x004fdde0
 */
glow_particle * halo::objects::GlowSystem::particle_datum_new()
{
    datum_index handle = halo::memory::datum_new(glow_particle_data);
    glow_particle *entry = 0;

    if (handle != k_datum_index_none) {
        int16_t index = (int16_t)handle;

        if (index >= 0 && index < glow_particle_data->last_index) {
            entry = (glow_particle *)((uint8_t *)glow_particle_data->data +
                                       glow_particle_data->size * index);
            if (entry->identifier == 0 ||
                ((int16_t)(handle >> 16) != 0 && (int16_t)(handle >> 16) != entry->identifier)) {
                entry = 0;
            }
        }
    }

    if (entry != 0) {
        entry->handle = handle;
    } else {

    }
    return entry;
}

/**
 * Moves a particle to a new randomised location around its marker, advancing its phase by phase_rate.
 *
 * @address 0x004fde40
 */
void halo::objects::GlowView::particle_reposition(uint8_t *particle, float phase_rate)
{
    glow *entry = self;
    uint8_t *e = (uint8_t *)entry;
    float t = *(float *)(particle + 0x28);
    int16_t segment_count = *(int16_t *)(e + 4) - 1;
    int16_t seg = 0;
    int32_t idx;

    real_point3d c0[4], c1[4], c2[4];
    float t0 = 0.0f, t1 = 0.0f, t2 = 0.0f, t3 = 0.0f;
    real_point3d out1, out2;

    if (segment_count < 1) {
        idx = (seg > segment_count) ? segment_count : seg;
    } else {
        idx = 0;
        do {
            float lo = *(float *)(e + 0x238 + idx * 4);
            if (lo != t && lo < t && t < *(float *)(e + 0x23c + idx * 4)) break;
            seg = seg + 1;
            idx = seg;
        } while (idx < segment_count);
        if (seg < 0) idx = 0;
    }
    seg = (int16_t)idx;
    *(int16_t *)(particle + 2) = seg;

    {
        int16_t marker_count = *(int16_t *)(e + 4);

        if (marker_count == 2) {
            c0[0].x = ((struct glow *)e)->markers[0].node_transform.position.x; c0[0].y = ((struct glow *)e)->markers[0].node_transform.position.y; c0[0].z = ((struct glow *)e)->markers[0].node_transform.position.z;
            {
                float ex = ((struct glow *)e)->markers[1].node_transform.position.x, ey = ((struct glow *)e)->markers[1].node_transform.position.y, ez = ((struct glow *)e)->markers[1].node_transform.position.z;
                c1[0].x = ((struct glow *)e)->markers[0].node_transform.up.i; c1[0].y = ((struct glow *)e)->markers[0].node_transform.up.j; c1[0].z = *(float *)(e + 100);
                {
                    float fx = *(float *)(e + 200), fy = ((struct glow *)e)->markers[1].node_transform.up.j, fz = ((struct glow *)e)->markers[1].node_transform.up.k;
                    t0 = ((struct glow *)e)->cumulative_length[0]; t1 = ((struct glow *)e)->cumulative_length[1];

                    c0[1].x = (ex - c0[0].x) * 0.25f + c0[0].x;
                    c0[1].y = (ey - c0[0].y) * 0.25f + c0[0].y;
                    c0[1].z = (ez - c0[0].z) * 0.25f + c0[0].y;

                    c2[0].x = (ex - c0[0].x) * 0.75f + c0[0].x;
                    c2[0].y = (ey - c0[0].y) * 0.75f + c0[0].y;
                    c2[0].z = (ez - c0[0].z) * 0.75f + c0[0].y;

                    {
                        float a8 = fz - c1[0].z;
                        c1[1].x = (fx - c1[0].x) * 0.25f + c1[0].x;
                        c1[1].y = (fy - c1[0].y) * 0.25f + c1[0].y;
                        c1[1].z = a8 * 0.25f + c1[0].y;
                        c2[1].x = (fx - c1[0].x) * 0.75f + c1[0].x;
                        c2[1].y = (fy - c1[0].y) * 0.75f + c1[0].y;
                        c2[1].z = a8 * 0.75f + c1[0].y;
                    }
                    t2 = (t1 - t0) * 0.25f + t0;
                    t3 = (t1 - t0) * 0.75f + t0;
                }
            }
            goto evaluate;
        }

        if (marker_count != 3) {
            int16_t bound = marker_count - 1;
            int16_t s2 = 0;
            int32_t idx2;

            if (bound < 1) {
                idx2 = (s2 > bound) ? bound : s2;
            } else {
                idx2 = 0;
                do {
                    float lo = *(float *)(e + 0x238 + idx2 * 4);
                    float hi = *(float *)(e + 0x23c + idx2 * 4);
                    if (lo != t && lo < t && hi != t && t < hi) break;
                    s2 = s2 + 1;
                    idx2 = s2;
                } while (idx2 < bound);
                if (s2 < 0) idx2 = 0;
            }
            {
                int32_t hi_idx = idx2 + 1;
                int16_t lo16 = (int16_t)idx2;
                int32_t span = hi_idx - lo16;

                while (span + 1 < 4) {
                    if (lo16 > 0) lo16 = lo16 - 1;
                    if (hi_idx < bound) hi_idx = hi_idx + 1;
                    span = hi_idx - lo16;
                }
                idx2 = lo16;
            }

            t0 = *(float *)(e + 0x238 + idx2 * 4);
            t1 = *(float *)(e + 0x238 + idx2 * 4 + 4);
            t2 = *(float *)(e + 0x238 + idx2 * 4 + 8);
            t3 = *(float *)(e + 0x238 + idx2 * 4 + 0xc);

            {
                int16_t *order = (int16_t *)(e + 0x22a + idx2 * 2);
                int32_t byte_off = 0;
                int32_t k;

                for (k = 0; k < 4; k++) {
                    uint8_t *m = e + order[k] * 0x6c;
                    float fx = *(float *)(m + 0x60), fy = *(float *)(m + 0x4c);
                    float fz = *(float *)(m + 0x48), fw = *(float *)(m + 100);
                    float ux = *(float *)(m + 0x68), uy = *(float *)(m + 0x6c), uz = *(float *)(m + 0x70);
                    float lx = *(float *)(m + 0x5c), ly = *(float *)(m + 0x60), lzv = *(float *)(m + 100);
                    float b0 = fx * fy - fz * fw;

                    *(float *)((uint8_t *)c0 + byte_off) = ux;
                    *(float *)((uint8_t *)c0 + byte_off + 4) = uy;
                    *(float *)((uint8_t *)c0 + byte_off + 8) = uz;
                    *(float *)((uint8_t *)c1 + byte_off) = lx;
                    *(float *)((uint8_t *)c1 + byte_off + 4) = ly;
                    *(float *)((uint8_t *)c1 + byte_off + 8) = lzv;

                    {
                        float a2 = *(float *)(m + 0x44) * *(float *)(m + 100) -
                                   lx * *(float *)(m + 0x4c);
                        float a3 = *(float *)(m + 0x48) * lx - *(float *)(m + 0x44) * *(float *)(m + 0x60);
                        *(float *)((uint8_t *)c2 + byte_off) = b0;
                        *(float *)((uint8_t *)c2 + byte_off + 4) = a2;
                        *(float *)((uint8_t *)c2 + byte_off + 8) = a3;
                    }

                    byte_off += 0xc;
                }
            }
            goto evaluate;
        }

        c0[0].x = ((struct glow *)e)->markers[0].node_transform.position.x; c0[0].y = ((struct glow *)e)->markers[0].node_transform.position.y; c0[0].z = ((struct glow *)e)->markers[0].node_transform.position.z;
        {
            float ex = ((struct glow *)e)->markers[2].node_transform.position.x, ey = ((struct glow *)e)->markers[2].node_transform.position.y, ez = ((struct glow *)e)->markers[2].node_transform.position.z;
            c1[0].x = ((struct glow *)e)->markers[0].node_transform.up.i; c1[0].y = ((struct glow *)e)->markers[0].node_transform.up.j; c1[0].z = *(float *)(e + 100);
            {
                float fx = ((struct glow *)e)->markers[2].node_transform.up.i, fy = ((struct glow *)e)->markers[2].node_transform.up.j, fz = ((struct glow *)e)->markers[2].node_transform.up.k;
                t0 = ((struct glow *)e)->cumulative_length[0]; t1 = ((struct glow *)e)->cumulative_length[2];

                if (seg == 0) {
                    c0[1].x = ((struct glow *)e)->markers[1].node_transform.position.x; c0[1].y = ((struct glow *)e)->markers[1].node_transform.position.y; c0[1].z = ((struct glow *)e)->markers[1].node_transform.position.z;
                    c1[1].x = *(float *)(e + 200); c1[1].y = ((struct glow *)e)->markers[1].node_transform.up.j; c1[1].z = ((struct glow *)e)->markers[1].node_transform.up.k;
                    t2 = ((struct glow *)e)->cumulative_length[1];
                    c2[0].x = (ex - c0[1].x) * 0.5f + c0[1].x;
                    c2[0].y = (ey - c0[1].y) * 0.5f + c0[1].y;
                    c2[0].z = (ez - c0[1].z) * 0.5f + c0[1].y;
                    {
                        float a8 = fz - c1[1].z;
                        c2[1].x = (fx - c1[1].x) * 0.5f + c1[1].x;
                        c2[1].y = (fy - c1[1].y) * 0.5f + c1[1].y;
                        c2[1].z = a8 * 0.5f + c1[1].y;
                    }
                    t3 = (t1 - t2) * 0.5f + t2;
                    goto evaluate;
                }
                if (seg != 1) {
                    goto evaluate;
                }
                c2[0].x = ((struct glow *)e)->markers[1].node_transform.position.x; c2[0].y = ((struct glow *)e)->markers[1].node_transform.position.y; c2[0].z = ((struct glow *)e)->markers[1].node_transform.position.z;
                t2 = *(float *)(e + 200);
                c0[1].x = (c2[0].x - c0[0].x) * 0.5f + c0[0].x;
                c0[1].y = (c2[0].y - c0[0].y) * 0.5f + c0[0].y;
                c0[1].z = (c2[0].z - c0[0].z) * 0.5f + c0[0].y;
                {
                    float local_54 = ((struct glow *)e)->markers[1].node_transform.up.k;
                    float a8 = local_54 - c1[0].z;
                    c1[1].x = (t2 - c1[0].x) * 0.5f + c1[0].x;
                    c1[1].y = (((struct glow *)e)->markers[1].node_transform.up.j - c1[0].y) * 0.5f + c1[0].y;
                    c1[1].z = a8 * 0.5f + c1[0].y;
                }
                t3 = (((struct glow *)e)->cumulative_length[1] - t0) * 0.5f;
                t3 = t3 + t0;
            }
        }
    }

evaluate:

    vector3d_cubic_interpolate_unresolved((real_point3d *)(particle + 0x2c), c0, t0, t1, t2, t3, *(float *)(particle + 0x28));
    vector3d_cubic_interpolate_unresolved(&out1, c1, t0, t1, t2, t3, *(float *)(particle + 0x28));
    vector3d_cubic_interpolate_unresolved(&out2, c2, t0, t1, t2, t3, *(float *)(particle + 0x28));

    {
        double angle = (double)phase_rate * (double)(*(float *)(particle + 0x28)) +
                        (double)(*(float *)(particle + 8));
        double s = sin(angle);
        double c = cos(angle);
        float scale = *(float *)(particle + 0x1c);

        *(float *)(particle + 0x2c) = (float)((out1.x * s + out2.x * c) * scale) + *(float *)(particle + 0x2c);
        *(float *)(particle + 0x30) = (float)((out1.y * s + out2.y * c) * scale) + *(float *)(particle + 0x30);
        *(float *)(particle + 0x34) = (float)((out1.z * s + out2.z * c) * scale) + *(float *)(particle + 0x34);
    }
}

/**
 * Renders the glow identified by the handle.
 *
 * Original register convention: `in_ECX` unresolved by Ghidra; ECX -> glow_handle per the call site.
 *
 * @address 0x004fe570
 */
void halo::objects::GlowSystem::render(datum_index glow_handle)
{
    uint8_t *entry = 0;
    build_sprite_data data;
    uint8_t *particle;
    int16_t index = (int16_t)glow_handle;
    int16_t salt = (int16_t)(glow_handle >> 16);

    if (index >= 0 && index < glow_data->last_index) {
        uint8_t *candidate = (uint8_t *)glow_data->data + glow_data->size * index;

        if (*(int16_t *)candidate != 0 && (salt == 0 || salt == *(int16_t *)candidate)) {
            entry = candidate;
        }
    }

    if (entry == 0) {
        return;
    }
    memset(&data, 0, sizeof(data));
    data.bitmap_group_index = *(datum_index *)((uint8_t *)halo::cache::globals().tag_instances[halo::datum_slot(*(datum_index *)(entry + 0x224))].data + 0x150);
    data.maximum_sprite_count = *(int16_t *)(entry + 0x24c);
    data.shader = (uint32_t)glow_sprite_shader;
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;
    for (particle = *(uint8_t **)(entry + 0x250); particle != 0; particle = *(uint8_t **)(particle + 0x5c)) {
        halo::render::build_sprite(&data, 0, 0, 0, (real_point3d *)(particle + 0x2c),
                     (real_vector3d *)(entry + *(int16_t *)(particle + 0x2) * 0x6c + 0x44), 0.0f,
                     *(float *)(particle + 0x24), (ColorARGB *)(particle + 0xc), *(float *)(particle + 0x58), 0);
    }
    halo::render::build_sprites_end(&data);
}
