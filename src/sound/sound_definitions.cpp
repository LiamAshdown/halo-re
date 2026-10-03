/**
 * @file src/sound/sound_definitions.cpp
 * Sound tag queries: promotion, audibility, distance, permutation and pitch choice.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "halo/core/lcg.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "internal/state.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"

namespace halo::sound {

namespace definitions {

float maximum_distance(datum_index sound_definition)
{
    Sound *tag = (Sound *)halo::cache::globals().tag_instances[sound_definition & halo::k_slot_mask].data;
    float distance = tag->maximum_distance;

    if (distance == 0.0f) {
        distance = sound_class_definitions[tag->sound_class].default_maximum_distance;
    }

    return distance;
}

int16_t pick_pitch_range(int16_t pitch_range_index, Sound *tag, float target_pitch)
{
    int32_t count = (int32_t)tag->pitch_ranges.count;
    SoundPitchRange *ranges = (SoundPitchRange *)tag->pitch_ranges.pointer;
    int16_t best = -1;
    float best_ratio = 3.4028235e+38f;
    int32_t i;

    if (pitch_range_index != -1 && pitch_range_index < count) {
        SoundPitchRange *current = &ranges[pitch_range_index];
        if (current->bend_bounds[0] <= target_pitch && target_pitch <= current->bend_bounds[1] &&
            current->permutations.count != 0) {
            return pitch_range_index;
        }
    }

    for (i = 0; i < count; i++) {
        SoundPitchRange *range = &ranges[i];

        if (range->permutations.count != 0) {
            float ratio;

            if (range->bend_bounds[0] <= target_pitch && target_pitch <= range->bend_bounds[1]) {
                return (int16_t)i;
            }

            if (target_pitch <= range->bend_bounds[1]) {
                ratio = range->bend_bounds[0] / target_pitch;
            } else {
                ratio = target_pitch / range->bend_bounds[1];
            }

            if (ratio < best_ratio) {
                best = (int16_t)i;
                best_ratio = ratio;
            }
        }
    }

    return best;
}

int16_t pick_permutation(int16_t pitch_range_index, int16_t explicit_permutation_index, Sound *tag)
{
    SoundPitchRange *range = (SoundPitchRange *)tag->pitch_ranges.pointer + pitch_range_index;
    SoundPermutation *permutations;
    int16_t chosen;
    int16_t attempts;
    int16_t sound_class;
    uint32_t candidate;

    if (range->discarded_permutation_index != (uint16_t)0xffff) {
        chosen = (int16_t)range->discarded_permutation_index;
        range->last_permutation_index = (uint16_t)chosen;
        range->discarded_permutation_index = halo::k_word_none;
        return chosen;
    }

    if ((tag->flags & 2) != 0 && explicit_permutation_index != -1) {
        permutations = (SoundPermutation *)range->permutations.pointer;
        return (int16_t)permutations[explicit_permutation_index].next_permutation_index;
    }

    halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
    candidate = (uint32_t)((halo::math::globals().effect_random_seed >> 16) * (int32_t)range->actual_permutation_count) >> 16;
    attempts = 0;
    permutations = (SoundPermutation *)range->permutations.pointer;

    for (;;) {
        uint32_t candidate_bit;

        chosen = (int16_t)candidate;

        if ((~range->permutation_flags & ((1u << (range->actual_permutation_count & 0x1f)) - 1)) == 0) {
            range->permutation_flags = 0;
            if (range->actual_permutation_count > 1) {
                range->permutation_flags = 1u << (range->last_permutation_index & 0x1f);
            }
        }

        candidate_bit = 1u << (candidate & 0x1f);

        if ((range->permutation_flags & candidate_bit) == 0) {
            range->permutation_flags |= candidate_bit;

            if (attempts == 0x10) {
                break;
            }

            halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
            attempts++;

            if (permutations[chosen].skip_fraction <= (float)(halo::math::globals().effect_random_seed >> 16) * halo::k_unit_word_scale) {
                break;
            }
        }

        candidate++;
        if ((int16_t)candidate == (int16_t)range->actual_permutation_count) {
            candidate = 0;
        }
    }

    range->last_permutation_index = (uint16_t)chosen;
    sound_class = tag->sound_class;

    if (sound_class != soundclass_unit_footsteps && sound_class != soundclass_unit_dialog &&
        (sound_class < soundclass_music || sound_class > soundclass_ambient_computers) && sound_class < soundclass_scripted_dialog_player) {
        int32_t limit = sound_permutation_limit;

        if (limit != 0) {
            limit -= 1;
            if (limit != 0) {
                return chosen;
            }
            if (range->actual_permutation_count > 1) {
                return (int16_t)halo::effects::effect_random_int_between(0, (int16_t)((int16_t)range->actual_permutation_count / 2));
            }
        }
        chosen = 0;
    }

    return chosen;
}

float compute_random_pitch(float pitch_bounds_min, float pitch_bounds_max, float zero_pitch_modifier, float one_pitch_modifier, float distance_scale)
{
    float distance_modifier;
    float random_pitch;

    distance_modifier = (one_pitch_modifier - zero_pitch_modifier) * distance_scale + zero_pitch_modifier;
    random_pitch = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed, pitch_bounds_min, pitch_bounds_max);
    return distance_modifier * random_pitch;
}

uint32_t has_audible_permutations(TagID sound_tag_id)
{
    Sound *sound;
    SoundPitchRange *pitch_range;

    sound = (Sound *)halo::cache::globals().tag_instances[sound_tag_id.index].data;
    if (sound->pitch_ranges.count != 0) {
        pitch_range = (SoundPitchRange *)sound->pitch_ranges.pointer;
        if (pitch_range->permutations.count != 0 &&
            sound_class_definitions[sound->sound_class].muted == 0) {
            return 1;
        }
    }
    return 0;
}

int16_t check_promotion(TagID sound_tag_id)
{
    Sound *sound;
    int32_t permutation_length;
    int32_t accumulated;
    int32_t threshold;

    sound = (Sound *)halo::cache::globals().tag_instances[sound_tag_id.index].data;
    if (sound->promotion_count == 0) {
        return 0;
    }

    permutation_length = (int32_t)sound->longest_permutation_length;
    accumulated = (int32_t)sound->promotion_counter + (sound->promotion_time - sound_time);
    if (accumulated < 0) {
        accumulated = 0;
    }
    accumulated = accumulated + permutation_length;
    sound->promotion_time = sound_time;
    sound->promotion_counter = accumulated;

    threshold = sound->promotion_count * permutation_length;
    if (threshold < accumulated) {
        if (sound->promotion_sound.tag_id.index != halo::k_word_none || sound->promotion_sound.tag_id.id != halo::k_word_none) {
            sound->promotion_counter = 0;
            return 1;
        }
        sound->promotion_counter = accumulated - permutation_length;
        return 2;
    }
    return 0;
}

void random_detail_direction(SoundLoopingDetail *detail, real_vector3d *out)
{
    float distance;
    float pitch;
    float yaw;

    distance = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed, detail->distance_bounds[0], detail->distance_bounds[1]);
    if (distance != 0.0f) {
        pitch = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed, detail->pitch_bounds[0], detail->pitch_bounds[1]);
        yaw = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed, detail->yaw_bounds[0], detail->yaw_bounds[1]);
        out->i = (float)(cos((double)yaw) * cos((double)pitch)) * distance;
        out->j = (float)(sin((double)yaw) * cos((double)pitch)) * distance;
        out->k = (float)sin((double)pitch) * distance;
    } else {
        *out = *(const real_vector3d *)global_origin3d_pointer;
    }
}

}  // namespace definitions


}  // namespace halo::sound
