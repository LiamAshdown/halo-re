#include "halo/ai/actor_alerts.hpp"
#include "halo/math/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/ai/records.hpp"

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
    actor *act = halo::ai::actor_at(actor_index);
    void *vehicle = halo::objects::object_try_and_get(*(datum_index *)&act->mode_data, 2);

    if (act->active_unit_index != k_datum_index_none) {
        ((uint8_t *)act)[0xa5] = 1;
    } else if (((uint8_t *)act)[0xa4] == 0) {
        if (vehicle == 0) {
            *(datum_index *)&act->mode_data = k_datum_index_none;
            ((uint8_t *)act)[0xa6] = 1;
        } else if (!halo::ai::actor_is_within_alert_range(((uint8_t *)act)[0xa2] == 0, *(float *)((uint8_t *)act + 0xbc), *(float *)((uint8_t *)act + 0xc0), 0, 1,
                                                actor_index, *(datum_index *)&act->mode_data)) {
            ((uint8_t *)act)[0xa6] = 1;
        } else {
            real_point3d entry;
            real_vector3d direction;
            real_point3d hint;
            uint8_t facing;
            uint8_t close;
            uint8_t in_front;

            if (game_time->game_time >= *(int32_t *)((uint8_t *)act + 0xac) + 150) {
                *(int32_t *)((uint8_t *)act + 0xac) = game_time->game_time;
                if (halo::math::vector3d_distance_squared(act->body_position, *(real_point3d *)((uint8_t *)act + 0xb0)) <= 25.0f) {
                    *(int16_t *)((uint8_t *)act + 0xaa) += 1;
                } else {
                    *(int16_t *)((uint8_t *)act + 0xaa) = 0;
                    *(real_point3d *)((uint8_t *)act + 0xb0) = *(real_point3d *)&act->body_position.x;
                }
            }
            if (*(int16_t *)((uint8_t *)act + 0xaa) >= 8 ||
                !halo::ai::actor_evaluate_search_node(actor_index, *(datum_index *)&act->mode_data, *(int16_t *)((uint8_t *)act + 0xa0), &entry,
                                            &direction, &hint, 0, &close, &facing, &in_front)) {
                ((uint8_t *)act)[0xa6] = 1;
            } else {
                if (in_front) {
                    *(int16_t *)((uint8_t *)act + 0xc6) += 1;
                    if (*(int16_t *)((uint8_t *)act + 0xc6) >= 30) {
                        facing = 1;
                        close = 1;
                    }
                } else {
                    *(int16_t *)((uint8_t *)act + 0xc6) = 0;
                }
                if (close) {
                    if (facing) {
                        halo::units::unit_enter_vehicle_seat(*(datum_index *)&act->mode_data, *(int16_t *)((uint8_t *)act + 0xa0),
                                                act->unit_index);
                        ((uint8_t *)act)[0xa4] = 1;
                    } else {
                        halo::ai::actor_movement_action_stop(actor_index);
                    }
                } else if (act->needs_new_path != 0) {
                    if (halo::ai::actor_avoid_obstacle_and_project(actor_index, *(datum_index *)&act->mode_data, &entry, &hint,
                                                         (uint8_t *)act + 0xa3, (real_point3d *)((uint8_t *)act + 0xcc),
                                                         (int32_t *)((uint8_t *)act + 0xe4)) &&
                        halo::ai::actor_movement_set_destination_point((real_point3d *)((uint8_t *)act + 0xcc), actor_index,
                                                             *(int32_t *)((uint8_t *)act + 0xe4), *(datum_index *)&act->mode_data)) {
                        *(int16_t *)((uint8_t *)act + 0xa8) = 0;
                    } else {
                        *(int16_t *)((uint8_t *)act + 0xa8) += 1;
                        if (*(int16_t *)((uint8_t *)act + 0xa8) > (((uint8_t *)act)[0xa2] != 0 ? 5 : 50)) {
                            ((uint8_t *)act)[0xa6] = 1;
                        }
                    }
                }
                ((uint8_t *)act)[0xc8] = (uint8_t)(halo::math::vector3d_distance_squared(entry, act->body_position) <= 1.0f);
                *(real_vector3d *)((uint8_t *)act + 0xd8) = direction;
                ((uint8_t *)act)[0xc5] = facing;
                ((uint8_t *)act)[0xc4] = close;
            }
        }
    }
    return ((uint8_t *)act)[0xa5] != 0 || ((uint8_t *)act)[0xa6] != 0;
}

namespace halo::ai {
int32_t actor_investigate_disturbance_update(uint32_t actor_index)
{
    return halo::ai::alert_ops(actor_index).investigate_disturbance_update();
}
}

