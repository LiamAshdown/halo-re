// physics_shape_test_ray  (Ghidra: FUN_00504bb0, still unnamed there; name from
// out/phase2/results/physics_00.json: "Same three-array (spheres/pills/shapes) iteration and
// shape-type switch as FUN_00504260, but dispatching to the ray-test variants FUN_00504430/
// FUN_005045c0/FUN_005048d0 and requiring the hit be facing against the direction (dot <
// -0.0001).")
// address 0x504bb0, size 603 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (step 1: control flow, constants (-0.0001 at 0x672bb8), record offsets and the three shape-test conventions checked against objdump -d 0x504bb0..0x504e0a)
// evidence: out/phase4/physics_types_notes.md section 2: physics_model's three counts read as
//   `(short *)(param_1 + type*2)`, and the sphere/pill/shape bases (0x0008, 0x1c08, 0x4408)
//   match exactly; physics_model_contact's fields (t 0x00, point 0x04, plane 0x10, object_index
//   0x20, surface_index 0x24, flags/breakable/material 0x28..0x2a) match every store here, and
//   its own struct comment ("writes t = 1 with point = origin + delta when nothing was hit") is
//   this function's own no-hit path; identical skeleton to physics_shape_test_point (0x504260,
//   this module), which this rewrite mirrors, substituting the ray-test callees and adding the
//   facing-direction rejection this function's caller (0x505880's collision_result, per
//   physics_model_contact's own struct comment on plane orientation) requires.
// register convention: none recognized as "in_EAX" etc by Ghidra -- all four (model, origin,
//   delta, out_contact) are its own ordinary stack parameters.
//   // blam-cc: stack -> model, origin, delta, out_contact
// UNSURE: the sphere/pill/shape ray-test calls show only 1-2 of their arguments in Ghidra's
// decompile; reconstructed using physics_shape_sphere_test_ray / physics_shape_pill_test_ray /
// physics_shape_polygon_test_ray's own established register conventions (this batch).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t physics_shape_sphere_test_ray(real_point3d *origin, real_vector3d *delta,
                                               physics_model_sphere *sphere, real_plane3d *out_plane,
                                               float *out_t); // 0x504430, this batch
extern uint8_t physics_shape_pill_test_ray(real_vector3d *delta, real_point3d *origin,
                                             real_plane3d *out_plane, physics_model_pill *pill,
                                             float *out_t); // 0x5045c0, this batch
extern uint8_t physics_shape_polygon_test_ray(real_point3d *origin, physics_model_shape *shape,
                                                real_vector3d *delta, float *out_t,
                                                real_plane3d *out_plane); // 0x5048d0, this batch

// Tests a world-space ray (origin, delta) against every sphere, pill and polygon shape in
// model, keeping the closest hit whose surface faces back against the ray (normal . delta <
// -0.0001, rejecting glancing/backfacing hits). Always fills out_contact's point; on a miss it
// is origin + delta with t = 1 and the function returns 0, matching
// physics_shape_test_point's sibling behaviour for the no-hit case.
// blam-cc: stack -> model, origin, delta, out_contact
uint32_t physics_shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta,
                                 physics_model_contact *out_contact)
{
    int32_t best_type = -1;
    int32_t best_index = -1;
    float best_t = 3.4028235e+38f; // FLT_MAX
    real_plane3d best_normal;
    int32_t type;

    for (type = 0; type < 3; type++) {
        int16_t count = ((int16_t *)model)[type];
        int32_t i;

        for (i = 0; i < count; i++) {
            float t;
            real_plane3d normal;
            uint32_t hit;

            if (type == 0) {
                hit = physics_shape_sphere_test_ray(origin, delta, &model->spheres[i], &normal, &t);
            } else if (type == 1) {
                hit = physics_shape_pill_test_ray(delta, origin, &normal, &model->pills[i], &t);
            } else {
                hit = physics_shape_polygon_test_ray(origin, &model->shapes[i], delta, &t, &normal);
            }

            if (hit && t < best_t &&
                (normal.normal.i * delta->i + normal.normal.k * delta->k +
                 normal.normal.j * delta->j) < -0.0001f) {
                best_t = t;
                best_type = type;
                best_index = i;
                best_normal = normal;
            }
        }
    }

    if (best_type == -1) {
        out_contact->t = 1.0f;
        out_contact->point_x = origin->x + delta->i;
        out_contact->point_y = origin->y + delta->j;
        out_contact->point_z = origin->z + delta->k;
        return 0;
    }

    out_contact->t = best_t;
    out_contact->point_x = best_t * delta->i + origin->x;
    out_contact->point_y = best_t * delta->j + origin->y;
    out_contact->point_z = best_t * delta->k + origin->z;
    out_contact->plane_i = best_normal.normal.i;
    out_contact->plane_j = best_normal.normal.j;
    out_contact->plane_k = best_normal.normal.k;
    out_contact->plane_d = best_normal.d;

    if (best_type == 0) {
        physics_model_sphere *sphere = &model->spheres[best_index];
        out_contact->object_index = sphere->object_index;
        out_contact->surface_index = sphere->surface_index;
        out_contact->surface_flags = sphere->surface_flags;
        out_contact->breakable_surface_index = sphere->breakable_surface_index;
        out_contact->material_type = sphere->material_type;
    } else if (best_type == 1) {
        physics_model_pill *pill = &model->pills[best_index];
        out_contact->object_index = pill->object_index;
        out_contact->surface_index = pill->surface_index;
        out_contact->surface_flags = pill->surface_flags;
        out_contact->breakable_surface_index = pill->breakable_surface_index;
        out_contact->material_type = pill->material_type;
    } else {
        physics_model_shape *shape = &model->shapes[best_index];
        out_contact->object_index = shape->object_index;
        out_contact->surface_index = shape->surface_index;
        out_contact->surface_flags = shape->surface_flags;
        out_contact->breakable_surface_index = shape->breakable_surface_index;
        out_contact->material_type = shape->material_type;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x504bb0):

undefined4 FUN_00504bb0(int param_1,float *param_2,float *param_3,float *param_4)

{
  char cVar1;
  float *pfVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float local_38;
  float local_34;
  int local_30;
  int local_2c;
  int local_28;
  short *local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar6 = -1;
  local_2c = -1;
  local_28 = -1;
  local_38 = 3.4028235e+38;
  local_30 = 0;
  do {
    local_24 = (short *)(param_1 + (short)local_30 * 2);
    iVar5 = 0;
    iVar4 = local_30;
    if (0 < *local_24) {
      do {
        sVar3 = (short)iVar4;
        if (sVar3 == 0) {
          cVar1 = FUN_00504430(&local_34);
joined_r0x00504c53:
          if (((cVar1 != '\0') && (local_34 < local_38)) &&
             (local_20 * *param_3 + local_18 * param_3[2] + local_1c * param_3[1] < -0.0001)) {
            local_38 = local_34;
            local_2c = iVar4;
            local_28 = iVar5;
            local_10 = local_20;
            local_c = local_1c;
            local_8 = local_18;
            local_4 = local_14;
          }
        }
        else {
          if (sVar3 == 1) {
            cVar1 = FUN_005045c0(&local_34);
            iVar4 = local_30;
            goto joined_r0x00504c53;
          }
          if (sVar3 == 2) {
            cVar1 = FUN_005048d0(&local_34,&local_20);
            goto joined_r0x00504c53;
          }
        }
        iVar5 = iVar5 + 1;
        iVar6 = local_28;
      } while ((short)iVar5 < *local_24);
    }
    local_30 = iVar4 + 1;
    if (2 < (short)local_30) {
      if ((short)local_2c == -1) {
        *param_4 = 1.0;
        param_4[1] = *param_2 + *param_3;
        param_4[2] = param_2[1] + param_3[1];
        param_4[3] = param_2[2] + param_3[2];
        return 0;
      }
      *param_4 = local_38;
      param_4[1] = local_38 * *param_3 + *param_2;
      param_4[2] = local_38 * param_3[1] + param_2[1];
      param_4[3] = local_38 * param_3[2] + param_2[2];
      param_4[4] = local_10;
      param_4[5] = local_c;
      param_4[6] = local_8;
      param_4[7] = local_4;
      sVar3 = (short)iVar6;
      if ((short)local_2c == 0) {
        pfVar2 = (float *)(sVar3 * 0x1c + 8 + param_1);
      }
      else if ((short)local_2c == 1) {
        pfVar2 = (float *)(param_1 + 0x1c08 + sVar3 * 0x28);
      }
      else {
        if ((short)local_2c != 2) {
          return 1;
        }
        pfVar2 = (float *)(sVar3 * 0x68 + 0x4408 + param_1);
      }
      param_4[8] = *pfVar2;
      param_4[9] = pfVar2[1];
      *(undefined1 *)(param_4 + 10) = *(undefined1 *)(pfVar2 + 2);
      *(undefined1 *)((int)param_4 + 0x29) = *(undefined1 *)((int)pfVar2 + 9);
      *(undefined2 *)((int)param_4 + 0x2a) = *(undefined2 *)((int)pfVar2 + 10);
      return 1;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
