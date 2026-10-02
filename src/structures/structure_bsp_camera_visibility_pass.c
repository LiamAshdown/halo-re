// structure_bsp_camera_visibility_pass  (Ghidra: FUN_005544f0, still unnamed there)
// address 0x5544f0, size 217 bytes
// name confidence: 0.55 -- matches the phase4 summary ("Drives the recursive portal-flood
//   visibility pass from the camera's starting cluster, then computes a clipped view frustum for
//   every cluster it determines is visible").
// rewrite confidence: 0.85 (FIXED first-boot track: the per-cluster calls now pass bounds/camera/frustum as the
//   registers at 0x554580..0x5545ae load them) -- was 0.5: Ghidra's decompile calls FUN_0050ddc0 and the two render_camera_*
//   helpers with no visible arguments at all; objdump disassembly recovers every register load.
// evidence: objdump -M intel disassembly of 0x5544f0..0x5545d0; types/structures.h
//   structure_bsp_visible_cluster (screen_bounds_x/y at +4, the render-owned frustum block at
//   +0x14) and the render camera block (0x7c3100..0x7c3170, camera position at +0x14).
// register convention: no incoming parameters; every input is a global.
// UNSURE: render_camera_compute_frustum_bounds and chimera__render_camera_build_frustum's full
//   contracts (out of this module, render); reproduced here only to the extent their call-site
//   register loads are visible (EAX/ECX both point at the fixed camera block 0x7c3114, plus a
//   local scratch buffer and, for the second call, a literal 0 stack argument).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t render_cluster_index; // 0x007c3348
extern uint32_t *flood_recursion_bits; // 0x006e3af8
extern int16_t visible_cluster_count; // 0x007d0390
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters]; // 0x007c3390

// render module, out of this batch; see structure_bsp_mirror_query.c.
// blam-cc: EAX -> out (float[4]), ECX -> camera
extern void render_frustum_compute_screen_clip_bounds(float *out, void *camera);

extern void camera_cluster_portal_flood_recursive(int16_t cluster_index,
    polygon2d *view_polygon); // 0x5545d0, this module // this batch

// render module, out of this batch. blam-cc: EAX -> camera (0x7c3114), ECX -> scratch,
// EDX -> &visible_clusters[i].screen_bounds_x
extern uint32_t render_camera_compute_frustum_bounds(void *camera, float bounds_out[4], float bounds_in[4]); // 0x50cb70,
    // blam-cc: EAX -> camera, ECX -> bounds_out, EDX -> bounds_in
extern void chimera__render_camera_build_frustum(float *frustum_bounds, void *camera, void *frustum,
    uint8_t build_projection); // 0x50cc40, blam-cc: EAX -> frustum_bounds, ECX -> camera, ESI -> frustum,
    // stack -> build_projection

void structure_bsp_camera_visibility_pass(void)
{
    if (render_cluster_index == -1) {
        return;
    }

    float screen_bounds[4];
    render_frustum_compute_screen_clip_bounds(screen_bounds, (void *)0x7c3168);
    // Build the same permuted 4-point clip polygon 0x553560 and 0x554850's caller build: a
    // {count=4, points[4]} polygon2d-shaped buffer, laid out as {left/top, right/top, right/top,
    // left/top, right/bottom, right/bottom, left/bottom, left/bottom} pairs of (x, y) -- matching
    // the 8-dword permutation the decompile shows byte for byte.
    // Declared as the shared polygon2d (types/structures.h) rather than as a local anonymous
    // struct: camera_cluster_portal_flood_recursive and polygon2d_clip_to_planes both treat it as
    // one. Only the first 4 points are written, which is what plane_count = 4 says downstream.
    polygon2d clip_polygon;
    clip_polygon.point_count = 4;
    clip_polygon.points[0].x = screen_bounds[0]; clip_polygon.points[0].y = screen_bounds[2];
    clip_polygon.points[1].x = screen_bounds[1]; clip_polygon.points[1].y = screen_bounds[2];
    clip_polygon.points[2].x = screen_bounds[1]; clip_polygon.points[2].y = screen_bounds[3];
    clip_polygon.points[3].x = screen_bounds[0]; clip_polygon.points[3].y = screen_bounds[3];

    uint32_t recursion_bits[0x10]; // 0x40 bytes, matches flood_recursion_bits's documented size
    for (int i = 0; i < 0x10; i++) {
        recursion_bits[i] = 0;
    }
    flood_recursion_bits = recursion_bits;

    visible_cluster_count = 0;
    camera_cluster_portal_flood_recursive(render_cluster_index, &clip_polygon);

    // 0x554580..0x5545be: each visible cluster's screen bounds (+4) are narrowed against the render camera into
    // the screen_bounds local (reused), and its frustum (+0x14) is built from them without a projection
    for (int16_t i = 0; i < visible_cluster_count; i++) {
        uint8_t *cluster = (uint8_t *)&visible_clusters[i];
        render_camera_compute_frustum_bounds((void *)0x7c3114, screen_bounds, (float *)(cluster + 4));
        chimera__render_camera_build_frustum(screen_bounds, (void *)0x7c3114, cluster + 0x14, 0);
    }
}

#if 0
Original Ghidra decompilation (0x5544f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005544f0(void)

{
  int iVar1;
  undefined4 extraout_EDX;
  short sVar2;
  undefined4 *puVar3;
  undefined4 local_844 [16];
  undefined2 local_804 [1026];

  if (_DAT_007c3348 != -1) {
    FUN_0050ddc0();
    DAT_006e3af8 = local_844;
    puVar3 = local_844;
    for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    local_804[0] = 4;
    camera_cluster_portal_flood_recursive(extraout_EDX,local_804);
    sVar2 = 0;
    if (0 < DAT_007d0390) {
      do {
        render_camera_compute_frustum_bounds();
        chimera__render_camera_build_frustum(0);
        sVar2 = sVar2 + 1;
      } while (sVar2 < DAT_007d0390);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
