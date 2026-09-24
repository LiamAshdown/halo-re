// structure_picked_polygon_draw  (Ghidra: FUN_005528f0; named here)
// address 0x5528f0, size 130 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x5528f0..0x552971) resolves the
//   render_flag save/force/restore (a plain 16-bit store is equivalent to the original's
//   read-high-half/CONCAT22/write-dword dance, since only the low 16 bits ever change) and the
//   structure_leaf_faces_for_each call's ECX/EAX arguments (visible_surface_indices /
//   visible_surface_count) and its four stack arguments (picked_surfaces_geometry then three
//   literal callback addresses and 0).
// register convention: none (void).
// UNSURE: the three callback addresses (0x511f20, 0x511f30, 0x44ad80) are not decompiled by this
//   pass; they are the render module's picked-polygon debug draw handlers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)
extern uint8_t picked_surfaces_valid;    // 0x006e3ad8, this module
extern int32_t picked_surfaces_geometry; // 0x006e3adc, this module
extern int16_t visible_surface_count;   // 0x00850394: every store in the binary is
    // a WORD op (`mov WORD PTR ds:0x850394,0` / `inc WORD PTR`); the two debug readers
    // here load it as a dword only because the callee consumes AX alone    // 0x00850394, this module
extern int32_t visible_surface_indices[k_maximum_visible_surfaces]; // 0x00850398, this module
extern int16_t render_force_flag; // 0x0069c67c, foreign render module; forced to 1 while drawing
extern int32_t rasterizer_device_version; // 0x007c118c, foreign render module (read, not owned)
extern void ***rasterizer_device_ptr; // 0x0071d174, foreign render module (read, not owned)

// TYPES-GAP: matches the callback typedefs declared in structure_leaf_faces_for_each.c.

extern void rasterizer_underwater_tint_set_states(void); // 0x51f030, foreign render module; UNSURE, no visible arguments
extern void structure_leaf_faces_for_each(int32_t render_context,
    structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb,
    structure_lightmap_end_callback lightmap_end,
    structure_transparent_material_callback transparent_material_cb,
    int32_t *surface_indices, int16_t surface_index_count); // 0x552de0, this batch

// When the picked-polygon debug geometry is valid, draws it: forces the render flag at 0x69c67c
// on for the duration (unless a bitmap-less BSP already has it on), enumerates the visible
// surfaces' faces through the debug draw callbacks, and pokes the rasterizer device once more if
// its version is old enough.
void structure_picked_polygon_draw(void)
{
    int16_t saved_render_flag;

    if (picked_surfaces_valid == 0) {
        return;
    }

    saved_render_flag = render_force_flag;
    if (structure_bsp->lightmaps_bitmap.tag_id.index == 0xffff && saved_render_flag == 0) {
        render_force_flag = 1;
    }

    rasterizer_underwater_tint_set_states(); // UNSURE: see file header

    structure_leaf_faces_for_each(picked_surfaces_geometry,
        (structure_lightmap_begin_callback)0x511f20, (structure_material_callback)0x511f30,
        (structure_lightmap_end_callback)0x44ad80, (structure_transparent_material_callback)0,
        visible_surface_indices, (int16_t)visible_surface_count);

    if (rasterizer_device_version < 0xffff0101) {
        void **device = *rasterizer_device_ptr;
        (*(void (**)(void *, int32_t, int32_t))((uint8_t *)device + 0xe4))(device, 0x89, 0);
    }

    render_force_flag = saved_render_flag;
}

#if 0
Original Ghidra decompilation (0x5528f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005528f0(void)

{
  short sVar1;

  if (DAT_006e3ad8 != '\0') {
    sVar1 = (short)_DAT_0069c67c;
    if ((*(int *)(DAT_00746f9c + 0xc) == -1) && (sVar1 == 0)) {
      _DAT_0069c67c = CONCAT22(DAT_0069c67e,1);
    }
    FUN_0051f030();
    FUN_00552de0(DAT_006e3adc,&LAB_00511f20,&DAT_00511f30,FUN_0044ad80,0);
    if (DAT_007c118c < 0xffff0101) {
      (**(code **)(*DAT_0071d174 + 0xe4))(DAT_0071d174,0x89,0);
    }
    _DAT_0069c67c = CONCAT22(DAT_0069c67e,sVar1);
  }
  return;
}
#endif
