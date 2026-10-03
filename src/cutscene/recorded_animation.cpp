#include "halo/cutscene/recorded_animation.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/flags.hpp"
#include "halo/memory/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/scenario/api.hpp"

extern "C" {
extern data_array *recorded_animations;
extern data_array *object_data;
extern recorded_animation_codec *recorded_animation_codecs_by_version[4];
extern int32_t player_index_from_unit_index(uint32_t unit_index);
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_handle, uint8_t attaching);
extern uint8_t unit_get_flag_bit6(datum_index unit_index);
extern void object_set_in_pvs_pass_flag(uint32_t object_index, uint8_t in_pvs);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void unit_apply_control_block(uint32_t unit_index, const unit_control_data *control, int32_t source_id);
extern char hs_object_hierarchy_test(datum_index object_index);
extern void object_delete(datum_index object_index);
extern void object_get_position(real_point3d *out, uint32_t object_index);
}

namespace halo::cutscene {

/**
 * Original function recorded_animation_start; the author notes are in
 * docs/original/cutscene/recorded_animation_start.c.txt.
 *
 * Register convention in the original: unit_index in EAX (in_EAX), scenario
 * recorded_animations index in CX (in_CX, 16-bit); one plain stack argument, extra_flags (a
 * uint16_t ORed into the record's flags at the end).
 *
 * @address 0x44a930
 */
uint8_t RecordedAnimationPlayer::start(int16_t scenario_animation_index, uint16_t extra_flags)
{
    datum_index unit_index = (datum_index)unit_handle;

    ScenarioRecordedAnimation *def;
    recorded_animation *record;
    datum_index existing_index;
    datum_index new_index;
    unit_data *unit;
    uint8_t version;

    if (unit_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    if (scenario_animation_index == -1) {
        return 0;
    }
    if ((int32_t)scenario_animation_index >= (int32_t)halo::scenario::globals().scenario->recorded_animations.count) {
        return 0;
    }

    player_index_from_unit_index((uint32_t)unit_index);
    record = halo::cutscene::recorded_animation_find_by_object(unit_index, &existing_index);
    def = (ScenarioRecordedAnimation *)global_scenario->recorded_animations.pointer + scenario_animation_index;

    if (halo::cutscene::recorded_animation_object_is_playing(unit_index) != 0) {
        return 0;
    }

    if (record == (recorded_animation *)0) {
        new_index = halo::memory::datum_new(recorded_animations);
        if (new_index == (datum_index)k_datum_index_none) {
            return 0;
        }
        record = &((recorded_animation *)recorded_animations->data)[halo::datum_slot(new_index)];
        if (record == (recorded_animation *)0) {
            return 0;
        }
    }

    record->unit_index = unit_index;
    record->event_ticks = 0;
    record->ticks_remaining = def->length_of_animation;
    record->event_cursor = (uint8_t *)def->recorded_animation_event_stream.pointer;
    version = (uint8_t)def->version;
    record->flags = record->flags & ~(uint16_t)_recorded_animation_flag_finished;
    record->codec_index = (int16_t)(version - 1);
    recorded_animation_codecs_by_version[record->codec_index]->begin(&record->decoder_state,
        &record->control_data, &record->event_cursor, (uint8_t)def->unit_control_data_version);

    unit_refresh_targeting_flag_and_weapons(unit_index, 1);
    if (unit_get_flag_bit6(unit_index) != 0) {
        record->flags = record->flags | _recorded_animation_flag_restore_object_flag_40;
    } else {
        record->flags = record->flags & ~(uint16_t)_recorded_animation_flag_restore_object_flag_40;
    }

    unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data + k_unit_data_offset);
    unit->flags = unit->flags & ~to_bits(unit_playback_flags::restore_marker);
    unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[halo::datum_slot(unit_index)].data + k_unit_data_offset);
    unit->flags = unit->flags | _unit_flag_unknown_8000000;

    object_set_in_pvs_pass_flag(unit_index, 0);
    record->flags = record->flags | extra_flags;
    return 1;
}

/**
 * Original function recorded_animation_object_is_playing; the author notes are in
 * docs/original/cutscene/recorded_animation_object_is_playing.c.txt.
 *
 * Register convention in the original: ESI = unit_index (unaff_ESI), never modified by this
 * function.
 *
 * @address 0x44acc0
 */
uint8_t RecordedAnimationPlayer::is_playing()
{
    datum_index unit_index = (datum_index)unit_handle;

    data_iterator iterator;
    recorded_animation *entry;

    iterator.data = recorded_animations;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    entry = (recorded_animation *)halo::memory::data_iterator_next(&iterator);
    for (;;) {
        if (entry == (recorded_animation *)0) {
            return 0;
        }
        if ((entry->unit_index == unit_index) &&
            ((entry->flags & _recorded_animation_flag_finished) == 0)) {
            break;
        }
        entry = (recorded_animation *)halo::memory::data_iterator_next(&iterator);
    }
    return 1;
}

/**
 * Original function recorded_animation_find_by_object; the author notes are in
 * docs/original/cutscene/recorded_animation_find_by_object.c.txt.
 *
 * Register convention in the original: `objdump -d -M intel --start-address=0x44ad20 --stop-
 * address=0x44ad80 bin/halo.exe`: EBX = unit_index to search for (unaff_EBX); one plain stack
 * argument (out_index, an optional datum_index * output).
 *
 * @address 0x44ad20
 */
recorded_animation * RecordedAnimationPlayer::find_by_object(datum_index *out_index)
{
    datum_index unit_index = (datum_index)unit_handle;

    data_iterator iterator;
    recorded_animation *entry;
    datum_index found_index = (datum_index)k_datum_index_none;

    iterator.data = recorded_animations;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    entry = (recorded_animation *)halo::memory::data_iterator_next(&iterator);
    while ((entry != (recorded_animation *)0) && (entry->unit_index != unit_index)) {
        entry = (recorded_animation *)halo::memory::data_iterator_next(&iterator);
    }
    if (entry != (recorded_animation *)0) {
        found_index = iterator.index;
    }
    if (out_index != (datum_index *)0) {
        *out_index = found_index;
    }
    return entry;
}

/**
 * Original function recorded_animation_find_by_name; the author notes are in
 * docs/original/cutscene/recorded_animation_find_by_name.c.txt.
 *
 * Register convention in the original: EBX = name (in_EBX), ESI = scenario (in_ESI); blam-cc:
 * (EBX, ESI) ->.
 *
 * @address 0x449f80
 */
int16_t RecordedAnimationPlayer::find_by_name(const char *name, Scenario *scenario)
{
    int16_t index;

    index = 0;
    if (0 < (int32_t)scenario->recorded_animations.count) {
        do {
            ScenarioRecordedAnimation *entries =
                (ScenarioRecordedAnimation *)scenario->recorded_animations.pointer;
            if (_stricmp(entries[index].name.string, name) == 0) {
                return index;
            }
            index += 1;
        } while ((int32_t)index < (int32_t)scenario->recorded_animations.count);
    }
    return -1;
}

/**
 * Ticks every live recorded_animation once: validates its unit (a biped or vehicle only,
 * type_mask 3), deletes the record if the unit is gone or of the wrong type, otherwise either
 * tears it down (when its finished flag is set: restores the biped "jumping"/PVS flags,
 * unlinks from device group tracking, optionally deletes the unit if it is no longer part of
 * the object hierarchy, optionally re-marks the unit's cached position, then deletes the
 * record) or advances it one tick (runs its codec's update, applies the decoded control block
 * to the unit, and sets or.
 *
 * Register convention in the original: no arguments; builds its own local data_iterator over
 * recorded_animations (same layout as its siblings recorded_animation_find_by_object /
 * recorded_animation_object_is_playing).
 *
 * @address 0x44aa90
 */
void RecordedAnimationPlayer::update_all()
{
    data_iterator iterator;
    recorded_animation *record;

    iterator.data = recorded_animations;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    record = (recorded_animation *)halo::memory::data_iterator_next(&iterator);
    while (record != (recorded_animation *)0) {
        object *unit_object = object_try_and_get(record->unit_index, 0x3);

        if (unit_object == (object *)0) {
            halo::memory::datum_delete(recorded_animations, iterator.index);
        } else if ((record->flags & _recorded_animation_flag_finished) != 0) {
            object_header *header = &((object_header *)object_data->data)[halo::datum_slot(record->unit_index)];
            unit_data *unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);

            if ((record->flags & _recorded_animation_flag_restore_object_flag_40) != 0) {
                unit->flags = unit->flags | to_bits(unit_playback_flags::restore_marker);
            } else {
                unit->flags = unit->flags & ~0x00000040u;
            }
            header = &((object_header *)object_data->data)[halo::datum_slot(record->unit_index)];
            unit = (unit_data *)((uint8_t *)header->data + k_unit_data_offset);
            unit->flags = unit->flags & ~(uint32_t)_unit_flag_unknown_8000000; 

            unit_refresh_targeting_flag_and_weapons(record->unit_index, 0);

            header = &((object_header *)object_data->data)[halo::datum_slot(record->unit_index)];
            header->flags = header->flags | _object_header_in_pvs_pass_bit;
            
            
            
            if ((header->data->parent_object == (datum_index)k_datum_index_none) &&
                (header->data->location_cluster_index == -1)) {
                if ((header->flags & _object_header_active_bit) != 0) {
                    header->flags = header->flags & ~(uint8_t)_object_header_active_bit;
                }
            }

            if (((record->flags & _recorded_animation_flag_delete_object_when_finished) != 0) && (record->unit_index != (datum_index)k_datum_index_none)) {
                if (hs_object_hierarchy_test(record->unit_index) == 0) {
                    object_delete(record->unit_index);
                }
            }
            if (((record->flags & _recorded_animation_flag_mark_object_when_finished) != 0) && (record->unit_index != (datum_index)k_datum_index_none)) {
                object *obj = ((object_header *)object_data->data)[halo::datum_slot(record->unit_index)].data;
                biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);
                object_get_position((real_point3d *)&biped->bump_object_index, record->unit_index);
                biped->flags = biped->flags | to_bits(biped_playback_flags::jumping);
            }
            halo::memory::datum_delete(recorded_animations, iterator.index);
        } else {
            recorded_animation_codec *codec;
            uint8_t not_finished;

            record->ticks_remaining = record->ticks_remaining - 1;
            codec = recorded_animation_codecs_by_version[record->codec_index];
            not_finished = codec->update(&record->decoder_state, &record->control_data,
                &record->event_ticks, &record->event_cursor);
            record->event_ticks = record->event_ticks + 1;
            unit_apply_control_block((uint32_t)record->unit_index, &record->control_data, -1);
            if (not_finished == 0) {
                record->flags = record->flags | _recorded_animation_flag_finished;
            } else {
                record->flags = record->flags & ~(uint16_t)_recorded_animation_flag_finished;
            }
        }
        record = (recorded_animation *)halo::memory::data_iterator_next(&iterator);
    }
}

}

namespace halo::cutscene {

uint8_t recorded_animation_start(datum_index unit_index, int16_t scenario_animation_index, uint16_t extra_flags)
{
    return halo::cutscene::RecordedAnimationPlayer(unit_index).start(scenario_animation_index, extra_flags);
}

uint8_t recorded_animation_object_is_playing(datum_index unit_index)
{
    return halo::cutscene::RecordedAnimationPlayer(unit_index).is_playing();
}

recorded_animation *recorded_animation_find_by_object(datum_index unit_index, datum_index *out_index)
{
    return halo::cutscene::RecordedAnimationPlayer(unit_index).find_by_object(out_index);
}

int16_t recorded_animation_find_by_name(const char *name, Scenario *scenario)
{
    return halo::cutscene::RecordedAnimationPlayer::find_by_name(name, scenario);
}

void recorded_animations_update(void)
{
    halo::cutscene::RecordedAnimationPlayer::update_all();
}

}
