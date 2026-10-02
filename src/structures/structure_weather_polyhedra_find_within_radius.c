// structure_weather_polyhedra_find_within_radius  (FUN_00458b50; orphan pass 4 first named it
//   structure_regions_find_within_radius, renamed in the same pass's review)
// address 0x458b50, size 148 bytes (0x458b50..0x458be3, single `ret`)
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: objdump 0x458b50..0x458be4. The array it walks is the ScenarioStructureBSP reflexive
//   at +0x1c0 (count) / +0x1c4 (pointer), which types/tags.h places exactly on
//   ScenarioStructureBSP.weather_polyhedra (weather_palette is at +0x1b4); the 0x20-byte records
//   read at +0x00..+0x0c are ScenarioStructureBSPWeatherPolyhedron.bounding_sphere_center /
//   bounding_sphere_radius. The only caller is the weather renderer
//   (weather_instance_build_render_geometry, 0x458c7a..0x458c82: `push radius; lea edi,[out]`).
//   Test (0x458b9d..0x458bc0): |center - camera|^2 < (radius + sphere radius)^2, summed x, z, y
//   in that order; `fcompp; test ah,0x41; jne skip`, so a NaN distance never matches. At most 8
//   indices are stored (`cmp dx,8; jge`), but the loop runs to the end and re-reads the count
//   every iteration. Returns the stored count in AX. The camera position is 0x007c3114
//   (render_camera_global, the name src/render/render_camera_facing_frame_build.c uses).
// register convention: EDI = int16_t out[8]; the radius is the one stack argument.
//   // blam-cc: EDI -> out, stack -> radius; returns int16 in AX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include <stdint.h> // uintptr_t: tag block pointers are 32-bit fields
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern float render_camera_global[3];            // 0x007c3114

// Collects (up to 8) indices of the BSP weather polyhedra whose bounding spheres come within
// `radius` of the camera, and returns how many it stored.
int16_t structure_weather_polyhedra_find_within_radius(int16_t *out, float radius)
{
    ScenarioStructureBSP *bsp = global_structure_bsp;
    int16_t found = 0;
    int16_t index;

    for (index = 0; index < (int32_t)bsp->weather_polyhedra.count; index++) {
        ScenarioStructureBSPWeatherPolyhedron *polyhedron =
            (ScenarioStructureBSPWeatherPolyhedron *)(uintptr_t)bsp->weather_polyhedra.pointer + index;
        float dx = polyhedron->bounding_sphere_center.x - render_camera_global[0];
        float dy = polyhedron->bounding_sphere_center.y - render_camera_global[1];
        float dz = polyhedron->bounding_sphere_center.z - render_camera_global[2];
        float reach = radius + polyhedron->bounding_sphere_radius;

        if (dx * dx + dz * dz + dy * dy < reach * reach && found < 8) {
            out[found] = index;
            found++;
        }
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x458b50):

undefined4 FUN_00458b50(float param_1)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  short sVar9;
  short sVar10;
  int unaff_EDI;

  iVar6 = DAT_00746f9c;
  iVar7 = *(int *)(DAT_00746f9c + 0x1c0);
  sVar10 = 0;
  sVar9 = 0;
  if (0 < iVar7) {
    iVar7 = 0;
    do {
      iVar1 = *(int *)(iVar6 + 0x1c4);
      iVar8 = iVar7 * 0x20 + iVar1;
      fVar2 = *(float *)(iVar7 * 0x20 + iVar1) - DAT_007c3114;
      fVar4 = *(float *)(iVar8 + 4) - DAT_007c3118;
      fVar5 = *(float *)(iVar8 + 8) - DAT_007c311c;
      fVar3 = param_1 + *(float *)(iVar8 + 0xc);
      if ((fVar4 * fVar4 + fVar5 * fVar5 + fVar2 * fVar2 < fVar3 * fVar3) && (sVar10 < 8)) {
        *(short *)(unaff_EDI + sVar10 * 2) = sVar9;
        sVar10 = sVar10 + 1;
      }
      sVar9 = sVar9 + 1;
      iVar7 = (int)sVar9;
    } while (iVar7 < *(int *)(iVar6 + 0x1c0));
  }
  return CONCAT22((short)((uint)iVar7 >> 0x10),sVar10);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
