// structure_debug_draw_surfaces_simple  (Ghidra: FUN_00552b40; named here)
// address 0x552b40, size 180 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x552b40..0x552bf3). The prologue is
//   `mov eax,0x4000; call __chkstk`, so the return address sits at esp+0x4000 and arg_n at
//   esp+0x4000+4n. Ghidra shows only `FUN_00553d80(local_4000,0x1000,param_2)`; the real call
//   passes seven stack arguments plus two register arguments, and every one of them is resolved
//   here against structure_bsp_query_surfaces' own confirmed convention:
//     ECX of the query  = arg3 of this function (the query box)
//     EDX of the query  = arg1 (the query point)
//     radius            = arg2
//     plane_count       = this function's own incoming ECX
//     planes            = arg4
//     cluster_count / cluster_indices = 0 / NULL
//   An earlier rewrite had only three stack parameters here and mapped the incoming ECX onto one
//   of them. Unlike its two siblings (structure_debug_draw_surfaces_in_box 0x552980,
//   structure_debug_draw_surfaces_in_box_alt 0x552a60) there is no cluster-list gate parameter --
//   the query always runs -- and no fallback to the picked-polygon globals.
// register convention: stack -> query_point, radius, query_box, planes; ECX -> plane_count.
//   // blam-cc: ECX -> plane_count
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#include "fn_structures.h"
#include "fn_render.h"

extern int16_t geometry_buffer_warning;  // 0x0069fa48: accessed as WORD in the binary  // 0x0069fa48, this module
extern void **rasterizer_dynamic_index_buffer; // 0x006e09e8, physics.h/objects.h (read, not owned)

// blam-cc: ECX -> query_box (NULL means "build one from query_point and radius"), EDX -> query_point

extern int32_t rasterizer_dynamic_index_cache_reserve(int16_t vertex_count); // 0x51bd60, foreign render module; UNSURE
extern void *rasterizer_dynamic_index_slot_lock(int32_t geometry_handle); // 0x511e80, foreign render module; UNSURE
extern void structure_leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces,
    int32_t *face_indices); // 0x552cf0, this batch

extern void structure_leaf_faces_for_each(int32_t render_context,
    structure_lightmap_begin_callback lightmap_begin, structure_material_callback material_cb,
    structure_lightmap_end_callback lightmap_end,
    structure_transparent_material_callback transparent_material_cb,
    int32_t *surface_indices, int16_t surface_index_count); // 0x552de0, this batch

// A simplified variant of the debug BSP surface query/draw pipeline: always runs the
// structure_bsp_query_surfaces query (no "is active" gate and no picked-polygon fallback), locks a
// geometry buffer sized for the result, gathers the matching surfaces' vertex/index data into it,
// submits it, and enumerates the result through the debug draw callback at 0x511f50.
// Parameters recovered from disassembly (prologue `mov eax,0x4000; call __chkstk`, so arg_n is at
// esp+0x4000+4n): arg1 becomes the query's EDX (query_point), arg2 its radius, arg3 its ECX
// (query_box) and arg4 its planes, while the incoming ECX is the plane count. An earlier rewrite
// had only three stack parameters and mapped ECX onto one of them.
// blam-cc: ECX -> plane_count


void structure_debug_draw_surfaces_simple(real_point3d *query_point, float radius,
    real_rectangle3d *query_box, real_plane3d *planes, int16_t plane_count)
{
    int32_t local_surface_indices[0x1000];
    int16_t surface_count = structure_bsp_query_surfaces(query_box, query_point,
        local_surface_indices, 0x1000, radius, plane_count, planes, 0, 0);

    if (surface_count > 0) {
        int32_t geometry_handle = rasterizer_dynamic_index_cache_reserve(surface_count);
        if (geometry_handle == -1) {
            if (geometry_buffer_warning != 0) {
                geometry_buffer_warning = 0;
            }
        } else {
            void *vertex_buffer = rasterizer_dynamic_index_slot_lock(geometry_handle);
            structure_leaf_faces_gather_list(surface_count,
                (ScenarioStructureBSPSurface *)vertex_buffer, local_surface_indices);
            (*(void (__stdcall **)(void *))((uint8_t *)*rasterizer_dynamic_index_buffer + 0x30))(rasterizer_dynamic_index_buffer);

            if (geometry_handle != -1) {
                structure_leaf_faces_for_each(geometry_handle, (structure_lightmap_begin_callback)0,
                    (structure_material_callback)render_window_structure_material_0x511f50, (structure_lightmap_end_callback)0,
                    (structure_transparent_material_callback)0, local_surface_indices, surface_count);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x552b40):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00552b40(undefined4 param_1,undefined4 param_2)

{
  short sVar1;
  int iVar2;
  undefined1 local_4000 [16380];
  undefined4 uStack_4;

  uStack_4 = 0x552b4a;
  sVar1 = FUN_00553d80(local_4000,0x1000,param_2);
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
    if (iVar2 != -1) {
      FUN_00552de0(iVar2,0,&LAB_00511f50,0,0);
    }
  }
  return;
}

Disassembly (objdump -d -M intel) shows the true 7-argument FUN_00553d80 call and the ECX-forwarded
third value that Ghidra's decompilation dropped:

00552b40: mov eax,[esp+0x4010] ; arg4 = planes  (retaddr is at esp+0x4000, so arg_n is at +0x4000+4n)
          mov edx,[esp+0x4008] ; arg2 = radius
          push 0 ; push 0 ; push eax ; push ecx (the incoming ECX = plane_count)
          mov ecx,[esp+0x4020] ; (reloaded after the push, only the pushed value matters)
          push edx
          mov edx,[esp+0x401c] ; (reloaded after the push, only the pushed value matters)
          lea eax,[esp+0x18] ; &local_4000
          push 0x1000 ; push eax
          call 0x553d80
#endif
