// chimera__bsp_poly_movsx_2  (Ghidra name via Chimera mod signature match; kept as-is)
// address 0x552d60, size 118 bytes
// name confidence: 0.55 (Chimera signature, not a Bungie-shaped name -- kept per project
//   convention that Chimera sigs are valid even when the label reads like a mod-tool artifact)
// rewrite confidence: 0.5
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x552d60..0x552dd5) resolves every
//   implicit register: `test di,di` / `movsx edx,di` -> EDI is visible_surface_count; the sole
//   caller (structure_picked_polygon_refresh, 0x5527f0) sets EDI from
//   *(int*)visible_surface_count and EBX from &surface_visible_bits before the call, matching
//   types/structures.h's globals. The two branches call rasterizer_dynamic_index_cache_reserve (lock a geometry buffer
//   sized for `count` vertices) then rasterizer_dynamic_index_slot_lock (returns the locked vertex destination
//   pointer), then either structure_leaf_faces_gather_masked (when surface_bits != NULL) or
//   structure_leaf_faces_gather_list (when it is NULL -- never true from the sole caller, but the
//   branch is real code and is preserved).
// register convention: stack -> visible_surface_indices, EBX -> surface_bits,
//   EDI -> visible_surface_count.
// UNSURE: rasterizer_dynamic_index_cache_reserve and rasterizer_dynamic_index_slot_lock (both foreign, well outside this module's address range)
//   are not examined beyond the one argument/return each visibly carries here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern int16_t geometry_buffer_warning;  // 0x0069fa48: accessed as WORD in the binary // 0x0069fa48, this module
extern void **rasterizer_device; // 0x006e09e8, physics.h/objects.h (read, not owned)

extern int32_t rasterizer_dynamic_index_cache_reserve(int16_t vertex_count); // 0x51bd60, foreign render module; UNSURE
    // blam-cc: EDX -> vertex_count
extern void *rasterizer_dynamic_index_slot_lock(int32_t geometry_handle); // 0x511e80, foreign render module; UNSURE
    // blam-cc: ECX -> geometry_handle; returns the locked vertex destination buffer
extern void structure_leaf_faces_gather_masked(int32_t *out_surface_indices, uint32_t *surface_bits,
    ScenarioStructureBSPSurface *out_faces); // 0x552c20, this batch
extern void structure_leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces,
    int32_t *face_indices); // 0x552cf0, this batch

// Locks a geometry buffer sized for `visible_surface_count` vertices, fills it with either the
// full ordered list of visible surfaces (when `surface_bits` is NULL) or the masked subset named
// by `surface_bits`, and submits the buffer through the rasterizer device's vtable. Returns the
// geometry handle, or -1 if there was nothing to draw or the buffer lock failed.
int32_t chimera__bsp_poly_movsx_2(int32_t *visible_surface_indices, uint32_t *surface_bits,
    int16_t visible_surface_count)
    // blam-cc: EBX -> surface_bits, EDI -> visible_surface_count
{
    void (**vtable)(void *);

    if (visible_surface_count > 0) {
        int32_t geometry_handle = rasterizer_dynamic_index_cache_reserve(visible_surface_count);
        if (geometry_handle != -1) {
            void *vertex_buffer = rasterizer_dynamic_index_slot_lock(geometry_handle);

            if (surface_bits != 0) {
                structure_leaf_faces_gather_masked(visible_surface_indices, surface_bits,
                    (ScenarioStructureBSPSurface *)vertex_buffer);
            } else {
                structure_leaf_faces_gather_list(visible_surface_count,
                    (ScenarioStructureBSPSurface *)vertex_buffer, visible_surface_indices);
            }

            vtable = *(void (***)(void *))rasterizer_device;
            vtable[0xc](rasterizer_device); // slot +0x30, submit
            return geometry_handle;
        }
        if (geometry_buffer_warning != 0) {
            geometry_buffer_warning = 0;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x552d60):

int chimera__bsp_poly_movsx_2(undefined4 param_1)

{
  int iVar1;
  int unaff_EBX;
  short unaff_DI;

  if (0 < unaff_DI) {
    iVar1 = FUN_0051bd60();
    if (iVar1 != -1) {
      FUN_00511e80();
      if (unaff_EBX == 0) {
        FUN_00552cf0();
        (**(code **)(*DAT_006e09e8 + 0x30))(DAT_006e09e8);
        return iVar1;
      }
      FUN_00552c20(param_1);
      (**(code **)(*DAT_006e09e8 + 0x30))(DAT_006e09e8);
      return iVar1;
    }
    if (DAT_0069fa48 != 0) {
      DAT_0069fa48 = 0;
    }
  }
  return -1;
}
#endif
