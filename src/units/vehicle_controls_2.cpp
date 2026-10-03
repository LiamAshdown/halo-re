#include "halo/objects/record_access.hpp"
#include "halo/units/records.hpp"
#include "halo/units/unit.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/core/libm.hpp"
#include "halo/units/api.hpp"


namespace halo::units {

/**
 * Computes a single-axis (steering-wheel-style) rotation control transform for a vehicle-type unit each tick:
 * accumulates and wraps the wheel-rotation angle, then either dispatches generically or writes a pair of
 * scalar+quaternion blocks (rotated by half the turning angle about a fixed axis) when the supporting
 * object's physics type is 2. FIXED (objdump 0x572d6e..0x572dc7): EDI is the caller's powered-mass-point
 * buffer;
 *
 * @address 0x572cd0
 */
void VehicleView::calculate_steering_wheel_controls(void *mass_points, float *powered_states)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    vehicle_data *vehicle = halo::units::vehicle_data_of(obj);
    Physics *physics_tag = halo::objects::tag_as<Physics>(halo::objects::tag_handle(((Unit *)tag)->base.physics));
    float *out_transform = powered_states;
    float wrapped;

    vehicle->wheel_rotation = vehicle->forward_velocity + vehicle->wheel_rotation;
    wrapped = (float)halo::libm::fmod(vehicle->wheel_rotation, tag->wheel_circumference);
    vehicle->wheel_rotation = wrapped;
    if (wrapped < 0.0f) {
        vehicle->wheel_rotation = wrapped + tag->wheel_circumference;
    }

    if (physics_tag->powered_mass_points.count != 2) {
        halo::physics::object_physics_tick(unit_index, 0, (uint32_t)mass_points, 0, 0);
        return;
    }

    {
        float turning = vehicle->turning_velocity;
        float c = (float)halo::libm::cos((double)turning * 0.5);
        float s = (float)halo::libm::sin((double)turning * 0.5);

        out_transform[0] = vehicle->forward_velocity;
        out_transform[7] = 0.0f;
        out_transform[8] = 0.0f;
        out_transform[9] = s;
        out_transform[10] = c;
        out_transform[0x18] = vehicle->forward_velocity;
        out_transform[0x1f] = 0.0f;
        out_transform[0x20] = 0.0f;
        out_transform[0x21] = -s;
        out_transform[0x22] = c;
    }
    halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)((uint32_t)out_transform), (uint32_t)mass_points, 0, 0);
}

/**
 * Computes the dual-axis (pitch/yaw) turret control transform for a vehicle-type unit each tick, accumulating
 * and wrapping left/right wheel-rotation-shaped angle accumulators, and dispatches to object_physics_tick --
 * either generically, or (when the supporting object's physics type is 2) by writing a pair of
 * scalar+identity-quaternion blocks directly into out_transform. FIXED (objdump 0x572c62..0x572ca0): EDI is
 * the caller's powered-mass-point buffer (vehicle_update [esp+0x88]);
 *
 * @address 0x572b60
 */
void VehicleView::calculate_turret_controls(void *mass_points, float *powered_states)
{
    uint32_t unit_index = datum_handle;
    object *obj = ((object_header *)halo::objects::globals().object_data->data)[halo::datum_slot(unit_index)].data;
    Vehicle *tag = (Vehicle *)halo::cache::globals().tag_instances[halo::datum_slot(obj->definition_tag)].data;
    vehicle_data *vehicle = halo::units::vehicle_data_of(obj);
    float *out_transform = powered_states;
    float forward = vehicle->forward_velocity;
    float turning = vehicle->turning_velocity;
    Physics *physics_tag = halo::objects::tag_as<Physics>(halo::objects::tag_handle(((Unit *)tag)->base.physics));
    float wrapped;

    vehicle->left_wheel_rotation = (forward - turning) + vehicle->left_wheel_rotation;
    wrapped = (float)halo::libm::fmod(vehicle->left_wheel_rotation, tag->wheel_circumference);
    vehicle->left_wheel_rotation = wrapped;
    if (wrapped < 0.0f) {
        vehicle->left_wheel_rotation = wrapped + tag->wheel_circumference;
    }

    vehicle->right_wheel_rotation = (turning + forward) + vehicle->right_wheel_rotation;
    wrapped = (float)halo::libm::fmod(vehicle->right_wheel_rotation, tag->wheel_circumference);
    vehicle->right_wheel_rotation = wrapped;
    if (wrapped < 0.0f) {
        vehicle->right_wheel_rotation = wrapped + tag->wheel_circumference;
    }

    if (physics_tag->powered_mass_points.count != 2) {
        halo::physics::object_physics_tick(unit_index, 0, (uint32_t)mass_points, 0, 0);
        return;
    }

    out_transform[0] = forward - turning;
    out_transform[7] = 0.0f;
    out_transform[8] = 0.0f;
    out_transform[9] = 0.0f;
    out_transform[10] = 1.0f;
    out_transform[0x18] = turning + forward;
    out_transform[0x1f] = 0.0f;
    out_transform[0x20] = 0.0f;
    out_transform[0x21] = 0.0f;
    out_transform[0x22] = 1.0f;
    halo::physics::object_physics_tick(unit_index, (powered_mass_point_state *)((uint32_t)out_transform), (uint32_t)mass_points, 0, 0);
}

}
