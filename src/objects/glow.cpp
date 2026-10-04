#include "halo/math/constants.hpp"
#include "halo/objects/record_access.hpp"
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
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/objects/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include "halo/units/api.hpp"

static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &glow_data = halo::link::ref<data_array *>(halo::objects::vars().glow_data);
static auto &glow_particle_data = halo::link::ref<data_array *>(halo::objects::vars().glow_particle_data);
static auto &glow_sprite_shader = halo::link::ref<uint8_t []>(halo::objects::vars().glow_sprite_shader);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);

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
    glow_data = halo::saved_games::game_state_new("glow", 8, 0x25c);
    if (glow_data != 0 && glow_particle_data == 0) {
        glow_particle_data = halo::saved_games::game_state_new("glow particles", 0x200, 0x64);
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
    glow *self;
    Glow *tag;
    datum_index bitmap_tag;
    Bitmap *bitmap;

    if (glow_tag == k_datum_index_none) {
        return k_datum_index_none;
    }
    index = halo::memory::datum_new(glow_data);
    if (index == k_datum_index_none) {
        return index;
    }
    self = reinterpret_cast<glow *>(datum_try_get(glow_data, index));
    tag = halo::objects::tag_as<Glow>(glow_tag);
    bitmap_tag = halo::objects::tag_handle(tag->texture);
    bitmap = halo::objects::tag_as<Bitmap>(bitmap_tag);
    if (*(int16_t *)bitmap == 3) {
        BitmapGroupSequence *sequence = halo::objects::block_elements<BitmapGroupSequence>(bitmap->bitmap_group_sequence);
        BitmapGroupSprite *sprite = halo::objects::block_elements<BitmapGroupSprite>(sequence->sprites);
        BitmapData *bitmap_data = reinterpret_cast<BitmapData *>(reinterpret_cast<uint8_t *>(halo::bitmaps::bitmap_group_sequence_get_bitmap_data(bitmap_tag, *(int16_t *)sprite, 0)));

        self->definition_tag = glow_tag;
        self->spawn_count = tag->number_of_particles;
        self->particle_count = (int16_t)(int32_t)(((double)sprite->right - sprite->left) *
            (double)bitmap_data->width);
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
    glow_particle *particle = reinterpret_cast<glow_particle *>(self->first_particle);

    while (particle != 0) {
        glow_particle *next = reinterpret_cast<glow_particle *>(particle->next);

        halo::memory::datum_delete(glow_particle_data, particle->handle);
        particle = reinterpret_cast<glow_particle *>(next);
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
        Glow *glow_tag_data = halo::objects::tag_as<Glow>(entry->definition_tag);

        object_marker marker;

        halo::objects::glow_update(object_index, entry);
        halo::objects::object_get_node_local_transform(object_index, glow_tag_data->attachment_marker.string, &marker, 1);
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
    Glow *glow_tag_data = halo::objects::tag_as<Glow>(entry->definition_tag);

    if (glow_tag_data == 0) {
        return;
    }

    {
        int16_t marker_count = (int16_t)halo::objects::object_get_node_local_transform(
            object_index, glow_tag_data->attachment_marker.string, entry->markers, 5);
        entry->marker_count = marker_count;

        if (entry->disabled == 0) {
            if (marker_count > 1) {

                int16_t nearest[5];
                int16_t i;

                for (i = 0; i < marker_count; i++) {
                    object_marker *mi = &entry->markers[i];
                    int16_t best = -1;
                    float best_score = 0.0f;
                    int16_t j;

                    for (j = 0; j < marker_count; j++) {
                        if (i != j) {
                            object_marker *mj = &entry->markers[j];
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
                        object_marker *a = &entry->markers[entry->marker_order[idx]];
                        object_marker *b = &entry->markers[entry->marker_order[idx + 1]];
                        float dx = b->node_transform.position.x - a->node_transform.position.x;
                        float dy = b->node_transform.position.y - a->node_transform.position.y;
                        float dz = b->node_transform.position.z - a->node_transform.position.z;
                        float length = (float)halo::libm::sqrt((double)(dx * dx + dy * dy + dz * dz)) + entry->total_length;

                        entry->total_length = length;
                        entry->cumulative_length[idx] = length;
                        idx = idx + 1;
                    } while (idx < marker_count - 1);
                }

                halo::objects::glow_chain_build(entry);
                entry->spawn_timer = 0;

                entry->disabled = 1;
                return;
            }
        } else if (marker_count > 1) {

            float rotation_rate = glow_tag_data->effect_rotational_velocity;
            float translation_rate;
            float driver = 0.0f;

            if (glow_tag_data->attachment_1 != -1) {
                int8_t ok = halo::objects::object_function_get_value(object_index, (int16_t)glow_tag_data->attachment_1, &driver);
                driver = ok ? driver : 0.0f;
                rotation_rate = ((glow_tag_data->effect_rot_vel_mul_high -
                                  glow_tag_data->effect_rot_vel_mul_low) * driver +
                                 glow_tag_data->effect_rot_vel_mul_low) * rotation_rate;
            }
            translation_rate = glow_tag_data->effect_translational_velocity;
            if (glow_tag_data->attachment_2 != -1) {
                int8_t ok = halo::objects::object_function_get_value(object_index, (int16_t)glow_tag_data->attachment_2, &driver);
                if (!ok) driver = 0.0f;
                translation_rate = ((glow_tag_data->effect_trans_vel_mul_high -
                                     glow_tag_data->effect_trans_vel_mul_low) * driver +
                                    glow_tag_data->effect_trans_vel_mul_low) * translation_rate;
            }
            driver = rotation_rate / translation_rate;
            (void)driver;
        }
    }

    entry->spawn_timer = (int16_t)(entry->spawn_timer +
        halo::game::globals().game_time->ticks_this_frame);

    if (entry->marker_count > 1) {
        glow_particle *p;

        for (p = entry->first_particle; p != 0; p = *(glow_particle **)&((struct glow_particle *)p)->next) {
            if (((uint8_t)p->flags & 2) == 0) {

                halo::objects::glow_particle_advance_time(object_index, entry, p,
                                            halo::render::globals().time_since_frame * halo::raw_at<float>(p, 0));
                halo::objects::glow_particle_compute_position(object_index, entry, p);
                p->scaled_rate = p->rate;
            }
        }

        for (p = entry->first_particle; p != 0; ) {
            glow_particle *next = *(glow_particle **)&((struct glow_particle *)p)->next;

            if (((uint8_t)p->flags & 2) != 0) {
                int16_t *age = (int16_t *)&p->age;
                int16_t *lifetime = (int16_t *)&p->lifetime;

                *age = (int16_t)(*age + halo::game::globals().game_time->ticks_this_frame);
                halo::objects::glow_particle_compute_fade(entry, p);

                if (test_flag(glow_tag_data->glow_flags, tags::glow_tag_flag::trailing_particles_shrink_over_time)) {
                    float fraction = 1.0f - (float)*age / (float)*lifetime;
                    if (fraction < 0.0f) fraction = 0.0f;
                    p->scaled_rate = fraction * p->rate;
                }
                halo::objects::glow_particle_compute_color(entry, p);

                p->position.x += halo::render::globals().time_since_frame * p->velocity[0];
                p->position.y += halo::render::globals().time_since_frame * p->velocity[1];
                p->position.z += halo::render::globals().time_since_frame * p->velocity[2];

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

    if (glow_tag_data->particle_generation_freq > 0.01f &&
        halo::game::globals().game_time->ticks_this_frame != 0) {
        float threshold = 30.0f / glow_tag_data->particle_generation_freq;
        int16_t timer;

        if (threshold < 1.0f) threshold = 1.0f;
        timer = entry->spawn_timer;

        while (threshold < (float)timer) {
            glow_particle *spawned = halo::objects::glow_particle_spawn(entry);
            if (spawned == 0) break;

            entry->spawn_count = entry->spawn_count + 1;
            if (entry->last_particle == 0) {
                entry->first_particle = spawned;
            } else {
                entry->last_particle->next = spawned;
                spawned->previous = entry->last_particle;
            }
            entry->last_particle = spawned;

            timer = (int16_t)(timer - halo::x87::__ftol((double)threshold));
            entry->spawn_timer = timer;
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
    Glow *tag = halo::objects::tag_as<Glow>(entry->definition_tag);

    if (!test_flag(tag->glow_flags, tags::glow_tag_flag::trailing_particles_fade_over_time)) {
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
    Glow *tag = halo::objects::tag_as<Glow>(entry->definition_tag);

    if (test_flag(tag->glow_flags, tags::glow_tag_flag::trailing_particles_slow_over_time)) {
        float fade = 1.0f - (float)particle->age / (float)particle->lifetime;
        if (fade < 0.0f) {
            fade = 0.0f;
        }
        particle->velocity[0] = fade * particle->base_velocity[0];
        particle->velocity[1] = fade * particle->base_velocity[1];
        particle->velocity[2] = fade * particle->base_velocity[2];
        return;
    }
    particle->velocity[0] = particle->base_velocity[0];
    particle->velocity[1] = particle->base_velocity[1];
    particle->velocity[2] = particle->base_velocity[2];
}

/**
 * Computes the world position of a glow particle from the owning object's marker and the particle's offset.
 *
 * @address 0x004fd4a0
 */
void halo::objects::GlowParticleView::compute_position(uint32_t object_index, glow *entry)
{
    glow_particle *particle = self;
    Glow *tag = halo::objects::tag_as<Glow>(entry->definition_tag);
    int16_t attachment = tag->attachment_5;
    glow_particle *p = reinterpret_cast<glow_particle *>(particle);

    if (attachment != -1) {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        float driver = obj->function_out_values[attachment];

        if (((1 << (attachment & 0x1f)) & ((struct object *)obj)->function_valid_flags) == 0) {
            driver = 0.0f;
        }

        p->color[0] = (tag->color_bound_1.red - tag->color_bound_0.red) * driver + tag->color_bound_0.red;
        p->color[1] = (tag->color_bound_1.green - tag->color_bound_0.green) * driver + tag->color_bound_0.green;
        p->alpha = 1.0f;
        p->color[2] = (tag->color_bound_1.blue - tag->color_bound_0.blue) * driver + tag->color_bound_0.blue;
    }

    if (test_flag(tag->glow_flags, tags::glow_tag_flag::modify_particle_color_in_range)) {
        float t = p->t;
        float rate = tag->color_rate_of_change;

        p->color[0] = (tag->color_bound_1.red - tag->color_bound_0.red) * rate * t + tag->color_bound_0.red;
        p->color[1] = (tag->color_bound_1.green - tag->color_bound_0.green) * rate * t + tag->color_bound_0.green;
        p->alpha = 1.0f;
        p->color[2] = (tag->color_bound_1.blue - tag->color_bound_0.blue) * rate * t + tag->color_bound_0.blue;
    }

    {
        float t = p->t / entry->total_length;
        float half_span = tag->fading_percentage_of_glow * 0.5f;
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
        p->fade = fade;
    }
}

/**
 * Advances one particle's animation phase by the given rate and respawns or repositions it as needed.
 *
 * @address 0x004fd650
 */
void halo::objects::GlowView::particle_advance_time(uint32_t object_index, glow_particle *particle, float rate)
{
    glow *entry = self;
    Glow *tag = halo::objects::tag_as<Glow>(entry->definition_tag);
    int16_t attachment = tag->attachment_3;
    int16_t loop_mode = tag->boundary_effect;
    uint32_t flags = particle->flags;
    float *t = (float *)(reinterpret_cast<uint8_t *>(particle) + 0x28);

    if (attachment != -1) {
        object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
        float driver = obj->function_out_values[attachment];

        if (((1 << (attachment & 0x1f)) & ((struct object *)obj)->function_valid_flags) == 0) {
            driver = 0.0f;
        }
        particle->distance =
            (tag->max_distance_particle_to_object - tag->min_distance_particle_to_object) *
            ((tag->distance_to_object_mul_high - tag->distance_to_object_mul_low) * driver + tag->distance_to_object_mul_low) +
            tag->min_distance_particle_to_object;
    }

    if ((flags & 1) == 0) {

        rate = rate + *t;
        *t = rate;

        if (loop_mode == 0) {
            if (entry->total_length < rate) {
                do {
                    *t = *t - entry->total_length;
                } while (entry->total_length < *t);
                particle->flags = flags | 1;
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
                particle->flags = flags & ~1U;
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
    Glow *tag = halo::objects::tag_as<Glow>(entry->definition_tag);
    int32_t i = 0;
    int alternate = 1;
    glow_particle *prev = 0;

    for (i = 0; i < entry->spawn_count; i++) {
        glow_particle *p = halo::objects::glow_particle_new(entry, (int16_t)i, entry->spawn_count);
        if (p == 0) {
            return;
        }

        if (test_flag(tag->glow_flags, tags::glow_tag_flag::particles_move_backwards)) {
            ((struct glow_particle *)p)->flags |= 1;
        }
        if (test_flag(tag->glow_flags, tags::glow_tag_flag::partices_move_in_both_directions)) {
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
    Glow *tag = halo::objects::tag_as<Glow>(entry->definition_tag);
    glow_particle *p = halo::objects::glow_particle_datum_new();

    if (p != 0) {
        glow_particle *pb = reinterpret_cast<glow_particle *>(p);

        if (tag->attachment_3 == -1) {
            pb->distance = (tag->max_distance_particle_to_object - tag->min_distance_particle_to_object) *
                                     glow_next_random_unit() + tag->min_distance_particle_to_object;
        }
        if (tag->attachment_4 == -1) {
            float v = tag->particle_size_bounds[0] +
                      (tag->particle_size_bounds[1] - tag->particle_size_bounds[0]) * glow_next_random_unit();
            pb->rate = v / (float)entry->particle_count;
        }
        if (tag->attachment_5 == -1 && !test_flag(tag->glow_flags, tags::glow_tag_flag::modify_particle_color_in_range)) {
            float t = glow_next_random_unit();
            pb->alpha = 1.0f;
            pb->color[0] = (tag->color_bound_1.red - tag->color_bound_0.red) * t + tag->color_bound_0.red;
            pb->color[1] = (tag->color_bound_1.green - tag->color_bound_0.green) * t + tag->color_bound_0.green;
            pb->color[2] = (tag->color_bound_1.blue - tag->color_bound_0.blue) * t + tag->color_bound_0.blue;
        }
        if (tag->normal_particle_distribution == 0) {
            pb->t = glow_next_random_unit() * entry->total_length;
            pb->angle = glow_next_random_unit() * halo::math::k_two_pi;
        } else if (tag->normal_particle_distribution == 1) {
            pb->t = ((float)index / (float)count) * entry->total_length;
            pb->angle = glow_next_random_unit() * halo::math::k_two_pi;
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
    Glow *tag = halo::objects::tag_as<Glow>(entry->definition_tag);
    glow_particle *p = halo::objects::glow_particle_datum_new();

    if (p != 0) {
        glow_particle *pb = reinterpret_cast<glow_particle *>(p);
        float size;

        if (entry->marker_count < 2) {
            pb->position.x = entry->markers[0].node_transform.position.x;
            pb->position.y = entry->markers[0].node_transform.position.y;
            pb->position.z = entry->markers[0].node_transform.position.z;
        } else {
            float lo = tag->trailing_particle_minimum_t * entry->total_length;
            float hi = tag->trailing_particle_maximum_t * entry->total_length;
            pb->t = (hi - lo) * glow_next_random_unit() + lo;
            halo::objects::glow_particle_reposition(entry, pb, 0.0f);
        }

        {
            int16_t velocity_mode = tag->trailing_particle_distribution;
            if (velocity_mode == 0) {
                pb->base_velocity[0] = 0.0f;
                pb->base_velocity[1] = 0.0f;
                pb->base_velocity[2] = 1.0f;
            } else if (velocity_mode == 1) {
                const real_vector3d &marker_up = entry->markers[pb->segment_index].node_transform.up;
                pb->base_velocity[0] = marker_up.i;
                pb->base_velocity[1] = marker_up.j;
                pb->base_velocity[2] = marker_up.k;
            } else if (velocity_mode == 2) {
                real_vector3d v;
                v.i = glow_next_random_unit() * 2.0f - 1.0f;
                v.j = glow_next_random_unit() * 2.0f - 1.0f;
                v.k = glow_next_random_unit() * 2.0f - 1.0f;
                halo::math::vector3d_normalize_with_length(v);
                pb->base_velocity[0] = v.i;
                pb->base_velocity[1] = v.j;
                pb->base_velocity[2] = v.k;
            }
        }

        {
            float speed = tag->velocity_of_trailing_particles * halo::math::k_seconds_per_tick;
            pb->base_velocity[0] *= speed;
            pb->base_velocity[1] *= speed;
            pb->base_velocity[2] *= speed;
        }

        size = tag->particle_size_bounds[0] +
               (tag->particle_size_bounds[1] - tag->particle_size_bounds[0]) * glow_next_random_unit();
        pb->rate = size / (float)entry->particle_count;

        pb->lifetime = (int16_t)halo::x87::__ftol((double)pb->rate);

        {
            float t = glow_next_random_unit();
            pb->alpha = 1.0f;
            pb->color[0] = (tag->color_bound_1.red - tag->color_bound_0.red) * t + tag->color_bound_0.red;
            pb->color[1] = (tag->color_bound_1.green - tag->color_bound_0.green) * t + tag->color_bound_0.green;
            pb->flags = pb->flags | 2;
            pb->color[2] = (tag->color_bound_1.blue - tag->color_bound_0.blue) * t + tag->color_bound_0.blue;
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
 * The z midpoints of the control-point interpolation add the y component of the base point (`... * 0.5f + c.y`, see
 * 0x4fe13d and 0x4fe189). That looks like a typo but it is what the retail code does, so it is kept as is.
 *
 * @address 0x004fde40
 */
void halo::objects::GlowView::particle_reposition(glow_particle *particle, float phase_rate)
{
    glow *entry = self;
    glow *e = reinterpret_cast<glow *>(entry);
    float t = particle->t;
    int16_t segment_count = e->marker_count - 1;
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
            float lo = e->cumulative_length[idx];
            if (lo != t && lo < t && t < e->cumulative_length[idx + 1]) break;
            seg = seg + 1;
            idx = seg;
        } while (idx < segment_count);
        if (seg < 0) idx = 0;
    }
    seg = (int16_t)idx;
    particle->segment_index = seg;

    [&]() {
        int16_t marker_count = e->marker_count;

        if (marker_count == 2) {
            c0[0].x = e->markers[0].node_transform.position.x; c0[0].y = e->markers[0].node_transform.position.y; c0[0].z = e->markers[0].node_transform.position.z;
            {
                float ex = e->markers[1].node_transform.position.x, ey = e->markers[1].node_transform.position.y, ez = e->markers[1].node_transform.position.z;
                c1[0].x = e->markers[0].node_transform.up.i; c1[0].y = e->markers[0].node_transform.up.j; c1[0].z = e->markers[0].node_transform.up.k;
                {
                    float fx = e->markers[1].node_transform.up.i, fy = e->markers[1].node_transform.up.j, fz = e->markers[1].node_transform.up.k;
                    t0 = e->cumulative_length[0]; t1 = e->cumulative_length[1];

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
            return;
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
                    float lo = e->cumulative_length[idx2];
                    float hi = e->cumulative_length[idx2 + 1];
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

            t0 = e->cumulative_length[idx2];
            t1 = e->cumulative_length[idx2 + 1];
            t2 = e->cumulative_length[idx2 + 2];
            t3 = e->cumulative_length[idx2 + 3];

            {
                int16_t *order = &e->marker_order[idx2];
                int32_t k;

                for (k = 0; k < 4; k++) {
                    const real_matrix4x3 &n = e->markers[order[k]].node_transform;
                    float fx = n.up.j, fy = n.forward.k;
                    float fz = n.forward.j, fw = n.up.k;
                    float ux = n.position.x, uy = n.position.y, uz = n.position.z;
                    float lx = n.up.i, ly = n.up.j, lzv = n.up.k;
                    float b0 = fx * fy - fz * fw;

                    c0[k].x = ux;
                    c0[k].y = uy;
                    c0[k].z = uz;
                    c1[k].x = lx;
                    c1[k].y = ly;
                    c1[k].z = lzv;

                    {
                        float a2 = n.forward.i * n.up.k - lx * n.forward.k;
                        float a3 = n.forward.j * lx - n.forward.i * n.up.j;
                        c2[k].x = b0;
                        c2[k].y = a2;
                        c2[k].z = a3;
                    }

                }
            }
            return;
        }

        c0[0].x = e->markers[0].node_transform.position.x; c0[0].y = e->markers[0].node_transform.position.y; c0[0].z = e->markers[0].node_transform.position.z;
        {
            float ex = e->markers[2].node_transform.position.x, ey = e->markers[2].node_transform.position.y, ez = e->markers[2].node_transform.position.z;
            c1[0].x = e->markers[0].node_transform.up.i; c1[0].y = e->markers[0].node_transform.up.j; c1[0].z = e->markers[0].node_transform.up.k;
            {
                float fx = e->markers[2].node_transform.up.i, fy = e->markers[2].node_transform.up.j, fz = e->markers[2].node_transform.up.k;
                t0 = e->cumulative_length[0]; t1 = e->cumulative_length[2];

                if (seg == 0) {
                    c0[1].x = e->markers[1].node_transform.position.x; c0[1].y = e->markers[1].node_transform.position.y; c0[1].z = e->markers[1].node_transform.position.z;
                    c1[1].x = e->markers[1].node_transform.up.i; c1[1].y = e->markers[1].node_transform.up.j; c1[1].z = e->markers[1].node_transform.up.k;
                    t2 = e->cumulative_length[1];
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
                    return;
                }
                if (seg != 1) {
                    return;
                }
                c2[0].x = e->markers[1].node_transform.position.x; c2[0].y = e->markers[1].node_transform.position.y; c2[0].z = e->markers[1].node_transform.position.z;
                t2 = e->markers[1].node_transform.up.i;
                c0[1].x = (c2[0].x - c0[0].x) * 0.5f + c0[0].x;
                c0[1].y = (c2[0].y - c0[0].y) * 0.5f + c0[0].y;
                c0[1].z = (c2[0].z - c0[0].z) * 0.5f + c0[0].y;
                {
                    float marker1_up_k = e->markers[1].node_transform.up.k;
                    float a8 = marker1_up_k - c1[0].z;
                    c1[1].x = (t2 - c1[0].x) * 0.5f + c1[0].x;
                    c1[1].y = (e->markers[1].node_transform.up.j - c1[0].y) * 0.5f + c1[0].y;
                    c1[1].z = a8 * 0.5f + c1[0].y;
                }
                t3 = (e->cumulative_length[1] - t0) * 0.5f;
                t3 = t3 + t0;
            }
        }
    }();

    {
        auto interpolate = [&](void *out, real_point3d *control) {
            halo::math::vector3d_cubic_interpolate(*(real_vector3d *)out, *(real_vector3d *)&control[0], *(real_vector3d *)&control[1],
                                                   *(real_vector3d *)&control[2], *(real_vector3d *)&control[3], t0, t1, t2, t3,
                                                   particle->t);
        };

        interpolate(&particle->position, c0);
        interpolate(&out1, c1);
        interpolate(&out2, c2);
    }

    {
        double angle = (double)phase_rate * (double)(particle->t) +
                        (double)(particle->angle);
        double s = halo::libm::sin(angle);
        double c = halo::libm::cos(angle);
        float scale = particle->distance;

        particle->position.x = (float)((out1.x * s + out2.x * c) * scale) + particle->position.x;
        particle->position.y = (float)((out1.y * s + out2.y * c) * scale) + particle->position.y;
        particle->position.z = (float)((out1.z * s + out2.z * c) * scale) + particle->position.z;
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
    glow *entry = 0;
    build_sprite_data data;
    glow_particle *particle;
    int16_t index = (int16_t)glow_handle;
    int16_t salt = (int16_t)(glow_handle >> 16);

    if (index >= 0 && index < glow_data->last_index) {
        glow *candidate = reinterpret_cast<glow *>(static_cast<uint8_t *>(glow_data->data) + glow_data->size * index);

        if (candidate->identifier != 0 && (salt == 0 || salt == candidate->identifier)) {
            entry = candidate;
        }
    }

    if (entry == 0) {
        return;
    }
    memset(&data, 0, sizeof(data));
    data.bitmap_group_index = halo::objects::tag_handle(halo::objects::tag_as<Glow>(entry->definition_tag)->texture);
    data.maximum_sprite_count = entry->spawn_count;
    data.shader = (uint32_t)glow_sprite_shader;
    data.sprite_count = 0;
    data.flags = 4;
    data.centroid = *global_zero_vector3d_pointer;
    data.group_count = 0;
    for (particle = entry->first_particle; particle != 0; particle = particle->next) {
        halo::render::build_sprite(&data, 0, 0, 0, &particle->position, &entry->markers[particle->segment_index].node_transform.forward, 0.0f,
                     particle->scaled_rate, reinterpret_cast<ColorARGB *>(&particle->alpha), particle->fade, 0);
    }
    halo::render::build_sprites_end(&data);
}
