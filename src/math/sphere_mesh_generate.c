// sphere_mesh_generate  (Ghidra: sphere_mesh_generate, already named)
// address 0x4ca4b0, size 317 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: types/math.h sphere_mesh section (GlobalAlloc(0,0x14) pins the header size; field
//   offsets confirmed there). Builds an octahedron (6 vertices, 8 faces from
//   k_octahedron_vertices/k_octahedron_faces), then subdivides each face via
//   sphere_mesh_build_face @0x4ca5f0 into a barycentric grid, caching shared edge points in a
//   64-entry (8x8) scratch table for the whole mesh.
// register convention: subdivisions arrives in AX (in_AX, the low half of EAX); no other
//   register-passed arguments.
//   // blam-cc: AX (low half of EAX) -> subdivisions
//
// VERIFIED against the disassembly at 0x4ca4b0 (objdump -d -M intel). The "next point index"
// bump counter, which the earlier reading could only infer, is visible there:
//   0x4ca558  mov [esp+0x10],ebp        strip_cursor  = 0   (the stack slot at frame-8)
//   0x4ca589  mov [esp+0x14],edx        next_point_index = 6 (frame-4; edx is the base-vertex
//                                        copy loop's counter, which ends at 6)
//   0x4ca5a3  lea ecx,[esp+0x14] / push ecx      -> &strip_cursor as the 4th stack argument
//   0x4ca5b0  lea eax,[esp+0x24]                 -> EAX = &next_point_index
//   0x4ca5b5  call 0x4ca5f0
// and sphere_mesh_build_face's own first instructions are `mov di,[esi]` (so ESI carries the
// mesh) and `mov ebx,eax` (so the counter pointer moves EAX -> EBX and is EBX from there down,
// which is exactly what sphere_mesh_get_face_point reads as unaff_EBX). The call's five stack
// arguments push in the order edge_cache, &strip_cursor, vertex_c, vertex_b, vertex_a, i.e.
// they arrive as (vertex_a, vertex_b, vertex_c, &strip_cursor, edge_cache), matching the
// prototype below; `mesh` and `next_point_index` are the two register arguments and are only
// written first in the prototype for readability.
// The face table is walked as 8 triples from EDI = 0x65c1da using [edi-2], [edi], [edi+2], so
// it genuinely begins at 0x0065c1d8, as types/math.h records.
//
// NOTE: the fill-with-0xffff loop below is written as a plain per-element loop rather than the
//   decompile's 32-iteration dword loop; both produce the identical final 0x80-byte table.
//   The failure paths free `mesh->points` / `mesh->indices` without clearing the now-dangling
//   pointers in `*mesh` -- kept as-is, matching the original (not "fixed").

#include "win32.h"
#include "tags.h"
#include "math.h"
#include "fn_math.h"


extern real_point3d k_octahedron_vertices[6]; // 0x0065c190
extern int16_t k_octahedron_faces[8][3];      // 0x0065c1d8

// Builds a subdivided-octahedron point/triangle-index mesh (allocated via GlobalAlloc) used to
// seed a table of quasi-uniform directions on the unit sphere.
sphere_mesh *sphere_mesh_generate(int16_t subdivisions)
{
    sphere_mesh *mesh;
    sphere_mesh_edge_cache *edge_cache;
    int16_t point_count;
    int16_t next_point_index;
    int16_t strip_cursor;
    int i;

    mesh = (sphere_mesh *)GlobalAlloc(0, sizeof(sphere_mesh));
    if (mesh == 0) {
        return 0;
    }

    mesh->triangle_count = (int16_t)(subdivisions * subdivisions * 8);
    point_count = (int16_t)((int16_t)(((subdivisions - 2) * (subdivisions - 1) * 8) / 2) - 6 +
                             subdivisions * 12);
    mesh->point_count = point_count;
    mesh->subdivisions = subdivisions;

    mesh->points = (real_point3d *)GlobalAlloc(0, (uint32_t)(point_count * (int)sizeof(real_point3d)));
    mesh->indices = (int16_t *)GlobalAlloc(0, (uint32_t)((int)mesh->triangle_count << 3));
    mesh->strip_count = 0;

    edge_cache = (sphere_mesh_edge_cache *)GlobalAlloc(0, sizeof(sphere_mesh_edge_cache));

    if (mesh->points != 0) {
        if (mesh->indices != 0 && edge_cache != 0) {
            int row, col;
            for (row = 0; row < 8; row++) {
                for (col = 0; col < 8; col++) {
                    edge_cache->point_index[row][col] = -1;
                }
            }

            for (i = 0; i < 6; i++) {
                mesh->points[i] = k_octahedron_vertices[i];
            }

            strip_cursor = 0;
            next_point_index = 6;
            for (i = 0; i < 8; i++) {
                sphere_mesh_build_face(&next_point_index, mesh,
                                        k_octahedron_faces[i][0], k_octahedron_faces[i][1],
                                        k_octahedron_faces[i][2], &strip_cursor, edge_cache);
            }
            GlobalFree(edge_cache);
            return mesh;
        }
        GlobalFree(mesh->points);
    }
    if (mesh->indices != 0) {
        GlobalFree(mesh->indices);
    }
    if (edge_cache != 0) {
        GlobalFree(edge_cache);
    }
    return mesh;
}

#if 0
Original Ghidra decompilation (0x4ca4b0):

short * sphere_mesh_generate(void)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  short in_AX;
  short sVar3;
  short *psVar4;
  HGLOBAL pvVar5;
  undefined4 *puVar6;
  int iVar7;
  undefined4 *puVar8;
  undefined2 *puVar9;
  code *pcVar10;
  undefined4 local_8;
  int local_4;

  psVar4 = GlobalAlloc(0,0x14);
  if (psVar4 == (short *)0x0) {
    return (short *)0x0;
  }
  psVar4[7] = in_AX * in_AX * 8;
  sVar3 = (short)(((in_AX + -2) * (in_AX + -1) * 8) / 2) + -6 + in_AX * 0xc;
  psVar4[6] = sVar3;
  *psVar4 = in_AX;
  pvVar5 = GlobalAlloc(0,sVar3 * 0xc);
  *(HGLOBAL *)(psVar4 + 2) = pvVar5;
  pvVar5 = GlobalAlloc(0,(int)psVar4[7] << 3);
  *(HGLOBAL *)(psVar4 + 4) = pvVar5;
  psVar4[8] = 0;
  puVar6 = GlobalAlloc(0,0x80);
  pcVar10 = GlobalFree_exref;
  pvVar5 = *(HGLOBAL *)(psVar4 + 2);
  if (pvVar5 != (HGLOBAL)0x0) {
    if ((*(int *)(psVar4 + 4) != 0) && (puVar6 != (undefined4 *)0x0)) {
      local_4 = 0;
      puVar8 = puVar6;
      for (iVar7 = 0x20; iVar7 != 0; iVar7 = iVar7 + -1) {
        *puVar8 = 0xffffffff;
        puVar8 = puVar8 + 1;
      }
      local_8 = 0;
      puVar8 = &DAT_0065c190;
      do {
        puVar2 = (undefined4 *)(*(int *)(psVar4 + 2) + (short)local_4 * 0xc);
        *puVar2 = *puVar8;
        puVar2[1] = puVar8[1];
        puVar1 = puVar8 + 2;
        local_4 = local_4 + 1;
        puVar8 = puVar8 + 3;
        puVar2[2] = *puVar1;
      } while ((short)local_4 < 6);
      puVar9 = &DAT_0065c1da;
      iVar7 = 8;
      do {
        sphere_mesh_build_face(puVar9[-1],*puVar9,puVar9[1],&local_8,puVar6);
        puVar9 = puVar9 + 3;
        iVar7 = iVar7 + -1;
        pcVar10 = GlobalFree_exref;
      } while (iVar7 != 0);
      goto LAB_004ca5dc;
    }
    if (pvVar5 != (HGLOBAL)0x0) {
      GlobalFree(pvVar5);
    }
  }
  if (*(HGLOBAL *)(psVar4 + 4) != (HGLOBAL)0x0) {
    GlobalFree(*(HGLOBAL *)(psVar4 + 4));
  }
LAB_004ca5dc:
  if (puVar6 != (undefined4 *)0x0) {
    (*pcVar10)(puVar6);
  }
  return psVar4;
}
#endif
