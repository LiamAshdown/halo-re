// structure_bsp_leaf_query  (Ghidra: FUN_005540c0, still unnamed there)
// address 0x5540c0, size 218 bytes
// name confidence: 0.55 -- the leaf-side counterpart of bsp3d_node_query_recursive (this batch),
//   named to match; phase4's summary ("Collects the newly-visible cluster indices referenced by a
//   BSP leaf") is for a different function (that one is 0x5540c0's neighbour address collision in
//   the phase4 list is coincidental -- this function collects *surface* indices from a leaf, not
//   cluster indices).
// rewrite confidence: 0.5 -- Ghidra recovered only 4 of this function's 9 real parameters (2
//   register-passed, 7 on the stack); objdump disassembly was required to recover the rest. See
//   bsp3d_node_query_recursive.c's header for the shared evidence trail.
// evidence: objdump -M intel disassembly of 0x5540c0..0x554125; types/tags.h ScenarioStructureBSPLeaf
//   (compressed bounds at +0x00, surface_reference_count at +0xa, surface_references at +0xc) and
//   ScenarioStructureBSPSurfaceReference (stride 8, .surface at +0).
// register convention: in_EAX -> raw_child (leaf reference, still tagged with bit 31; masked with
//   0x7fffffff here rather than relying on the multiply-by-16 overflow cancellation
//   bsp3d_node_find_leaf's caller uses -- both give the identical address), in_ECX ->
//   inherited_classification. 7 stack parameters (parent_bounds, visited_bits, output_array,
//   max_count, query_box, plane_count, planes).
//   // blam-cc: EAX -> raw_child, ECX -> inherited_classification, stack -> the rest
// UNSURE: none outstanding for this function's own body.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits]; // 0x007d0394, this module

// blam-cc: ECX -> parent_bounds, EDX -> compressed_bounds, ESI -> out
extern void bsp3d_node_bounds_decompress(real_rectangle3d *parent_bounds,
    uint8_t *compressed_bounds, real_rectangle3d *out); // 0x553380, this module

// blam-cc: ECX -> box_a, EDX -> box_b
extern structure_bsp_overlap aabb_overlap_classify(real_rectangle3d *box_a,
    real_rectangle3d *box_b); // 0x5541b0, this module
// blam-cc: EAX -> box, EBX -> planes, DI -> plane_count
extern structure_bsp_overlap frustum_planes_classify_box(real_rectangle3d *box,
    real_plane3d *planes, int16_t plane_count); // 0x554260, this module

// blam-cc: EAX -> raw_child, ECX -> inherited_classification, stack -> the rest
// The return is `mov ax,bp`, so only the low 16 bits are meaningful; max_count is likewise only
// ever compared 16 bits wide (`cmp bp,WORD PTR [esp+0x3c]`).
int16_t structure_bsp_leaf_query(int32_t raw_child, int16_t inherited_classification,
                                  real_rectangle3d *parent_bounds, uint32_t *visited_bits,
                                  int32_t *output_array, int32_t max_count,
                                  real_rectangle3d *query_box, int16_t plane_count,
                                  real_plane3d *planes)
{
    int32_t leaf_index = raw_child & 0x7fffffff;
    ScenarioStructureBSPLeaf *leaf =
        &((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf_index];
    int32_t written = 0;

    real_rectangle3d leaf_box;
    bsp3d_node_bounds_decompress(parent_bounds, (uint8_t *)leaf, &leaf_box);

    int16_t classification = inherited_classification;
    if (inherited_classification != 2) {
        structure_bsp_overlap aabb_result = aabb_overlap_classify(query_box, &leaf_box);
        structure_bsp_overlap frustum_result =
            frustum_planes_classify_box(&leaf_box, planes, plane_count);
        classification = (int16_t)((aabb_result <= frustum_result) ? aabb_result : frustum_result);
    }
    if (classification == 0) {
        return 0;
    }

    ScenarioStructureBSPSurfaceReference *leaf_surfaces =
        (ScenarioStructureBSPSurfaceReference *)global_structure_bsp->leaf_surfaces.pointer;
    int32_t first = leaf->surface_references;
    int32_t end = first + leaf->surface_reference_count;
    for (int32_t i = first; i < end; i++) {
        int32_t surface = leaf_surfaces[i].surface;
        int32_t word = surface >> 5;
        uint32_t mask = 1u << (surface & 0x1f);
        if ((surface_visible_bits[word] & mask) == 0) {
            continue;
        }
        if ((visited_bits[word] & mask) != 0) {
            continue;
        }
        if (written >= max_count) {
            break;
        }
        visited_bits[word] |= mask;
        output_array[written] = surface;
        written++;
    }
    return written;
}

#if 0
Original Ghidra decompilation (0x5540c0):

short FUN_005540c0(undefined4 param_1,int param_2,int param_3,short param_4)

{
  uint *puVar1;
  int iVar2;
  short sVar3;
  int in_EAX;
  int iVar4;
  int iVar5;
  short in_CX;
  uint uVar6;
  short sVar7;
  int iVar8;

  iVar4 = in_EAX * 0x10 + *(int *)(DAT_00746f9c + 0xe4);
  sVar7 = 0;
  FUN_00553380();
  if (in_CX != 2) {
    sVar3 = aabb_overlap_classify();
    in_CX = frustum_planes_classify_box();
    if (sVar3 <= in_CX) {
      in_CX = sVar3;
    }
  }
  if ((in_CX != 0) && (iVar8 = *(int *)(iVar4 + 0xc), iVar8 < *(short *)(iVar4 + 10) + iVar8)) {
    do {
      iVar2 = *(int *)(*(int *)(DAT_00746f9c + 0xf0) + iVar8 * 8);
      uVar6 = 1 << ((byte)iVar2 & 0x1f);
      iVar5 = (iVar2 >> 5) * 4;
      if ((((&DAT_007d0394)[iVar2 >> 5] & uVar6) != 0) &&
         ((*(uint *)(iVar5 + param_2) & uVar6) == 0)) {
        if (param_4 <= sVar7) {
          return sVar7;
        }
        puVar1 = (uint *)(iVar5 + param_2);
        *puVar1 = *puVar1 | uVar6;
        *(int *)(param_3 + sVar7 * 4) = iVar2;
        sVar7 = sVar7 + 1;
      }
      iVar8 = iVar8 + 1;
    } while (iVar8 < (int)*(short *)(iVar4 + 10) + *(int *)(iVar4 + 0xc));
  }
  return sVar7;
}
#endif
