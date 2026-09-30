// structure_picked_polygon_refresh  (Ghidra: FUN_005527f0; named here)
// address 0x5527f0, size 244 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x5527f0..0x552838) confirms the
//   call to chimera__bsp_poly_movsx_2(&visible_surface_indices) and the leaf-index/leaf_map
//   arguments to structure_leaf_portal_vertex_count_debug (EAX -> leaf index, ECX -> &leaf_map);
//   every other global here is already named in types/structures.h's globals list (picked-polygon
//   debug visualisation and the fog plane vector).
// register convention: none (void).
// UNSURE: the two debug vertex-count loops (picked portal, and "count every portal") compute a
//   value that is discarded, same as structure_leaf_portal_vertex_count_debug (0x5520b0); kept
//   for parity with that function rather than dropped.
// reconciled: R76 0x00696714 k_default_fog_plane_vector -> math.h const real_point3d *global_origin3d_pointer (points at the (0,0,0) constant 0x0065c230)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#include "fn_structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)
extern int16_t visible_surface_count;   // 0x00850394: every store in the binary is
    // a WORD op (`mov WORD PTR ds:0x850394,0` / `inc WORD PTR`); the two debug readers
    // here load it as a dword only because the callee consumes AX alone    // 0x00850394, this module
extern int32_t visible_surface_indices[k_maximum_visible_surfaces]; // 0x00850398, this module
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits]; // 0x007d0394, this module
extern uint8_t picked_surfaces_valid;    // 0x006e3ad8, this module
extern int32_t picked_surfaces_geometry; // 0x006e3adc, this module
extern int32_t picked_leaf_map_leaf;     // 0x0069fa40, this module
extern int32_t picked_leaf_map_portal;   // 0x0069fa44, this module
extern uint8_t debug_count_all_leaf_portals; // 0x00724a46, this module
extern const real_point3d *global_origin3d_pointer; // 0x00696714, math.h (== 0x0065c230, the zero point)
extern uint8_t fog_plane_vector_valid;  // 0x006e3ae0: accessed as BYTE   // 0x006e3ae0, this module
extern real_vector3d fog_plane_vector;   // 0x006e3ae4, this module

extern int32_t chimera__bsp_poly_movsx_2(int32_t *visible_surface_indices, uint32_t *surface_bits,
    int16_t visible_surface_count); // 0x552d60, this batch
extern void structure_leaf_portal_vertex_count_debug(int32_t leaf_index, structure_bsp_leaf_map *leaf_map); // 0x5520b0, this batch

// Refreshes the debug "picked BSP polygon" geometry handle from the current visible-surface list,
// re-runs the (discarded-result) leaf/portal debug counting passes when their console-controlled
// indices or toggle are active, and resets the fog plane vector to its per-frame default ahead of
// structure_bsp_find_mirror / structure_fog_environment_build filling it in.
void structure_picked_polygon_refresh(void)
{
    structure_bsp_leaf_map *leaf_map = (structure_bsp_leaf_map *)((uint8_t *)global_structure_bsp + 0x26c);

    picked_surfaces_geometry = chimera__bsp_poly_movsx_2(visible_surface_indices, surface_visible_bits,
        (int16_t)visible_surface_count);
    picked_surfaces_valid = picked_surfaces_geometry != -1;

    if (picked_leaf_map_leaf > -1 && picked_leaf_map_leaf < leaf_map->leaves.count) {
        structure_leaf_portal_vertex_count_debug(picked_leaf_map_leaf, leaf_map);
    }

    if (picked_leaf_map_portal > -1 && picked_leaf_map_portal < leaf_map->portals.count) {
        ScenarioStructureBSPGlobalLeafPortal *portals =
            (ScenarioStructureBSPGlobalLeafPortal *)leaf_map->portals.pointer;
        int32_t vertex_count = portals[picked_leaf_map_portal].vertices.count;
        int16_t discarded_count = 2;
        while (discarded_count < vertex_count) {
            discarded_count = discarded_count + 1;
        }
    }

    if (debug_count_all_leaf_portals != 0) {
        ScenarioStructureBSPGlobalLeafPortal *portals =
            (ScenarioStructureBSPGlobalLeafPortal *)leaf_map->portals.pointer;
        int32_t i;
        for (i = 0; i < leaf_map->portals.count; i = i + 1) {
            int32_t vertex_count = portals[i].vertices.count;
            int16_t discarded_count = 2;
            while (discarded_count < vertex_count) {
                discarded_count = discarded_count + 1;
            }
        }
    }

    fog_plane_vector_valid = 0;
    fog_plane_vector = *(const real_vector3d *)global_origin3d_pointer;
}

#if 0
Original Ghidra decompilation (0x5527f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005527f0(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  uint uVar4;

  iVar2 = DAT_00746f9c;
  DAT_006e3adc = chimera__bsp_poly_movsx_2(&DAT_00850398);
  DAT_006e3ad8 = DAT_006e3adc != -1;
  if ((-1 < DAT_0069fa40) && (DAT_0069fa40 < *(int *)(iVar2 + 0x270))) {
    FUN_005520b0();
  }
  if ((-1 < (int)DAT_0069fa44) && ((int)DAT_0069fa44 < *(int *)(iVar2 + 0x27c))) {
    iVar1 = *(int *)(*(int *)(iVar2 + 0x280) + ((DAT_0069fa44 & 0x7fffffff) + DAT_0069fa44 * 2) * 8
                    + 0xc);
    sVar3 = 2;
    if (2 < iVar1) {
      do {
        sVar3 = sVar3 + 1;
      } while (sVar3 < iVar1);
    }
  }
  if (DAT_00724a46 != '\0') {
    uVar4 = 0;
    if (0 < *(int *)(iVar2 + 0x27c)) {
      do {
        iVar1 = *(int *)(*(int *)(iVar2 + 0x280) + ((uVar4 & 0x7fffffff) + uVar4 * 2) * 8 + 0xc);
        sVar3 = 2;
        if (2 < iVar1) {
          do {
            sVar3 = sVar3 + 1;
          } while (sVar3 < iVar1);
        }
        uVar4 = uVar4 + 1;
      } while ((int)uVar4 < *(int *)(iVar2 + 0x27c));
    }
  }
  DAT_006e3ae0 = 0;
  _DAT_006e3ae4 = *(undefined4 *)PTR_DAT_00696714;
  _DAT_006e3ae8 = *(undefined4 *)(PTR_DAT_00696714 + 4);
  _DAT_006e3aec = *(undefined4 *)(PTR_DAT_00696714 + 8);
  return;
}
#endif
