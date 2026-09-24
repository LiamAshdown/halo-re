// structure_leaf_faces_gather_masked  (Ghidra: FUN_00552c20; named here)
// address 0x552c20, size 192 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: sole caller chimera__bsp_poly_movsx_2 (0x552d60, this batch); disassembly
//   (objdump -d -M intel bin/halo.exe, 0x552d84..0x552d9b) resolves the call as
//   `push eax; push ebx; push ebp; call 0x552c20`, i.e. (out_surface_indices=ebp,
//   surface_bits=ebx, out_faces=eax) in left-to-right C order. Every field is
//   ScenarioStructureBSP.surfaces (types/tags.h/types/structures.h).
// register convention: all three arguments are pushed on the stack by the sole caller (no
//   registers survive into this function itself).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)

// Copies the vertex/index records (and their global surface index) of every BSP surface whose
// bit is set in `surface_bits` into `out_faces` / `out_surface_indices`, in ascending surface
// order. `surface_bits` is a one-bit-per-surface array (types/structures.h surface_visible_bits).
void structure_leaf_faces_gather_masked(int32_t *out_surface_indices, uint32_t *surface_bits,
    ScenarioStructureBSPSurface *out_faces)
{
    int32_t surface_count = structure_bsp->surfaces.count;
    ScenarioStructureBSPSurface *surfaces = (ScenarioStructureBSPSurface *)structure_bsp->surfaces.pointer;
    int32_t surface_index = 0;
    int32_t out_count = 0;

    while (surface_index < surface_count) {
        if (*surface_bits == 0) {
            surface_index = surface_index + 32;
        } else {
            int32_t bit;
            for (bit = 0; bit < 32 && surface_index < surface_count; bit = bit + 1, surface_index = surface_index + 1) {
                if ((*surface_bits & (1u << bit)) != 0) {
                    out_surface_indices[out_count] = surface_index;
                    out_faces[out_count] = surfaces[surface_index];
                    out_count = out_count + 1;
                }
            }
        }
        surface_bits = surface_bits + 1;
    }
}

#if 0
Original Ghidra decompilation (0x552c20):

void FUN_00552c20(int *param_1,uint *param_2,int param_3)

{
  undefined2 *puVar1;
  int iVar2;
  undefined2 *puVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  short sVar7;

  sVar7 = 0;
  iVar5 = 0;
  if (0 < *(int *)(DAT_00746f9c + 0xf8)) {
    do {
      if (*param_2 == 0) {
        iVar5 = iVar5 + 0x20;
      }
      else {
        sVar4 = 0;
        iVar6 = iVar5 * 6;
        do {
          if (*(int *)(DAT_00746f9c + 0xf8) <= iVar5) break;
          if ((*param_2 & 1 << ((byte)sVar4 & 0x1f)) != 0) {
            iVar2 = *(int *)(DAT_00746f9c + 0xfc);
            *param_1 = iVar5;
            puVar3 = (undefined2 *)(iVar2 + iVar6);
            puVar1 = (undefined2 *)(param_3 + sVar7 * 6);
            *puVar1 = *puVar3;
            puVar1[1] = puVar3[1];
            param_1 = param_1 + 1;
            puVar1[2] = puVar3[2];
            sVar7 = sVar7 + 1;
          }
          sVar4 = sVar4 + 1;
          iVar5 = iVar5 + 1;
          iVar6 = iVar6 + 6;
        } while (sVar4 < 0x20);
      }
      param_2 = param_2 + 1;
    } while (iVar5 < *(int *)(DAT_00746f9c + 0xf8));
  }
  return;
}
#endif
