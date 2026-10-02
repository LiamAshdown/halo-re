// path_find_trace_cluster_boundary_from_vertex  (Ghidra: path_find_trace_cluster_boundary_from_vertex, renamed)
// address 0x43d790, size 383 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (verified against objdump 0x43d790..0x43d90e)
// evidence: re-derived from the disassembly (0x43d790..0x43d90e). The function walks a 2D ray
//   across the projected surfaces of the context's collision BSP: it clips the ray against the
//   current surface with collision_bsp_surface_clip_line_2d (0x5017f0, physics), then crosses
//   into the neighbouring surface on the enter side or the exit side while that surface is
//   passable, and reports where the walk stopped.
// reconciled: R53 one real signature: uint8_t f(void *context /*EAX*/, uint8_t ignore_permission,
//   real_point2d *point, int32_t start_index, real_vector2d *direction, float max_distance,
//   path_find_boundary_trace_result *out), cdecl, 6 stack arguments, returns AL. The old
//   parameter roles were wrong: arg2 is an int32 surface index (EBP, replaced by each surface
//   crossed into: mov ebp,eax; push eax; jmp 0x43d7ce), arg4 is max_distance (the float every
//   comparison uses), arg1/arg3 go straight through to 0x5017f0, and the out block's second
//   and third slots are indices, not floats. out[0] = distance, out[1] = the surface the walk
//   ended in, out[2] = the edge hit (-1 and out[0] = max_distance on a miss).
// reconciled: R79 0x006b8d78 is physics.h breakable_surface_globals (ai_path_permission_table ->
//   breakable_surface_state) and 0x0069e8d8 the structure BSP index (local_command_list_generation
//   -> global_structure_bsp_index): the permission row is active[bsp index] (shl 5, +1, 0x43d7bb).
//
// register convention: EAX -> context; stack -> ignore_permission, point, start_index,
//   direction, max_distance, out.
//   // blam-cc: EAX -> context, stack -> ignore_permission, point, start_index, direction,
//   //   max_distance, out
// Layout the function reads: context +0xb4 is the ModelCollisionGeometryBSP *, context +0x1e8
//   a per-surface permission byte array (0 = never passable; sign bit set = passable only while
//   the surface's breakable surface bit is set in the current BSP's row).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "physics.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern breakable_surface_globals *breakable_surface_state; // 0x006b8d78, physics.h
extern int16_t global_structure_bsp_index;                 // 0x0069e8d8, physics.h
extern uint32_t collision_bsp_surface_clip_line_2d(collision_bsp_boundary_clip *clip,
                                                   ModelCollisionGeometryBSP *bsp,
                                                   int32_t surface_index, real_point2d *origin,
                                                   real_vector2d *direction);
    // 0x5017f0, src/physics/collision_bsp_surface_clip_line_2d.c; ECX -> clip

// The shared passability test (0x43d7ea..0x43d82d for the enter side, 0x43d845..0x43d888 for
// the exit side). The permission byte is read before the -1 test, exactly as the binary does.
static uint8_t path_find_surface_passable(const uint8_t *surface_permissions, uint8_t ignore_permission,
                                          const ModelCollisionGeometryBSP *bsp,
                                          const uint32_t *intact_row, int32_t surface_index)
{
    uint8_t permission = surface_permissions[surface_index];
    uint32_t breakable;

    if (permission == 0) {
        return 0;
    }
    if (ignore_permission != 0 || (int8_t)permission >= 0) {
        return 1;
    }
    breakable = (uint8_t)((const ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer)[surface_index].breakable_surface;
    return (intact_row[breakable >> 5] & (1u << (breakable & 0x1f))) != 0;
}

// blam-cc: EAX -> context, stack -> ignore_permission, point, start_index, direction,
//   max_distance, out
uint8_t path_find_trace_cluster_boundary_from_vertex(void *context, uint8_t ignore_permission,
                                                     real_point2d *point, int32_t start_index,
                                                     real_vector2d *direction, float max_distance,
                                                     path_find_boundary_trace_result *out)
{
    ModelCollisionGeometryBSP *bsp = *(ModelCollisionGeometryBSP **)((uint8_t *)context + 0xb4);
    uint8_t *surface_permissions = *(uint8_t **)((uint8_t *)context + 0x1e8);
    uint32_t *intact_row = breakable_surface_state->active[global_structure_bsp_index];
    int32_t surface_index = start_index;   // EBP
    collision_bsp_boundary_clip clip;      // [esp+0x14], ECX of the 0x5017f0 call

    for (;;) {
        collision_bsp_surface_clip_line_2d(&clip, bsp, surface_index, point, direction);

        if (max_distance < clip.enter.t &&
            path_find_surface_passable(surface_permissions, ignore_permission, bsp, intact_row,
                                       clip.enter.surface_index) &&
            clip.enter.surface_index != -1) {
            surface_index = clip.enter.surface_index;
            continue;
        }
        if (clip.exit.t < max_distance &&
            path_find_surface_passable(surface_permissions, ignore_permission, bsp, intact_row,
                                       clip.exit.surface_index) &&
            clip.exit.surface_index != -1) {
            surface_index = clip.exit.surface_index;
            continue;
        }
        break;
    }

    if (max_distance < clip.enter.t) {         // 0x43d8a3
        out->distance = clip.enter.t;
        out->surface_index = surface_index;
        out->edge_index = clip.enter.edge_index;
        return 1;
    }
    if (clip.exit.t < max_distance) {          // 0x43d8cc
        out->distance = clip.exit.t;
        out->surface_index = surface_index;
        out->edge_index = clip.exit.edge_index;
        return 1;
    }
    out->distance = max_distance;              // 0x43d8f5
    out->surface_index = surface_index;
    out->edge_index = -1;
    return 0;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043d790 @ 0x43d790) ----
undefined4
FUN_0043d790(char param_1,undefined4 param_2,float param_3,undefined4 param_4,float param_5,
            float *param_6)

{
  int iVar1;
  char cVar2;
  byte bVar3;
  int iVar4;
  int iVar5;
  int in_EAX;
  float fVar6;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar4 = *(int *)(in_EAX + 0xb4);
  iVar5 = *(int *)(in_EAX + 0x1e8);
  iVar1 = DAT_0069e8d8 * 0x20 + 1 + DAT_006b8d78;
LAB_0043d7ce:
  do {
    fVar6 = param_3;
    FUN_005017f0(iVar4,fVar6,param_2,param_4);
    if (((param_5 < local_18) && (cVar2 = *(char *)((int)local_10 + iVar5), cVar2 != '\0')) &&
       ((param_1 != '\0' ||
        ((-1 < cVar2 ||
         (bVar3 = *(byte *)(*(int *)(iVar4 + 0x40) + (int)local_10 * 0xc + 9),
         (*(uint *)(iVar1 + (uint)(bVar3 >> 5) * 4) & 1 << (bVar3 & 0x1f)) != 0)))))) {
      param_3 = local_10;
      if (local_10 != -NAN) goto LAB_0043d7ce;
    }
    if (((param_5 <= local_c) || (cVar2 = *(char *)((int)local_4 + iVar5), cVar2 == '\0')) ||
       (((param_1 == '\0' &&
         ((cVar2 < '\0' &&
          (bVar3 = *(byte *)(*(int *)(iVar4 + 0x40) + (int)local_4 * 0xc + 9),
          (*(uint *)(iVar1 + (uint)(bVar3 >> 5) * 4) & 1 << (bVar3 & 0x1f)) == 0)))) ||
        (param_3 = local_4, local_4 == -NAN)))) {
      if (param_5 < local_18) {
        param_6[1] = fVar6;
        param_6[2] = local_14;
        *param_6 = local_18;
        return 1;
      }
      if (param_5 <= local_c) {
        param_6[1] = fVar6;
        *param_6 = param_5;
        param_6[2] = -NAN;
        return 0;
      }
      param_6[1] = fVar6;
      *param_6 = local_c;
      param_6[2] = local_8;
      return 1;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
