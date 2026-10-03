/**
 * @file include/halo/cutscene/api.hpp
 * Functions of the cutscene module that other modules and the data tables call (namespace halo::cutscene). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct cinematic_globals;
struct cinematic_screen_effect_globals;

struct Scenario;
struct real_vector3d;
struct recorded_animation;
struct recorded_animation_angle_vector_set_event_v1;
struct recorded_animation_angles;
struct recorded_animation_char_difference;
struct recorded_animation_decoder_state;
struct recorded_animation_event_v1;
struct recorded_animation_short_difference;
struct unit_control_data;
typedef uint32_t datum_index;

namespace halo::cutscene {

/**
 * The engine globals the cutscene module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    cinematic_globals *&cinematic_globals;
    float &cinematic_saved_music_gain;
    cinematic_screen_effect_globals *&cinematic_screen_effect_state;
};

Globals &globals();

void chimera__letterbox(void);
void cutscene_start(void);
void cutscene_stop(void);
void cutscene_title_queue(int16_t title_index, float delay_seconds);
void recorded_animation_angle_to_vector(real_vector3d *out, recorded_animation_angles *angles);
void recorded_animation_apply_char_difference(recorded_animation_angles *angles, recorded_animation_char_difference *delta);
void recorded_animation_apply_short_difference(recorded_animation_angles *angles, recorded_animation_short_difference *delta);
void recorded_animation_compressed_begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version);
uint8_t recorded_animation_compressed_update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor);
void recorded_animation_decode_aiming_speed_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
void recorded_animation_decode_aiming_speed_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_aiming_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_angle_vector_event_v1(unit_control_data *control, recorded_animation_angle_vector_set_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_animation_state_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
void recorded_animation_decode_animation_state_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_char_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
void recorded_animation_decode_control_flags_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
void recorded_animation_decode_control_flags_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_facing_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_looking_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_multi_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_short_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
void recorded_animation_decode_throttle_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
void recorded_animation_decode_throttle_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
void recorded_animation_decode_weapon_index_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor);
void recorded_animation_decode_weapon_index_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor);
int16_t recorded_animation_find_by_name(const char *name, Scenario *scenario);
recorded_animation * recorded_animation_find_by_object(datum_index unit_index, datum_index *out_index);
uint8_t recorded_animation_object_is_playing(datum_index unit_index);
uint8_t recorded_animation_start(datum_index unit_index, int16_t scenario_animation_index, uint16_t extra_flags);
void recorded_animation_v1_begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version);
uint8_t recorded_animation_v1_update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor);
void recorded_animations_update(void);
void unit_control_data_unpack(unit_control_data *control, uint8_t **cursor, uint8_t version);

}
