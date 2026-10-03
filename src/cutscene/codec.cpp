#include "halo/cutscene/codec.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/core/link.hpp"
#include "halo/cutscene/vars.hpp"
#include "halo/core/libm.hpp"

static auto &recorded_animation_angle_scale = halo::link::ref<float>(halo::cutscene::vars().recorded_animation_angle_scale);
namespace {
/** Field layouts of the recorded unit control data, one table per stream version (version 1 uses table 0 only, version n >= 2 uses tables 0..n-1). */
unit_control_data_field_layout k_control_layout_v0[] = {
    { nullptr, 1, 0x00 }, { nullptr, 1, 0x01 }, { nullptr, 2, 0x02 }, { nullptr, 2, 0x04 }, { nullptr, 2, -1 },
    { nullptr, 8, 0x0c }, { nullptr, 12, 0x1c }, { nullptr, 12, 0x28 }, { nullptr, 12, 0x34 }, { nullptr, -1, -1 },
};
unit_control_data_field_layout k_control_layout_v1[] = { { nullptr, 4, 0x18 }, { nullptr, -1, -1 } };
unit_control_data_field_layout k_control_layout_v2[] = { { nullptr, 2, 0x06 }, { nullptr, -1, -1 } };
unit_control_data_field_layout k_control_layout_v3[] = { { nullptr, 2, 0x08 }, { nullptr, -1, -1 } };
unit_control_data_field_layout *const unit_control_data_version_layouts[4] = { k_control_layout_v0, k_control_layout_v1, k_control_layout_v2, k_control_layout_v3 };
}
static auto &recorded_animation_compressed_event_handlers = halo::link::ref<recorded_animation_compressed_event_proc [k_recorded_animation_event_type_count]>(halo::cutscene::vars().recorded_animation_compressed_event_handlers);
static auto &recorded_animation_v1_event_handlers = halo::link::ref<recorded_animation_v1_event_proc [k_recorded_animation_event_type_count]>(halo::cutscene::vars().recorded_animation_v1_event_handlers);

namespace halo::cutscene {

/**
 * Original function recorded_animation_angle_to_vector; the author notes are in
 * docs/original/cutscene/recorded_animation_angle_to_vector.c.txt.
 *
 * Register convention in the original: EAX = out (in_EAX), ECX = angles (in_ECX).
 *
 * @address 0x44a190
 */
void RecordedAngles::angle_to_vector(real_vector3d *out, recorded_animation_angles *angles)
{
    double cos_pitch;
    double cos_yaw;
    double sin_yaw;

    cos_pitch = halo::libm::cos((double)angles->pitch * (double)recorded_animation_angle_scale);
    cos_yaw = halo::libm::cos((double)angles->yaw * (double)recorded_animation_angle_scale);
    out->i = (float)(cos_yaw * cos_pitch);
    sin_yaw = halo::libm::sin((double)angles->yaw * (double)recorded_animation_angle_scale);
    out->j = (float)(sin_yaw * cos_pitch);
    out->k = (float)halo::libm::sin((double)angles->pitch * (double)recorded_animation_angle_scale);
}

/**
 * Original function recorded_animation_apply_char_difference; the author notes are in
 * docs/original/cutscene/recorded_animation_apply_char_difference.c.txt.
 *
 * Register convention in the original: EAX = angles (in_EAX), EDX = delta (in_EDX).
 *
 * @address 0x44a110
 */
void RecordedAngles::apply_char_difference(recorded_animation_angles *angles, recorded_animation_char_difference *delta)
{
    int16_t yaw;

    angles->yaw = angles->yaw + (int16_t)delta->yaw;
    yaw = angles->yaw;
    if (1000 < yaw) {
        angles->yaw = yaw - 1000;
        angles->pitch = angles->pitch + (int16_t)delta->pitch;
        return;
    }
    if (yaw < -1000) {
        angles->yaw = yaw + 1000;
    }
    angles->pitch = angles->pitch + (int16_t)delta->pitch;
}

/**
 * Original function recorded_animation_apply_short_difference; the author notes are in
 * docs/original/cutscene/recorded_animation_apply_short_difference.c.txt.
 *
 * Register convention in the original: EAX = angles (in_EAX), EDX = delta (in_EDX).
 *
 * @address 0x44a150
 */
void RecordedAngles::apply_short_difference(recorded_animation_angles *angles, recorded_animation_short_difference *delta)
{
    int16_t yaw;

    angles->yaw = angles->yaw + delta->yaw;
    yaw = angles->yaw;
    if (1000 < yaw) {
        angles->yaw = yaw - 1000;
        angles->pitch = angles->pitch + delta->pitch;
        return;
    }
    if (yaw < -1000) {
        angles->yaw = yaw + 1000;
    }
    angles->pitch = angles->pitch + delta->pitch;
}

/**
 * Original function unit_control_data_unpack; the author notes are in
 * docs/original/cutscene/unit_control_data_unpack.c.txt.
 *
 * Register convention in the original: EBX = control (unaff_EBX); stack (cursor, version) in
 * Ghidra's own.
 *
 * @address 0x449fd0
 */
void UnitControlUnpacker::unpack(unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    int32_t i;
    int32_t table_count;
    int32_t table_index;

    for (i = 0; i < (int32_t)(sizeof(unit_control_data) / 4); i += 1) {
        ((uint32_t *)control)[i] = 0;
    }
    control->zoom_level = -1;

    table_count = (version < 2) ? 1 : (int32_t)version;
    for (table_index = 0; table_index < table_count; table_index += 1) {
        unit_control_data_field_layout *record = unit_control_data_version_layouts[table_index];

        while (record->size != -1) {
            if (record->offset != -1) {
                uint8_t *src = *cursor;
                uint8_t *dst = (uint8_t *)control + record->offset;
                int32_t size = record->size;
                int32_t dwords = size >> 2;
                int32_t bytes = size & 3;

                while (dwords != 0) {
                    *(uint32_t *)dst = *(uint32_t *)src;
                    src += 4;
                    dst += 4;
                    dwords -= 1;
                }
                while (bytes != 0) {
                    *dst = *src;
                    src += 1;
                    dst += 1;
                    bytes -= 1;
                }
            }
            *cursor += record->size;
            record += 1;
        }
    }
}

/**
 * Original function recorded_animation_compressed_begin; the author notes are in
 * docs/original/cutscene/recorded_animation_compressed_begin.c.txt.
 *
 * Register convention in the original: objdump-traced call site 0x44a930
 * (recorded_animation_start) pushes.
 *
 * @address 0x44a550
 */
void CompressedCodec::begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    uint32_t *source;
    uint32_t *destination;

    halo::cutscene::unit_control_data_unpack(control, cursor, version);

    source = (uint32_t *)*cursor;
    destination = (uint32_t *)state;
    destination[0] = source[0];
    destination[1] = source[1];
    destination[2] = source[2];
    *cursor += 0x0c;
}

/**
 * Original function recorded_animation_compressed_update; the author notes are in
 * docs/original/cutscene/recorded_animation_compressed_update.c.txt.
 *
 * Register convention in the original: recorded_animation_update_proc (state, control,
 * event_ticks, cursor), all.
 *
 * @address 0x44a590
 */
uint8_t CompressedCodec::update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor)
{
    uint8_t *event;
    int32_t delay;
    int32_t header_bytes;
    recorded_animation_compressed_event_proc handler;

    for (;;) {
        event = *cursor;

        switch (*event & k_recorded_animation_compressed_delay_mask) {
        case _recorded_animation_delay_none:
            delay = 0;
            header_bytes = 1;
            break;
        case _recorded_animation_delay_one_tick:
            delay = 1;
            header_bytes = 1;
            break;
        case _recorded_animation_delay_byte:
            delay = event[1];
            header_bytes = 2;
            break;
        default: 
            delay = *(uint16_t *)(event + 1);
            header_bytes = 3;
            break;
        }

        if (*event_ticks < delay || (*event & 0xfc) == 4) {
            break;
        }

        *cursor = event + header_bytes;
        handler = recorded_animation_compressed_event_handlers[*event >> k_recorded_animation_compressed_type_shift];
        if (handler != (recorded_animation_compressed_event_proc)0) {
            handler(state, control, event, cursor);
        }
        *event_ticks -= delay;
    }

    if ((*event & 0xfc) == 4 && *event_ticks == delay) {
        return 0;
    }
    return 1;
}

/**
 * Original function recorded_animation_decode_aiming_speed_event; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_aiming_speed_event.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (state, control, header,
 * cursor), same slots as.
 *
 * @address 0x44a080
 */
void CompressedCodec::decode_aiming_speed_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->aiming_speed = *(int8_t *)*cursor;
    *cursor += 1;
}

/**
 * Original function recorded_animation_decode_animation_state_event; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_animation_state_event.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (state, control, header,
 * cursor), same slots as.
 *
 * @address 0x44a060
 */
void CompressedCodec::decode_animation_state_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->animation_state = *(int8_t *)*cursor;
    *cursor += 1;
}

/**
 * Original function recorded_animation_decode_char_difference_event; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_char_difference_event.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (state, control, header,
 * cursor), matching.
 *
 * @address 0x44a1d0
 */
void CompressedCodec::decode_char_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    recorded_animation_char_difference *delta;
    uint8_t mask;
    uint8_t facing_bit;
    int16_t yaw;
    int16_t pitch;
    double cos_pitch;
    double cos_yaw;

    delta = (recorded_animation_char_difference *)*cursor;
    mask = (uint8_t)((*header >> 2) - 7);
    facing_bit = mask & 1;

    if (facing_bit != 0) {
        state->facing.yaw = state->facing.yaw + (int16_t)delta->yaw;
        yaw = state->facing.yaw;
        if (1000 < yaw) {
            yaw = yaw - 1000;
            state->facing.yaw = yaw;
        } else if (yaw < -1000) {
            yaw = yaw + 1000;
            state->facing.yaw = yaw;
        }
        state->facing.pitch = state->facing.pitch + (int16_t)delta->pitch;
        yaw = state->facing.yaw;
        pitch = state->facing.pitch;
        cos_pitch = halo::libm::cos((double)pitch * (double)recorded_animation_angle_scale);
        cos_yaw = halo::libm::cos((double)yaw * (double)recorded_animation_angle_scale);
        control->facing_vector.i = (float)(cos_yaw * cos_pitch);
        control->facing_vector.j = (float)(halo::libm::sin((double)yaw * (double)recorded_animation_angle_scale) * cos_pitch);
        control->facing_vector.k = (float)halo::libm::sin((double)pitch * (double)recorded_animation_angle_scale);
    }

    if ((mask & 2) != 0) {
        if (facing_bit == 0) {
            halo::cutscene::recorded_animation_apply_char_difference(&state->aiming, delta);
            yaw = state->aiming.yaw;
            pitch = state->aiming.pitch;
            cos_pitch = halo::libm::cos((double)pitch * (double)recorded_animation_angle_scale);
            cos_yaw = halo::libm::cos((double)yaw * (double)recorded_animation_angle_scale);
            control->aiming_vector.i = (float)(cos_yaw * cos_pitch);
            control->aiming_vector.j = (float)(halo::libm::sin((double)yaw * (double)recorded_animation_angle_scale) * cos_pitch);
            control->aiming_vector.k = (float)halo::libm::sin((double)pitch * (double)recorded_animation_angle_scale);
        } else {
            state->aiming = state->facing;
            control->aiming_vector = control->facing_vector;
        }
    }

    if ((mask & 4) != 0) {
        if (facing_bit != 0) {
            state->looking = state->facing;
            control->looking_vector = control->facing_vector;
            *cursor += 2;
            return;
        }
        if ((mask & 2) != 0) {
            state->looking = state->aiming;
            control->looking_vector = control->aiming_vector;
            *cursor += 2;
            return;
        }
        halo::cutscene::recorded_animation_apply_char_difference(&state->looking, delta);
        halo::cutscene::recorded_animation_angle_to_vector(&control->looking_vector, &state->looking);
    }
    *cursor += 2;
}

/**
 * Original function recorded_animation_decode_control_flags_event; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_control_flags_event.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (state, control, header,
 * cursor); objdump confirms.
 *
 * @address 0x44a0a0
 */
void CompressedCodec::decode_control_flags_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->control_flags = *(uint16_t *)*cursor;
    *cursor += 2;
}

/**
 * Original function recorded_animation_decode_short_difference_event; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_short_difference_event.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (state, control, header,
 * cursor), matching.
 *
 * @address 0x44a390
 */
void CompressedCodec::decode_short_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    recorded_animation_short_difference *delta;
    uint8_t mask;
    uint8_t facing_bit;
    int16_t yaw;
    int16_t pitch;
    double cos_pitch;
    double cos_yaw;

    delta = (recorded_animation_short_difference *)*cursor;
    mask = (uint8_t)((*header >> 2) - 0xf);
    facing_bit = mask & 1;

    if (facing_bit != 0) {
        state->facing.yaw = state->facing.yaw + delta->yaw;
        yaw = state->facing.yaw;
        if (1000 < yaw) {
            yaw = yaw - 1000;
            state->facing.yaw = yaw;
        } else if (yaw < -1000) {
            yaw = yaw + 1000;
            state->facing.yaw = yaw;
        }
        state->facing.pitch = state->facing.pitch + delta->pitch;
        yaw = state->facing.yaw;
        pitch = state->facing.pitch;
        cos_pitch = halo::libm::cos((double)pitch * (double)recorded_animation_angle_scale);
        cos_yaw = halo::libm::cos((double)yaw * (double)recorded_animation_angle_scale);
        control->facing_vector.i = (float)(cos_yaw * cos_pitch);
        control->facing_vector.j = (float)(halo::libm::sin((double)yaw * (double)recorded_animation_angle_scale) * cos_pitch);
        control->facing_vector.k = (float)halo::libm::sin((double)pitch * (double)recorded_animation_angle_scale);
    }

    if ((mask & 2) != 0) {
        if (facing_bit == 0) {
            halo::cutscene::recorded_animation_apply_short_difference(&state->aiming, delta);
            yaw = state->aiming.yaw;
            pitch = state->aiming.pitch;
            cos_pitch = halo::libm::cos((double)pitch * (double)recorded_animation_angle_scale);
            cos_yaw = halo::libm::cos((double)yaw * (double)recorded_animation_angle_scale);
            control->aiming_vector.i = (float)(cos_yaw * cos_pitch);
            control->aiming_vector.j = (float)(halo::libm::sin((double)yaw * (double)recorded_animation_angle_scale) * cos_pitch);
            control->aiming_vector.k = (float)halo::libm::sin((double)pitch * (double)recorded_animation_angle_scale);
        } else {
            state->aiming = state->facing;
            control->aiming_vector = control->facing_vector;
        }
    }

    if ((mask & 4) != 0) {
        if (facing_bit != 0) {
            state->looking = state->facing;
            control->looking_vector = control->facing_vector;
            *cursor += 4;
            return;
        }
        if ((mask & 2) != 0) {
            state->looking = state->aiming;
            control->looking_vector = control->aiming_vector;
            *cursor += 4;
            return;
        }
        halo::cutscene::recorded_animation_apply_short_difference(&state->looking, delta);
        halo::cutscene::recorded_animation_angle_to_vector(&control->looking_vector, &state->looking);
    }
    *cursor += 4;
}

/**
 * Original function recorded_animation_decode_throttle_event; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_throttle_event.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (state, control, header,
 * cursor); objdump confirms.
 *
 * @address 0x44a0e0
 */
void CompressedCodec::decode_throttle_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    uint32_t *source;

    source = (uint32_t *)*cursor;
    *(uint32_t *)&control->throttle.i = source[0];
    *(uint32_t *)&control->throttle.j = source[1];
    control->throttle.k = 0.0f;
    *cursor += 8;
}

/**
 * Original function recorded_animation_decode_weapon_index_event; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_weapon_index_event.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (state, control, header,
 * cursor); objdump confirms.
 *
 * @address 0x44a0c0
 */
void CompressedCodec::decode_weapon_index_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    control->weapon_index = *(int16_t *)*cursor;
    *cursor += 2;
}

/**
 * Original function recorded_animation_v1_begin; the author notes are in
 * docs/original/cutscene/recorded_animation_v1_begin.c.txt.
 *
 * Register convention in the original: same recorded_animation_begin_proc shape and call site
 * as.
 *
 * @address 0x44a890
 */
void LegacyCodec::begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    (void)state;
    halo::cutscene::unit_control_data_unpack(control, cursor, version);
}

/**
 * Original function recorded_animation_v1_update; the author notes are in
 * docs/original/cutscene/recorded_animation_v1_update.c.txt.
 *
 * Register convention in the original: recorded_animation_update_proc (state, control,
 * event_ticks, cursor),.
 *
 * @address 0x44a8b0
 */
uint8_t LegacyCodec::update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor)
{
    recorded_animation_event_v1 *event;
    recorded_animation_v1_event_proc handler;

    (void)state;

    event = (recorded_animation_event_v1 *)*cursor;

    while (event->type != _recorded_animation_event_end && event->delay_ticks <= *event_ticks) {
        handler = recorded_animation_v1_event_handlers[event->type];
        if (handler != (recorded_animation_v1_event_proc)0) {
            handler(control, event, cursor);
        } else {
            *cursor = (uint8_t *)(event + 1);
        }
        *event_ticks -= event->delay_ticks;
        event = (recorded_animation_event_v1 *)*cursor;
    }

    if (event->type == _recorded_animation_event_end && *event_ticks == event->delay_ticks) {
        return 0;
    }
    return 1;
}

/**
 * Original function recorded_animation_decode_aiming_speed_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_aiming_speed_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a670
 */
void LegacyCodec::decode_aiming_speed_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_aiming_speed_set_event_v1 *set_event =
        (recorded_animation_aiming_speed_set_event_v1 *)event;

    control->aiming_speed = set_event->aiming_speed;
    *cursor += sizeof(recorded_animation_aiming_speed_set_event_v1);
}

/**
 * Original function recorded_animation_decode_aiming_vector_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_aiming_vector_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a730
 */
void LegacyCodec::decode_aiming_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_multi_vector_set_event_v1 *vector_event =
        (recorded_animation_multi_vector_set_event_v1 *)event;

    control->aiming_vector = vector_event->vector;
    *cursor += sizeof(recorded_animation_multi_vector_set_event_v1);
}

/**
 * Original function recorded_animation_decode_angle_vector_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_angle_vector_event_v1.c.txt.
 *
 * Register convention in the original: cdecl stack parameters (control, event, cursor),
 * matching Ghidra's own.
 *
 * @address 0x44a790
 */
void LegacyCodec::decode_angle_vector_event(unit_control_data *control, recorded_animation_angle_vector_set_event_v1 *event, uint8_t **cursor)
{
    double cos_pitch;
    double cos_yaw;
    float i;
    float j;
    float k;

    cos_pitch = halo::libm::cos((double)event->angles.pitch);
    cos_yaw = halo::libm::cos((double)event->angles.yaw);
    i = (float)(cos_yaw * cos_pitch);
    j = (float)(halo::libm::sin((double)event->angles.yaw) * cos_pitch);
    k = (float)halo::libm::sin((double)event->angles.pitch);

    if (event->header.type != 0x15) {
        control->facing_vector.i = i;
        control->facing_vector.j = j;
        control->facing_vector.k = k;
    }
    if (event->header.type != 0x14) {
        control->aiming_vector.i = i;
        control->aiming_vector.j = j;
        control->aiming_vector.k = k;
    }
    if (event->header.type != 0x13) {
        control->looking_vector.i = i;
        control->looking_vector.j = j;
        control->looking_vector.k = k;
    }
    *cursor += 0xc;
}

/**
 * Original function recorded_animation_decode_animation_state_event_v1; the author notes are
 * in docs/original/cutscene/recorded_animation_decode_animation_state_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a650
 */
void LegacyCodec::decode_animation_state_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_animation_state_set_event_v1 *set_event =
        (recorded_animation_animation_state_set_event_v1 *)event;

    control->animation_state = set_event->animation_state;
    *cursor += sizeof(recorded_animation_animation_state_set_event_v1);
}

/**
 * Original function recorded_animation_decode_control_flags_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_control_flags_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a690
 */
void LegacyCodec::decode_control_flags_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_control_flags_set_event_v1 *set_event =
        (recorded_animation_control_flags_set_event_v1 *)event;

    control->control_flags = set_event->control_flags;
    *cursor += sizeof(recorded_animation_control_flags_set_event_v1);
}

/**
 * Original function recorded_animation_decode_facing_vector_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_facing_vector_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a700
 */
void LegacyCodec::decode_facing_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_multi_vector_set_event_v1 *vector_event =
        (recorded_animation_multi_vector_set_event_v1 *)event;

    control->facing_vector = vector_event->vector;
    *cursor += sizeof(recorded_animation_multi_vector_set_event_v1);
}

/**
 * Original function recorded_animation_decode_looking_vector_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_looking_vector_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a760
 */
void LegacyCodec::decode_looking_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_multi_vector_set_event_v1 *vector_event =
        (recorded_animation_multi_vector_set_event_v1 *)event;

    control->looking_vector = vector_event->vector;
    *cursor += sizeof(recorded_animation_multi_vector_set_event_v1);
}

/**
 * Original function recorded_animation_decode_multi_vector_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_multi_vector_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a820
 */
void LegacyCodec::decode_multi_vector_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_multi_vector_set_event_v1 *vector_event =
        (recorded_animation_multi_vector_set_event_v1 *)event;

    if (event->type != 0xe) {
        control->facing_vector = vector_event->vector;
    }
    if (event->type != 0xd) {
        control->aiming_vector = vector_event->vector;
    }
    if (event->type != 0xc) {
        control->looking_vector = vector_event->vector;
    }
    *cursor += sizeof(recorded_animation_multi_vector_set_event_v1);
}

/**
 * Original function recorded_animation_decode_throttle_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_throttle_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a6d0
 */
void LegacyCodec::decode_throttle_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_throttle_set_event_v1 *set_event =
        (recorded_animation_throttle_set_event_v1 *)event;

    control->throttle.i = set_event->throttle.i;
    control->throttle.j = set_event->throttle.j;
    control->throttle.k = 0.0f;
    *cursor += sizeof(recorded_animation_throttle_set_event_v1);
}

/**
 * Original function recorded_animation_decode_weapon_index_event_v1; the author notes are in
 * docs/original/cutscene/recorded_animation_decode_weapon_index_event_v1.c.txt.
 *
 * Register convention in the original: recorded_animation_v1_event_proc (control, event,
 * cursor), all cdecl.
 *
 * @address 0x44a6b0
 */
void LegacyCodec::decode_weapon_index_event(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    recorded_animation_weapon_index_set_event_v1 *set_event =
        (recorded_animation_weapon_index_set_event_v1 *)event;

    control->weapon_index = set_event->weapon_index;
    *cursor += sizeof(recorded_animation_weapon_index_set_event_v1);
}

}

namespace halo::cutscene {

void recorded_animation_angle_to_vector(real_vector3d *out, recorded_animation_angles *angles)
{
    halo::cutscene::RecordedAngles::angle_to_vector(out, angles);
}

void recorded_animation_apply_char_difference(recorded_animation_angles *angles, recorded_animation_char_difference *delta)
{
    halo::cutscene::RecordedAngles::apply_char_difference(angles, delta);
}

void recorded_animation_apply_short_difference(recorded_animation_angles *angles, recorded_animation_short_difference *delta)
{
    halo::cutscene::RecordedAngles::apply_short_difference(angles, delta);
}

void unit_control_data_unpack(unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    halo::cutscene::UnitControlUnpacker::unpack(control, cursor, version);
}

void recorded_animation_compressed_begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    halo::cutscene::CompressedCodec::begin(state, control, cursor, version);
}

uint8_t recorded_animation_compressed_update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor)
{
    return halo::cutscene::CompressedCodec::update(state, control, event_ticks, cursor);
}

void recorded_animation_decode_aiming_speed_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    halo::cutscene::CompressedCodec::decode_aiming_speed_event(state, control, header, cursor);
}

void recorded_animation_decode_animation_state_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    halo::cutscene::CompressedCodec::decode_animation_state_event(state, control, header, cursor);
}

void recorded_animation_decode_char_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    halo::cutscene::CompressedCodec::decode_char_difference_event(state, control, header, cursor);
}

void recorded_animation_decode_control_flags_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    halo::cutscene::CompressedCodec::decode_control_flags_event(state, control, header, cursor);
}

void recorded_animation_decode_short_difference_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    halo::cutscene::CompressedCodec::decode_short_difference_event(state, control, header, cursor);
}

void recorded_animation_decode_throttle_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    halo::cutscene::CompressedCodec::decode_throttle_event(state, control, header, cursor);
}

void recorded_animation_decode_weapon_index_event(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t *header, uint8_t **cursor)
{
    halo::cutscene::CompressedCodec::decode_weapon_index_event(state, control, header, cursor);
}

void recorded_animation_v1_begin(recorded_animation_decoder_state *state, unit_control_data *control, uint8_t **cursor, uint8_t version)
{
    halo::cutscene::LegacyCodec::begin(state, control, cursor, version);
}

uint8_t recorded_animation_v1_update(recorded_animation_decoder_state *state, unit_control_data *control, int32_t *event_ticks, uint8_t **cursor)
{
    return halo::cutscene::LegacyCodec::update(state, control, event_ticks, cursor);
}

void recorded_animation_decode_aiming_speed_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_aiming_speed_event(control, event, cursor);
}

void recorded_animation_decode_aiming_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_aiming_vector_event(control, event, cursor);
}

void recorded_animation_decode_angle_vector_event_v1(unit_control_data *control, recorded_animation_angle_vector_set_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_angle_vector_event(control, event, cursor);
}

void recorded_animation_decode_animation_state_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_animation_state_event(control, event, cursor);
}

void recorded_animation_decode_control_flags_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_control_flags_event(control, event, cursor);
}

void recorded_animation_decode_facing_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_facing_vector_event(control, event, cursor);
}

void recorded_animation_decode_looking_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_looking_vector_event(control, event, cursor);
}

void recorded_animation_decode_multi_vector_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_multi_vector_event(control, event, cursor);
}

void recorded_animation_decode_throttle_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_throttle_event(control, event, cursor);
}

void recorded_animation_decode_weapon_index_event_v1(unit_control_data *control, recorded_animation_event_v1 *event, uint8_t **cursor)
{
    halo::cutscene::LegacyCodec::decode_weapon_index_event(control, event, cursor);
}

}
