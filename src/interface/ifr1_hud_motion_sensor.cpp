#include "halo/interface/ifr1_hud_motion_sensor.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern game_time_globals *game_time;
extern motion_sensor_globals *motion_sensor;
extern player_globals *local_player_globals;
extern data_array *player_data;
extern HUDGlobals *hud_globals_tag_data;
extern game_engine_definition *current_game_engine;
extern float motion_sensor_sweep;
extern float motion_sensor_sweep_scale;
extern double fmod(double x, double y);
extern uint8_t motion_sensor_object_is_detected(datum_index unit_index);
extern void motion_sensor_blip_fill(int16_t local_player_index, datum_index object_index,
                                    motion_sensor_blip *blip);
}

static int16_t motion_sensor_next_local_player(int16_t local_player_index)
{
    return (local_player_globals->local_players[0] != (datum_index)-1 && local_player_index < 0) ? 0 : -1;
}

namespace halo::interface {

/**
 * Original engine function chimera__motion_sensor_update; the author notes are in
 * docs/original/interface/chimera__motion_sensor_update.txt.
 *
 * @address 0x4b3920
 */
void HudMotionSensor::update(void)
{
    float t;
    int32_t now;
    int16_t frame_index;
    int16_t player_count;

    t = (float)fmod((double)((float)game_time->game_time * 0.03333333507180214f), 2.0999999046325684);
    if (t < 2.0374999046325684f) {
        motion_sensor_sweep = 1.0f / ((t + 0.0625f) * motion_sensor_sweep_scale);
    } else {
        motion_sensor_sweep = 0.4f;
    }

    now = game_time->game_time;
    frame_index = (int16_t)((int16_t)(motion_sensor->frame_index + 1) % 10);
    motion_sensor->enabled = 1;
    motion_sensor->update_time = now;
    motion_sensor->frame_index = frame_index;

    if (now % 15 != 0 && now != 0) {
        int16_t previous = (int16_t)((frame_index + 9) % 10);
        int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;

        for (player_count = local_player_globals->local_player_count; player_count > 0; player_count--) {
            motion_sensor->players[local_player_index].history[frame_index] =
                motion_sensor->players[local_player_index].history[previous];
            local_player_index = motion_sensor_next_local_player(local_player_index);
        }
        return;
    }

    {
        int16_t local_players[1 + 1];
        int16_t blip_counts[2];
        real_point3d cameras[2];
        struct {
            object_iterator iterator;
            uint32_t signature;
        } walk;
        uint8_t all_full = 0;
        int16_t local_player_index = local_player_globals->local_players[0] != (datum_index)-1 ? 0 : -1;
        int16_t count = local_player_globals->local_player_count;
        int16_t k;

        blip_counts[0] = 0;
        for (k = 0; k < count; k++) {
            motion_sensor_frame *frame = &motion_sensor->players[local_player_index].history[motion_sensor->frame_index];
            datum_index unit_index = (datum_index)-1;
            int32_t i;

            if (local_player_index != -1 && local_player_index < 1 &&
                local_player_globals->local_players[local_player_index] != (datum_index)-1) {
                unit_index = ((player *)((uint8_t *)player_data->data +
                                         (local_player_globals->local_players[local_player_index] & 0xffff) * 0x200))->unit;
            }
            local_players[k] = local_player_index;
            cameras[local_player_index].x = 0.0f;
            cameras[local_player_index].y = 0.0f;
            cameras[local_player_index].z = 0.0f;
            if (unit_index != (datum_index)-1) {
                halo::units::unit_get_camera_position(unit_index, &cameras[local_player_index]);
            }
            frame->blip_count = 0;
            for (i = 0; i < 0x10; i++) {
                frame->blips[i].type = _blip_type_empty;
            }
            local_player_index = (local_player_globals->local_players[0] != (datum_index)-1 &&
                                  local_player_index < count) ? 0 : -1;
        }

        walk.iterator.type_mask = 3;
        walk.iterator.flags_mask = 1;
        walk.iterator.index = 0;
        walk.iterator.handle = (datum_index)-1;
        walk.signature = 0x86868686;
        while (halo::objects::object_iterator_next(&walk.iterator) != 0 && !all_full) {
            datum_index object_index = walk.iterator.handle;
            uint8_t *header = 0;
            int16_t full_players;

            if (object_index != (datum_index)-1 && (int16_t)object_index >= 0 &&
                (int16_t)object_index < halo::objects::globals().object_data->maximum_count) {
                uint8_t *candidate = (uint8_t *)halo::objects::globals().object_data->data + (int16_t)object_index * halo::objects::globals().object_data->size;
                int16_t salt = (int16_t)((uint32_t)object_index >> 16);
                if (*(int16_t *)candidate != 0 && (salt == 0 || *(int16_t *)candidate == salt)) {
                    header = candidate;
                }
            }
            if (header == 0 || ((1u << (header[3] & 0x1f)) & 3) == 0 || *(uint8_t **)(header + 8) == 0 ||
                ((*(uint8_t **)(header + 8))[0x106] & 4) != 0 || motion_sensor_object_is_detected(object_index) == 0) {
                continue;
            }
            {
                real_point3d position = *(real_point3d *)((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[object_index & 0xffff].data + 0xa0);

                full_players = 0;
                for (k = 0; k < count; k++) {
                    int16_t index = local_players[k];
                    datum_index player_index;
                    motion_sensor_player_state *state;
                    motion_sensor_frame *frame;

                    if (index == -1 || index >= 1) {
                        continue;
                    }
                    player_index = local_player_globals->local_players[index];
                    if (player_index == (datum_index)-1 ||
                        ((player *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200))->unit == (datum_index)-1) {
                        continue;
                    }
                    if (blip_counts[index] >= 0x10) {
                        full_players++;
                        continue;
                    }
                    position.z = cameras[index].z;
                    if (current_game_engine == 0) {
                        float dx = position.x - cameras[index].x;
                        float dy = position.y - cameras[index].y;
                        float dz = position.z - cameras[index].z;
                        float range = *(float *)((uint8_t *)hud_globals_tag_data + 0x2d0);
                        if (range * range < dz * dz + dy * dy + dx * dx) {
                            continue;
                        }
                    }
                    state = &motion_sensor->players[index];
                    frame = &state->history[motion_sensor->frame_index];
                    motion_sensor_blip_fill(index, object_index, &frame->blips[blip_counts[index]]);
                    state->tracked_objects[blip_counts[index]] = object_index;
                    blip_counts[index]++;
                    frame->blip_count++;
                }
                if (full_players == count) {
                    all_full = 1;
                }
            }
        }
    }
}

}
