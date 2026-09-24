// sphere_mesh_build_face  (Ghidra: sphere_mesh_build_face, already named)
// address 0x4ca5f0, size 450 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: types/math.h sphere_mesh_face_cache section; out/phase4/math_functions.md
//   ("Subdivides one face of the base octahedron into a barycentric grid of points and index
//   triples, caching shared points via a scratch table"). Allocates a (subdivisions+1)^2 scratch
//   table (sphere_mesh_face_cache) filled with -1, then for each row 1..subdivisions walks
//   columns 1..row, resolving the diamond of four grid points (row-1,col-1) (row,col-1)
//   (row,col) (row-1,col) via sphere_mesh_get_face_point @0x4ca7c0 and writing a triangle-strip
//   run of length 2*row+1 into mesh->indices at *strip_cursor.
// register convention: mesh in ESI (unaff_ESI, confirmed: *unaff_ESI read as subdivisions,
//   unaff_ESI[8] incremented as strip_count -- matches the field offsets in sphere_mesh),
//   next_point_index in EAX (confirmed, moved to EBX at 0x4ca601 and carried in EBX from
//   there down the call chain), then the 5 stack params exactly as sphere_mesh_generate's call
//   site shows them (vertex_a, vertex_b, apex, strip_cursor, edge_cache).
//   // blam-cc: EAX -> next_point_index, ESI -> mesh, stack -> (vertex_a, vertex_b, apex, strip_cursor, edge_cache)
//
// VERIFIED against the disassembly at 0x4ca5f0 (objdump -d -M intel): the first three
// instructions after the prologue are
//   0x4ca5f8  mov di,WORD PTR [esi]     mesh->subdivisions, so ESI is the mesh   (confirmed)
//   0x4ca5fb  mov ebp,[esp+0x34]        the 5th stack argument, edge_cache
//   0x4ca601  mov ebx,eax               the next_point_index pointer arrives in EAX and is
//                                        moved into EBX for the rest of the call chain
// so next_point_index is an EAX argument here that becomes the EBX that
// sphere_mesh_get_face_point reads as unaff_EBX -- the earlier inference was right about the
// value and one register off about where it enters. sphere_mesh_generate's header records the
// matching call site.
// UNSURE: this function's own apex/third vertex is never read in its body -- pure passthrough
//   into sphere_mesh_get_face_point.
//   The fill-with-0xffff loop below is written as a plain per-element loop rather than the
//   decompile's dword-then-leftover-word split; both produce the identical final table.

#include "tags.h"
#include "math.h"

extern void *GlobalAlloc(uint32_t flags, uint32_t bytes); // 0x0063a0b0 import thunk
extern void *GlobalFree(void *block); // 0x0063a0bc import thunk; returns NULL on success, like the Win32 original
extern int16_t sphere_mesh_get_face_point(sphere_mesh *mesh, int16_t *next_point_index, int16_t apex,
                                           int16_t vertex_a, int16_t vertex_b, int16_t row, int16_t col,
                                           sphere_mesh_edge_cache *edge_cache,
                                           sphere_mesh_face_cache *face_cache); // 0x4ca7c0

// Subdivides one face of the base octahedron into a barycentric grid of points and index
// triples, caching shared points via a scratch table.
void sphere_mesh_build_face(int16_t *next_point_index, sphere_mesh *mesh, int16_t vertex_a,
                             int16_t vertex_b, int16_t apex, int16_t *strip_cursor,
                             sphere_mesh_edge_cache *edge_cache)
{
    sphere_mesh_face_cache *face_cache;
    int32_t entry_count;
    int32_t i;
    int16_t row;
    int16_t col;
    int16_t strip_length;
    int16_t tl, bl, br, tr;

    entry_count = (int16_t)((mesh->subdivisions + 1) * (mesh->subdivisions + 1));
    face_cache = (sphere_mesh_face_cache *)GlobalAlloc(0, (uint32_t)(entry_count * 2));
    if (face_cache != 0) {
        if (0 < entry_count) {
            for (i = 0; i < entry_count; i++) {
                face_cache->point_index[i] = -1;
            }
        }
        row = 1;
        if (0 < mesh->subdivisions) {
            strip_length = 3;
            do {
                mesh->indices[*strip_cursor] = strip_length;
                *strip_cursor = *strip_cursor + 1;
                mesh->strip_count = mesh->strip_count + 1;
                col = 1;
                if (2 < strip_length) {
                    do {
                        tl = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                         (int16_t)(row - 1), (int16_t)(col - 1), edge_cache, face_cache);
                        bl = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                         row, (int16_t)(col - 1), edge_cache, face_cache);
                        br = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                         row, col, edge_cache, face_cache);
                        if (col == 1) {
                            mesh->indices[*strip_cursor] = bl;
                            *strip_cursor = *strip_cursor + 1;
                            mesh->indices[*strip_cursor] = tl;
                            *strip_cursor = *strip_cursor + 1;
                        }
                        mesh->indices[*strip_cursor] = br;
                        *strip_cursor = *strip_cursor + 1;
                        if (col < row) {
                            tr = sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b,
                                                             (int16_t)(row - 1), col, edge_cache, face_cache);
                            mesh->indices[*strip_cursor] = tr;
                            *strip_cursor = *strip_cursor + 1;
                        }
                        col = col + 1;
                    } while (col <= row);
                }
                row = row + 1;
                strip_length = strip_length + 2;
            } while (row <= mesh->subdivisions);
        }
        GlobalFree(face_cache);
    }
}

#if 0
Original Ghidra decompilation (0x4ca5f0):

void sphere_mesh_build_face
               (undefined4 param_1,undefined4 param_2,undefined4 param_3,short *param_4,
               undefined4 param_5)

{
  ushort uVar1;
  undefined2 uVar2;
  undefined2 uVar3;
  undefined2 uVar4;
  short sVar5;
  undefined4 *hMem;
  uint uVar6;
  short *unaff_ESI;
  undefined4 *puVar7;
  int iVar8;
  int local_18;

  uVar1 = (*unaff_ESI + 1) * (*unaff_ESI + 1);
  hMem = GlobalAlloc(0,(int)(short)uVar1 << 1);
  if (hMem != (undefined4 *)0x0) {
    if (0 < (short)uVar1) {
      puVar7 = hMem;
      for (uVar6 = (uint)(uVar1 >> 1); uVar6 != 0; uVar6 = uVar6 - 1) {
        *puVar7 = 0xffffffff;
        puVar7 = puVar7 + 1;
      }
      for (uVar6 = (uint)((uVar1 & 1) != 0); uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined2 *)puVar7 = 0xffff;
        puVar7 = (undefined4 *)((int)puVar7 + 2);
      }
    }
    local_18 = 1;
    if (0 < *unaff_ESI) {
      sVar5 = 3;
      do {
        *(short *)(*(int *)(unaff_ESI + 4) + *param_4 * 2) = sVar5;
        *param_4 = *param_4 + 1;
        unaff_ESI[8] = unaff_ESI[8] + 1;
        iVar8 = 1;
        if (2 < sVar5) {
          do {
            uVar2 = sphere_mesh_get_face_point
                              (param_1,param_2,local_18 + -1,iVar8 + -1,param_5,hMem);
            uVar3 = sphere_mesh_get_face_point(param_1,param_2,local_18,iVar8 + -1,param_5,hMem);
            uVar4 = sphere_mesh_get_face_point(param_1,param_2,local_18,iVar8,param_5,hMem);
            if ((short)iVar8 == 1) {
              *(undefined2 *)(*(int *)(unaff_ESI + 4) + *param_4 * 2) = uVar3;
              *param_4 = *param_4 + 1;
              *(undefined2 *)(*(int *)(unaff_ESI + 4) + *param_4 * 2) = uVar2;
              *param_4 = *param_4 + 1;
            }
            *(undefined2 *)(*(int *)(unaff_ESI + 4) + *param_4 * 2) = uVar4;
            *param_4 = *param_4 + 1;
            if ((short)iVar8 < (short)local_18) {
              uVar2 = sphere_mesh_get_face_point(param_1,param_2,local_18 + -1,iVar8,param_5,hMem);
              *(undefined2 *)(*(int *)(unaff_ESI + 4) + *param_4 * 2) = uVar2;
              *param_4 = *param_4 + 1;
            }
            iVar8 = iVar8 + 1;
          } while ((short)iVar8 <= (short)local_18);
        }
        local_18 = local_18 + 1;
        sVar5 = sVar5 + 2;
      } while ((short)local_18 <= *unaff_ESI);
    }
    GlobalFree(hMem);
  }
  return;
}
#endif
