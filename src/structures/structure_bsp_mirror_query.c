// structure_bsp_mirror_query  (Ghidra: FUN_00553560, still unnamed there)
// address 0x553560, size 577 bytes
// name confidence: 0.55 -- out/phase4/structures_types_notes.md corrects the phase2 summary
//   ("scans for a face/material") to a mirror search: the stride off cluster->mirrors (+0x50 count
//   / +0x54 pointer) and the out-block shape (structure_bsp_mirror_result, plane + two floats +
//   cluster index) are unambiguous. No established name exists yet, so this batch names it after
//   what it does: given a caller-space clip polygon, walk the camera's currently-visible clusters'
//   mirror lists and report the last mirror whose polygon (projected to view space and clipped)
//   intersects that clip polygon.
// rewrite confidence: 0.55 -- the bit-scan over the PVS row and the polygon clip/intersect calls
//   are preserved exactly; the one register artifact (extraout_DX after the FUN_0050ddc0 call) is
//   resolved by disassembly, not guessed -- see below.
// evidence: types/structures.h structure_bsp_mirror_result and the ScenarioStructureBSPMirror /
//   Shader / ShaderEnvironment fields it documents; disassembly of 0x553560 (objdump -d) confirms:
//     - the call to FUN_0050ddc0 (0x50ddc0, render module, below this batch) never touches EDX in
//       its own body (verified: its instructions only use ECX/EAX/ESP/x87), so the "extraout_DX"
//       Ghidra reports right after that call is simply the render_cluster_index value loaded into
//       EDX two instructions earlier, untouched by the call -- not a real second return value.
//     - param_1 (shown unused by Ghidra) is the camera/projection pointer forwarded into
//       FUN_0050ddc0's ECX input; Ghidra elided the visible argument list for that call.
// register convention: cdecl, 3 stack parameters. param_1 is the camera POSITION reference
//   structure_bsp_portal_project takes in ECX; param_2 is the camera/projection context, forwarded
//   both to FUN_0050ddc0 (in ECX) and to structure_bsp_portal_project as its `camera` stack
//   argument. Both are opaque pointers this function never dereferences itself; param_3 is the
//   out-block. (An earlier rewrite had param_1 and param_2 swapped; the frame offsets
//   esp+0x1830 / esp+0x1834, with the prologue `mov eax,0x182c; call __chkstk`, settle it.)
//   // blam-cc: stack params in order (camera_ref, camera, out)
// UNSURE: param_1's exact type/owner (render module "camera" struct); it is only ever forwarded,
//   never read here, so an opaque void* is exact and sufficient.
// reconciled: R47 ShaderEnvironment._pad_30c[8] -> runtime_mirror_value_0/_1 (floats at +0x30c/+0x310); structure_bsp_mirror_result.unknown_10/_14 -> shader_mirror_value_0/_1

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "structures.h"
#include "fn_math.h"

extern ScenarioStructureBSP *global_structure_bsp;   // 0x00746f9c
extern int32_t render_cluster_index;          // 0x007c3348
extern tag_instance *tag_instances;           // 0x0087bc14, stride 0x20, tag data at +0x14

// render module, below this batch's assigned range. Computes the camera's screen-space clip
// rectangle (left, right, top, bottom) from a projection/camera struct into a 4-float buffer.
// blam-cc: EAX -> out (float[4]), ECX -> camera
extern void render_frustum_compute_screen_clip_bounds(float *out, void *camera);

// math module, canonical form (src/math/polygon2d_clip_to_planes.c). blam-cc: ECX -> vertex_count,
// EDX -> vertices. Disassembly at 0x5536ba..0x5536e1 gives the full argument list: the SUBJECT is
// the mirror's projected polygon (ECX = its count at esp+0x834, EDX = its points at esp+0x838),
// the CLIP polygon is the 4-point screen rectangle, and the destination is the point array of the
// result polygon whose count is stored separately at esp+0x1038.
// clips a fan of `plane_count` (x,y) plane pairs built from `points` against the polygon in
// `out_polygon` (already resident there from a previous fill); returns the surviving point count,
// -1 for "fully inside, unchanged" or 0 for "fully clipped away". The 0x38d1b717 magic constant is
// the same one every caller in this module passes -- an internal epsilon/version tag the routine
// itself owns.


// this batch (0x554850): projects a portal/mirror polygon into view space, near-clips it and
// perspective-projects the survivors into `out`. Returns 0 (fully visible), 1 (fully behind/culled)
// or 2 (straddles the near plane, `out` not filled by this call).
// blam-cc: EAX -> plane, ECX -> camera_ref, EDX -> vertices, stack -> the rest
extern uint8_t structure_bsp_portal_project(real_plane3d *plane, void *camera_ref,
                                             real_point3d *vertices, void *camera,
                                             uint32_t vertex_count, int16_t winding,
                                             polygon2d *out);

static ShaderEnvironment *mirror_shader_environment(ScenarioStructureBSPMirror *mirror)
{
    tag_instance *instance = &tag_instances[mirror->shader.tag_id.index];
    return (ShaderEnvironment *)instance->data;
}

// blam-cc: stack params (camera_ref, camera, out)
// Parameter naming corrected against the frame (`mov eax,0x182c; call __chkstk`, so arg_n is at
// esp+0x182c+4n): arg1 is the pointer handed to structure_bsp_portal_project in ECX (the camera
// POSITION the plane-side test uses), and arg2 is the one handed both to FUN_0050ddc0 and to
// structure_bsp_portal_project as its `camera` transform context. An earlier rewrite had the two
// the other way round.
uint8_t structure_bsp_mirror_query(void *camera_ref, void *camera,
                                    structure_bsp_mirror_result *out)
{
    uint8_t found = 0;
    float screen_bounds[4]; // left, right, top, bottom -- consumed only as the clip-plane pairs
                             // built below (points_and_planes), never read directly here.
    // Sized to match the original stack buffer (local_1808, 0x100 real_point2d) even though only
    // the first 4 are ever written; polygon2d_clip_to_planes is told plane_count=4 either way.
    real_point2d clip_points[0x100];
    polygon2d clip_polygon; // local_1008 in the decompile, reused as the running clip target
    polygon2d project_out;  // local_1808, the mirror's projected/clipped polygon

    if (render_cluster_index == -1) {
        return 0;
    }

    // FUN_0050ddc0 fills screen_bounds = {left, right, top, bottom} from the camera; the caller
    // then builds a 4-point clip rectangle out of it in the permuted order the decompile shows
    // (left/top, right/top, right/bottom, left/bottom -- i.e. the box corners in winding order).
    render_frustum_compute_screen_clip_bounds(screen_bounds, camera);
    clip_points[0].x = screen_bounds[0]; clip_points[0].y = screen_bounds[2];
    clip_points[1].x = screen_bounds[1]; clip_points[1].y = screen_bounds[2];
    clip_points[2].x = screen_bounds[1]; clip_points[2].y = screen_bounds[3];
    clip_points[3].x = screen_bounds[0]; clip_points[3].y = screen_bounds[3];

    int32_t cluster_count = global_structure_bsp->clusters.count;
    if (cluster_count <= 0) {
        return found;
    }

    // The camera's own PVS row: cluster_data.pointer + row_dwords * render_cluster_index * 4,
    // row_dwords = (cluster_count + 0x1f) >> 5. Verified by disassembly (extraout_DX above).
    int32_t row_dwords = (cluster_count + 0x1f) >> 5;
    uint32_t *pvs_row = (uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                      row_dwords * render_cluster_index * 4);

    for (int16_t cluster_index = 0; cluster_index < cluster_count; pvs_row++) {
        if (*pvs_row == 0) {
            cluster_index = (int16_t)(cluster_index + 0x20);
            continue;
        }
        for (int bit = 0; bit < 0x20 && cluster_index < cluster_count; bit++, cluster_index++) {
            if ((*pvs_row & (1u << bit)) == 0) {
                continue;
            }
            ScenarioStructureBSPCluster *cluster =
                &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_index];
            for (int32_t m = 0; m < (int32_t)cluster->mirrors.count; m++) {
                ScenarioStructureBSPMirror *mirror =
                    &((ScenarioStructureBSPMirror *)cluster->mirrors.pointer)[m];
                int16_t project_result = structure_bsp_portal_project(
                    (real_plane3d *)&mirror->plane, camera_ref,
                    (real_point3d *)mirror->vertices.pointer, camera,
                    mirror->vertices.count, 1, &project_out);
                int16_t clip_result = 0;
                if (project_result == 0) {
                    clip_result = polygon2d_clip_to_planes(project_out.point_count,
                                        &project_out.points[0], 4, clip_points, 0x100,
                                        &clip_polygon.points[0], 9.99999975e-05f); // float bits 0x38d1b717
                    clip_polygon.point_count = clip_result;
                } else if (project_result == 2) {
                    clip_result = 1; // fall straight into the "accept" path, matching the goto
                } else {
                    continue;
                }
                if (clip_result == 0) {
                    continue;
                }
                ShaderEnvironment *shader_env = mirror_shader_environment(mirror);
                if (shader_env->base.shader_type == shadertype_environment) {
                    out->shader_mirror_value_0 = shader_env->runtime_mirror_value_0; // 0x55371b
                    out->shader_mirror_value_1 = shader_env->runtime_mirror_value_1; // 0x553724
                } else {
                    out->shader_mirror_value_0 = 0.0f;
                    out->shader_mirror_value_1 = 0.0f;
                }
                out->plane.normal.i = mirror->plane.vector.i;
                out->plane.normal.j = mirror->plane.vector.j;
                out->plane.normal.k = mirror->plane.vector.k;
                out->plane.d = mirror->plane.w;
                out->cluster_index = cluster_index;
                found = 1;
            }
        }
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x553560):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined1 FUN_00553560(undefined4 param_1,undefined4 param_2,undefined4 *param_3)

{
  undefined1 uVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  short extraout_DX;
  short sVar5;
  short sVar6;
  undefined4 *puVar8;
  undefined1 local_1829;
  uint *local_1828;
  undefined1 local_1808 [2048];
  undefined1 local_1008 [2052];
  short local_804;
  undefined1 local_800 [2044];
  undefined4 uStack_4;

  iVar4 = DAT_00746f9c;
  uStack_4 = 0x55356a;
  local_1829 = 0;
  uVar1 = 0;
  if (_DAT_007c3348 != -1) {
    FUN_0050ddc0();
    iVar7 = *(int *)(iVar4 + 0x134);
    local_1828 = (uint *)(*(int *)(iVar4 + 0x14c) + (iVar7 + 0x1f >> 5) * (int)extraout_DX * 4);
    sVar6 = 0;
    uVar1 = local_1829;
    if (0 < iVar7) {
      do {
        if (*local_1828 == 0) {
          sVar6 = sVar6 + 0x20;
        }
        else {
          sVar5 = 0;
          do {
            if (*(int *)(DAT_00746f9c + 0x134) <= (int)sVar6) break;
            if ((*local_1828 & 1 << ((byte)sVar5 & 0x1f)) != 0) {
              iVar4 = sVar6 * 0x68 + *(int *)(DAT_00746f9c + 0x138);
              iVar7 = 0;
              sVar3 = 0;
              if (0 < *(int *)(iVar4 + 0x50)) {
                do {
                  puVar8 = (undefined4 *)(iVar7 * 0x40 + *(int *)(iVar4 + 0x54));
                  sVar2 = FUN_00554850(param_2,*(undefined2 *)(puVar8 + 0xd),1,local_1008);
                  if (sVar2 == 0) {
                    local_804 = polygon2d_clip_to_planes(4,local_1808,0x100,local_800,0x38d1b717);
                    if (local_804 != 0) {
LAB_005536fe:
                      iVar7 = *(int *)((puVar8[0xc] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
                      if (*(short *)(iVar7 + 0x24) == 3) {
                        param_3[4] = *(undefined4 *)(iVar7 + 0x30c);
                        param_3[5] = *(undefined4 *)(iVar7 + 0x310);
                      }
                      else {
                        param_3[4] = 0;
                        param_3[5] = 0;
                      }
                      *param_3 = *puVar8;
                      param_3[1] = puVar8[1];
                      param_3[2] = puVar8[2];
                      param_3[3] = puVar8[3];
                      *(short *)(param_3 + 6) = sVar6;
                      local_1829 = 1;
                    }
                  }
                  else if (sVar2 == 2) goto LAB_005536fe;
                  sVar3 = sVar3 + 1;
                  iVar7 = (int)sVar3;
                } while (iVar7 < *(int *)(iVar4 + 0x50));
              }
            }
            sVar5 = sVar5 + 1;
            sVar6 = sVar6 + 1;
          } while (sVar5 < 0x20);
        }
        local_1828 = local_1828 + 1;
        uVar1 = local_1829;
      } while ((int)sVar6 < *(int *)(DAT_00746f9c + 0x134));
    }
  }
  return uVar1;
}
#endif
