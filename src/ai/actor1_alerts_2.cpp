#include "halo/ai/actor_alerts.hpp"
#include "halo/math/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"

namespace c_actor_investigate_disturbance_update {
extern "C" {
extern game_time_globals *game_time;

}
}


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
    uint8_t *act = (uint8_t *)halo::ai::globals().actor_data->data + (actor_index & halo::k_slot_mask) * k_actor_size;
    void *vehicle = halo::objects::object_try_and_get(*(datum_index *)&((struct actor *)act)->mode_data, 2);

    if (((actor *)act)->active_unit_index != k_datum_index_none) {
        act[0xa5] = 1;
    } else if (act[0xa4] == 0) {
        if (vehicle == 0) {
            *(datum_index *)&((struct actor *)act)->mode_data = k_datum_index_none;
            act[0xa6] = 1;
        } else if (!halo::ai::actor_is_within_alert_range(act[0xa2] == 0, *(float *)(act + 0xbc), *(float *)(act + 0xc0), 0, 1,
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
                if (halo::math::vector3d_distance_squared(((struct actor *)act)->body_position, *(real_point3d *)(act + 0xb0)) <= 25.0f) {
                    *(int16_t *)(act + 0xaa) += 1;
                } else {
                    *(int16_t *)(act + 0xaa) = 0;
                    *(real_point3d *)(act + 0xb0) = *(real_point3d *)&((actor *)act)->body_position.x;
                }
            }
            if (*(int16_t *)(act + 0xaa) >= 8 ||
                !halo::ai::actor_evaluate_search_node(actor_index, *(datum_index *)&((struct actor *)act)->mode_data, *(int16_t *)(act + 0xa0), &entry,
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
                        halo::units::unit_enter_vehicle_seat(*(datum_index *)&((struct actor *)act)->mode_data, *(int16_t *)(act + 0xa0),
                                                ((actor *)act)->unit_index);
                        act[0xa4] = 1;
                    } else {
                        halo::ai::actor_movement_action_stop(actor_index);
                    }
                } else if (act[0x4c] != 0) {
                    if (halo::ai::actor_avoid_obstacle_and_project(actor_index, *(datum_index *)&((struct actor *)act)->mode_data, &entry, &hint,
                                                         act + 0xa3, (real_point3d *)(act + 0xcc),
                                                         (int32_t *)(act + 0xe4)) &&
                        halo::ai::actor_movement_set_destination_point((real_point3d *)(act + 0xcc), actor_index,
                                                             *(int32_t *)(act + 0xe4), *(datum_index *)&((struct actor *)act)->mode_data)) {
                        *(int16_t *)(act + 0xa8) = 0;
                    } else {
                        *(int16_t *)(act + 0xa8) += 1;
                        if (*(int16_t *)(act + 0xa8) > (act[0xa2] != 0 ? 5 : 50)) {
                            act[0xa6] = 1;
                        }
                    }
                }
                act[0xc8] = (uint8_t)(halo::math::vector3d_distance_squared(entry, ((struct actor *)act)->body_position) <= 1.0f);
                *(real_vector3d *)(act + 0xd8) = direction;
                act[0xc5] = facing;
                act[0xc4] = close;
            }
        }
    }
    return act[0xa5] != 0 || act[0xa6] != 0;
}

namespace halo::ai {
int32_t actor_investigate_disturbance_update(uint32_t actor_index)
{
    return halo::ai::alert_ops(actor_index).investigate_disturbance_update();
}
}

