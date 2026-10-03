#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

namespace halo::cutscene {
namespace {

/**
 * Angle helpers shared by the recorded animation codecs: converting packed angles to a vector
 * and applying compressed angle deltas.
 */
class RecordedAngles {
public:
    static void angle_to_vector(real_vector3d *out, recorded_animation_angles *angles);
    static void apply_char_difference(recorded_animation_angles *angles, recorded_animation_char_difference *delta);
    static void apply_short_difference(recorded_animation_angles *angles, recorded_animation_short_difference *delta);
};

/**
 * Unpacks the initial unit control snapshot at the head of a recorded animation stream.
 */
class UnitControlUnpacker {
public:
    static void unpack(unit_control_data *control, uint8_t **cursor, uint8_t version);
};

/**
 * Decoder for the compressed (version 4) recorded animation stream: the begin and per-tick
 * update entry points and one static decoder per event type; the event handler table in the
 * data section points at the C shims of these decoders.
 */
class CompressedCodec {
public:
    static void begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version);
    static uint8_t update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor);
    static void decode_aiming_speed_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
    static void decode_animation_state_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
    static void decode_char_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
    static void decode_control_flags_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
    static void decode_short_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
    static void decode_throttle_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
    static void decode_weapon_index_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
};

/**
 * Decoder for the version 1 recorded animation stream: the begin and per-tick update entry
 * points and one static decoder per event type.
 */
class LegacyCodec {
public:
    static void begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version);
    static uint8_t update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor);
    static void decode_aiming_speed_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_aiming_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_angle_vector_event(unit_control_data *control, recorded_animation_angle_vector_set_event_v1 *event, uint8_t **cursor);
    static void decode_animation_state_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_control_flags_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_facing_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_looking_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_multi_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_throttle_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
    static void decode_weapon_index_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
};

}
}
