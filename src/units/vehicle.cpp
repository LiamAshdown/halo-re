#include <string.h>
#include "halo/units/unit.hpp"
#include "halo/units/flags.hpp"
#include "halo/objects/flags.hpp"
#include "halo/core/flag_bits.hpp"
#include "game.h"
#include "hs.h"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"

extern "C" {
extern data_array *object_data;
extern int16_t network_game_mode;
extern game_time_globals *game_time;
extern int32_t k_vehicle_minimum_age_ticks;
extern int32_t vehicle_network_update_period;
extern uint8_t unit_updates_suppressed;
extern uint8_t *global_structure_bsp;
extern Globals *global_globals;
extern double atan2(double y, double x);
extern double fabs(double x);
extern uint8_t physics_scalar_step_to_target_clamped(void *rates, float *value, float target, float step);
extern uint8_t physics_scalar_move_toward_target(void *range, float *value, uint8_t wrap, float target, float rate);
extern void object_physics_tick(uint32_t object_index, void *powered_states, void *mass_points, real_vector3d *extra_force, real_vector3d *extra_torque);
extern void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter, char use_matched_index);
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t node_index, int16_t region_index, int16_t material_index, uint32_t plane);
extern char s_blur_permutation[];
}

namespace halo::units {

namespace vehicle_create_local {

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + halo::datum_slot(object_index) * 0xc + 8);
}

static uint8_t *object_definition(uint8_t *object)
{
    return (uint8_t *)tag_instances[halo::datum_slot(*(datum_index *)object)].data;
}

}

/**
 * Engine function vehicle_create.
 *
 * @address 0x570bb0
 */
uint8_t VehicleView::create()
{
    using namespace vehicle_create_local;
    datum_index object_index = datum_handle;
    uint8_t *object = object_get(object_index);
    uint8_t *definition = object_definition(object);
    int32_t i;

    VehicleView(object_index).reset_state();
    if (*(int32_t *)(definition + 0x8c) == -1) {
        set_flag(((struct object *)object)->flags, objects::object_flag::at_rest);
    } else {
        clear_flag(((struct object *)object)->flags, objects::object_flag::at_rest);
        ((struct object *)object)->position.z += *(float *)(definition + 4) * 0.5f;
    }
    if (network_game_mode == 1 || network_game_mode == 2) {
        ((struct vehicle_object *)object)->vehicle.unknown_525 = 0;
        ((struct vehicle_object *)object)->vehicle.unknown_526 = 0;
        ((struct vehicle_object *)object)->vehicle.network_update_sequence = 0;
        ((struct object *)object)->network_state_009 = 0;
    }
    ((struct vehicle_object *)object)->vehicle.network_update_tick = (uint32_t)game_time->game_time;
    for (i = 0; i < 3; i++) {
        ((uint32_t *)&((struct vehicle_object *)object)->vehicle.unknown_5b2[2])[i] = ((uint32_t *)&((struct object *)object)->position)[i];
    }
    ((struct vehicle_object *)object)->vehicle.collision_update_pending = 0;
    return 1;
}

/**
 * Engine function vehicle_is_old_enough.
 *
 * @address 0x572a30
 */
uint8_t VehicleView::is_old_enough()
{
    uint32_t object_index = datum_handle;
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    int32_t stamp = ((unit_object *)obj)->base.network_update_tick;

    if (stamp == -1) {
        return 1;
    }
    if (game_time->game_time >= stamp + k_vehicle_minimum_age_ticks) {
        return 1;
    }
    return (uint8_t)(((struct vehicle_object *)obj)->vehicle.collision_update_pending == 1);
}

/**
 * Clears the live part of a vehicle's vehicle_data extension (0x4cc..0x520) to zero, e.g. on possession
 * change or respawn. FIXED (register inputs, objdump): the original never reads EAX as an input (it
 * overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x570b00
 */
void VehicleView::reset_state()
{
    uint32_t object_index = datum_handle;
    object *obj = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);

    vehicle->flags = 0;
    vehicle->decay_ticks_remaining = 0;
    vehicle->airborne_ticks = 0;
    vehicle->push_direction = 0;
    vehicle->push_ticks = 0;
    vehicle->landing_ticks = 0;
    vehicle->forward_velocity = 0.0f;
    vehicle->sideways_velocity = 0.0f;
    vehicle->turning_velocity = 0.0f;
    vehicle->wheel_rotation = 0.0f;
    vehicle->left_wheel_rotation = 0.0f;
    vehicle->right_wheel_rotation = 0.0f;
    vehicle->ground_lean = 0.0f;
    vehicle->ground_contact_fraction = 0.0f;
    *(uint32_t *)&vehicle->contact_point_traction[0] = 0;
    *(uint32_t *)&vehicle->contact_point_traction[4] = 0;
    vehicle->accumulated_force.i = 0.0f;
    vehicle->accumulated_force.j = 0.0f;
    vehicle->accumulated_force.k = 0.0f;
    vehicle->accumulated_torque.i = 0.0f;
    vehicle->accumulated_torque.j = 0.0f;
    vehicle->accumulated_torque.k = 0.0f;
    vehicle->active_marker_mask = 0;
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[halo::datum_slot((h))].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[halo::datum_slot((t))].data)
#define F(p, o) (*(float *)((p) + (o)))
/**
 * Engine function vehicle_update.
 *
 * @address 0x570ee0
 */
uint32_t VehicleView::update()
{
    uint32_t object_index = datum_handle;
    uint8_t *obj = OBJECT_DATA(object_index);
    uint8_t *tag = TAG_DATA(*(datum_index *)obj);
    real_vector3d *forward = (real_vector3d *)&((struct object *)obj)->forward;
    real_vector3d *up = (real_vector3d *)&((struct object *)obj)->up;
    static uint8_t node_output[0xc00];
    static uint8_t contact_points[0x2600];

    if (network_game_mode == 2 && ((struct vehicle_object *)obj)->vehicle.network_update_tick != -1 && vehicle_network_update_period != 0 &&
        game_time->game_time >= ((struct vehicle_object *)obj)->vehicle.network_update_tick + vehicle_network_update_period) {
        if (halo::math::vector3d_distance((real_point3d *)&((struct vehicle_object *)obj)->vehicle.unknown_5b2[2], (real_point3d *)&((struct object *)obj)->position) > 1.5f &&
            UnitView(object_index).get_recently_updated_flag() == 1 && !UnitView(object_index).has_child_of_type5()) {
            UnitView(object_index).set_facing_from_index_table();
        }
        ((struct vehicle_object *)obj)->vehicle.network_update_tick = game_time->game_time;
    }

    if (((unit_object *)obj)->base.parent_object != k_datum_index_none) {
        ((struct object *)obj)->angular_velocity.i = 0.0f;
        ((struct object *)obj)->angular_velocity.j = 0.0f;
        ((struct object *)obj)->angular_velocity.k = 0.0f;
        ((struct object *)obj)->velocity.i = 0.0f;
        ((struct object *)obj)->velocity.j = 0.0f;
        ((struct object *)obj)->velocity.k = 0.0f;
        clear_flag(((unit_object *)obj)->base.flags, objects::object_flag::at_rest);
    } else {
        uint32_t control = ((unit_object *)obj)->unit.control_flags;
        real_vector3d a;
        real_vector3d b;
        float angle;
        float throttle = ((struct unit_object *)obj)->unit.throttle.i;
        float speed = ((struct vehicle_object *)obj)->vehicle.forward_velocity;

        if (control & 1) {
            set_flag(((struct vehicle_object *)obj)->vehicle.flags, units::vehicle_flag::has_ground_contact);
        } else {
            clear_flag(((struct vehicle_object *)obj)->vehicle.flags, units::vehicle_flag::has_ground_contact);
        }
        if ((control & 2) ||
            ((((struct Vehicle *)tag)->vehicle_flags & 0x10) &&
             ((throttle > 0.0f && speed < 0.0f) || (throttle < 0.0f && speed > 0.0f)))) {
            set_flag(((struct vehicle_object *)obj)->vehicle.flags, units::vehicle_flag::hovering);
        } else {
            clear_flag(((struct vehicle_object *)obj)->vehicle.flags, units::vehicle_flag::hovering);
        }

        a.i = forward->k * up->j - up->k * forward->j;
        a.j = up->k * forward->i - forward->k * up->i;
        a.k = up->i * forward->j - forward->i * up->j;
        b = a;
        angle = (float)atan2(b.j * ((struct unit_object *)obj)->unit.desired_facing_vector.j + b.k * ((struct unit_object *)obj)->unit.desired_facing_vector.k + b.i * ((struct unit_object *)obj)->unit.desired_facing_vector.i,
                             ((struct unit_object *)obj)->unit.desired_facing_vector.k * forward->k + ((struct unit_object *)obj)->unit.desired_facing_vector.j * forward->j + ((struct unit_object *)obj)->unit.desired_facing_vector.i * forward->i);
        if ((((unit_object *)obj)->base.network_role == 2 || ((unit_object *)obj)->base.network_role == 1) && ((struct object *)obj)->network_position_valid == 1) {
            UnitView(object_index).any_flagged_seat_occupied();
        }

        {
            uint8_t direction = ((struct vehicle_object *)obj)->vehicle.unknown_4d1;

            if ((((struct vehicle_object *)obj)->vehicle.flags & 0x10) && direction != 0 && ((struct vehicle_object *)obj)->vehicle.unknown_4d2 < 0x1e && up->k <= 0.9f) {
                float sign = (direction == 2 || direction == 4) ? 0.3f : -0.3f;
                float spin;

                if (direction == 4 || direction == 3) {
                    halo::math::vector3d_cross_product(a, *up, *forward);
                } else {
                    a = *forward;
                }
                spin = up->k * -2.0f;
                if (!(spin >= ((struct Vehicle *)tag)->minimum_flipping_angular_velocity)) {
                    spin = ((struct Vehicle *)tag)->minimum_flipping_angular_velocity;
                } else if (!(spin <= ((struct Vehicle *)tag)->maximum_flipping_angular_velocity)) {
                    spin = ((struct Vehicle *)tag)->maximum_flipping_angular_velocity;
                }
                spin *= sign;
                clear_flag(((unit_object *)obj)->base.flags, objects::object_flag::at_rest);
                if (direction == 2 || direction == 1) {
                    float k = -forward->k;

                    halo::math::vector3d_cross_product(b, *up, *forward);
                    a.i += b.i * k;
                    a.j += b.j * k;
                    a.k += k * b.k;
                }
                ((struct object *)obj)->angular_velocity.i = a.i * spin;
                ((struct object *)obj)->angular_velocity.j = a.j * spin;
                ((struct object *)obj)->angular_velocity.k = a.k * spin;
                if (((struct Vehicle *)tag)->vehicle_type == 0) {
                    float along = forward->k * ((struct object *)obj)->velocity.k + forward->j * ((struct object *)obj)->velocity.j + ((struct object *)obj)->velocity.i * forward->i;

                    ((struct object *)obj)->velocity.i = along * forward->i;
                    ((struct object *)obj)->velocity.j = along * forward->j;
                    ((struct object *)obj)->velocity.k = along * forward->k;
                } else if (((struct Vehicle *)tag)->vehicle_type == 5) {
                    if (-0.01f <= ((struct object *)obj)->velocity.k) {
                        ((struct object *)obj)->velocity.k = -0.01f;
                    }
                }
                ((struct vehicle_object *)obj)->vehicle.unknown_4d2++;
            } else {
                clear_flag(((struct vehicle_object *)obj)->vehicle.flags, units::vehicle_flag::controls_active);
                ((struct vehicle_object *)obj)->vehicle.unknown_4d2 = 0;
                ((struct vehicle_object *)obj)->vehicle.unknown_4d1 = 0;
            }
        }

        if ((uint8_t)((struct vehicle_object *)obj)->vehicle.flags & 8) {
            halo::physics::physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)&((struct vehicle_object *)obj)->vehicle.forward_velocity, 0.0f, 1.0f);
        } else {
            halo::physics::physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)&((struct vehicle_object *)obj)->vehicle.forward_velocity, ((struct unit_object *)obj)->unit.throttle.i, 1.0f);
            halo::physics::physics_scalar_step_to_target_clamped(tag + 0x330, (float *)&((struct vehicle_object *)obj)->vehicle.sideways_velocity, ((struct unit_object *)obj)->unit.throttle.j, 1.0f);
        }
        if (((struct Vehicle *)tag)->vehicle_type != 0) {
            float target = ((struct vehicle_object *)obj)->vehicle.forward_velocity >= 0.0f ? angle : -angle;
            float low = ((struct Vehicle *)tag)->maximum_right_turn * 0.017453292f;

            if (!(target >= low)) {
                target = low;
            } else {
                float high = ((struct Vehicle *)tag)->maximum_left_turn * 0.017453292f;

                if (!(target <= high)) {
                    target = high;
                }
            }
            halo::physics::physics_scalar_move_toward_target(tag + 0x308, (float *)&((struct vehicle_object *)obj)->vehicle.turning_velocity, 0, target,
                                              ((struct Vehicle *)tag)->turn_rate * 0.017453292f * 0.033333335f);
        } else if (((struct vehicle_object *)obj)->vehicle.forward_velocity == 0.0f) {
            halo::physics::physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)&((struct vehicle_object *)obj)->vehicle.turning_velocity, 0.0f, 1.0f);
        } else {
            float target = angle * 0.63661975f;

            if (!(target >= -1.0f)) {
                target = -1.0f;
            } else if (!(target <= 1.0f)) {
                target = 1.0f;
            }
            halo::physics::physics_scalar_step_to_target_clamped(tag + 0x2f8, (float *)&((struct vehicle_object *)obj)->vehicle.turning_velocity, target * ((struct Vehicle *)tag)->maximum_forward_speed, 2.0f);
        }

        if (*(datum_index *)&((Unit *)tag)->base.physics.tag_id != k_datum_index_none) {
            uint32_t flags = ((struct Vehicle *)tag)->vehicle_flags;

            if (((flags & 1) && ((struct vehicle_object *)obj)->vehicle.forward_velocity != 0.0f) || ((flags & 2) && ((struct vehicle_object *)obj)->vehicle.turning_velocity != 0.0f) ||
                ((flags & 4) && ((struct unit_object *)obj)->unit.driver_seat_power != 0.0f) || ((flags & 8) && ((struct unit_object *)obj)->unit.gunner_seat_power != 0.0f) ||
                ((flags & 0x20) && ((struct vehicle_object *)obj)->vehicle.sideways_velocity != 0.0f)) {
                clear_flag(((unit_object *)obj)->base.flags, objects::object_flag::at_rest);
            }
        }
        if (*(datum_index *)&((Unit *)tag)->base.physics.tag_id != k_datum_index_none && !(test_flag(((unit_object *)obj)->base.flags, objects::object_flag::at_rest))) {
            b = *(real_vector3d *)&((unit_object *)obj)->base.velocity.i;
            switch (((struct Vehicle *)tag)->vehicle_type) {
            case 0: VehicleView(object_index).calculate_turret_controls(contact_points, (float *)node_output); break;
            case 1: VehicleView(object_index).calculate_steering_wheel_controls(contact_points, (float *)node_output); break;
            case 2: VehicleView(object_index).calculate_lean_controls(contact_points, (float *)node_output); break;
            case 3: VehicleView(object_index).calculate_ground_lean_controls(contact_points); break;
            case 4: VehicleView(object_index).calculate_wing_flex_controls(angle, node_output, contact_points); break;
            case 5: VehicleView(object_index).calculate_mounted_controls_dispatch(contact_points, node_output); break;
            case 6: halo::physics::object_physics_tick(object_index, 0, (uint32_t)contact_points, 0, 0); break;
            default: break;
            }
            if (!unit_updates_suppressed) {
                UnitView(object_index).update_marker_skid_effects(contact_points);
            }
            if (!(uint8_t)UnitView(object_index).update_marker_traction_effects() && !unit_updates_suppressed) {
                UnitView(object_index).update_steering_deviation_effects(&b, contact_points);
            }
            UnitView(object_index).update_ground_contact_counter(contact_points);
            if (((unit_object *)obj)->base.flags & 0x20) {
                ((struct vehicle_object *)obj)->vehicle.decay_ticks_remaining = 15;
            }
            if (!(test_flag(((unit_object *)obj)->base.flags, objects::object_flag::unknown_1000000)) &&
                ((1u << ((uint8_t)((struct Vehicle *)tag)->vehicle_type & 0x1f)) & 0x28)) {
                float floor_z = F(global_structure_bsp, 0x10);
                float ceiling_z = F(global_structure_bsp, 0x14);

                if (floor_z != 0.0f && ((struct object *)obj)->position.z < floor_z) {
                    ((struct object *)obj)->velocity.k += ((floor_z - ((struct object *)obj)->position.z) * 0.015625f - ((struct object *)obj)->velocity.k * 0.0625f) * ((struct unit_object *)obj)->unit.driver_seat_power;
                }
                if (ceiling_z != 0.0f && ((struct object *)obj)->position.z > ceiling_z) {
                    ((struct object *)obj)->velocity.k -= ((((struct object *)obj)->position.z - ceiling_z) * 0.015625f + ((struct object *)obj)->velocity.k * 0.0625f) * ((struct unit_object *)obj)->unit.driver_seat_power;
                }
            }
        } else if (((struct vehicle_object *)obj)->vehicle.decay_ticks_remaining > 0) {
            UnitView(object_index).update_recoil_decay();
            UnitView(object_index).update_marker_traction_effects();
        }

        if ((((struct Vehicle *)tag)->vehicle_flags & 0x40) && !unit_updates_suppressed) {
            uint8_t *impact = (uint8_t *)global_globals->falling_damage.pointer;

            if (((struct object *)obj)->velocity.k < -F(impact, 0x8c)) {
                datum_index child = ((unit_object *)obj)->base.first_child_object;

                while (child != k_datum_index_none) {
                    uint8_t *child_obj = OBJECT_DATA(child);
                    damage_data dd;

                    memset(&dd, 0, sizeof(dd));
                    dd.damage_effect_tag = *(datum_index *)(impact + 0x38);
                    dd.material_type = -1;
                    dd.responsible_player = k_datum_index_none;
                    dd.responsible_object = k_datum_index_none;
                    dd.team_index = -1;
                    dd.location_cluster_index = -1;
                    dd.random_blend = 1.0f;
                    dd.multiplier = 1.0f;
                    object_apply_damage(&dd, child, -1, -1, -1, 0);
                    child = ((struct object *)child_obj)->next_object;
                }
            }
        }
    }

    if (*(datum_index *)&((Unit *)tag)->base.animation_graph.tag_id != k_datum_index_none) {
        int8_t request[2] = {0, 0};

        UnitView(object_index).update_animation_state_machine(request);
    }
    {
        uint8_t over_blur = (uint8_t)(((struct Vehicle *)tag)->blur_speed <= (float)fabs(((struct vehicle_object *)obj)->vehicle.forward_velocity));

        if (over_blur != ((uint8_t)((struct vehicle_object *)obj)->vehicle.flags & 1)) {
            object_set_permutation_by_name(object_index, s_blur_permutation, -1, (char)over_blur);
            if (over_blur) {
                set_flag(((struct vehicle_object *)obj)->vehicle.flags, units::vehicle_flag::over_blur_speed);
            } else {
                clear_flag(((struct vehicle_object *)obj)->vehicle.flags, units::vehicle_flag::over_blur_speed);
            }
        }
    }
    return 1;
}
#undef OBJECT_DATA
#undef TAG_DATA
#undef F

}
