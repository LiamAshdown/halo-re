// physics_point_find_clear_position  (Ghidra: FUN_00507170; renamed)
// address 0x507170, size 695 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: out/phase4/physics_functions.md summary ("Searches for a nearby non-colliding
//   position for a physics point by sampling a ring of candidate offsets when the direct
//   position is blocked, then settles the point there"); types/physics.h globals
//   k_physics_displacement_directions (0x0069c460, "0x00507170 samples 0x11 [17] offsets") and
//   k_physics_displacement_direction_count = 17; physics_model_contact field layout (t, point,
//   plane) matching local_ac34/30/2c/28/../1c's relative offsets; src/items/item_update.c's own
//   global_down3d_pointer (0x0069672c) reused here for the same indirect vector
//   pointer.
// register convention: unaff_ESI -> current_position (real_point3d *). param_1..param_6 are
//   Ghidra's own recognized parameters (flags, sample_radius, x_margin, y_margin,
//   exclude_object_index, out_position).
//   // blam-cc: ESI -> current_position, stack -> flags, sample_radius, x_margin, y_margin,
//   //          exclude_object_index, out_position
// UNSURE: the very first physics_shape_test_point call (testing current_position itself, before the ring
//   search) has no visible point/out_contact arguments; this rewrite supplies
//   current_position and a throwaway scratch contact, on the assumption the registers set up
//   for the immediately preceding physics_model_build_from_sphere_query call are still live.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"
#include "fn_physics.h"

extern real_vector3d *global_down3d_pointer; // 0x0069672c
extern float k_physics_displacement_directions[k_physics_displacement_direction_count][3]; // 0x0069c460

extern uint8_t physics_shape_test_point(physics_model *model, real_point3d *point,
    physics_model_contact *out_contact); // 0x504260, this module (higher half)
extern uint8_t physics_shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta,
    physics_model_contact *out_contact); // 0x504bb0, this module (higher half)

extern uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center,
    float radius, float x_offset, float y_offset, uint32_t exclude_object_index,
    physics_model *model); // 0x506440, this module (higher half)

                                                                                    // this module

// Builds a physics_model around current_position (sphere radius = x_margin/2 + sample_radius +
// y_margin) and checks whether current_position itself is already clear (no model overlap, and
// object_collision_test_cluster_group reports no nearby-object collision either). If so, returns it unchanged. Otherwise
// samples the 17-direction displacement ring (k_physics_displacement_directions) at
// sample_radius, looking for a candidate that both misses the model and passes object_collision_test_cluster_group;
// among those, prefers the first one whose separating-plane Z component exceeds cos(40 degrees)
// (a roughly floor-like recovery direction) and walks toward it via physics_point_walk_toward_target.
// If none qualify but at least one clear candidate was found, walks toward the FIRST clear
// candidate instead, sliding from current_position to it. Returns whether a position was found.
uint8_t physics_point_find_clear_position(uint32_t flags, real_point3d *current_position,
    float sample_radius, float x_margin, float y_margin, uint32_t exclude_object_index,
    real_point3d *out_position)
{
    physics_model model;
    physics_model_contact contact;
    real_point3d sweep_center;
    uint8_t have_fallback = 0;
    real_point3d fallback_candidate;
    uint16_t i;

    sweep_center.x = current_position->x;
    sweep_center.y = current_position->y;
    sweep_center.z = x_margin * 0.5f + current_position->z;

    physics_model_build_from_sphere_query(flags, &sweep_center,
        x_margin * 0.5f + sample_radius + y_margin, x_margin, y_margin, exclude_object_index,
        &model);

    if (!physics_shape_test_point(&model, current_position, &contact)) { // UNSURE: point/out_contact args, see header
        // 0x507204: EDI = current_position
        if (!object_collision_test_cluster_group(flags, current_position, exclude_object_index)) {
            *out_position = *current_position;
            return 1;
        }
    }

    for (i = 0; i < k_physics_displacement_direction_count; i++) {
        real_point3d candidate;

        candidate.x = sample_radius * k_physics_displacement_directions[i][0] + current_position->x;
        candidate.y = sample_radius * k_physics_displacement_directions[i][1] + current_position->y;
        candidate.z = sample_radius * k_physics_displacement_directions[i][2] + current_position->z;

        if (!physics_shape_test_point(&model, &candidate, &contact) &&
            !object_collision_test_cluster_group(flags, &candidate, exclude_object_index)) { // 0x5072ad: EDI = &candidate
            real_vector3d probe;
            probe.i = sample_radius * global_down3d_pointer->i;
            probe.j = sample_radius * global_down3d_pointer->j;
            probe.k = sample_radius * global_down3d_pointer->k;

            if (physics_shape_test_ray(&model, &candidate, &probe, &contact) && 0.76604444f < contact.plane_k) {
                // state = &contact: physics_model_contact's {t; point_x,y,z} matches
                // physics_point_walk_state's {t; position} field for field, and start_position
                // is the candidate the ray was cast from (contact.point = candidate + probe*t).
                physics_point_walk_toward_target((physics_point_walk_state *)&contact, &candidate, flags, &probe,
                    exclude_object_index);
                out_position->x = contact.point_x;
                out_position->y = contact.point_y;
                out_position->z = contact.point_z;
                return 1;
            }

            if (!have_fallback) {
                fallback_candidate = candidate;
                have_fallback = 1;
            }
        }
    }

    if (have_fallback) {
        real_vector3d to_fallback;
        to_fallback.i = current_position->x - fallback_candidate.x;
        to_fallback.j = current_position->y - fallback_candidate.y;
        to_fallback.k = current_position->z - fallback_candidate.z;
        physics_shape_test_ray(&model, &fallback_candidate, &to_fallback, &contact);
        physics_point_walk_toward_target((physics_point_walk_state *)&contact, &fallback_candidate, flags, &to_fallback,
            exclude_object_index);
        out_position->x = contact.point_x;
        out_position->y = contact.point_y;
        out_position->z = contact.point_z;
        return 1;
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x507170):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4
FUN_00507170(undefined4 param_1,float param_2,float param_3,float param_4,undefined4 param_5,
            float *param_6)

{
  bool bVar1;
  char cVar2;
  int iVar3;
  ushort uVar4;
  float *unaff_ESI;
  float local_ac58;
  float local_ac54;
  float local_ac50;
  float local_ac4c;
  float local_ac48;
  float local_ac44;
  float local_ac40;
  float local_ac3c;
  float local_ac38;
  undefined1 local_ac34 [4];
  float local_ac30;
  float local_ac2c;
  float local_ac28;
  float local_ac1c;
  undefined1 local_ac08 [44036];
  undefined4 uStack_4;

  uStack_4 = 0x50717a;
  local_ac58 = *unaff_ESI;
  local_ac54 = unaff_ESI[1];
  local_ac50 = param_3 * 0.5 + unaff_ESI[2];
  FUN_00506440(param_1,&local_ac58,param_3 * 0.5 + param_2 + param_4,param_3,param_4,param_5,
               local_ac08);
  cVar2 = FUN_00504260(local_ac08);
  if (cVar2 == '\0') {
    cVar2 = FUN_00505490(param_1,param_5);
    if (cVar2 == '\0') {
      *param_6 = *unaff_ESI;
      param_6[1] = unaff_ESI[1];
      param_6[2] = unaff_ESI[2];
      return 1;
    }
  }
  bVar1 = false;
  uVar4 = 0;
  do {
    iVar3 = (int)(short)uVar4;
    local_ac4c = param_2 * (float)(&DAT_0069c460)[iVar3 * 3] + *unaff_ESI;
    local_ac48 = param_2 * (float)(&DAT_0069c464)[iVar3 * 3] + unaff_ESI[1];
    local_ac44 = param_2 * (float)(&DAT_0069c468)[iVar3 * 3] + unaff_ESI[2];
    cVar2 = FUN_00504260(local_ac08,&local_ac4c,local_ac34);
    if (cVar2 == '\0') {
      cVar2 = FUN_00505490(param_1,param_5);
      if (cVar2 == '\0') {
        local_ac58 = param_2 * *(float *)PTR_DAT_0069672c;
        local_ac54 = param_2 * *(float *)(PTR_DAT_0069672c + 4);
        local_ac50 = param_2 * *(float *)(PTR_DAT_0069672c + 8);
        cVar2 = FUN_00504bb0(local_ac08,&local_ac4c,&local_ac58,local_ac34);
        if (cVar2 != '\0') {
          if (0.76604444 < local_ac1c) {
            FUN_005070d0(param_1,&local_ac58,param_5);
            *param_6 = local_ac30;
            param_6[1] = local_ac2c;
            param_6[2] = local_ac28;
            return 1;
          }
        }
        if (!bVar1) {
          local_ac40 = local_ac4c;
          local_ac3c = local_ac48;
          local_ac38 = local_ac44;
          bVar1 = true;
        }
      }
    }
    uVar4 = uVar4 + 1;
  } while (uVar4 < 0x11);
  if (bVar1) {
    local_ac58 = *unaff_ESI - local_ac40;
    local_ac54 = unaff_ESI[1] - local_ac3c;
    local_ac50 = unaff_ESI[2] - local_ac38;
    FUN_00504bb0(local_ac08,&local_ac40,&local_ac58,local_ac34);
    FUN_005070d0(param_1,&local_ac58,param_5);
    *param_6 = local_ac30;
    param_6[1] = local_ac2c;
    param_6[2] = local_ac28;
    return 1;
  }
  return 0;
}
#endif
