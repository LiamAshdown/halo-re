// DRAFT (not in src/ until complete: a partial definition would replace the linked original).
// biped_movement_solve  (Ghidra: FUN_0055efd0)
// address 0x55efd0, size 5157 bytes
// Written section by section from objdump -d 0x55efd0..0x5603f5 (python scratchpad/annot.py).
// Frame: Ghidra local_X lives at esp0 + (0xafb0 - X), esp0 = esp after the four register pushes.
// Section status:
//   [x] 1. target velocity (0x55efd0..0x55f6d6)
//   [ ] 2. capsule sweep + ground-edge snapping (0x55f6d6..0x55fce4)
//   [ ] 3. ground contact choice (0x55fce4..0x560088)
//   [ ] 4. ground object / result tail (0x560088..0x5603f5)
//
// NOTE: biped_movement_solver_flags bit 0 is used as "airborne" here (2D air control with
// airborne_acceleration, gravity applied), and the tail sets result bit 0 when no ground plane
// was found; types/units.h calls both "grounded" -- rename once the solver is in.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "physics.h"

extern const real_vector3d *global_forward3d_pointer; // 0x00696718
extern const real_vector3d *global_up3d_pointer;      // 0x00696720
extern float world_gravity_scale;                     // 0x0069c52c
extern double sqrt(double x);

extern void real_matrix4x3_rotation_from_forward(real_vector3d *forward, real_vector3d *left,
    real_vector3d *up); // 0x55eed0, blam-cc: ESI forward, EBX left, EDI up
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, blam-cc: ECX v
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, blam-cc: ECX v
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, blam-cc: EAX out, ECX a, stack b -- computes b x a
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale);
    // 0x401930, blam-cc: EAX out, ECX direction, stack base, scale

#define K_GROUND_NORMAL_OFFSET 0.0078125f   // 0x672ed0, 1/128
#define K_FLAT_GROUND_K        0.0001f      // 0x672bbc

// blam-cc: stack -> solve
void biped_movement_solve(biped_movement_solver_data *solve)
{
    uint16_t flags = (uint16_t)solve->flags;          // [esp+0x44]
    uint16_t *result_flags = (uint16_t *)&solve->result_flags; // written as a word at 0xa0
    uint8_t climbs_any_surface = (uint8_t)((flags >> 9) & 1); // [esp+0x5f]
    float lateral_x = 0.0f;                            // [esp+0x48] local_af68
    float lateral_y = 0.0f;                            // [esp+0x70] local_af40
    real_vector3d a;                                   // [esp+0x14] local_af9c
    real_vector3d b;                                   // [esp+0x24] local_af8c
    real_vector3d c;                                   // [esp+0x30] local_af80
    real_vector3d direction;                           // [esp+0x4c] local_af64
    real_vector3d e;                                   // [esp+0x60] local_af50
    float speed;                                       // [esp+0x58] local_af58
    float one_minus_frozen;

    *result_flags = 0;

    // ---- section 1: target velocity -> solve->result_velocity (0x55efd0..0x55f6d6)
    if ((flags & 0x10) != 0) {
        // flying (0x55f022): movement delta in the facing frame, acceleration-limited
        real_vector3d world;
        float length;

        real_matrix4x3_rotation_from_forward(&solve->facing, &a, &b); // a = left, b = up
        world.i = a.i * solve->movement_delta.j + solve->facing.i * solve->movement_delta.i +
                  b.i * solve->movement_delta.k;
        world.j = solve->facing.j * solve->movement_delta.i + a.j * solve->movement_delta.j +
                  b.j * solve->movement_delta.k;
        world.k = solve->facing.k * solve->movement_delta.i + a.k * solve->movement_delta.j +
                  b.k * solve->movement_delta.k;
        one_minus_frozen = 1.0f - solve->unknown_48;
        a.i = one_minus_frozen * world.i - solve->velocity.i;
        a.j = one_minus_frozen * world.j - solve->velocity.j;
        a.k = one_minus_frozen * world.k - solve->velocity.k;
        b = a;
        length = vector3d_normalize_with_length(&b);
        if (length > solve->maximum_acceleration) {
            a.i = b.i * solve->maximum_acceleration;
            a.j = b.j * solve->maximum_acceleration;
            a.k = b.k * solve->maximum_acceleration;
        }
        solve->result_velocity.i = a.i + solve->velocity.i;
        solve->result_velocity.j = a.j + solve->velocity.j;
        solve->result_velocity.k = a.k + solve->velocity.k;
        *result_flags = (uint16_t)((*result_flags & 0xfffd) | 1);
    } else if ((flags & 0x20) != 0) {
        // 0x55f159: the planar delta rotated by facing is the velocity, vertical taken as is
        solve->result_velocity.k = solve->movement_delta.k;
        lateral_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
        solve->result_velocity.i = lateral_x;
        lateral_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
        solve->result_velocity.j = lateral_y;
    } else if ((flags & 0x1) != 0) {
        // airborne (0x55f19f): 2D air control limited by airborne_acceleration, then gravity
        real_vector2d delta2;
        float rotated_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
        float rotated_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
        float dx, dy, length;

        one_minus_frozen = 1.0f - solve->unknown_48;
        dx = one_minus_frozen * rotated_x - solve->velocity.i;
        dy = one_minus_frozen * rotated_y - solve->velocity.j;
        delta2.i = dx;
        delta2.j = dy;
        length = vector2d_normalize_with_length(&delta2);
        if (length > solve->airborne_acceleration) {
            dx = delta2.i * solve->airborne_acceleration;
            dy = solve->airborne_acceleration * delta2.j;
        }
        direction.i = delta2.i; // [esp+0x4c] keeps the normalized 2D delta
        direction.j = delta2.j;
        *result_flags = (uint16_t)(flags & 2);
        solve->result_velocity.i = dx + solve->velocity.i;
        solve->result_velocity.j = dy + solve->velocity.j;
        solve->result_velocity.k = solve->velocity.k - world_gravity_scale;
    } else {
        // on the ground (0x55f270): delta along the ground, scaled by slope, acceleration-limited
        real_vector3d *ground_normal = &solve->ground_normal;
        uint8_t jumping = 0;
        real_vector3d delta;
        float length;
        float scaled;

        speed = (float)sqrt((double)(solve->movement_delta.i * solve->movement_delta.i +
                                     solve->movement_delta.j * solve->movement_delta.j +
                                     solve->movement_delta.k * solve->movement_delta.k));
        if ((flags & 0x200) != 0) {
            // 0x55f2a4: ground frame from the aiming vector (any surface is walkable)
            b = solve->aiming;
            vector3d_cross_product(&c, &b, ground_normal);              // c = normal x aiming
            if (vector3d_normalize_with_length(&c) == 0.0f) {
                vector3d_cross_product(&c, global_up3d_pointer, ground_normal);      // normal x up
                if (vector3d_normalize_with_length(&c) == 0.0f) {
                    vector3d_cross_product(&c, global_forward3d_pointer, ground_normal); // normal x forward
                    vector3d_normalize_with_length(&c);
                }
            }
            vector3d_cross_product(&b, ground_normal, &c);              // b = c x normal
            vector3d_normalize_with_length(&b);
            direction.i = c.i * solve->movement_delta.j + b.i * solve->movement_delta.i;
            direction.j = c.j * solve->movement_delta.j + b.j * solve->movement_delta.i;
            direction.k = c.k * solve->movement_delta.j + b.k * solve->movement_delta.i + solve->movement_delta.k;
        } else if (ground_normal->k > K_FLAT_GROUND_K) {
            // 0x55f3a0: rotate the planar delta by facing and lift it onto the ground plane
            lateral_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
            direction.i = lateral_x;
            lateral_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
            direction.j = lateral_y;
            direction.k = solve->movement_delta.k -
                          (lateral_y * ground_normal->j + lateral_x * ground_normal->i) / ground_normal->k;
        } else {
            // 0x55f3fa: ground too steep to lift onto: build the frame from aiming projected
            // onto the ground plane, and push the vertical part five times harder
            c = solve->aiming;
            vector3d_cross_product(&e, &solve->aiming, global_up3d_pointer); // e = up x aiming
            vector3d_normalize_with_length(&e);
            point3d_add_scaled((real_point3d *)&c, ground_normal, (real_point3d *)&c,
                -(c.k * ground_normal->k + c.j * ground_normal->j + c.i * ground_normal->i));
            point3d_add_scaled((real_point3d *)&e, ground_normal, (real_point3d *)&e,
                -(e.k * ground_normal->k + e.j * ground_normal->j + e.i * ground_normal->i));
            lateral_x = solve->facing.i * solve->movement_delta.i - solve->movement_delta.j * solve->facing.j;
            lateral_y = solve->movement_delta.j * solve->facing.i + solve->movement_delta.i * solve->facing.j;
            direction.i = e.i * solve->movement_delta.j + c.i * solve->movement_delta.i;
            direction.j = c.j * solve->movement_delta.i + e.j * solve->movement_delta.j;
            direction.k = c.k * solve->movement_delta.i + e.k * solve->movement_delta.j + solve->movement_delta.k;
            if (climbs_any_surface == 0) { // always true on this branch
                direction.k = direction.k * 5.0f; // 0x672c40
            }
        }
        vector3d_normalize_with_length(&direction);

        // 0x55f508: slope speed scaling on the normalized direction's k (the sine of the slope)
        scaled = speed;
        if ((flags & 0x200) == 0) {
            float z = direction.k;
            if (z <= solve->negative_sine_downhill_cutoff_angle) {
                scaled = speed * solve->downhill_velocity_scale;
            } else if (z < solve->negative_sine_downhill_falloff_angle) {
                scaled = ((solve->downhill_velocity_scale - 1.0f) * (z - solve->negative_sine_downhill_falloff_angle)) /
                         (solve->negative_sine_downhill_cutoff_angle - solve->negative_sine_downhill_falloff_angle) + 1.0f;
                scaled = scaled * speed;
            } else if (z >= solve->sine_uphill_cutoff_angle) { // 0x55f55f: test ah,1 -- NaN goes on
                scaled = speed * solve->uphill_velocity_scale;
            } else if (z > solve->sine_uphill_falloff_angle) {
                scaled = ((solve->uphill_velocity_scale - 1.0f) * (z - solve->sine_uphill_falloff_angle)) /
                         (solve->sine_uphill_cutoff_angle - solve->sine_uphill_falloff_angle) + 1.0f;
                scaled = scaled * speed;
            }
        }
        scaled = (1.0f - solve->unknown_48) * scaled;

        delta.i = direction.i * scaled - solve->velocity.i;
        delta.j = direction.j * scaled - solve->velocity.j;
        delta.k = direction.k * scaled - solve->velocity.k;
        b = delta;
        length = vector3d_normalize_with_length(&b);
        if (length > solve->maximum_acceleration) {
            if ((flags & 0x200) == 0) {
                jumping = (uint8_t)((flags >> 1) & 1);
            }
            c.i = b.i * solve->maximum_acceleration;
            c.j = b.j * solve->maximum_acceleration;
            c.k = b.k * solve->maximum_acceleration;
        } else {
            c = delta;
        }
        *result_flags = jumping ? 2 : 0;
        solve->result_velocity.i = (c.i - ground_normal->i * K_GROUND_NORMAL_OFFSET) + solve->velocity.i;
        solve->result_velocity.j = (c.j - ground_normal->j * K_GROUND_NORMAL_OFFSET) + solve->velocity.j;
        solve->result_velocity.k = (c.k - ground_normal->k * K_GROUND_NORMAL_OFFSET) + solve->velocity.k;
        if ((*result_flags & 2) != 0) {
            solve->result_velocity.k -= world_gravity_scale;
        }
    }

    // ---- section 2 (0x55f6d6..): TODO
}
