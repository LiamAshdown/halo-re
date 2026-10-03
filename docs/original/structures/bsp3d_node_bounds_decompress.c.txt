// bsp3d_node_bounds_decompress  (Ghidra: FUN_00553380; named here)
// address 0x553380, size 265 bytes
// name confidence: 0.35   rewrite confidence: 0.75
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x553380..0x553488) resolves every
//   implicit register Ghidra's decompilation left as in_ECX/in_EDX/unaff_ESI: ECX is a
//   real_rectangle3d of world bounds, EDX is a compressed ScenarioStructureBSPNode's 6 packed
//   bytes (types/tags.h bounds_x/y/z[2]), ESI is the decompressed real_rectangle3d output. The
//   sole in-range caller, bsp3d_node_query_recursive (0x553f10, this batch), passes
//   ECX = &structure_bsp->world_bounds_x[0] and EDX = &structure_bsp->collision_bsp... no --
//   EDX = nodes.pointer + node_index * sizeof(ScenarioStructureBSPNode) (structure_bsp->nodes,
//   +0xc0/+0xbc), confirming types/structures.h's "+0x0c8 world_bounds_x/y/z the real_rectangle3d
//   that decompresses node bounds" note. The phase4 one-line summary ("an animated/periodic
//   color or value") is a misread of the same byte-per-axis-endpoint unpacking pattern used
//   elsewhere for colors; here there is no color, only a bounding box. Corrected here.
// register convention: ECX -> world_bounds, EDX -> compressed_bounds, ESI -> out.
// UNSURE: the function's own return value (EAX) is left set only on paths that fall through to
//   the final two instructions in the original; the sole caller never reads it, so this rewrite
//   returns void.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

// Decompresses one BSP3D node's byte-quantized per-axis bounds (each byte 0..255 mapping
// linearly across the corresponding world_bounds range, with 0xff mapping to exactly the range's
// upper bound rather than an approximation of it) into a real_rectangle3d.
// The second parameter is typed as six raw bytes rather than as ScenarioStructureBSPNode because
// both callers pass a different record whose first six bytes have that shape:
// bsp3d_node_query_recursive passes a ScenarioStructureBSPNode, structure_bsp_leaf_query passes a
// ScenarioStructureBSPLeaf (bounds_x/y/z then cluster).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void bsp3d_node_bounds_decompress(real_rectangle3d *parent_bounds, uint8_t *compressed_bounds,
    real_rectangle3d *out)
    // blam-cc: ECX -> parent_bounds, EDX -> compressed_bounds, ESI -> out
{
    const real_bounds *ranges = &parent_bounds->x;
    real_bounds *outputs = &out->x;
    const uint8_t *bytes = compressed_bounds;
    int32_t axis;

    for (axis = 0; axis < 3; axis = axis + 1) {
        float lower = ranges[axis].lower;
        float upper = ranges[axis].upper;
        uint8_t low_byte = bytes[axis * 2];
        uint8_t high_byte = bytes[axis * 2 + 1];

        outputs[axis].lower = (low_byte == 0xff) ? upper : (float)low_byte * 0.003921569f * (upper - lower) + lower;
        outputs[axis].upper = (high_byte == 0xff) ? upper : (float)high_byte * 0.003921569f * (upper - lower) + lower;
    }
}

#if 0
Original Ghidra decompilation (0x553380):

void FUN_00553380(void)

{
  float fVar1;
  float *in_ECX;
  byte *in_EDX;
  float *unaff_ESI;

  fVar1 = in_ECX[1];
  if (*in_EDX != 0xff) {
    fVar1 = (float)*in_EDX * 0.003921569 * (fVar1 - *in_ECX) + *in_ECX;
  }
  *unaff_ESI = fVar1;
  fVar1 = in_ECX[1];
  if (in_EDX[1] != 0xff) {
    fVar1 = (float)in_EDX[1] * 0.003921569 * (fVar1 - *in_ECX) + *in_ECX;
  }
  unaff_ESI[1] = fVar1;
  fVar1 = in_ECX[3];
  if (in_EDX[2] != 0xff) {
    fVar1 = (float)in_EDX[2] * 0.003921569 * (fVar1 - in_ECX[2]) + in_ECX[2];
  }
  unaff_ESI[2] = fVar1;
  fVar1 = in_ECX[3];
  if (in_EDX[3] != 0xff) {
    fVar1 = (float)in_EDX[3] * 0.003921569 * (fVar1 - in_ECX[2]) + in_ECX[2];
  }
  unaff_ESI[3] = fVar1;
  fVar1 = in_ECX[5];
  if (in_EDX[4] != 0xff) {
    fVar1 = (float)in_EDX[4] * 0.003921569 * (fVar1 - in_ECX[4]) + in_ECX[4];
  }
  unaff_ESI[4] = fVar1;
  if (in_EDX[5] != 0xff) {
    unaff_ESI[5] = (float)in_EDX[5] * 0.003921569 * (in_ECX[5] - in_ECX[4]) + in_ECX[4];
    return;
  }
  unaff_ESI[5] = in_ECX[5];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
