// DRAFT (not in src/ until complete: a partial definition would replace the linked original).
// biped_movement_solve  (Ghidra: FUN_0055efd0)
// address 0x55efd0, size 5157 bytes
// Written section by section from objdump -d 0x55efd0..0x5603f5 (python scratchpad/annot.py).
// Frame: Ghidra local_X lives at esp0 + (0xafb0 - X), esp0 = esp after the four register pushes.
// Section status:
//   [x] 1. target velocity (0x55efd0..0x55f6d6)
//   [x] 2. capsule sweep + ground-edge snapping (0x55f6d6..0x55fce4)
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
extern double fabs(double x);
extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98

extern void real_matrix4x3_rotation_from_forward(real_vector3d *forward, real_vector3d *left,
    real_vector3d *up); // 0x55eed0, blam-cc: ESI forward, EBX left, EDI up
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, blam-cc: ECX v
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0, blam-cc: ECX v
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
    // 0x4052c0, blam-cc: EAX out, ECX a, stack b -- computes b x a
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale);
    // 0x401930, blam-cc: EAX out, ECX direction, stack base, scale

extern int16_t physics_sweep_capsule_step(real_point3d *origin, real_vector3d *delta, real_vector3d *out_velocity,
    uint32_t exclude_object_index, uint32_t flags, float pill_height, float pill_radius,
    real_point3d *out_position, int16_t max_contacts, physics_model_contact *contacts);
    // 0x506fb0, blam-cc: EDI origin, ESI delta, EBX out_velocity, ECX exclude, stack the rest
extern void structure_bsp_plane_fetch_signed(real_plane3d *out, void *planes_owner, int32_t signed_index);
    // 0x44dad0, blam-cc: EAX out, EDX signed_index, stack planes_owner

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
    int16_t contact_count;                             // [esp+0x58] (reuses speed's slot)
    real_point3d swept_position;                       // [esp+0x74] local_af3c
    real_vector3d swept_velocity;                      // [esp+0x94] local_af1c
    physics_model_contact contacts[16];                // [esp+0xbc] local_aef4

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

    // ---- section 2: capsule sweep and ground-edge snapping (0x55f6d6..0x55fce4)
    {
        uint32_t model_flags;
        real_vector3d delta;

        if ((flags & 0x40) != 0) {
            model_flags = 0;
        } else if ((flags & 0x80) != 0) {
            model_flags = 0xc0a0;
        } else if ((flags & 0x100) != 0) {
            model_flags = 0xc2a0;       // 0x20c3a0 + 0xffdfff00
        } else {
            model_flags = 0x20c3a0;
        }
        a = *(real_vector3d *)&solve->start_position;
        delta = solve->result_velocity;
        delta.k = delta.k + solve->height_change;
        e = delta;                                         // [esp+0x60] local_af50
        contact_count = physics_sweep_capsule_step((real_point3d *)&a, &e, &swept_velocity, solve->object_index,
            model_flags, solve->pill_height, solve->pill_radius, &swept_position, 16, contacts);
        if (contact_count < 16) {
            solve->result_flags &= 0xf7;
        } else {
            solve->result_flags |= 0x08;
        }
    }
    solve->unknown_a8 = 0xffffffff;
    if (contact_count == 0 && solve->ground_surface_index != 0xffffffff) {
        ModelCollisionGeometryBSP *bsp = global_structure_collision_bsp;
        int32_t surface_index = (int32_t)solve->ground_surface_index;
        int32_t best_surface = -1;                          // [esp+0x6c] local_af44
        float best_distance_squared = 3.4028235e+38f;       // [esp+0x80] local_af30
        float best_dot = 0.0f;                              // [esp+0x4c] local_af64
        real_plane3d best_plane;                            // [esp+0xa8] local_af08

        if (surface_index >= 0 && surface_index < (int32_t)bsp->surfaces.count) {
            ModelCollisionGeometryBSPSurface *surfaces = (ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer;
            ModelCollisionGeometryBSPEdge *edges = (ModelCollisionGeometryBSPEdge *)bsp->edges.pointer;
            uint32_t start_edge = surfaces[surface_index].first_edge;
            uint32_t edge_index = start_edge;
            real_plane3d plane;                             // [esp+0x84] local_af2c
            float along;

            structure_bsp_plane_fetch_signed(&plane, bsp, (int32_t)surfaces[surface_index].plane);
            // a = the swept position dropped onto our ground plane
            along = -((plane.normal.i * swept_position.x + plane.normal.k * swept_position.z +
                       plane.normal.j * swept_position.y) - plane.d);
            a.i = plane.normal.i * along + swept_position.x;
            a.j = plane.normal.j * along + swept_position.y;
            a.k = plane.normal.k * along + swept_position.z;

            do {
                ModelCollisionGeometryBSPEdge *edge = &edges[edge_index];
                uint8_t ours_on_right = solve->ground_surface_index == edge->right_surface;
                uint32_t other = ours_on_right ? edge->left_surface : edge->right_surface;

                if (other != 0xffffffff &&
                    ((flags & 0x200) != 0 || (surfaces[other].flags & 4) != 0)) {
                    float dot;

                    structure_bsp_plane_fetch_signed(&plane, bsp, (int32_t)surfaces[other].plane);
                    dot = plane.normal.i * swept_velocity.i + plane.normal.j * swept_velocity.j +
                          plane.normal.k * swept_velocity.k;
                    if (dot > 0.0f &&
                        solve->pill_radius * -0.5f <
                            (plane.normal.i * swept_position.x + plane.normal.k * swept_position.z +
                             plane.normal.j * swept_position.y) - plane.d) {
                        ModelCollisionGeometryBSPVertex *vertices = (ModelCollisionGeometryBSPVertex *)bsp->vertices.pointer;
                        real_point3d *v0 = (real_point3d *)&vertices[edge->start_vertex].point;
                        real_point3d *v1 = (real_point3d *)&vertices[edge->end_vertex].point;
                        float t;
                        float dx, dy, dz, distance_squared;

                        c.i = v1->x - v0->x;
                        c.j = v1->y - v0->y;
                        c.k = v1->z - v0->z;
                        t = ((a.i - v0->x) * c.i + (a.k - v0->z) * c.k + (a.j - v0->y) * c.j) /
                            (c.i * c.i + c.k * c.k + c.j * c.j);
                        if (t < 0.0f) {
                            b = *(real_vector3d *)v0;
                        } else if (t > 1.0f) {
                            b = *(real_vector3d *)v1;
                        } else {
                            point3d_add_scaled((real_point3d *)&b, &c, v0, t);
                        }
                        dx = b.i - a.i;
                        dy = b.j - a.j;
                        dz = b.k - a.k;
                        distance_squared = dx * dx + dy * dy + dz * dz;
                        if (distance_squared < best_distance_squared) {
                            best_distance_squared = distance_squared;
                            best_plane = plane;
                            best_surface = (int32_t)other;
                            best_dot = dot;
                        }
                    }
                }
                edge_index = ours_on_right ? edge->reverse_edge : edge->forward_edge;
            } while (edge_index != start_edge);
        }

        // 0x55fad3: step over onto the neighbouring surface when it is close and shallow
        if (best_surface != -1) {
            float reach = solve->pill_radius + solve->pill_radius;
            if (!(best_distance_squared > reach * reach) && !(best_dot > 0.053333335f)) {
                float height = (swept_position.x * best_plane.normal.i + best_plane.normal.k * swept_position.z +
                                best_plane.normal.j * swept_position.y) - (best_plane.d + solve->pill_radius);
                if (!(solve->pill_radius * 0.5f < (float)fabs((double)height))) {
                    physics_model_contact *contact = &contacts[0];
                    float into = swept_velocity.i * best_plane.normal.i + best_plane.normal.j * swept_velocity.j +
                                 best_plane.normal.k * swept_velocity.k;

                    swept_position.x = best_plane.normal.i * -height + swept_position.x;
                    swept_position.y = best_plane.normal.j * -height + swept_position.y;
                    swept_position.z = best_plane.normal.k * -height + swept_position.z;
                    if (into > -0.033333335f) {
                        float push = -(into + 0.033333335f);
                        swept_velocity.i = best_plane.normal.i * push + swept_velocity.i;
                        swept_velocity.j = best_plane.normal.j * push + swept_velocity.j;
                        swept_velocity.k = best_plane.normal.k * push + swept_velocity.k;
                    }
                    contact->t = 0.0f;
                    contact->point_x = best_plane.normal.i * -solve->pill_radius + swept_position.x;
                    contact->point_y = best_plane.normal.j * -solve->pill_radius + swept_position.y;
                    contact->point_z = best_plane.normal.k * -solve->pill_radius + swept_position.z;
                    contact->plane_i = best_plane.normal.i;
                    contact->plane_j = best_plane.normal.j;
                    contact->plane_k = best_plane.normal.k;
                    contact->plane_d = best_plane.d;
                    contact->object_index = 0xffffffff;
                    contact->surface_index = best_surface;
                    contact->surface_flags = 0;
                    contact->breakable_surface_index = 0;
                    contact->material_type = -1;
                    contact_count = 1;
                    solve->unknown_a8 = (uint32_t)best_surface;
                }
            }
        }
    }

    // ---- section 3 (0x55fce7..): TODO
}
