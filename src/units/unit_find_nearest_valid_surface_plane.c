// unit_find_nearest_valid_surface_plane  (Ghidra: unit_find_nearest_valid_surface_plane)
// address 0x560630, size 446 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.9
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
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78
extern int16_t global_structure_bsp_index; // 0x0069e8d8

extern void unit_get_crouch_height_offset(real_point3d *object_position, uint32_t object_index, float *pill_height,
    float *pill_radius_out); // 0x55a2e0, EAX, ECX, stack, EBX
extern uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count,
    collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center,
    float radius); // 0x501980, EAX, ECX, ESI, stack

// REWRITTEN from objdump 0x560630..0x5607ef. ECX: unit (biped_create calls it for bipeds with tag +0x2f4 bit 6).
//   Gathers the structure surfaces within the unit's pill radius + 0.05 of its position (0x501980, up to 0x100),
//   picks the one whose plane (negated for a sign-bit plane index) has the smallest signed distance to the
//   position, and stores the surface (+0x4d8), its plane (+0x514) and the normal as the unit's up (+0x80). The
//   draft called the pill query and the sphere query with the wrong operands.
void unit_find_nearest_valid_surface_plane(uint32_t unit_index) // blam-cc: ECX -> unit_index
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data; // [esp+0x2c]
    ModelCollisionGeometryBSP *bsp = global_structure_collision_bsp;                          // edi
    real_point3d position;                  // [esp+0x30]
    float pill_height;                      // [esp+0x20]
    float pill_radius;                      // [esp+0xc]
    collision_bsp_sphere_result result;     // [esp+0x4c]
    real_plane3d best_plane;                // esi, ebx, edi, [esp+0x48]
    float best_distance = 3.4028235e+38f;   // [esp+0x28]
    int32_t best_surface = -1;              // [esp+0xc]
    uint8_t *surfaces;
    uint8_t *planes;
    int16_t i;

    unit_get_crouch_height_offset(&position, unit_index, &pill_height, &pill_radius);
    if (!(uint8_t)collision_bsp_query_sphere_init(bsp, 0x100, &result,
            breakable_surface_state->active[global_structure_bsp_index], &position, pill_radius + 0.05f)) {
        return;
    }
    if (result.surface_count <= 0) {
        return;
    }
    surfaces = *(uint8_t **)&((struct ModelCollisionGeometryBSP *)bsp)->surfaces.pointer;
    planes = *(uint8_t **)&((struct ModelCollisionGeometryBSP *)bsp)->planes.pointer;
    for (i = 0; (int32_t)i < result.surface_count; i++) {
        int32_t surface = result.surfaces[i];
        int32_t plane_reference = *(int32_t *)(surfaces + surface * 0xc);
        float *plane = (float *)(planes + (plane_reference & 0x7fffffff) * 0x10);
        real_plane3d candidate;
        float distance;

        if (plane_reference < 0) {
            candidate.normal.i = -plane[0];
            candidate.normal.j = -plane[1];
            candidate.normal.k = -plane[2];
            candidate.d = -plane[3];
        } else {
            candidate.normal.i = plane[0];
            candidate.normal.j = plane[1];
            candidate.normal.k = plane[2];
            candidate.d = plane[3];
        }
        distance = position.x * candidate.normal.i + position.z * candidate.normal.k + position.y * candidate.normal.j - candidate.d;
        if (distance < best_distance) {
            best_plane = candidate;
            best_surface = surface;
            best_distance = distance;
        }
    }
    if (best_surface == -1) {
        return;
    }
    *(int32_t *)(obj + 0x4d8) = best_surface;
    *(real_plane3d *)(obj + 0x514) = best_plane;
    *(real_vector3d *)&((unit_object *)obj)->base.up.i = best_plane.normal;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
