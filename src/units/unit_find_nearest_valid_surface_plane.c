// unit_find_nearest_valid_surface_plane  (Ghidra: unit_find_nearest_valid_surface_plane)
// address 0x560630, size 446 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.15
// evidence: types/units.h biped_data.ground_surface_index/.ground_normal/.unknown_520
//   (0x4d8/0x514/0x520), and the comment "0x560630 found" on ground_surface_index; also writes
//   object.up (0x80, "puVar1+0x80/0x84/0x88" here matches the current up-vector unit_update_up_vector
//   (0x560800) reads).
// register convention: unit index in ECX.
//   // blam-cc: in_ECX -> unit_index
// UNSURE: this function collects candidate BSP surfaces via collision_bsp_query_sphere_init into a caller-local
//   array (aiStack_100c, sized as if for over a thousand entries) and a count (local_1010) that
//   Ghidra never shows an initializing write for in this decompile -- almost certainly an output
//   parameter of unit_get_crouch_height_offset or collision_bsp_query_sphere_init that the decompiler lost, the same class of gap as
//   the "hidden output" calls elsewhere in this batch. The BSP globals (DAT_00746f98,
//   DAT_0069e8d8, DAT_006b8d78) and the plane-table walk (iVar3+0x40 surface->plane index,
//   iVar3+0x10 plane array, stride 0x10 = normal.xyz + d) belong to the collision/BSP module,
//   not this one, and are reproduced only as raw offsets. ground_surface_index is stored here as
//   a genuine int-to-float *conversion* of the winning array index (`(float)aiStack_100c[i]`),
//   not a reinterpreted bit pattern, even though types/units.h and unit_update_up_vector
//   (0x560800) both treat the field as an integer datum_index compared with `== -1`; that
//   mismatch is reproduced literally rather than resolved.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0
extern uint8_t bsp_collision_globals[]; // 0x00746f98, DAT_00746f98, UNSURE shape (collision/BSP module)
extern int32_t bsp_cluster_index_006b8d78;  // 0x006b8d78, UNSURE
extern int32_t bsp_leaf_scale_0069e8d8;     // 0x0069e8d8, UNSURE

extern void unit_get_crouch_height_offset(uint32_t object_index, float *pill_height, float *pill_radius_out); // 0x55a2e0, collision pill height + radius
extern uint8_t collision_bsp_query_sphere_init(int32_t leaf_key, float *out_direction, float radius); // 0x501980, UNSURE signature

void unit_find_nearest_valid_surface_plane(uint32_t unit_index) // blam-cc: in_ECX -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    float position[3]; // UNSURE: local_103c, populated by unit_get_crouch_height_offset
    float pill_radius = 0.0f;            // the EBX-carried second output; the caller's sink for
                                         // it is not visible in this decompilation
    // ECX -> object index, EBX -> pill_radius_out; Ghidra bound only the stack
    // argument, which is the 2nd parameter.
    unit_get_crouch_height_offset(unit_index, position, &pill_radius);

    float direction[4]; // UNSURE: local_1050.. (the +0.05 radius bias applies to direction[0])
    int32_t candidates[1026];  // UNSURE: aiStack_100c
    int32_t candidate_count = 0; // UNSURE: never visibly initialized in the decompile, see file header

    if (collision_bsp_query_sphere_init(bsp_leaf_scale_0069e8d8 * 0x20 + 1 + bsp_cluster_index_006b8d78, direction,
                      direction[0] + 0.05f)) {
        float best_index = -1.0f; // local_1050 sentinel "-NAN"
        float best_score = 3.4028235e+38f;
        real_vector3d best_normal = {0}; // .i=plane.x, .j=plane.y, .k=plane.z
        float best_d = 0.0f;             // plane.d

        for (int32_t i = 0; i < candidate_count; i++) {
            int32_t plane_index = *(int32_t *)(*(uint8_t **)(bsp_collision_globals + 0x40) + candidates[i] * 0xc);
            float *plane = (float *)(*(uint8_t **)(bsp_collision_globals + 0x10) + plane_index * 0x10);
            real_vector3d normal;
            float d;
            if (plane_index < 0) {
                normal.i = -plane[0];
                normal.j = -plane[1];
                normal.k = -plane[2];
                d = -plane[3];
            } else {
                normal.i = plane[0];
                normal.j = plane[1];
                normal.k = plane[2];
                d = plane[3];
            }
            float score = (direction[2] * normal.j + direction[1] * normal.k + direction[3] * normal.i) - d;
            if (score < best_score) {
                best_d = d;
                best_normal.j = normal.j;
                best_normal.i = normal.i;
                best_normal.k = normal.k;
                best_index = (float)candidates[i];
                best_score = score;
            }
        }

        if (best_index != -1.0f) { // UNSURE: see file header on the -NAN sentinel comparison
            *(float *)&biped->ground_surface_index = best_index; // UNSURE: see file header
            biped->ground_normal.i = best_normal.i;
            biped->ground_normal.j = best_normal.j;
            obj->up.i = best_normal.i;
            biped->ground_normal.k = best_normal.k;
            obj->up.j = best_normal.j;
            biped->unknown_520 = *(uint32_t *)&best_d;
            obj->up.k = best_normal.k;
        }
    }
}

#if 0
Original Ghidra decompilation (0x560630):

void FUN_00560630(void)

{
  int iVar1;
  float fVar2;
  int iVar3;
  char cVar4;
  short sVar5;
  int iVar6;
  float *pfVar7;
  uint in_ECX;
  float local_1050;
  float local_104c;
  float local_1048;
  float local_1044;
  float local_1040;
  undefined1 local_103c [8];
  float local_1034;
  int local_1030;
  float local_102c;
  float local_1028;
  float local_1024;
  float local_1020;
  float local_101c;
  float local_1018;
  float local_1014;
  int local_1010;
  int aiStack_100c [1026];
  undefined4 uStack_4;

  iVar3 = DAT_00746f98;
  uStack_4 = 0x56063a;
  local_1030 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  FUN_0055a2e0(local_103c);
  cVar4 = FUN_00501980(DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78,&local_102c,local_1050 + 0.05);
  if (cVar4 != '\0') {
    iVar6 = 0;
    local_1050 = -NAN;
    local_1034 = 3.4028235e+38;
    sVar5 = 0;
    if (0 < local_1010) {
      do {
        iVar1 = *(int *)(*(int *)(iVar3 + 0x40) + aiStack_100c[iVar6] * 0xc);
        pfVar7 = (float *)(iVar1 * 0x10 + *(int *)(iVar3 + 0x10));
        if (iVar1 < 0) {
          local_104c = -*pfVar7;
          local_1048 = -pfVar7[1];
          local_1044 = -pfVar7[2];
          local_1040 = -pfVar7[3];
        }
        else {
          local_104c = *pfVar7;
          local_1048 = pfVar7[1];
          local_1044 = pfVar7[2];
          local_1040 = pfVar7[3];
        }
        fVar2 = (local_1028 * local_1048 + local_1024 * local_1044 + local_102c * local_104c) -
                local_1040;
        if (fVar2 < local_1034) {
          local_1014 = local_1040;
          local_101c = local_1048;
          local_1020 = local_104c;
          local_1018 = local_1044;
          local_1050 = (float)aiStack_100c[iVar6];
          local_1034 = fVar2;
        }
        sVar5 = sVar5 + 1;
        iVar6 = (int)sVar5;
      } while (iVar6 < local_1010);
      if (local_1050 != -NAN) {
        *(float *)(local_1030 + 0x4d8) = local_1050;
        *(float *)(local_1030 + 0x514) = local_1020;
        *(float *)(local_1030 + 0x518) = local_101c;
        *(float *)(local_1030 + 0x80) = local_1020;
        *(float *)(local_1030 + 0x51c) = local_1018;
        *(float *)(local_1030 + 0x84) = local_101c;
        *(float *)(local_1030 + 0x520) = local_1014;
        *(float *)(local_1030 + 0x88) = local_1018;
      }
    }
  }
  return;
}
#endif
