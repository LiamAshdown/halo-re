/**
 * @file src/sound/channels.cpp
 * Logical playback channels: assignment, stealing and the channel parameter policies.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "internal/state.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

namespace halo::sound {

namespace channels {

void update_streaming(void)
{
    int16_t i;

    for (i = 0; i < directsound_first_channel_of_type[1]; i++) {
        if (directsound_channels[i].streaming != 0) {
            directsound_device().stream_update(i, 0);
        }
    }
}

uint8_t type_flags_match(int16_t compressed_requested, int16_t stereo_requested, uint16_t sample_rate_44khz_requested, uint16_t flags, int16_t requested_3d)
{
    uint8_t matches = (uint16_t)((flags >> 2) & 1) == sample_rate_44khz_requested &&
        ((flags & _sound_channel_stereo_bit) != 0) == (stereo_requested != 0) &&
        ((flags & _sound_channel_compressed_bit) != 0) == (compressed_requested != 0);

    if ((flags & _sound_channel_stereo_bit) == 0 &&
        ((flags & _sound_channel_3d_bit) != 0) != (requested_3d != 0)) {
        matches = 0;
    }

    return matches;
}

void assign(void)
{
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    SoundPitchRange *pitch_range;
    int16_t target_channel;
    datum_index occupant;

    sound_handle = halo::memory::datum_next(-1, sound_data);
    while (sound_handle != halo::k_dword_none) {
        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & halo::k_slot_mask) * sizeof(sound));

        if (instance->start_time <= sound_time) {
            if (instance->channel_index == -1 &&
                !halo::cache::sound_cache_touch(1, 1, 0, (SoundPermutation *)((SoundPitchRange *)((Sound *)halo::cache::globals().tag_instances[
                    instance->definition_index & halo::k_slot_mask].data)->pitch_ranges.pointer + instance->pitch_range_index)->
                    permutations.pointer + instance->permutation_index)) {
                if (instance->channel_index == -1 && instance->location_proc != looping::detail_location_proc) {
                    definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & halo::k_slot_mask].data;
                    if (sound_class_definitions[definition->sound_class].discard_on_cache_miss == 0) {
                        pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
                        if (pitch_range->discarded_permutation_index == halo::k_word_none) {
                            pitch_range->discarded_permutation_index = (uint16_t)instance->permutation_index;
                        }
                        instances::stop(sound_handle);
                    }
                }
            } else {
                instance->flags |= _sound_channel_requested_bit;
                target_channel = channels::pick_for_instance(sound_handle);
                if (target_channel == -1) {
                    instances::stop(sound_handle);
                } else {
                    occupant = sound_channels[target_channel].sound_index;
                    if (occupant != sound_handle) {
                        if (occupant != halo::k_dword_none) {
                            instances::stop(occupant);
                        }
                        sound_channels[target_channel].sound_index = sound_handle;
                        instance->start_time = sound_time;
                    }
                }
            }
        }

        sound_handle = halo::memory::datum_next((int16_t)sound_handle, sound_data);
    }
}

void build_candidates(sound_channel_candidate_list *out, datum_index sound_handle)
{
    sound *candidate;
    Sound *definition;
    int16_t channel_index;
    datum_index other_handle;
    sound *other;

    candidate = (sound *)((uint8_t *)sound_data->data + (sound_handle & halo::k_slot_mask) * sizeof(sound));
    definition = (Sound *)halo::cache::globals().tag_instances[candidate->definition_index & halo::k_slot_mask].data;

    out->tag_match_count = 0;
    out->owner_match_count = 0;
    out->tag_match_limit = sound_class_definitions[definition->sound_class].maximum_sounds_per_tag;
    out->owner_match_limit = sound_class_definitions[definition->sound_class].maximum_sounds_per_object;

    for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
        other_handle = sound_channels[channel_index].sound_index;
        if (other_handle != halo::k_dword_none && other_handle != sound_handle) {
            other = (sound *)((uint8_t *)sound_data->data + (other_handle & halo::k_slot_mask) * sizeof(sound));
            if (channels::type_flags_match(definition->format, definition->channel_count, definition->sample_rate,
                    sound_channels[channel_index].type_flags, candidate->location.type) &&
                candidate->definition_index == other->definition_index) {
                out->tag_matches[out->tag_match_count] = channel_index;
                out->tag_match_count += 1;
                if (candidate->owner_index != halo::k_dword_none && candidate->owner_index == other->owner_index) {
                    out->owner_matches[out->owner_match_count] = channel_index;
                    out->owner_match_count += 1;
                }
            }
        }
    }
}

int16_t pick_for_instance(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    sound_channel_candidate_list candidates;
    int16_t channel_index;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & halo::k_slot_mask) * sizeof(sound));

    if (instance->channel_index == -1) {
        definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & halo::k_slot_mask].data;

        if (sound_class_definitions[definition->sound_class].dialog == 0 || instance->owner_index == halo::k_dword_none) {
            channels::build_candidates(&candidates, sound_handle);
            if (candidates.owner_match_limit <= candidates.owner_match_count) {
                return channels::pick_replaceable(sound_handle, candidates.owner_matches, candidates.owner_match_count);
            }
            if (candidates.tag_match_limit <= candidates.tag_match_count) {
                return channels::pick_replaceable(sound_handle, candidates.tag_matches, candidates.tag_match_count);
            }
        } else {
            for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
                if (sound_channels[channel_index].sound_index != halo::k_dword_none) {
                    sound *other = (sound *)((uint8_t *)sound_data->data +
                        (sound_channels[channel_index].sound_index & halo::k_slot_mask) * sizeof(sound));
                    if (other->owner_index == instance->owner_index) {
                        Sound *other_definition = (Sound *)halo::cache::globals().tag_instances[other->definition_index & halo::k_slot_mask].data;
                        if (sound_class_definitions[other_definition->sound_class].dialog != 0) {
                            instance->location.type = other->location.type;
                            return channel_index;
                        }
                    }
                }
            }
        }

        return channels::find_lowest_priority(sound_handle);
    }
    return instance->channel_index;
}

int16_t find_lowest_priority(datum_index sound_handle)
{
    sound *candidate;
    Sound *definition;
    int16_t channel_index;
    int16_t best_channel;
    float candidate_distance_squared;
    float best_distance_squared = 0.0f;
    uint16_t type_flags;
    datum_index best_handle = k_datum_index_none;

    candidate = (sound *)((uint8_t *)sound_data->data + (sound_handle & halo::k_slot_mask) * sizeof(sound));
    definition = (Sound *)halo::cache::globals().tag_instances[candidate->definition_index & halo::k_slot_mask].data;
    best_channel = -1;
    candidate_distance_squared = spatial::location_distance_squared(candidate->listener_index, &candidate->location);

    for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
        type_flags = sound_channels[channel_index].type_flags;

        if (((type_flags & _sound_channel_stereo_bit) != 0 ||
             (uint32_t)(!(type_flags & _sound_channel_3d_bit)) == (uint32_t)(candidate->location.type == _sound_location_none)) &&
            (((type_flags >> 2) & 1) == (uint16_t)definition->sample_rate &&
             (uint32_t)(!((type_flags >> 1) & 1)) == (uint32_t)(definition->channel_count == 0) &&
             (uint32_t)(!((type_flags >> 3) & 1)) == (uint32_t)(definition->format == 0))) {
            if (sound_channels[channel_index].sound_index == halo::k_dword_none) {
                return channel_index;
            }
            if (channels::compare_priority(sound_channels[channel_index].sound_index, sound_handle, candidate_distance_squared) &&
                (best_channel == -1 ||
                 channels::compare_priority(sound_channels[channel_index].sound_index, best_handle, best_distance_squared))) {
                sound *occupant = (sound *)((uint8_t *)sound_data->data +
                    (sound_channels[channel_index].sound_index & halo::k_slot_mask) * sizeof(sound));
                best_handle = sound_channels[channel_index].sound_index;
                best_channel = channel_index;
                best_distance_squared = spatial::location_distance_squared(occupant->listener_index, &occupant->location);
            }
        }
    }
    return best_channel;
}

int16_t pick_replaceable(datum_index sound_handle, int16_t *candidate_channels, int16_t count)
{
    sound *candidate;
    Sound *definition;
    float candidate_distance_squared;
    int32_t minimum_replace_time;
    int16_t i;
    sound *occupant;
    float occupant_distance_squared;

    candidate = (sound *)((uint8_t *)sound_data->data + (sound_handle & halo::k_slot_mask) * sizeof(sound));
    definition = (Sound *)halo::cache::globals().tag_instances[candidate->definition_index & halo::k_slot_mask].data;
    candidate_distance_squared = spatial::location_distance_squared(candidate->listener_index, &candidate->location);
    minimum_replace_time = sound_class_definitions[definition->sound_class].minimum_replace_time;

    for (i = 0; i < count; i++) {
        occupant = (sound *)((uint8_t *)sound_data->data +
            (sound_channels[candidate_channels[i]].sound_index & halo::k_slot_mask) * sizeof(sound));
        if (minimum_replace_time <= sound_time - occupant->start_time) {
            occupant_distance_squared = spatial::location_distance_squared(occupant->listener_index, &occupant->location);
            if (candidate_distance_squared - occupant_distance_squared < 1.0f) {
                return candidate_channels[i];
            }
        }
    }
    return -1;
}

uint32_t compare_priority(datum_index sound_a, datum_index sound_b, float distance_a_squared)
{
    sound *a;
    sound *b;
    Sound *definition_a;
    Sound *definition_b;
    int16_t priority_a;
    int16_t priority_b;

    a = (sound *)((uint8_t *)sound_data->data + (sound_a & halo::k_slot_mask) * sizeof(sound));
    b = (sound *)((uint8_t *)sound_data->data + (sound_b & halo::k_slot_mask) * sizeof(sound));
    definition_a = (Sound *)halo::cache::globals().tag_instances[a->definition_index & halo::k_slot_mask].data;
    definition_b = (Sound *)halo::cache::globals().tag_instances[b->definition_index & halo::k_slot_mask].data;
    priority_a = sound_class_definitions[definition_a->sound_class].priority;
    priority_b = sound_class_definitions[definition_b->sound_class].priority;

    if (priority_a < priority_b) {
        return 1;
    }
    if (priority_a == priority_b &&
        distance_a_squared < spatial::location_distance_squared(a->listener_index, &a->location)) {
        return 1;
    }
    return 0;
}

void set_next_permutation(int16_t channel_index, SoundPermutation *permutation, int16_t unknown, int16_t sound_class, uint8_t streaming)
{
    sound_channel *channel;

    channel = &sound_channels[channel_index];

    if (channel->next_permutation != 0) {
        if (channel->next_permutation->samples_pointer != halo::k_dword_none) {
            sound_cache_entry *entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                (channel->next_permutation->samples_pointer & halo::k_slot_mask) * sizeof(sound_cache_entry));
            entry->lock_count -= 1;
        }
    }

    audio_device().channel_play(channel_index, permutation, unknown, sound_class, streaming);

    if (channel->current_permutation != 0) {
        channel->next_permutation = permutation;
    } else {
        channel->current_permutation = permutation;
        channel->play_time = 0.0f;
    }
}

void apply_default_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update, int16_t sound_class)
{
    sound_channel *channel = &sound_channels[channel_index];
    sound *self = 0;
    uint8_t first_person;

    if (channel->sound_index != k_datum_index_none) {
        self = (sound *)((uint8_t *)sound_data->data + (channel->sound_index & halo::k_slot_mask) * 0xb0);
    }
    if (!update) {
        channel->current_pitch = parameters->pitch;
    }
    if (self == 0) {
        first_person = 0;
    } else {
        if (self->location.type == _sound_location_absolute) {
            float scaled;
            int32_t combined;
            int32_t attenuation;

            scaled = (float)((double)gain::linear_to_millibels_clamped(-10000, 1.0f - self->location.occlusion, 0) *
                0.1f);
            combined = (int32_t)((double)gain::linear_to_millibels_clamped(-10000, parameters->gain, 0) +
                scaled);
            scaled = (float)combined;
            attenuation = (int32_t)((double)gain::linear_to_millibels_clamped(-10000,
                1.0f - self->location.obstruction, 0) * 0.1f + scaled);
            if (attenuation < -10000) {
                attenuation = -10000;
            } else if (attenuation > 0) {
                attenuation = 0;
            }
            parameters->gain = gain::evaluate_volume_curve(attenuation, 0, 1);
        }
        first_person = self->first_person;
    }
    audio_device().channel_set_parameters(channel_index, parameters, update);
}

void apply_eax_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update, int16_t sound_class)
{
    sound_channel *channel = &sound_channels[channel_index];
    uint8_t first_person = 0;

    if (channel->sound_index != k_datum_index_none) {
        sound *self = (sound *)((uint8_t *)sound_data->data + (channel->sound_index & halo::k_slot_mask) * 0xb0);

        if (self != 0) {
            first_person = self->first_person;
        }
    }
    if (!update) {
        channel->current_pitch = parameters->pitch;
    }
    if (first_person && sound_class == 4) {
        double gain = parameters->gain * 1.2;

        if (!(gain <= 0.6)) {
            gain = 0.6;
        }
        parameters->gain = (float)gain;
    }
    audio_device().channel_set_parameters(channel_index, parameters, update);
}

int16_t release_detail_buffers(int16_t channel_index)
{
    sound_channel *channel;
    sound_cache_entry *entry;
    int16_t channel_state;

    channel = &sound_channels[channel_index];
    channel_state = audio_device().channel_get_state(channel_index);

    if (channel->next_permutation != 0 && channel_state < 2) {
        if (channel->current_permutation->samples_pointer != halo::k_dword_none) {
            entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                (channel->current_permutation->samples_pointer & halo::k_slot_mask) * sizeof(sound_cache_entry));
            entry->lock_count -= 1;
        }
        channel->current_permutation = channel->next_permutation;
        channel->next_permutation = 0;
        channel->play_time = 0.0f;
        if (!halo::cache::sound_cache_touch(0, 0, 0, channel->current_permutation)) {
            channel_state = 0;
        }
    }

    if (channel->current_permutation != 0 && channel_state < 1) {
        if (channel->current_permutation->samples_pointer != halo::k_dword_none) {
            entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                (channel->current_permutation->samples_pointer & halo::k_slot_mask) * sizeof(sound_cache_entry));
            entry->lock_count -= 1;
        }
        channel->current_permutation = 0;
    }

    channel->play_time = sound_time_delta * channel->current_pitch + channel->play_time;
    return channel_state;
}

void release_permutations(int16_t channel_index)
{
    sound_channel *channel;
    sound_cache_entry *entry;

    channel = &sound_channels[channel_index];
    if (channel != 0) {
        if (channel->next_permutation != 0) {
            if (channel->next_permutation->samples_pointer != halo::k_dword_none) {
                entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                    (channel->next_permutation->samples_pointer & halo::k_slot_mask) * sizeof(sound_cache_entry));
                entry->lock_count -= 1;
            }
            channel->next_permutation = 0;
        }
        if (channel->current_permutation != 0) {
            if (channel->current_permutation->samples_pointer != halo::k_dword_none) {
                entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                    (channel->current_permutation->samples_pointer & halo::k_slot_mask) * sizeof(sound_cache_entry));
                entry->lock_count -= 1;
            }
            channel->current_permutation = 0;
        }
    }

    audio_device().channel_stop(channel_index);
}

}  // namespace channels


}  // namespace halo::sound
