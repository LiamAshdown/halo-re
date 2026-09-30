// bsp_compressed_rendered_vertex_unpack_normal  (Ghidra: FUN_00513490; was
//   lens_flare_unpack_direction, renamed by the orphan pass 4 review)
// address 0x513490, size 34 bytes (0x513490..0x5134b1)
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: objdump 0x513490..0x5134b2: `mov ecx,[eax+0xc]` reads the packed dword at +0x0c of the
//   record EAX points at, vector3d_unpack_normal_11_11_10 (0x513400, EAX out, ECX packed) unpacks
//   it into a 12-byte stack temporary, and the three floats are copied to ESI. Its only callers
//   are the three calls at 0x4f276f/0x4f2787/0x4f279f in object_lighting_sample_point, which pass
//   EAX = &compressed_vertices[vertex_index] with a 0x20 stride
//   (ScenarioStructureBSPMaterialCompressedRenderedVertex, types/tags.h: position 0x00, packed
//   normal 0x0c) and ESI = the output normal. The old name guessed a lens flare field and the
//   old register reading (EAX scratch, ECX packed value) did not match any caller.
// register convention: EAX = vertex, ESI = out.
//   // blam-cc: EAX -> vertex, ESI -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"


// Unpacks the 11:11:10 normal of a compressed BSP rendered vertex.
void bsp_compressed_rendered_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedRenderedVertex *vertex,
    real_vector3d *out)
{
    real_vector3d unpacked;

    *out = *vector3d_unpack_normal_11_11_10(&unpacked, vertex->normal);
}

#if 0
Original Ghidra decompilation (0x513490):

void FUN_00513490(void)

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
