// structure_bsp_expand_visible_clusters_by_plane  (Ghidra: FUN_00553a70, still unnamed)
// address 0x553a70, size 452 bytes
// name confidence: 0.6 -- phase4 summary: "Expands cluster visibility by classifying each
//   cluster's reference points against the frustum side planes and marking newly visible leaves"
//   (again "leaves" should read "surfaces": the bits this sets are surface_visible_bits).
// rewrite confidence: 0.5 -- required objdump disassembly to recover what Ghidra collapsed to
//   three bare `render_frustum_classify_point_side_planes()` calls. Findings, all verified against
//   the machine code:
//   - the SAME frustum-pointer selection as 0x553920 (this batch) picks either this visible
//     cluster's own clipped frustum (&visible_clusters[i].unknown_014) or the fixed global
//     camera/projection block at 0x7c3168, and is loaded into ECX before each of the 3 calls.
//   - structure_bsp_cluster_surface_run's two "unresolved" leading dwords
//     (types/structures.h) are a (lightmap_index, material_index) pair: the code indexes
//     tag->lightmaps[run.unknown_00].materials.pointer[run.unknown_04] once per run, then reuses
//     that ScenarioStructureBSPMaterial for every surface the run lists.
//   - each surface's three vertex indices (ScenarioStructureBSPSurface.vertex0/1/2_index) are
//     looked up in material->compressed_vertices.pointer (stride 0x20,
//     ScenarioStructureBSPMaterialCompressedRenderedVertex) and each vertex pointer is classified
//     against the chosen frustum with render_frustum_classify_point_side_planes, EDX = the vertex
//     pointer. Three 6-bit outside-plane masks are ANDed together (masked to 0x3f); a common
//     outside plane across all three vertices rejects the triangle (surface stays not-visible),
//     otherwise the surface bit is set.
// evidence: objdump -M intel disassembly of 0x553a70..0x553c40; types/tags.h
//   ScenarioStructureBSPLightmap.materials, ScenarioStructureBSPMaterial.compressed_vertices,
//   ScenarioStructureBSPSurface; types/structures.h structure_bsp_cluster_surface_run and
//   structure_bsp_visible_cluster.
// register convention: cdecl, one stack parameter (the resident structure_bsp tag pointer).
// UNSURE: none left for this function's own logic; render_frustum_classify_point_side_planes's
//   full contract (out of module, render, 0x50d4c0) is only known well enough to reproduce this
//   call site: ECX -> frustum_or_camera, EDX -> vertex pointer, returns a 6-bit side-plane mask.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#include "fn_structures.h"

extern int32_t render_cluster_index;                     // 0x007c3348
extern uint8_t debug_render_cluster_pvs;                  // 0x00724a45
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters]; // 0x007c3390
extern int16_t visible_cluster_count;                     // 0x007d0390
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits]; // 0x007d0394
extern int16_t visible_surface_count;                     // 0x00850394

// render module, out of this batch. blam-cc: ECX -> frustum_or_camera, EDX -> vertex position
// pointer (ScenarioStructureBSPMaterialCompressedRenderedVertex *); returns a bitmask, one bit per
// side plane the vertex is outside of (bits above 0x3f are not meaningful to this caller).
extern uint16_t render_frustum_classify_point_side_planes(void *frustum_or_camera, void *vertex);

void structure_bsp_expand_visible_clusters_by_plane(ScenarioStructureBSP *tag)
{
    ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)tag->clusters.pointer;
    ScenarioStructureBSPLightmap *lightmaps = (ScenarioStructureBSPLightmap *)tag->lightmaps.pointer;
    ScenarioStructureBSPSurface *surfaces = (ScenarioStructureBSPSurface *)tag->surfaces.pointer;

    for (int16_t i = 0; i < visible_cluster_count; i++) {
        ScenarioStructureBSPCluster *cluster = &clusters[visible_clusters[i].cluster_index];
        void *frustum_or_camera;
        if (debug_render_cluster_pvs != 0 || render_cluster_index == -1) {
            frustum_or_camera = (void *)0x7c3168;
        } else {
            frustum_or_camera = &visible_clusters[i].unknown_014;
        }

        // surface_indices is the run-encoded form (structure_bsp_cluster_surface_run): a 3-dword
        // header (lightmap_index, material_index, surface_count) followed by that many surface
        // indices, repeated until the reflexive's total int32 count (+0x44, here "run_dwords") is
        // exhausted. `cursor` is a raw dword cursor across the whole block (not per-run) to match
        // the original's exact stopping point: if the 0x4000 visible-surface cap is hit partway
        // through a run's surface list, the original does NOT skip to the next run header -- it
        // just stops the inner scan where it is and re-enters the outer loop, which then
        // reinterprets whatever dwords are left (however many surface indices of the current run
        // remain) as a fresh 3-dword header. That is almost certainly a latent bug in the
        // original (only reachable when a single BSP has close to 0x4000 visible surfaces), but
        // it is preserved here rather than "fixed".
        int32_t *cursor = (int32_t *)cluster->surface_indices.pointer;
        int32_t consumed = 0;
        int32_t run_dwords = cluster->surface_indices.count;
        while (consumed < run_dwords) {
            int32_t lightmap_index = cursor[0];
            int32_t material_index = cursor[1];
            int32_t surface_count = cursor[2];
            cursor += 3;
            consumed += 3;
            ScenarioStructureBSPMaterial *material =
                &((ScenarioStructureBSPMaterial *)lightmaps[lightmap_index].materials.pointer)
                    [material_index];
            ScenarioStructureBSPMaterialCompressedRenderedVertex *vertices =
                (ScenarioStructureBSPMaterialCompressedRenderedVertex *)
                    material->compressed_vertices.pointer;

            int32_t run_end = consumed + surface_count;
            while (consumed < run_end && visible_surface_count < 0x4000) {
                int32_t surface_index = *cursor;
                int32_t word = surface_index >> 5;
                uint32_t mask = 1u << (surface_index & 0x1f);
                cursor++;
                if ((surface_visible_bits[word] & mask) == 0) {
                    ScenarioStructureBSPSurface *surface = &surfaces[surface_index];
                    uint16_t outside0 = render_frustum_classify_point_side_planes(
                        frustum_or_camera, &vertices[surface->vertex0_index]);
                    int rejected = 0;
                    if (outside0 != 0) {
                        uint16_t outside1 = render_frustum_classify_point_side_planes(
                            frustum_or_camera, &vertices[surface->vertex1_index]);
                        if (outside1 != 0) {
                            uint16_t outside2 = render_frustum_classify_point_side_planes(
                                frustum_or_camera, &vertices[surface->vertex2_index]);
                            if (outside2 != 0 && (outside2 & outside0 & 0x3f & outside1) != 0) {
                                rejected = 1;
                            }
                        }
                    }
                    if (!rejected) {
                        surface_visible_bits[word] |= mask;
                        visible_surface_count++;
                    }
                }
                consumed++;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x553a70):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00553a70(int param_1)

{
  int iVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  int iVar5;
  int iVar6;
  short sVar7;
  uint uVar8;
  int *piVar9;
  int iVar10;

  sVar7 = 0;
  if (0 < DAT_007d0390) {
    do {
      iVar5 = (short)(&DAT_007c3390)[sVar7 * 0xd0] * 0x68 + *(int *)(param_1 + 0x138);
      piVar9 = *(int **)(iVar5 + 0x48);
      iVar10 = 0;
      if (0 < *(int *)(iVar5 + 0x44)) {
        do {
          iVar1 = piVar9[2] + 3 + iVar10;
          iVar10 = iVar10 + 3;
          piVar9 = piVar9 + 3;
          while ((iVar10 < iVar1 && ((short)DAT_00850394 < 0x4000))) {
            uVar8 = 1 << ((byte)*piVar9 & 0x1f);
            iVar6 = *piVar9 >> 5;
            piVar9 = piVar9 + 1;
            if ((uVar8 & (&DAT_007d0394)[iVar6]) == 0) {
              uVar2 = render_frustum_classify_point_side_planes();
              if (uVar2 != 0) {
                uVar3 = render_frustum_classify_point_side_planes();
                if (uVar3 != 0) {
                  uVar4 = render_frustum_classify_point_side_planes();
                  if ((uVar4 != 0) && ((uVar4 & uVar2 & 0x3f & uVar3) != 0)) goto LAB_00553bfe;
                }
              }
              (&DAT_007d0394)[iVar6] = (&DAT_007d0394)[iVar6] | uVar8;
              DAT_00850394._0_2_ = (short)DAT_00850394 + 1;
            }
LAB_00553bfe:
            iVar10 = iVar10 + 1;
          }
        } while (iVar10 < *(int *)(iVar5 + 0x44));
      }
      sVar7 = sVar7 + 1;
    } while (sVar7 < DAT_007d0390);
  }
  return;
}
#endif
