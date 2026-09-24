// path_find_gather_adjacent_edges  (Ghidra: path_find_gather_adjacent_edges, renamed)
// address 0x43b1c0, size 227 bytes
// name confidence: 0.45  rewrite confidence: 0.2
// evidence: phase-4 summary "gathers the navigation-mesh edges adjacent to a given vertex for
// expansion during pathfinding search." The base pointer this reads (`context+0xb4` as a
// structure_bsp-shaped pointer, +0x40 edge-index-by-vertex table, +0x4c edge records at
// stride 0x18, +0x58 vertex positions at stride 0x10) does not correspond to any named
// path_find_context field at that offset (path_find_context+0xb4 falls inside nodes[0]),
// and structure_bsp's own edge/vertex table layout is owned by the (already completed)
// structures module, not types/ai.h. Kept as raw offsets on an opaque context pointer rather
// than asserting a type this module does not otherwise use.
// register convention: EAX -> context, stack -> vertex_id, out_edges (a caller-owned array
//   of up to 64 32-byte edge records).
//   // blam-cc: EAX -> context, stack -> vertex_id, out_edges
//
// UNSURE: essentially every offset here (context+0xb4/+0x1e8, and the structure_bsp-shaped
// +0x40/+0x4c/+0x58 sub-fields) is unconfirmed; this rewrite preserves Ghidra's exact
// arithmetic without asserting field names for them. The 32-byte output record's own layout
// (edge id, a flag byte, a start vertex position, and a direction vector) is inferred purely
// from the write pattern below. Ghidra shows every position/direction field written via
// `piVarN[k] = (int)float_value` -- but path_find_run.c (this rewrite) reads the very same
// fields back with genuine float arithmetic (`pfVar10[-1] * 0.5 + pfVar10[-4]`, etc, no
// int-to-float conversion), which only makes sense if this is Ghidra's familiar
// "raw bit copy through a differently-typed (`undefined4 *`/`int *`) pointer" artifact, not
// a real truncation. Declared and written as plain floats accordingly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// TYPES-GAP: the caller-owned output record this function fills, one per adjacent edge.
// Ghidra shows every field written via `(int)float_value` (a truncating cast, not a
// reinterpret), so the position/direction fields below are genuinely integers here.
// blam-cc: EAX -> context, stack -> vertex_id, out_edges
//
// UNSURE: Ghidra decompiled this as `void`, but its only caller (path_find_run.c, this
// rewrite) reads the result as an edge count (`if (0 < (short)result)`); the natural trip
// count `count` this function computes is left in AX at both exit points and is returned
// explicitly here instead.
int16_t path_find_gather_adjacent_edges(void *context, int32_t vertex_id, path_find_adjacent_edge *out_edges)
{
    uint8_t *base = (uint8_t *)context;
    uint8_t *flag_table = *(uint8_t **)(base + 0x1e8);
    uint8_t *bsp = *(uint8_t **)(base + 0xb4);
    int32_t *vertex_edge_index = *(int32_t **)(bsp + 0x40) + vertex_id * 3;
    int32_t edge_index = vertex_edge_index[1];
    int32_t first_edge_index = edge_index;
    int16_t count = 0;
    int32_t *edge;
    uint8_t is_second_vertex;

    do {
        edge = (int32_t *)(*(uint8_t **)(bsp + 0x4c) + edge_index * 0x18);
        is_second_vertex = (vertex_id == *(int32_t *)((uint8_t *)edge + 0x14));

        edge_index = edge[(is_second_vertex ? 0 : 1) + 4];
        out_edges[count].edge_id = edge_index;
        out_edges[count].flag = flag_table[edge_index];

        {
            float *point_a = (float *)(*(uint8_t **)(bsp + 0x58) + edge[0] * 0x10);
            float *point_b = (float *)(*(uint8_t **)(bsp + 0x58) + edge[1] * 0x10);
            out_edges[count].start_x = point_a[0];
            out_edges[count].start_y = point_a[1];
            out_edges[count].start_z = point_a[2];
            out_edges[count].direction_x = point_b[0] - point_a[0];
            out_edges[count].direction_y = point_b[1] - point_a[1];
            out_edges[count].direction_z = point_b[2] - point_a[2];
        }

        count = count + 1;
        if (count == 0x40) {
            return count;
        }
        edge_index = edge[(is_second_vertex ? 1 : 0) + 2];
    } while (edge_index != first_edge_index);

    return count;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043b1c0 @ 0x43b1c0) ----
void FUN_0043b1c0(int param_1,int param_2)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  int in_EAX;
  int *piVar6;
  float *pfVar7;
  float *pfVar8;
  int iVar9;
  bool bVar10;

  iVar3 = *(int *)(in_EAX + 0x1e8);
  iVar4 = *(int *)(in_EAX + 0xb4);
  iVar1 = *(int *)(iVar4 + 0x40) + param_1 * 0xc;
  iVar9 = *(int *)(iVar1 + 4);
  sVar5 = 0;
  do {
    piVar2 = (int *)(*(int *)(iVar4 + 0x4c) + iVar9 * 0x18);
    bVar10 = param_1 == *(int *)(*(int *)(iVar4 + 0x4c) + 0x14 + iVar9 * 0x18);
    piVar6 = (int *)(sVar5 * 0x20 + param_2);
    sVar5 = sVar5 + 1;
    iVar9 = piVar2[!bVar10 + 4];
    *piVar6 = iVar9;
    *(undefined1 *)(piVar6 + 1) = *(undefined1 *)(iVar9 + iVar3);
    pfVar7 = (float *)(*piVar2 * 0x10 + *(int *)(iVar4 + 0x58));
    pfVar8 = (float *)(piVar2[1] * 0x10 + *(int *)(iVar4 + 0x58));
    piVar6[2] = (int)*pfVar7;
    piVar6[3] = (int)pfVar7[1];
    piVar6[4] = (int)pfVar7[2];
    piVar6[5] = (int)(*pfVar8 - *pfVar7);
    piVar6[6] = (int)(pfVar8[1] - pfVar7[1]);
    piVar6[7] = (int)(pfVar8[2] - pfVar7[2]);
    if (sVar5 == 0x40) {
      return;
    }
    iVar9 = piVar2[bVar10 + 2];
  } while (iVar9 != *(int *)(iVar1 + 4));
  return;
}
#endif
