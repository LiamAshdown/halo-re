#include "halo/ai/actor_alerts.hpp"

namespace c_actor_investigate_disturbance_update {
extern "C" {
extern data_array *actor_data;
extern game_time_globals *game_time;

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b);
extern uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index);
extern uint8_t actor_evaluate_search_node(datum_index actor_index, datum_index vehicle_index, int16_t seat_index,
    real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score,
    uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front);
extern uint8_t actor_avoid_obstacle_and_project(datum_index actor_index, datum_index vehicle_index, real_point3d *entry,
    real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index);
extern void actor_movement_action_stop(datum_index actor_index);
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra);
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index);
}
}

extern "C" int32_t actor_investigate_disturbance_update(uint32_t actor_index);

/**
 * actor_investigate_disturbance_update: behaviour unchanged from the original routine. The original author notes and decompile
 * remain in docs/original/ai/actor_investigate_disturbance_update.c.txt.
 *
 * @address 0x408ba0
 */
int32_t halo::ai::alert_ops::investigate_disturbance_update()
{
    using namespace c_actor_investigate_disturbance_update;
    uint32_t actor_index = datum;
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    void *vehicle = object_try_and_get(*(datum_index *)&((struct actor *)act)->mode_data, 2);

    if (((actor *)act)->active_unit_index != k_datum_index_none) {
        act[0xa5] = 1;
    } else if (act[0xa4] == 0) {
        if (vehicle == 0) {
            *(datum_index *)&((struct actor *)act)->mode_data = k_datum_index_none;
            act[0xa6] = 1;
        } else if (!actor_is_within_alert_range(act[0xa2] == 0, *(float *)(act + 0xbc), *(float *)(act + 0xc0), 0, 1,
                                                actor_index, *(datum_index *)&((struct actor *)act)->mode_data)) {
            act[0xa6] = 1;
        } else {
            real_point3d entry;
            real_vector3d direction;
            real_point3d hint;
            uint8_t facing;
            uint8_t close;
            uint8_t in_front;

            if (game_time->game_time >= *(int32_t *)(act + 0xac) + 150) {
                *(int32_t *)(act + 0xac) = game_time->game_time;
                if (vector3d_distance_squared((real_point3d *)(act + 0x12c), (real_point3d *)(act + 0xb0)) <= 25.0f) {
                    *(int16_t *)(act + 0xaa) += 1;
                } else {
                    *(int16_t *)(act + 0xaa) = 0;
                    *(real_point3d *)(act + 0xb0) = *(real_point3d *)&((actor *)act)->body_position.x;
                }
            }
            if (*(int16_t *)(act + 0xaa) >= 8 ||
                !actor_evaluate_search_node(actor_index, *(datum_index *)&((struct actor *)act)->mode_data, *(int16_t *)(act + 0xa0), &entry,
                                            &direction, &hint, 0, &close, &facing, &in_front)) {
                act[0xa6] = 1;
            } else {
                if (in_front) {
                    *(int16_t *)(act + 0xc6) += 1;
                    if (*(int16_t *)(act + 0xc6) >= 30) {
                        facing = 1;
                        close = 1;
                    }
                } else {
                    *(int16_t *)(act + 0xc6) = 0;
                }
                if (close) {
                    if (facing) {
                        unit_enter_vehicle_seat(*(datum_index *)&((struct actor *)act)->mode_data, *(int16_t *)(act + 0xa0),
                                                ((actor *)act)->unit_index);
                        act[0xa4] = 1;
                    } else {
                        actor_movement_action_stop(actor_index);
                    }
                } else if (act[0x4c] != 0) {
                    if (actor_avoid_obstacle_and_project(actor_index, *(datum_index *)&((struct actor *)act)->mode_data, &entry, &hint,
                                                         act + 0xa3, (real_point3d *)(act + 0xcc),
                                                         (int32_t *)(act + 0xe4)) &&
                        actor_movement_set_destination_point((real_point3d *)(act + 0xcc), actor_index,
                                                             *(int32_t *)(act + 0xe4), *(datum_index *)&((struct actor *)act)->mode_data)) {
                        *(int16_t *)(act + 0xa8) = 0;
                    } else {
                        *(int16_t *)(act + 0xa8) += 1;
                        if (*(int16_t *)(act + 0xa8) > (act[0xa2] != 0 ? 5 : 50)) {
                            act[0xa6] = 1;
                        }
                    }
                }
                act[0xc8] = (uint8_t)(vector3d_distance_squared(&entry, (real_point3d *)(act + 0x12c)) <= 1.0f);
                *(real_vector3d *)(act + 0xd8) = direction;
                act[0xc5] = facing;
                act[0xc4] = close;
            }
        }
    }
    return act[0xa5] != 0 || act[0xa6] != 0;
}

extern "C" int32_t actor_investigate_disturbance_update(uint32_t actor_index)
{
    return halo::ai::alert_ops(actor_index).investigate_disturbance_update();
}

