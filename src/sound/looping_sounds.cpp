/**
 * @file src/sound/looping_sounds.cpp
 * Looping sound table: tracks, detail sounds and the per-update state machine.
 * The original author notes and decompiles are in docs/original/sound/.
 */

#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "internal/state.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"

#define TRACK_TAG(track, field) (halo::tag_id_bits((track)->field.tag_id))

namespace halo::sound {

namespace {

/** Releases the cached sample pages of every permutation of a sound tag that nothing has locked. */
void sound_looping_evict_sound_samples(Sound *tag)
{
    int32_t i;

    for (i = 0; i < (int32_t)tag->pitch_ranges.count; i++) {
        SoundPitchRange *range = (SoundPitchRange *)tag->pitch_ranges.pointer + i;
        int32_t j;

        for (j = 0; j < (int32_t)range->permutations.count; j++) {
            SoundPermutation *permutation = (SoundPermutation *)range->permutations.pointer + j;
            datum_index cache_index = (datum_index)permutation->samples_pointer;

            if (cache_index != (datum_index)0xffffffff &&
                ((sound_cache_entry *)halo::cache::globals().sound_cache_entries->data)[cache_index & halo::k_slot_mask].lock_count == 0) {
                halo::memory::cache_evict_entry(cache_index, halo::cache::globals().sound_cache);
                permutation->samples_pointer = halo::k_dword_none;
                *(uint32_t *)&((struct SoundPermutation *)permutation)->cache_page = 0;
            }
        }
    }
}

/** Releases the cached sample pages of a sound tag unless the tag id is null. */
void sound_looping_evict_samples(uint32_t tag_id)
{
    if (tag_id != halo::k_dword_none) {
        sound_looping_evict_sound_samples((Sound *)halo::cache::globals().tag_instances[tag_id & halo::k_slot_mask].data);
    }
}

/** Releases the cached sample pages of a music-class sound tag. Returns 1 for a null tag or a music tag and 0 for any other class. */
uint8_t sound_looping_evict_music_samples(uint32_t tag_id)
{
    Sound *tag;

    if (tag_id == halo::k_dword_none) {
        return 1;
    }
    tag = (Sound *)halo::cache::globals().tag_instances[tag_id & halo::k_slot_mask].data;
    if (tag->sound_class != soundclass_music) {
        return 0;
    }
    sound_looping_evict_sound_samples(tag);
    return 1;
}

/** Releases the unused cached sample pages of a music-class sound tag; returns whether it did. */
uint8_t sound_release_unused_pages_if_music(TagID tag_id)
{
    Sound *definition;
    int32_t range_index, permutation_index;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    sound_cache_entry *entry;

    definition = (Sound *)halo::cache::globals().tag_instances[tag_id.index].data;
    if (definition->sound_class != soundclass_music) {
        return 0;
    }

    for (range_index = 0; range_index < definition->pitch_ranges.count; range_index++) {
        pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + range_index;
        for (permutation_index = 0; permutation_index < (int32_t)pitch_range->permutations.count;
             permutation_index++) {
            permutation = (SoundPermutation *)pitch_range->permutations.pointer + permutation_index;
            if (permutation->samples_pointer != halo::k_dword_none) {
                entry = (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                    (permutation->samples_pointer & halo::k_slot_mask) * sizeof(sound_cache_entry));
                if (entry->lock_count == 0) {
                    halo::memory::cache_evict_entry((datum_index)permutation->samples_pointer, halo::cache::globals().sound_cache);
                    permutation->samples_pointer = halo::k_dword_none;
                    permutation->cache_page = 0;
                }
            }
        }
    }
    return 1;
}

}  // namespace

namespace looping {

void predict(datum_index looping_definition)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[looping_definition & halo::k_slot_mask].data;
        int32_t i;

        for (i = 0; i < (int32_t)definition->tracks.count; i++) {
            SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;
            uint32_t loop_sound = halo::tag_id_bits(track->loop.tag_id);

            if (loop_sound != halo::k_dword_none) {
                Sound *loop_tag = (Sound *)halo::cache::globals().tag_instances[loop_sound & halo::k_slot_mask].data;

                if (loop_tag->pitch_ranges.count == 1 &&
                    ((SoundPitchRange *)loop_tag->pitch_ranges.pointer)->permutations.count != 0) {
                    halo::cache::sound_cache_touch(1, 0, 0, (SoundPermutation *)((SoundPitchRange *)loop_tag->pitch_ranges.pointer)->
                        permutations.pointer);
                }
            }
        }
    }
}

void start(datum_index definition_index, datum_index object_index, float scale)
{
    SoundLooping *definition;
    datum_index new_sound;

    if (definition_index == k_datum_index_none) {
        return;
    }

    definition = (SoundLooping *)halo::cache::globals().tag_instances[definition_index & halo::k_slot_mask].data;
    looping::stop(definition_index);
    if ((definition->flags & 0x04) != 0) {
        game_looping::stop_loops_conflicting_with_music();
    }

    new_sound = game_looping::create(object_index, definition_index, k_empty_string, -1);
    if (new_sound != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[new_sound & halo::k_slot_mask];
        self->flags |= _game_looping_sound_script_gain_bit;
        self->scale = scale;
    }

    *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)new_sound;
    if (new_sound != k_datum_index_none) {
        ((game_looping_sound *)game_looping_sound_data->data)[new_sound & halo::k_slot_mask].flags |=
            _game_looping_sound_scripted_bit;
    }
}

void stop(datum_index looping_definition)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[looping_definition & halo::k_slot_mask].data;
        datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

        if (scripted_sound != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

            self->flags &= ~_game_looping_sound_scripted_bit;
            self->flags |= _game_looping_sound_stop_requested_bit;

            *(uint32_t *)&definition->runtime_scripting_sound = (uint32_t)k_datum_index_none;
        }
    }
}

void set_scale(datum_index looping_definition, float gain)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[looping_definition & halo::k_slot_mask].data;
        datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

        if (scripted_sound != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

            if (gain < 0.0f) {
                self->scale = 0.0f;
            } else if (gain > 1.0f) {
                self->scale = 1.0f;
            } else {
                self->scale = gain;
            }
        }
    }
}

void set_alternate(datum_index looping_definition, uint8_t alternate)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[looping_definition & halo::k_slot_mask].data;
        datum_index scripted_sound = *(datum_index *)&definition->runtime_scripting_sound;

        if (scripted_sound != k_datum_index_none) {
            game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[(uint16_t)scripted_sound];

            if (alternate != 0) {
                self->flags |= _game_looping_sound_alternate_bit;
            } else {
                self->flags &= ~_game_looping_sound_alternate_bit;
            }
        }
    }
}

datum_index start_ambient(datum_index object_index, datum_index definition_index, float scale)
{
    datum_index new_sound = game_looping::create(object_index, definition_index, k_empty_string, -1);

    if (new_sound != k_datum_index_none) {
        game_looping_sound *self = &((game_looping_sound *)game_looping_sound_data->data)[new_sound & halo::k_slot_mask];
        self->flags |= _game_looping_sound_script_gain_bit;
        self->scale = scale;
    }

    return new_sound;
}

uint32_t definition_has_music_loop(datum_index looping_definition)
{
    SoundLooping *definition = (SoundLooping *)halo::cache::globals().tag_instances[looping_definition & halo::k_slot_mask].data;
    int32_t i;

    for (i = 0; i < (int32_t)definition->tracks.count; i++) {
        SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;
        uint32_t loop_sound = halo::tag_id_bits(track->loop.tag_id);

        if (loop_sound != halo::k_dword_none) {
            Sound *loop_tag = (Sound *)halo::cache::globals().tag_instances[loop_sound & halo::k_slot_mask].data;
            if (loop_tag->sound_class == soundclass_music) {
                return 1;
            }
        }
    }

    return 0;
}

void touch(int32_t reference)
{
    if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
        datum_index found = looping::find_by_owner(reference);

        if (found != (datum_index)0xffffffff) {
            ((looping_sound *)looping_sound_data->data)[(uint16_t)found].update_toggle = sound_update_toggle;
        }
    }
}

uint8_t set_state(int32_t owner, datum_index definition_index, sound_location *location, int16_t state, uint8_t alternate, float fade_duration)
{
    SoundLooping *definition;
    looping_sound *self;
    datum_index handle;
    uint8_t is_new;
    int16_t i;

    looping::check_audibility_gate(definition_index, location);

    if (sound_initialized == 0 || sound_enabled == 0 || sound_disabled != 0) {
        return state == 2;
    }

    handle = looping::find_by_owner(owner);
    is_new = 0;
    if (handle == (datum_index)0xffffffff) {
        if (state == 2) {
            return 1;
        }
        handle = looping::state_new(definition_index, owner, location);
        is_new = 1;
        if (handle == (datum_index)0xffffffff) {
            return 0;
        }
    }

    self = &((looping_sound *)looping_sound_data->data)[handle & halo::k_slot_mask];
    definition = (SoundLooping *)halo::cache::globals().tag_instances[definition_index & halo::k_slot_mask].data;

    self->location = *location;
    self->update_toggle = sound_update_toggle;

    if ((state == 2 || self->finished != 0) && self->active_sound_count == 0) {
        if ((definition->flags & 0x02) != 0) {
            for (i = 0; i < (int32_t)definition->tracks.count; i++) {
                SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;

                sound_looping_evict_samples(TRACK_TAG(track, start));
                sound_looping_evict_samples(TRACK_TAG(track, end));
                sound_looping_evict_samples(TRACK_TAG(track, alternate_end));
                sound_looping_evict_samples(TRACK_TAG(track, loop));
                sound_looping_evict_samples(TRACK_TAG(track, alternate_loop));
            }
        }
        halo::memory::datum_delete(looping_sound_data, handle);
        return 1;
    }

    if (halo::tag_id_bits(definition->continuous_damage_effect.tag_id) != halo::k_dword_none) {
        halo::effects::player_effect_apply_at_object(halo::tag_id_bits(definition->continuous_damage_effect.tag_id), 0,
            (real_point3d *)&location->position);
    }

    for (i = 0; i < (int32_t)definition->tracks.count; i++) {
        SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;
        bool stop_track = false;

        if (is_new) {
            self->track_sounds[i] = (datum_index)0xffffffff;
        }

        if (state == 0) {
            if (TRACK_TAG(track, start) != halo::k_dword_none) {
                self->track_sounds[i] = looping::create_detail_sound(handle, TRACK_TAG(track, start),
                    i, _sound_play_loop_start);
            }
        } else if (state == 2) {
            stop_track = true;
        }

        if (self->finished != 0) {
            stop_track = true;
        }

        if (!stop_track) {
            uint32_t loop_tag = TRACK_TAG(track, loop);

            if (alternate != 0 && TRACK_TAG(track, alternate_loop) != halo::k_dword_none) {
                loop_tag = TRACK_TAG(track, alternate_loop);
            }
            if (loop_tag == halo::k_dword_none) {
                continue;
            }

            if (self->track_sounds[i] == (datum_index)0xffffffff ||
                (state == 0 && (track->flags & 0x01) != 0)) {
                datum_index new_sound = looping::create_detail_sound(handle, loop_tag, i, _sound_play_loop);

                if (new_sound != (datum_index)0xffffffff) {
                    if (state != 0) {
                        instances::schedule_gain_fade(new_sound, _sound_fade_linear, 2.0f, (datum_index)0xffffffff);
                    } else if ((track->flags & 0x01) != 0) {
                        instances::schedule_gain_fade(new_sound, _sound_fade_linear, track->fade_in_duration,
                            (datum_index)0xffffffff);
                    }
                    self->track_sounds[i] = new_sound;
                }
            } else if (alternate != self->alternate && (track->flags & 0x04) != 0) {
                datum_index new_sound = looping::create_detail_sound(handle, loop_tag, i, _sound_play_loop);

                if (new_sound != (datum_index)0xffffffff) {
                    instances::schedule_gain_fade(new_sound, _sound_fade_linear, track->fade_out_duration,
                        self->track_sounds[i]);
                    self->track_sounds[i] = new_sound;
                }
            } else if (!is_new) {
                instances::queue_definition_switch(self->track_sounds[i], loop_tag);
            }
            continue;
        }

        if (self->state == 2) {
            continue;
        }
        if (fade_duration != 0.0f) {
            instances::schedule_gain_fade((datum_index)0xffffffff, _sound_fade_linear, fade_duration,
                self->track_sounds[i]);
            continue;
        }

        if (self->track_sounds[i] != (datum_index)0xffffffff &&
            ((track->flags & 0x02) != 0 ||
             (TRACK_TAG(track, end) == halo::k_dword_none && (definition->flags & 0x02) == 0))) {
            instances::schedule_gain_fade((datum_index)0xffffffff, _sound_fade_linear, track->fade_out_duration,
                self->track_sounds[i]);
        }

        if (TRACK_TAG(track, end) != halo::k_dword_none) {
            uint32_t end_tag = TRACK_TAG(track, end);

            if (alternate != 0 && TRACK_TAG(track, alternate_end) != halo::k_dword_none) {
                end_tag = TRACK_TAG(track, alternate_end);
            }

            if ((track->flags & 0x02) != 0) {
                looping::create_detail_sound(handle, end_tag, i, _sound_play_loop_end);
            } else {
                datum_index track_sound = self->track_sounds[i];

                if (track_sound != (datum_index)0xffffffff) {
                    sound *playing = &((sound *)sound_data->data)[track_sound & halo::k_slot_mask];

                    if (playing->channel_index != -1) {
                        instances::queue_definition_switch(track_sound, end_tag);
                        playing->play_state = _sound_play_loop_stopping;
                    }
                }
            }
        }
    }

    if (self->active_sound_count == 0 &&
        view(location)->check_audibility(definition->maximum_distance) == -1) {
        for (i = 0; i < (int32_t)definition->tracks.count; i++) {
            SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;

            if (!sound_looping_evict_music_samples(TRACK_TAG(track, start)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, end)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, alternate_end)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, loop)) ||
                !sound_looping_evict_music_samples(TRACK_TAG(track, alternate_loop))) {
                break;
            }
        }
        halo::memory::datum_delete(looping_sound_data, handle);
    }

    self->state = state;
    self->alternate = alternate;
    return 0;
}

datum_index state_new(datum_index definition_index, int32_t owner, sound_location *location)
{
    datum_index handle;
    looping_sound *state;
    SoundLooping *definition;
    int16_t detail_index;

    if (!sound_initialized || !sound_enabled || sound_disabled) {
        return 0xffffffff;
    }

    handle = halo::memory::datum_new(looping_sound_data);
    if (handle == halo::k_dword_none) {
        return handle;
    }

    state = (looping_sound *)((uint8_t *)looping_sound_data->data + (handle & halo::k_slot_mask) * sizeof(looping_sound));
    definition = (SoundLooping *)halo::cache::globals().tag_instances[definition_index & halo::k_slot_mask].data;

    state->definition_index = definition_index;
    state->owner = owner;
    state->active_sound_count = 0;
    state->finished = 0;

    for (detail_index = 0; detail_index < (int32_t)definition->detail_sounds.count; detail_index++) {
        SoundLoopingDetail *detail = (SoundLoopingDetail *)definition->detail_sounds.pointer + detail_index;
        float random_value = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed, detail->random_period_bounds[0],
            detail->random_period_bounds[1]);
        float period = definition->zero_detail_sound_period +
            (definition->one_detail_sound_period - definition->zero_detail_sound_period) * location->scale;

        state->detail_next_time[detail_index] = (int32_t)((float)sound_time + random_value * period * 1000.0f);
    }

    return handle;
}

void update_states(void)
{
    datum_index handle;
    looping_sound *state;
    SoundLooping *definition;
    int32_t detail_index;
    SoundLoopingDetail *detail;
    float period;
    float random_value;
    real_vector3d direction;
    sound_location location;
    int32_t track_index;
    SoundLoopingTrack *track;

    handle = halo::memory::datum_next(-1, looping_sound_data);
    while (handle != halo::k_dword_none) {
        state = (looping_sound *)((uint8_t *)looping_sound_data->data + (handle & halo::k_slot_mask) * sizeof(looping_sound));
        definition = (SoundLooping *)halo::cache::globals().tag_instances[state->definition_index & halo::k_slot_mask].data;

        if (state->update_toggle == sound_update_toggle) {
            if (state->state != 2) {
                for (detail_index = 0; detail_index < (int32_t)definition->detail_sounds.count; detail_index++) {
                    detail = (SoundLoopingDetail *)definition->detail_sounds.pointer + detail_index;
                    if (state->detail_next_time[detail_index] < sound_time &&
                        (detail->sound.tag_id.index != halo::k_word_none || detail->sound.tag_id.id != halo::k_word_none)) {
                        Sound *detail_tag = (Sound *)halo::cache::globals().tag_instances[detail->sound.tag_id.index].data;
                        uint8_t skip_with_alternate = (detail->flags & 1) != 0 && state->alternate != 0;
                        uint8_t skip_without_alternate = (detail->flags & 2) != 0 && state->alternate == 0;
                        if (!skip_with_alternate && !skip_without_alternate) {
                            location.type = (int16_t)((state->location.type == _sound_location_none) + 1);
                            location.scale = state->location.scale;
                            location.gain = detail->gain;
                            definitions::random_detail_direction(detail, &direction);
                            looping::detail_location_proc(handle, &direction, &location);
                            instances::play_new(halo::tag_id_bits(detail->sound.tag_id), &location, handle,
                                looping::detail_location_proc, &direction, sizeof(direction), 0);
                        }

                        period = definition->zero_detail_sound_period +
                            (definition->one_detail_sound_period - definition->zero_detail_sound_period) *
                                state->location.scale;
                        random_value = halo::math::random_real_range_seeded(halo::math::globals().effect_random_seed, detail->random_period_bounds[0],
                            detail->random_period_bounds[1]);
                        state->detail_next_time[detail_index] = (int32_t)(random_value * period * 1000.0f +
                            (float)(int32_t)detail_tag->longest_permutation_length + (float)sound_time);
                    }
                }
            }
        } else {
            for (track_index = 0; track_index < definition->tracks.count; track_index++) {
                track = (SoundLoopingTrack *)definition->tracks.pointer + track_index;
                if ((track->start.tag_id.index != halo::k_word_none || track->start.tag_id.id != halo::k_word_none) &&
                    !sound_release_unused_pages_if_music(track->start.tag_id)) {
                    break;
                }
                if ((track->end.tag_id.index != halo::k_word_none || track->end.tag_id.id != halo::k_word_none) &&
                    !sound_release_unused_pages_if_music(track->end.tag_id)) {
                    break;
                }
                if ((track->alternate_end.tag_id.index != halo::k_word_none || track->alternate_end.tag_id.id != halo::k_word_none) &&
                    !sound_release_unused_pages_if_music(track->alternate_end.tag_id)) {
                    break;
                }
                if ((track->loop.tag_id.index != halo::k_word_none || track->loop.tag_id.id != halo::k_word_none) &&
                    !sound_release_unused_pages_if_music(track->loop.tag_id)) {
                    break;
                }
                if ((track->alternate_loop.tag_id.index != halo::k_word_none || track->alternate_loop.tag_id.id != halo::k_word_none) &&
                    !sound_release_unused_pages_if_music(track->alternate_loop.tag_id)) {
                    break;
                }
            }
            halo::memory::datum_delete(looping_sound_data, handle);
        }

        handle = halo::memory::datum_next((int16_t)handle, looping_sound_data);
    }
}

datum_index create_detail_sound(datum_index owner, datum_index definition_index, int16_t track_index, int16_t play_state)
{
    looping_sound *state;
    Sound *definition;
    float owner_scale;
    float max_distance;
    int16_t listener_index;
    datum_index handle;
    sound *instance;
    float pitch;
    float pitch_modifier;

    state = (looping_sound *)((uint8_t *)looping_sound_data->data + (owner & halo::k_slot_mask) * sizeof(looping_sound));
    owner_scale = state->location.scale;
    definition = (Sound *)halo::cache::globals().tag_instances[definition_index & halo::k_slot_mask].data;

    if (definition->pitch_ranges.count == 0) {
        return 0xffffffff;
    }
    if (((SoundPitchRange *)definition->pitch_ranges.pointer)->permutations.count == 0 ||
        sound_class_definitions[definition->sound_class].muted != 0) {
        return 0xffffffff;
    }

    max_distance = definition->maximum_distance;
    if (max_distance == 0.0f) {
        max_distance = sound_class_definitions[definition->sound_class].default_maximum_distance;
    }
    listener_index = view(&state->location)->check_audibility(max_distance);
    if (listener_index == -1) {
        return 0xffffffff;
    }

    handle = halo::memory::datum_new(sound_data);
    if (handle == halo::k_dword_none) {
        return handle;
    }

    instance = (sound *)((uint8_t *)sound_data->data + (handle & halo::k_slot_mask) * sizeof(sound));
    instance->definition_index = definition_index;
    instance->channel_index = -1;
    instance->listener_index = listener_index;
    instance->flags = 0;
    pitch = halo::math::random_range_real(definition->random_pitch_bounds[0], definition->random_pitch_bounds[1]);
    instance->pitch = pitch;
    instance->owner_index = owner;
    instance->location = state->location;
    instance->track_index = track_index;
    instance->start_time = sound_time;
    instance->play_state = play_state;
    instance->location_proc = looping::track_location_proc;
    instance->fade_end_time = 0;
    instance->fade_start_time = 0;
    instance->pending_definition_index = halo::k_dword_none;

    pitch_modifier = (definition->one_pitch_modifier - definition->zero_pitch_modifier) * owner_scale +
        definition->zero_pitch_modifier;
    instance->pitch_range_index = definitions::pick_pitch_range(-1, definition, pitch_modifier * pitch);
    instance->permutation_index = definitions::pick_permutation(instance->pitch_range_index, -1, definition);

    {
        SoundPitchRange *range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        halo::cache::sound_cache_touch(1, 0, 0, (SoundPermutation *)range->permutations.pointer + instance->permutation_index);
    }
    state->active_sound_count += 1;

    return handle;
}

uint8_t track_location_proc(datum_index owner, void *callback_data, sound_location *location)
{
    int16_t index = (int16_t)owner;
    int16_t salt = (int16_t)(owner >> 16);
    looping_sound *state;

    (void)callback_data;
    if (owner == k_datum_index_none || index < 0 || index >= looping_sound_data->maximum_count) {
        return 0;
    }
    state = (looping_sound *)((uint8_t *)looping_sound_data->data + (int32_t)looping_sound_data->size * index);
    if (state->identifier == 0 || (salt != 0 && state->identifier != salt)) {
        return 0;
    }
    *location = state->location;
    return 1;
}

uint8_t detail_location_proc(datum_index owner, void *callback_data, sound_location *location)
{
    looping_sound *owner_sound;
    float *offset = (float *)callback_data;

    owner_sound = (looping_sound *)halo::memory::datum_get(owner, looping_sound_data);
    if (owner_sound == 0) {
        return 0;
    }

    location->obstruction = owner_sound->location.obstruction;
    location->occlusion = owner_sound->location.occlusion;

    if (owner_sound->location.type == _sound_location_none) {
        location->forward = *(Vector3D *)halo::math::globals().global_forward3d_pointer;
        location->velocity = *(Vector3D *)global_origin3d_pointer;
    } else {
        location->forward = owner_sound->location.forward;
        location->velocity = owner_sound->location.velocity;
        *(uint32_t *)&location->leaf_index = *(uint32_t *)&owner_sound->location.leaf_index;
        *(uint32_t *)&location->cluster_index = *(uint32_t *)&owner_sound->location.cluster_index;
    }

    location->position.x = offset[0];
    location->position.y = offset[1];
    location->position.z = offset[2];
    if (location->type == _sound_location_absolute) {
        location->position.x += owner_sound->location.position.x;
        location->position.y += owner_sound->location.position.y;
        location->position.z += owner_sound->location.position.z;
    }

    return 1;
}

void update_gain(int16_t channel_index, float external_gain_multiplier)
{
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    looping_sound *owner_state;
    SoundLooping *looping_definition;
    SoundLoopingTrack *track;
    float scale;
    float pitch;
    float zero_gain, class_gain, gain;
    sound_channel_parameters params;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    int16_t streaming_flag = 0;

    sound_handle = sound_channels[channel_index].sound_index;
    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & halo::k_slot_mask) * sizeof(sound));
    scale = instance->location.scale;
    definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & halo::k_slot_mask].data;
    owner_state = (looping_sound *)((uint8_t *)looping_sound_data->data +
        (instance->owner_index & halo::k_slot_mask) * sizeof(looping_sound));
    looping_definition = (SoundLooping *)halo::cache::globals().tag_instances[owner_state->definition_index & halo::k_slot_mask].data;
    track = (SoundLoopingTrack *)looping_definition->tracks.pointer + instance->track_index;

    pitch = ((definition->one_pitch_modifier - definition->zero_pitch_modifier) * scale +
        definition->zero_pitch_modifier) * instance->pitch;

    params.minimum_distance = definition->minimum_distance;
    if (params.minimum_distance == 0.0f) {
        params.minimum_distance = sound_class_definitions[definition->sound_class].default_minimum_distance;
    }
    params.maximum_distance = 3.4028235e+38f;
    params.pitch = pitch;
    params.inner_cone_angle = definition->inner_cone_angle;
    params.outer_cone_angle = definition->outer_cone_angle;
    params.outer_cone_gain = definition->outer_cone_gain;
    params.eax_value = sound_class_definitions[definition->sound_class].eax_value;

    zero_gain = definition->zero_gain_modifier;
    class_gain = classes::compute_gain(definition->sound_class);
    gain = (definition->one_gain_modifier - zero_gain) * scale + zero_gain;
    gain = gain * class_gain * track->gain * definition->random_gain_modifier * instance->location.gain *
        external_gain_multiplier;
    params.gain = gain;

    if (instance->channel_index == -1) {
        if (definition->sound_class == soundclass_music && sound_music_gain == 0.0f) {
            return;
        }
        pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
        params.gain = params.gain * permutation->gain;
        params.pitch = pitch * pitch_range->playback_rate;
        sound_channel_parameters_proc_ptr(channel_index, &params, 0, definition->sound_class);
        channels::set_next_permutation(channel_index, permutation, instance->first_person,
            definition->sound_class, 0);
        instance->channel_index = channel_index;
        audio_device().channel_continue(channel_index, 0, definition->sound_class);
        return;
    }

    if (definition->sound_class == soundclass_music && sound_music_gain == 0.0f) {
        instances::stop(sound_handle);
        return;
    }

    pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
    {
        float bent_pitch = gain::clamp_by_ratio(pitch, sound_channels[channel_index].current_pitch *
            pitch_range->natural_pitch, definition->maximum_bend_per_second);
        params.pitch = bent_pitch * pitch_range->playback_rate;
    }

    if (instance->play_state == _sound_play_loop &&
        (instance->fade_start_time == instance->fade_end_time || instance->fade_end_gain != 0.0f) &&
        definitions::pick_pitch_range(instance->pitch_range_index, definition, pitch) != instance->pitch_range_index &&
        sound_channels[channel_index].sound_index == owner_state->track_sounds[instance->track_index] &&
        !sound_idle_update_active) {
        datum_index detail_handle = looping::create_detail_sound(instance->owner_index,
            instance->definition_index, instance->track_index, _sound_play_loop);
        if (detail_handle != halo::k_dword_none) {
            instances::schedule_gain_fade(detail_handle, _sound_fade_power, 0.5f, sound_channels[channel_index].sound_index);
            owner_state->track_sounds[instance->track_index] = detail_handle;
        }
    }

    if (instance->play_state != _sound_play_loop_end &&
        (instance->play_state != _sound_play_loop_start || !(track->flags & 1)) &&
        (channels::release_detail_buffers(channel_index) != 2 ||
         (instance->flags & _sound_permutation_pending_bit) != 0 ||
         (sound_channels[channel_index].current_permutation != 0 &&
          sound_channels[channel_index].current_permutation->next_permutation_index == halo::k_word_none &&
          instance->pending_definition_index != halo::k_dword_none))) {

        if (instance->pending_definition_index == halo::k_dword_none ||
            (sound_channels[channel_index].current_permutation != 0 &&
             sound_channels[channel_index].current_permutation->next_permutation_index != halo::k_word_none)) {
            if (!(instance->flags & _sound_permutation_pending_bit)) {
                int16_t new_permutation = definitions::pick_permutation(instance->pitch_range_index,
                    instance->permutation_index, definition);
                streaming_flag = 1;
                if (new_permutation == -1) {
                    if (!(looping_definition->flags & 2)) {
                        new_permutation = definitions::pick_permutation(instance->pitch_range_index, -1, definition);
                        if (new_permutation != -1) {
                            instance->flags |= _sound_permutation_pending_bit;
                            instance->permutation_index = new_permutation;
                        }
                    } else {
                        instance->play_state = _sound_play_loop_end;
                        owner_state->finished = 1;
                    }
                } else {
                    instance->flags |= _sound_permutation_pending_bit;
                    instance->permutation_index = new_permutation;
                }
            }
        } else {
            instances::apply_pending_definition_switch(sound_handle);
            definition = (Sound *)halo::cache::globals().tag_instances[instance->definition_index & halo::k_slot_mask].data;
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        }

        permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
        if (instance->play_state != _sound_play_loop_end &&
            halo::cache::sound_cache_touch(1, 1, 0, permutation)) {
            instance->flags &= ~_sound_permutation_pending_bit;
            channels::set_next_permutation(channel_index, permutation, instance->first_person,
                definition->sound_class, 1);
            if (instance->pending_definition_index == halo::k_dword_none && permutation->next_permutation_index == halo::k_word_none) {
                if (instance->play_state == _sound_play_loop_start) {
                    instance->play_state = _sound_play_loop;
                } else if (instance->play_state == _sound_play_loop_stopping) {
                    instance->play_state = _sound_play_loop_end;
                }
            }
        }
    }

    permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
    params.gain = params.gain * permutation->gain;
    sound_channel_parameters_proc_ptr(channel_index, &params, 0, definition->sound_class);
    audio_device().channel_continue(channel_index, streaming_flag, definition->sound_class);
}

datum_index find_by_owner(int32_t owner)
{
    datum_index handle;
    looping_sound *state;

    handle = halo::memory::datum_next(-1, looping_sound_data);
    while (handle != halo::k_dword_none) {
        state = (looping_sound *)((uint8_t *)looping_sound_data->data + (handle & halo::k_slot_mask) * sizeof(looping_sound));
        if (state->owner == owner) {
            return handle;
        }
        handle = halo::memory::datum_next((int16_t)handle, looping_sound_data);
    }
    return 0xffffffff;
}

void check_audibility_gate(datum_index definition_index, sound_location *location)
{
    SoundLooping *definition;
    int16_t i;

    if (!sound_looping_audibility_check || location->type != _sound_location_absolute) {
        return;
    }

    definition = (SoundLooping *)halo::cache::globals().tag_instances[definition_index & halo::k_slot_mask].data;

    if ((int32_t)definition->tracks.count > 0) {
        SoundLoopingTrack *first_track = (SoundLoopingTrack *)definition->tracks.pointer;

        for (i = 0; i < (int32_t)definition->tracks.count; i++) {
            if (halo::tag_id_bits(first_track->loop.tag_id) != halo::k_dword_none) {
                Sound *loop_sound = (Sound *)halo::cache::globals().tag_instances[first_track->loop.tag_id.index].data;

                if (loop_sound->minimum_distance != 0.0f) {
                    return;
                }
                if (sound_class_definitions[loop_sound->sound_class].default_minimum_distance != 0.0f) {
                    return;
                }
                break;
            }
        }
    }

    if ((int32_t)definition->detail_sounds.count > 0) {
        SoundLoopingDetail *first_detail = (SoundLoopingDetail *)definition->detail_sounds.pointer;

        for (i = 0; i < (int32_t)definition->detail_sounds.count; i++) {
            if (halo::tag_id_bits(first_detail->sound.tag_id) != halo::k_dword_none) {
                return;
            }
        }
    }
}

}  // namespace looping


}  // namespace halo::sound
