#pragma once

#include "tags.h"
#include "memory.h"
#include "camera.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "structures.h"
#include "crt.h"
#include "units.h"
#include "hs.h"
#include "game.h"
#include "projectiles.h"
#include <string.h>

namespace halo::camera {
namespace {

/**
 * View of the observer (the camera simulation state) of one local player: per-tick advance,
 * spline evaluation and the commit of the final camera.
 */
class ObserverHandle {
public:
    explicit ObserverHandle(int16_t value) : player_handle(value) {}
    void advance();
    void commit();
    void set_command();
    void compute_spline_coefficients();
    void evaluate_spline_acceleration();
    void evaluate_spline_value_and_orthonormalize();
    void evaluate_spline_velocity();
    observer_camera * get_camera();

    int16_t player_handle;
};

/**
 * Observer subsystem entry points that are not tied to one local player: initialisation,
 * update, collision avoidance and BSP location refresh.
 */
class ObserverSystem {
public:
    static void initialize();
    static void construct(observer *self);
    static void update(float dt, uint8_t add_bob);
    static void update_location();
    static void avoid_collision(real_vector3d *forward, real_point3d *position, real_vector3d *up, float *distance, float radius_scale);
    static uint8_t collision_test_ray(real_point3d *origin, uint8_t use_alternate_mask, real_point3d *target, float *out_fraction);
    static void compute_remaining_offset(float *target, float *current, float *out);
};

}
}
