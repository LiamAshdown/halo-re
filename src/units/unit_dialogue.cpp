#include "halo/units/unit.hpp"
#include "game.h"

extern "C" {
extern data_array *object_data;
extern tag_instance *tag_instances;
extern int32_t unit_dialogue_variant_counter;
extern random_seed random_seed_global;
}

namespace halo::units {

/**
 * Engine function unit_choose_dialogue_variant.
 *
 * Original register convention: in_EAX -> unit_index.
 *
 * @address 0x561990
 */
void UnitView::choose_dialogue_variant()
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Unit *unit_tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t permutation_group = *(int16_t *)((uint8_t *)obj + 0xbe);

    TagID chosen;
    if (permutation_group > 0) {
        chosen = ::halo::units::unit_pick_random_dialogue_variant(unit_tag, permutation_group);
        if (*(uint32_t *)&chosen != (uint32_t)-1) {
            goto done;
        }
    }
    chosen = ::halo::units::unit_pick_random_dialogue_variant(unit_tag, 0);
    if (*(uint32_t *)&chosen == (uint32_t)-1) {
        chosen = ::halo::units::unit_pick_random_dialogue_variant(unit_tag, -1);
    }

done:
    unit->dialogue_tag_index = *(datum_index *)&chosen;
}

/**
 * Engine function unit_commit_speech.
 *
 * Original register convention: see file header.
 *
 * @address 0x560f20
 */
int32_t UnitView::commit_speech(const unit_speech *source, int16_t mode)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if ((obj->vitality_flags & _object_health_frozen_bit) == 0 || source->priority == 10) {
        if (mode > 1) {
            unit->current_speech = *source;

            if (mode == 3 && unit->pending_speech.priority > 0) {
                unit->pending_speech.priority = 0;
            }
            unit->speech_started = 0;
            unit->speech_lipsync_stopped = 0;
            unit->speech_finished = 0;
            unit->speech_tail_ticks = unit->current_speech.tail_ticks;
            unit->speech_sound_handle = (datum_index)-1;
            unit->speech_delay_ticks = unit->current_speech.delay_ticks;
            unit->speech_lipsync_ticks = unit->current_speech.lipsync_ticks;

            if (unit->current_speech.sound_tag == (datum_index)-1) {
                unit->speech_duration_ticks = 0x2d;
                return -1;
            }

            Sound *sound_tag = (Sound *)tag_instances[unit->current_speech.sound_tag & 0xffff].data;
            int32_t length = *(int32_t *)((uint8_t *)sound_tag + 0x84) * 0x1e;
            unit->speech_duration_ticks = (int16_t)(length / 1000);
            return (int32_t)((int64_t)length * 0x10624dd3);
        } else if (mode == 1) {
            unit->pending_speech = *source;
        }
    }
    return (int32_t)((unit_index & 0xffff) * 3);
}

/**
 * Gives a new unit a dialogue variant when its placement did not choose one.
 *
 * @address 0x5618e0
 */
void UnitView::dialogue_determine_variant()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    int16_t *variant = (int16_t *)((uint8_t *)obj + 0xbe);
    int16_t candidates[16];
    uint16_t count = 0;
    int16_t i;

    if (*variant != 0) {
        return;
    }
    for (i = 0; (int32_t)i < (int32_t)tag->dialogue_variants.count; i++) {
        int16_t number = ((UnitDialogueVariant *)tag->dialogue_variants.pointer)[i].variant_number;
        if (number < 100) {
            if (count >= 16) {
                break;
            }
            candidates[(int16_t)count] = number;
            count++;
        }
    }
    if ((int16_t)count > 0) {
        int32_t pick = unit_dialogue_variant_counter % (int32_t)(int16_t)count;
        unit_dialogue_variant_counter++;
        *variant = candidates[pick];
    }
}

/**
 * Engine function unit_pick_random_dialogue_variant.
 *
 * Original register convention: see file header.
 *
 * @address 0x561a00
 */
TagID unit_pick_random_dialogue_variant(Unit *unit_tag, int16_t variant_number)
{
    UnitDialogueVariant *variants = (UnitDialogueVariant *)unit_tag->dialogue_variants.pointer;
    int32_t count = (int32_t)unit_tag->dialogue_variants.count;
    int16_t matches[16];
    int16_t match_count = 0;

    if (count > 0) {
        for (int16_t i = 0; i < count; i++) {
            if (variant_number == -1 || variants[i].variant_number == variant_number) {
                matches[match_count] = i;
                match_count = match_count + 1;
            }
        }
        if (match_count > 0) {
            int16_t chosen;
            if (match_count == 1) {
                chosen = matches[0];
            } else {
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                chosen = matches[(int16_t)((random_seed_global >> 0x10) * (uint32_t)match_count >> 0x10)];
            }
            return variants[chosen].dialogue.tag_id;
        }
    }
    TagID none = {0xffff, 0xffff};
    return none;
}

}
