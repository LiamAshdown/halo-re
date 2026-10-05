#include "halo/core/libm.hpp"
#include "halo/camera/camera_math.hpp"
#include "halo/math/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/core/x87.hpp"


namespace halo::camera {

/**
 * Returns whether a and b are equal within a small epsilon (0.001), guarding against NaN.
 *
 * Register convention in the original: __cdecl, both values on the stack (Ghidra's recognized
 * param_1, param_2).
 *
 * @address 0x447680
 */
uint8_t CameraMath::approximately_equal(float a, float b)
{
    float difference;

    if (halo::libm::is_nan((double)(a - b)) != 0) {
        return 0;
    }
    difference = a - b;
    if (difference < 0.0f) {
        difference = -difference;
    }
    return (uint8_t)(difference < 0.001);
}

/**
 * Returns whether value is a valid (non-NaN) float.
 *
 * Register convention in the original: __cdecl, single stack parameter (Ghidra's recognized
 * param_1).
 *
 * @address 0x4476c0
 */
uint8_t CameraMath::is_valid(float value)
{
    return (uint8_t)(halo::libm::is_nan((double)value) == 0);
}

/**
 *
 * Register convention in the original: __cdecl, all seven parameters on the stack.
 *
 * @address 0x447000
 */
double CameraMath::scalar_catmull_rom(float value0, float value1, float value2, float value3, float time0, float dt, float time)
{
    float second_difference;

    second_difference = (value2 - value1) - (value1 - value0);
    return (double)(((time - time0) / dt) *
                     (((time - (time0 + dt)) *
                       ((((value3 - value2) - (value2 - value1)) - second_difference) *
                        (time - (time0 + dt + dt)) /
                        (dt * 3.0)
                        + second_difference)) /
                      (dt + dt)
                      + (value1 - value0))
                     + value0);
}

/**
 *
 * Register convention in the original: source1 in EBX (unaff_EBX), source3 in ESI (unaff_ESI),
 * source2 in EDI.
 *
 * @address 0x447080
 */
void CameraMath::vector3d_catmull_rom_interpolate(Vector3D *source1, Vector3D *source3, Vector3D *source2, Vector3D *out, Vector3D *source0, float time0, float dt, float time)
{
    double component;

    component = halo::camera::scalar_catmull_rom_interpolate(source0->i, source1->i, source2->i, source3->i,
        time0, dt, time);
    out->i = (float)component;
    component = halo::camera::scalar_catmull_rom_interpolate(source0->j, source1->j, source2->j, source3->j,
        time0, dt, time);
    out->j = (float)component;
    component = halo::camera::scalar_catmull_rom_interpolate(source0->k, source1->k, source2->k, source3->k,
        time0, dt, time);
    out->k = (float)component;
}

/**
 *
 * Register convention in the original: ESI -> forward (unaff_ESI), EDI -> up (unaff_EDI); no
 * stack parameters.
 *
 * @address 0x4479c0
 */
void CameraMath::compute_up_from_forward(Vector3D *forward, Vector3D *up)
{
    real_vector3d horizontal_perp;
    real length;
    float forward_i, forward_j, forward_k;

    horizontal_perp.i = forward->j;
    horizontal_perp.j = -forward->i;
    horizontal_perp.k = 0.0f;

    length = halo::math::vector3d_normalize_with_length(horizontal_perp);
    if (length == 0.0f) {
        
        
        horizontal_perp.i = 1.0f;
        horizontal_perp.j = 0.0f;
    }

    forward_i = forward->i;
    forward_j = forward->j;
    forward_k = forward->k;

    up->i = horizontal_perp.j * forward_k - forward_j * 0.0f;
    up->j = forward_i * 0.0f - horizontal_perp.i * forward_k;
    up->k = horizontal_perp.i * forward_j - horizontal_perp.j * forward_i;
}

/**
 *
 * Register convention in the original: vector pointer in EAX (in_EAX); no stack parameters.
 *
 * @address 0x4476e0
 */
uint8_t CameraMath::is_unit_length(Vector3D *v)
{
    float length_squared_minus_one;

    length_squared_minus_one = (v->i * v->i + v->j * v->j + v->k * v->k) - 1.0f;
    if (halo::libm::is_nan((double)length_squared_minus_one) != 0) {
        return 0;
    }
    if (length_squared_minus_one < 0.0f) {
        length_squared_minus_one = -length_squared_minus_one;
    }
    return (uint8_t)(length_squared_minus_one < 0.001);
}

/**
 *
 * Register convention in the original: axis_angle vector in EAX (in_EAX); forward and up on
 * the stack (Ghidra.
 *
 * @address 0x448880
 */
void CameraMath::rotate_basis_by_axis_angle(Vector3D *axis_angle, Vector3D *forward, Vector3D *up)
{
    real_vector3d axis = *(real_vector3d *)axis_angle;
    real angle;
    real sin_angle, cos_angle;

    angle = halo::math::vector3d_normalize_with_length(axis);
    if (angle != 0.0f) {
        sin_angle = (real)halo::x87::fsin((double)angle);
        cos_angle = (real)halo::x87::fcos((double)angle);
        halo::math::vector3d_rotate_about_axis(*(real_vector3d *)forward, axis, sin_angle, cos_angle);
        halo::math::vector3d_rotate_about_axis(*(real_vector3d *)up, axis, sin_angle, cos_angle);
    }
}

}

namespace halo::camera {

uint8_t real_approximately_equal(float a, float b)
{
    return halo::camera::CameraMath::approximately_equal(a, b);
}

uint8_t real_is_valid(float value)
{
    return halo::camera::CameraMath::is_valid(value);
}

double scalar_catmull_rom_interpolate(float value0, float value1, float value2, float value3, float time0, float dt, float time)
{
    return halo::camera::CameraMath::scalar_catmull_rom(value0, value1, value2, value3, time0, dt, time);
}

void vector3d_catmull_rom_interpolate(Vector3D *source1, Vector3D *source3, Vector3D *source2, Vector3D *out, Vector3D *source0, float time0, float dt, float time)
{
    halo::camera::CameraMath::vector3d_catmull_rom_interpolate(source1, source3, source2, out, source0, time0, dt, time);
}

void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *up)
{
    halo::camera::CameraMath::compute_up_from_forward(forward, up);
}

uint8_t vector3d_is_unit_length(Vector3D *v)
{
    return halo::camera::CameraMath::is_unit_length(v);
}

void vector3d_rotate_basis_by_axis_angle(Vector3D *axis_angle, Vector3D *forward, Vector3D *up)
{
    halo::camera::CameraMath::rotate_basis_by_axis_angle(axis_angle, forward, up);
}

}
