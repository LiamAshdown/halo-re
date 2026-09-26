// physics_shape_test_point  (Ghidra: FUN_00504260, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x504260, size 455 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/physics_types_notes.md section 2: physics_model's three counts read as
//   `(short *)(param_1 + type*2)`, and the sphere/pill/shape bases (0x0008, 0x1c08, 0x4408)
//   match exactly; physics_model_contact's fields (t 0x00, plane 0x10, object_index 0x20,
//   surface_index 0x24, flags/breakable/material 0x28..0x2a) match every store here.
// register convention: none -- all three arguments are Ghidra-recognized stack parameters.
//   // blam-cc: stack -> model, point, out_contact
// UNSURE: the sphere and pill test calls show only the out_depth argument in Ghidra's decompile;
// reconstructed using physics_shape_sphere_test_point / physics_shape_pill_test_point's own
// established EAX/ECX/EDX register convention (point, shape, out_normal).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

extern uint8_t physics_shape_sphere_test_point(real_point3d *point, physics_model_sphere *sphere,
                                                 real_plane3d *out_normal,
                                                 float *out_depth); // 0x503ec0, this batch
extern uint8_t physics_shape_pill_test_point(real_point3d *point, physics_model_pill *pill,
                                               real_plane3d *out_normal,
                                               float *out_depth); // 0x503f90, this batch
extern uint8_t physics_shape_polygon_test_point(physics_model_shape *shape, real_point3d *point,
                                                  float *out_depth,
                                                  real_plane3d *out_normal); // 0x504120, this batch

// blam-cc: stack -> model, point, out_contact
uint32_t physics_shape_test_point(physics_model *model, real_point3d *point,
                                   physics_model_contact *out_contact)
{
    int32_t best_type = -1;
    int32_t best_index = -1;
    float best_depth = -3.4028235e+38f;
    real_plane3d best_normal;
    int32_t type;

    for (type = 0; type < 3; type++) {
        int16_t count = ((int16_t *)model)[type];
        int32_t i;
        for (i = 0; i < count; i++) {
            float depth;
            real_plane3d normal;
            uint32_t hit;

            if (type == 0) {
                hit = physics_shape_sphere_test_point(point, &model->spheres[i], &normal, &depth);
            } else if (type == 1) {
                hit = physics_shape_pill_test_point(point, &model->pills[i], &normal, &depth);
            } else {
                hit = physics_shape_polygon_test_point(&model->shapes[i], point, &depth, &normal);
            }

            if (hit && (best_depth < depth)) {
                best_depth = depth;
                best_index = i;
                best_type = type;
                best_normal = normal;
            }
        }
    }

    if (best_type == -1) {
        return 0;
    }

    out_contact->t = best_depth;
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
Original Ghidra decompilation (0x504260):

undefined4 FUN_00504260(int param_1,undefined4 param_2,float *param_3)

{
  char cVar1;
  short sVar2;
  float *pfVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  float local_34;
  float local_30;
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

  iVar5 = -1;
  local_28 = -1;
  local_2c = -1;
  local_30 = -3.4028235e+38;
  iVar7 = 0;
  do {
    sVar2 = (short)iVar7;
    local_24 = (short *)(param_1 + sVar2 * 2);
    iVar6 = 0;
    if (0 < *local_24) {
      do {
        if (sVar2 == 0) {
          cVar1 = FUN_00503ec0(&local_34);
joined_r0x005042fb:
          if ((cVar1 != '\0') && (local_30 < local_34)) {
            local_30 = local_34;
            local_2c = iVar6;
            local_28 = iVar7;
            local_20 = local_10;
            local_1c = local_c;
            local_18 = local_8;
            local_14 = local_4;
          }
        }
        else {
          if (sVar2 == 1) {
            cVar1 = FUN_00503f90(&local_34);
            goto joined_r0x005042fb;
          }
          if (sVar2 == 2) {
            cVar1 = FUN_00504120(&local_34,&local_10);
            goto joined_r0x005042fb;
          }
        }
        iVar6 = iVar6 + 1;
        iVar5 = local_2c;
      } while ((short)iVar6 < *local_24);
    }
    iVar7 = iVar7 + 1;
    if (2 < (short)iVar7) {
      sVar2 = (short)local_28;
      if (sVar2 == -1) {
        return 0;
      }
      *param_3 = local_30;
      param_3[4] = local_20;
      param_3[5] = local_1c;
      param_3[6] = local_18;
      param_3[7] = local_14;
      sVar4 = (short)iVar5;
      if (sVar2 == 0) {
        pfVar3 = (float *)(sVar4 * 0x1c + 8 + param_1);
      }
      else if (sVar2 == 1) {
        pfVar3 = (float *)(param_1 + 0x1c08 + sVar4 * 0x28);
      }
      else {
        if (sVar2 != 2) {
          return 1;
        }
        pfVar3 = (float *)(sVar4 * 0x68 + 0x4408 + param_1);
      }
      param_3[8] = *pfVar3;
      param_3[9] = pfVar3[1];
      *(undefined1 *)(param_3 + 10) = *(undefined1 *)(pfVar3 + 2);
      *(undefined1 *)((int)param_3 + 0x29) = *(undefined1 *)((int)pfVar3 + 9);
      *(undefined2 *)((int)param_3 + 0x2a) = *(undefined2 *)((int)pfVar3 + 10);
      return 1;
    }
  } while( true );
}
#endif
