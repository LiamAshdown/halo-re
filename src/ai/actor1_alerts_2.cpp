#include "halo/ai/actor_alerts.hpp"
#include "halo/math/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/game/api.hpp"
#include "halo/ai/records.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

namespace c_actor_investigate_disturbance_update {
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
}


/**
 * actor_investigate_disturbance_update: behaviour unchanged from the original routine.
 *
 * @address 0x408ba0
 */
int32_t halo::ai::alert_ops::investigate_disturbance_update()
{
    using namespace c_actor_investigate_disturbance_update;
    uint32_t actor_index = datum;
    actor *act = halo::ai::actor_at(actor_index);
    void *vehicle = halo::objects::object_try_and_get(act->mode_data.vehicle.vehicle_index, 2);

    if (act->active_unit_index != k_datum_index_none) {
        act->mode_data.vehicle.unit_replaced = 1;
    } else if (act->mode_data.vehicle.seated == 0) {
        if (vehicle == 0) {
            act->mode_data.vehicle.vehicle_index = k_datum_index_none;
            act->mode_data.vehicle.failed = 1;
        } else if (!halo::ai::actor_is_within_alert_range(act->mode_data.vehicle.unknown_06 == 0, act->mode_data.vehicle.alert_range_min, act->mode_data.vehicle.alert_range_max, 0, 1,
                                                actor_index, act->mode_data.vehicle.vehicle_index)) {
            act->mode_data.vehicle.failed = 1;
        } else {
            real_point3d entry;
            real_vector3d direction;
            real_point3d hint;
            uint8_t facing;
            uint8_t close;
            uint8_t in_front;

            if (game_time->game_time >= act->mode_data.vehicle.last_progress_time + 150) {
                act->mode_data.vehicle.last_progress_time = game_time->game_time;
                if (halo::math::vector3d_distance_squared(act->body_position, act->mode_data.vehicle.last_progress_position) <= 25.0f) {
                    act->mode_data.vehicle.stuck_count += 1;
                } else {
                    act->mode_data.vehicle.stuck_count = 0;
                    act->mode_data.vehicle.last_progress_position = *(real_point3d *)&act->body_position.x;
                }
            }
            if (act->mode_data.vehicle.stuck_count >= 8 ||
                !halo::ai::actor_evaluate_search_node(actor_index, act->mode_data.vehicle.vehicle_index, act->mode_data.vehicle.seat_index, &entry,
                                            &direction, &hint, 0, &close, &facing, &in_front)) {
                act->mode_data.vehicle.failed = 1;
            } else {
                if (in_front) {
                    act->mode_data.vehicle.in_front_ticks += 1;
                    if (act->mode_data.vehicle.in_front_ticks >= 30) {
                        facing = 1;
                        close = 1;
                    }
                } else {
                    act->mode_data.vehicle.in_front_ticks = 0;
                }
                if (close) {
                    if (facing) {
                        halo::units::unit_enter_vehicle_seat(act->mode_data.vehicle.vehicle_index, act->mode_data.vehicle.seat_index,
                                                act->unit_index);
                        act->mode_data.vehicle.seated = 1;
                    } else {
                        halo::ai::actor_movement_action_stop(actor_index);
                    }
                } else if (act->needs_new_path != 0) {
                    if (halo::ai::actor_avoid_obstacle_and_project(actor_index, act->mode_data.vehicle.vehicle_index, &entry, &hint,
                                                         &act->mode_data.vehicle.near_line, &act->mode_data.vehicle.path_destination,
                                                         &act->mode_data.vehicle.path_surface) &&
                        halo::ai::actor_movement_set_destination_point(&act->mode_data.vehicle.path_destination, actor_index,
                                                             act->mode_data.vehicle.path_surface, act->mode_data.vehicle.vehicle_index)) {
                        act->mode_data.vehicle.path_failures = 0;
                    } else {
                        act->mode_data.vehicle.path_failures += 1;
                        if (act->mode_data.vehicle.path_failures > (act->mode_data.vehicle.unknown_06 != 0 ? 5 : 50)) {
                            act->mode_data.vehicle.failed = 1;
                        }
                    }
                }
                act->mode_data.vehicle.entry_reached = (uint8_t)(halo::math::vector3d_distance_squared(entry, act->body_position) <= 1.0f);
                act->mode_data.vehicle.entry_direction = direction;
                act->mode_data.vehicle.facing = facing;
                act->mode_data.vehicle.close = close;
            }
        }
    }
    return act->mode_data.vehicle.unit_replaced != 0 || act->mode_data.vehicle.failed != 0;
}

namespace halo::ai {
int32_t actor_investigate_disturbance_update(uint32_t actor_index)
{
    return halo::ai::alert_ops(actor_index).investigate_disturbance_update();
}
}

