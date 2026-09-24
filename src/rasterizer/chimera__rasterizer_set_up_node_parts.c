// chimera__rasterizer_set_up_node_parts  (Ghidra: chimera__rasterizer_set_up_node_parts, already
// named -- Chimera name, hint only)
// address 0x526cf0, size 607 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: gathers rasterizer_skinning_matrix entries (0x007c04e0, stride 0xc dwords, matching
//   rasterizer_skinning_palette exactly) indexed by a byte array, into a local buffer of up to
//   k_rasterizer_maximum_node_parts (0x16=22) entries (the clamp `if (0x15 < in_EAX) iVar5 = 0x16`
//   matches the type header's own note for this exact function), then uploads the whole buffer as
//   vertex shader constants at register 0x1d (device vtable+0x178, the same
//   SetVertexShaderConstantF slot and register used by chimera__rasterizer_set_model_skinning),
//   and stores the source pointer/count into rasterizer_node_part_indices (0x0071d19c) and
//   rasterizer_node_part_count (0x0071d1a0). The gather loop is manually unrolled 4-wide by the
//   compiler (a run of 4 identical index-byte/gather blocks, then a 1-wide remainder loop for the
//   tail); collapsed here into a single equivalent loop, matching this codebase's convention for
//   compiler-unrolled loops (e.g. point3d_array_project_to_xy_plane.c) since it performs the exact
//   same memory writes in the exact same order.
// register convention: node part count in EAX, node part index byte array in unaff_ESI.
//   // blam-cc: EAX -> node_part_count, ESI -> node_part_indices

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_skinning_matrix rasterizer_skinning_palette[63]; // 0x007c04e0
extern void *rasterizer_device;                                   // 0x0071d174
extern uint8_t *rasterizer_node_part_indices;                     // 0x0071d19c
extern int32_t rasterizer_node_part_count;                        // 0x0071d1a0

typedef int32_t (__stdcall *d3d_set_vertex_shader_constant_fn)(void *device, uint32_t reg, const void *data, uint32_t count);

// blam-cc: EAX -> node_part_count, ESI -> node_part_indices
// Uploads a model's per-node-part skeleton transforms (gathered from the skinning palette by
// index byte) as vertex-shader constants for hardware skinning, clamped to
// k_rasterizer_maximum_node_parts entries.
void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices)
{
    int32_t count;
    rasterizer_skinning_matrix parts[k_rasterizer_maximum_node_parts];
    int32_t i;
    void **vtable;

    count = (node_part_count > 0x15) ? k_rasterizer_maximum_node_parts : node_part_count;

    for (i = 0; i < count; i++) {
        parts[i] = rasterizer_skinning_palette[node_part_indices[i]];
    }

    rasterizer_node_part_indices = node_part_indices;
    rasterizer_node_part_count = node_part_count;

    vtable = *(void ***)rasterizer_device;
    ((d3d_set_vertex_shader_constant_fn)vtable[0x178 / 4])(rasterizer_device, 0x1d, parts,
                                                             (uint32_t)(count * 3));
}

#if 0
Original Ghidra decompilation (0x526cf0):

void chimera__rasterizer_set_up_node_parts(void)

{
  undefined4 uVar1;
  int in_EAX;
  undefined4 *puVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int unaff_ESI;
  undefined1 local_420 [8];
  undefined4 local_418 [262];

  iVar5 = in_EAX;
  if (0x15 < in_EAX) {
    iVar5 = 0x16;
  }
  iVar4 = 0;
  if (3 < iVar5) {
    puVar2 = local_418;
    do {
      uVar3 = (uint)*(byte *)(unaff_ESI + iVar4);
      puVar2[-2] = (&DAT_007c04e0)[uVar3 * 0xc];
      puVar2[-1] = (&DAT_007c04e4)[uVar3 * 0xc];
      *puVar2 = (&DAT_007c04e8)[uVar3 * 0xc];
      puVar2[1] = (&DAT_007c04ec)[uVar3 * 0xc];
      puVar2[2] = (&DAT_007c04f0)[uVar3 * 0xc];
      puVar2[3] = (&DAT_007c04f4)[uVar3 * 0xc];
      puVar2[4] = (&DAT_007c04f8)[uVar3 * 0xc];
      puVar2[5] = (&DAT_007c04fc)[uVar3 * 0xc];
      puVar2[6] = (&DAT_007c0500)[uVar3 * 0xc];
      puVar2[7] = (&DAT_007c0504)[uVar3 * 0xc];
      uVar1 = (&DAT_007c050c)[uVar3 * 0xc];
      puVar2[8] = (&DAT_007c0508)[uVar3 * 0xc];
      puVar2[9] = uVar1;
      uVar3 = (uint)*(byte *)(unaff_ESI + 1 + iVar4);
      puVar2[10] = (&DAT_007c04e0)[uVar3 * 0xc];
      puVar2[0xb] = (&DAT_007c04e4)[uVar3 * 0xc];
      puVar2[0xc] = (&DAT_007c04e8)[uVar3 * 0xc];
      puVar2[0xd] = (&DAT_007c04ec)[uVar3 * 0xc];
      puVar2[0xe] = (&DAT_007c04f0)[uVar3 * 0xc];
      puVar2[0xf] = (&DAT_007c04f4)[uVar3 * 0xc];
      puVar2[0x10] = (&DAT_007c04f8)[uVar3 * 0xc];
      puVar2[0x11] = (&DAT_007c04fc)[uVar3 * 0xc];
      puVar2[0x12] = (&DAT_007c0500)[uVar3 * 0xc];
      puVar2[0x13] = (&DAT_007c0504)[uVar3 * 0xc];
      uVar1 = (&DAT_007c0508)[uVar3 * 0xc];
      puVar2[0x15] = (&DAT_007c050c)[uVar3 * 0xc];
      uVar3 = (uint)*(byte *)(unaff_ESI + 2 + iVar4);
      puVar2[0x14] = uVar1;
      puVar2[0x16] = (&DAT_007c04e0)[uVar3 * 0xc];
      puVar2[0x17] = (&DAT_007c04e4)[uVar3 * 0xc];
      puVar2[0x18] = (&DAT_007c04e8)[uVar3 * 0xc];
      puVar2[0x19] = (&DAT_007c04ec)[uVar3 * 0xc];
      puVar2[0x1a] = (&DAT_007c04f0)[uVar3 * 0xc];
      puVar2[0x1b] = (&DAT_007c04f4)[uVar3 * 0xc];
      puVar2[0x1c] = (&DAT_007c04f8)[uVar3 * 0xc];
      puVar2[0x1d] = (&DAT_007c04fc)[uVar3 * 0xc];
      puVar2[0x1e] = (&DAT_007c0500)[uVar3 * 0xc];
      puVar2[0x1f] = (&DAT_007c0504)[uVar3 * 0xc];
      uVar1 = (&DAT_007c0508)[uVar3 * 0xc];
      puVar2[0x21] = (&DAT_007c050c)[uVar3 * 0xc];
      uVar3 = (uint)*(byte *)(unaff_ESI + 3 + iVar4);
      puVar2[0x20] = uVar1;
      puVar2[0x22] = (&DAT_007c04e0)[uVar3 * 0xc];
      puVar2[0x23] = (&DAT_007c04e4)[uVar3 * 0xc];
      puVar2[0x24] = (&DAT_007c04e8)[uVar3 * 0xc];
      puVar2[0x25] = (&DAT_007c04ec)[uVar3 * 0xc];
      puVar2[0x26] = (&DAT_007c04f0)[uVar3 * 0xc];
      puVar2[0x27] = (&DAT_007c04f4)[uVar3 * 0xc];
      puVar2[0x28] = (&DAT_007c04f8)[uVar3 * 0xc];
      puVar2[0x29] = (&DAT_007c04fc)[uVar3 * 0xc];
      puVar2[0x2a] = (&DAT_007c0500)[uVar3 * 0xc];
      puVar2[0x2b] = (&DAT_007c0504)[uVar3 * 0xc];
      uVar1 = (&DAT_007c050c)[uVar3 * 0xc];
      puVar2[0x2c] = (&DAT_007c0508)[uVar3 * 0xc];
      puVar2[0x2d] = uVar1;
      iVar4 = iVar4 + 4;
      puVar2 = puVar2 + 0x30;
    } while (iVar4 < iVar5 + -3);
  }
  if (iVar4 < iVar5) {
    puVar2 = local_418 + iVar4 * 0xc;
    do {
      uVar3 = (uint)*(byte *)(iVar4 + unaff_ESI);
      puVar2[-2] = (&DAT_007c04e0)[uVar3 * 0xc];
      puVar2[-1] = (&DAT_007c04e4)[uVar3 * 0xc];
      *puVar2 = (&DAT_007c04e8)[uVar3 * 0xc];
      puVar2[1] = (&DAT_007c04ec)[uVar3 * 0xc];
      puVar2[2] = (&DAT_007c04f0)[uVar3 * 0xc];
      puVar2[3] = (&DAT_007c04f4)[uVar3 * 0xc];
      puVar2[4] = (&DAT_007c04f8)[uVar3 * 0xc];
      puVar2[5] = (&DAT_007c04fc)[uVar3 * 0xc];
      puVar2[6] = (&DAT_007c0500)[uVar3 * 0xc];
      puVar2[7] = (&DAT_007c0504)[uVar3 * 0xc];
      uVar1 = (&DAT_007c050c)[uVar3 * 0xc];
      puVar2[8] = (&DAT_007c0508)[uVar3 * 0xc];
      puVar2[9] = uVar1;
      iVar4 = iVar4 + 1;
      puVar2 = puVar2 + 0xc;
    } while (iVar4 < iVar5);
  }
  DAT_0071d19c = unaff_ESI;
  DAT_0071d1a0 = in_EAX;
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0x1d,local_420,iVar5 * 3);
  return;
}
#endif
