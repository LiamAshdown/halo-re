#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"
#include "objects.h"
#include "units.h"
#include "cache.h"
#include "rasterizer.h"
#include "halo/camera/layout.hpp"

namespace halo::camera {

/**
 * First-person camera: point-of-view computation, weapon offsets and per-unit camera
 * properties.
 */
class FirstPersonCamera {
public:
    static void compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
    static void apply_weapon_offset(real_point3d *position, datum_index unit, real_vector3d *aiming_direction);
    static void command_for_unit(datum_index unit, observer_command *command);
    static void deterministic(Point3D *out_position, datum_index unit, Vector3D *out_direction);
    static void for_unit_and_vector(observer_command *command, Vector3D *vector, datum_index unit);
    static void track_offset(unit_camera_properties *properties, float angle, Vector3D *out);
    static unit_camera_properties * unit_properties(datum_index unit);
};

/**
 * Third-person camera point-of-view computation.
 */
class ThirdPersonCamera {
public:
    static void compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
};

/**
 * Scripted track (animated) camera point-of-view computation.
 */
class TrackCamera {
public:
    static void compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
};

/**
 * Debug camera point-of-view computation.
 */
class DebugCamera {
public:
    static void compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
};

/**
 * Editor camera: point-of-view computation and positioning.
 */
class EditorCamera {
public:
    static void compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
    static void set_position_and_direction(editor_camera_data *out, Vector3D *direction, Point3D *position);
};

/**
 * Free flying and orbiting camera: attach, enter modes, initialise, update and point-of-view
 * computation.
 */
class FlyingCamera {
public:
    static void attach_to_object(datum_index object_index);
    static void compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
    static void enter_flying(editor_camera_data *data);
    static void enter_orbiting(editor_camera_data *data);
    static void initialize(editor_camera_data *data, int16_t local_player_index);
    static void update(director_camera_data *data, camera_input *input, observer_command *command);
};

/**
 * Orbiting camera update.
 */
class OrbitingCamera {
public:
    static void update(director_camera_data *data, camera_input *input, observer_command *command);
};

}
