// bsp_compressed_lightmap_vertex_unpack_normal  (Ghidra: FUN_005134c0; was lens_flare_unpack_up,
//   renamed by the orphan pass 4 review)
// address 0x5134c0, size 33 bytes (0x5134c0..0x5134e0)
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: objdump 0x5134c0..0x5134e1: `mov ecx,[eax]` reads the packed dword at +0x00 of the
//   record EAX points at, vector3d_unpack_normal_11_11_10 (0x513400, EAX out, ECX packed) unpacks
//   it into a stack temporary, and the three floats are copied to ESI. Its only callers are the
//   three calls at 0x4f2862/0x4f2874/0x4f2886 in object_lighting_sample_point, which pass
//   EAX = &compressed_vertices[rendered_vertices_count * 4 + vertex_index] with an 8-byte stride,
//   i.e. a ScenarioStructureBSPMaterialCompressedLightmapVertex (types/tags.h: packed normal at
//   0x00, texture coordinates at 0x04/0x06) in the lightmap section of the vertex buffer, and
//   ESI = the output normal. The sibling for rendered vertices is 0x513490.
// register convention: EAX = vertex, ESI = out.
//   // blam-cc: EAX -> vertex, ESI -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern real_vector3d *vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed); // 0x513400, blam-cc: EAX out, ECX packed

// Unpacks the 11:11:10 normal of a compressed BSP lightmap vertex.
void bsp_compressed_lightmap_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedLightmapVertex *vertex,
    real_vector3d *out)
{
    real_vector3d unpacked;

    *out = *vector3d_unpack_normal_11_11_10(&unpacked, vertex->normal);
}

#if 0
Original Ghidra decompilation (0x5134c0):

void FUN_005134c0(void)

{
  undefined4 *puVar1;
  undefined4 *unaff_ESI;

  puVar1 = (undefined4 *)vector3d_unpack_normal_11_11_10();
  *unaff_ESI = *puVar1;
  unaff_ESI[1] = puVar1[1];
  unaff_ESI[2] = puVar1[2];
  return;
}
#endif
