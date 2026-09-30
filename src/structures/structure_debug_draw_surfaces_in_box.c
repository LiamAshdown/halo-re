// structure_debug_draw_surfaces_in_box  (Ghidra: FUN_00552980; named here)
// address 0x552980, size 221 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/structures.h documents the callees this drives (structure_leaf_faces_for_each
//   at 0x552de0, this module); structure_bsp_query_surfaces (0x553d80, this module -- it was
//   described as "foreign" by an earlier rewrite that only covered the first half of the range) is
//   the shared BSP surface-in-volume query every one of this function's two siblings
//   (structure_debug_draw_surfaces_in_box_alt 0x552a60, structure_debug_draw_surfaces_simple
//   0x552b40) also calls the same way.
// register convention: stack -> all five parameters. Ghidra's decompilation shows `point`
//   (its param_1) as unread, but disassembly (objdump -d -M intel bin/halo.exe,
//   0x552980..0x552a5c) shows it IS read, as a plain stack value reloaded into EAX right before
//   the rasterizer_projected_light_constants_build call -- Ghidra simply failed to render that as an argument. Disassembly also
//   confirms the `is_box == 0` fallback path: it loads geometry_handle, surface_count and
//   surface_indices from picked_surfaces_geometry / visible_surface_count / visible_surface_indices
//   (this module's globals), which Ghidra's decompilation drops entirely from that branch.
// UNSURE: rasterizer_dynamic_index_cache_reserve/rasterizer_dynamic_index_slot_lock/rasterizer_projected_light_constants_build (foreign) are not examined beyond the one
//   argument/return each visibly carries. structure_bsp_query_surfaces' parameters are resolved:
//   see the note above the function body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#include "fn_structures.h"
#include "fn_render.h"

extern int32_t picked_surfaces_geometry; // 0x006e3adc, this module
extern int16_t geometry_buffer_warning;  // 0x0069fa48: accessed as WORD in the binary  // 0x0069fa48, this module
extern void **rasterizer_dynamic_index_buffer; // 0x006e09e8, physics.h/objects.h (read, not owned)

extern int16_t visible_surface_count;   // 0x00850394: every store in the binary is
    // a WORD op (`mov WORD PTR ds:0x850394,0` / `inc WORD PTR`); the two debug readers
    // here load it as a dword only because the callee consumes AX alone    // 0x00850394, this module
extern int32_t visible_surface_indices[k_maximum_visible_surfaces]; // 0x00850398, this module

// blam-cc: ECX -> query_box (NULL means "build one from query_point and radius"), EDX -> query_point

extern int32_t rasterizer_dynamic_index_cache_reserve(int16_t vertex_count); // 0x51bd60, foreign render module; UNSURE
extern void *rasterizer_dynamic_index_slot_lock(int32_t geometry_handle); // 0x511e80, foreign render module; UNSURE
extern void structure_leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces,
    int32_t *face_indices); // 0x552cf0, this batch
extern void rasterizer_projected_light_constants_build(void *point); // 0x521750, foreign render module; UNSURE, register arg
    // dropped by Ghidra's decompilation (disassembly shows `mov eax,[esp+0x4010]` -- this
    // function's own `point` parameter -- immediately before the call)

extern void structure_leaf_faces_for_each(int32_t render_context,
    structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb,
    structure_lightmap_end_callback lightmap_end,
    structure_transparent_material_callback transparent_material_cb,
    int32_t *surface_indices, int16_t surface_index_count); // 0x552de0, this batch

// When `cluster_indices` is non-NULL, queries the BSP for every surface within `radius` of
// `query_point` (the query is always issued with no query box), locks a
// geometry buffer sized for the result, gathers the matching surfaces' vertex/index data into it,
// submits it, and enumerates the result through the debug draw callback at 0x511f80. Falls back
// to redrawing the already-picked polygon's geometry when `cluster_indices` is NULL.
// Parameter naming corrected from disassembly (prologue `mov eax,0x40xx; call __chkstk`, so arg_n
// sits at esp+0x40xx+4n): the trailing two arguments this function forwards to
// structure_bsp_query_surfaces are its cluster_count / cluster_indices pair, not a "box"/"is_box"
// pair, and the gate on the fallback path is `cluster_indices != NULL`. The query itself is always
// issued with ECX = 0 (no query box) and EDX = this function's query_point.


void structure_debug_draw_surfaces_in_box(void *render_point, real_point3d *query_point,
    float radius, int16_t cluster_count, int16_t *cluster_indices)
{
    int32_t local_surface_indices[0x1000];
    int32_t geometry_handle;
    int32_t *surface_indices;
    int16_t surface_count;

    geometry_handle = -1; // overwritten in both branches below; no path leaves it unset
    if (cluster_indices != 0) {
        surface_count = structure_bsp_query_surfaces(0, query_point, local_surface_indices,
            0x1000, radius, 0, 0, cluster_count, cluster_indices);
        surface_indices = local_surface_indices;
        geometry_handle = -1;
        if (surface_count > 0) {
            geometry_handle = rasterizer_dynamic_index_cache_reserve(surface_count);
            if (geometry_handle == -1) {
                if (geometry_buffer_warning != 0) {
                    geometry_buffer_warning = 0;
                }
            } else {
                void *vertex_buffer = rasterizer_dynamic_index_slot_lock(geometry_handle);
                structure_leaf_faces_gather_list(surface_count,
                    (ScenarioStructureBSPSurface *)vertex_buffer, local_surface_indices);
                (*(void (__stdcall **)(void *))((uint8_t *)*rasterizer_dynamic_index_buffer + 0x30))(rasterizer_dynamic_index_buffer);
            }
        }
    } else {
        // Fall back to redrawing the already-picked polygon's own surface list.
        geometry_handle = picked_surfaces_geometry;
        surface_count = (int16_t)visible_surface_count;
        surface_indices = visible_surface_indices;
    }

    if (geometry_handle != -1) {
        rasterizer_projected_light_constants_build(render_point); // UNSURE: see file header
        structure_leaf_faces_for_each(geometry_handle, (structure_lightmap_begin_callback)0,
            (structure_material_callback)render_window_structure_material_0x511f80, (structure_lightmap_end_callback)0,
            (structure_transparent_material_callback)0, surface_indices, surface_count);
    }
}

#if 0
Original Ghidra decompilation (0x552980):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00552980(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 int param_5)

{
  short sVar1;
  int iVar2;
  undefined1 local_4000 [16380];
  undefined4 uStack_4;

  uStack_4 = 0x55298a;
  iVar2 = DAT_006e3adc;
  if (param_5 != 0) {
    sVar1 = FUN_00553d80(local_4000,0x1000,param_3,0,0,param_4,param_5);
    iVar2 = -1;
    if (0 < sVar1) {
      iVar2 = FUN_0051bd60();
      if (iVar2 == -1) {
        if (DAT_0069fa48 != 0) {
          DAT_0069fa48 = 0;
        }
      }
      else {
        FUN_00511e80();
        FUN_00552cf0((int)sVar1);
        (**(code **)(*DAT_006e09e8 + 0x30))(DAT_006e09e8);
      }
    }
  }
  if (iVar2 != -1) {
    FUN_00521750();
    FUN_00552de0(iVar2,0,&LAB_00511f80,0,0);
  }
  return;
}
#endif
