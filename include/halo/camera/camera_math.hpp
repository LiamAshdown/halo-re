#pragma once

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "camera.h"
#include "math.h"

namespace halo::camera {
namespace {

/**
 * Numeric helpers used by the camera code: validity and tolerance tests, Catmull-Rom
 * interpolation and basis construction.
 */
class CameraMath {
public:
    static uint8_t approximately_equal(float a, float b);
    static uint8_t is_valid(float value);
    static double scalar_catmull_rom(float value0, float value1, float value2, float value3, float time0, float dt, float time);
    static void vector3d_catmull_rom_interpolate(Vector3D *source1, Vector3D *source3, Vector3D *source2, Vector3D *out, Vector3D *source0, float time0, float dt, float time);
    static void compute_up_from_forward(Vector3D *forward, Vector3D *up);
    static uint8_t is_unit_length(Vector3D *v);
    static void rotate_basis_by_axis_angle(Vector3D *axis_angle, Vector3D *forward, Vector3D *up);
};

}
}
