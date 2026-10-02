// chimera__rasterizer_set_model_skinning  (Ghidra: chimera__rasterizer_set_model_skinning,
// already named -- Chimera name, hint only)
// address 0x518b40, size 195 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: transposes each real_matrix4x3 (scaled by its scale) into a rasterizer_skinning_matrix
//   row triple with the translation in the fourth column, matching the type header's own
//   evidence note for this exact function/global pair.
// register convention: node matrices pointer/count block (&context->node_matrices) in
//   unaff_EDI, upload flag in the recognized stack parameter.
//   // blam-cc: unaff_EDI -> rasterizer_node_matrices, stack -> upload

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t rasterizer_maximum_skinning_nodes; // 0x0069c67e (usually 0x3f)
extern rasterizer_skinning_matrix rasterizer_skinning_palette[63]; // 0x007c04e0
extern void *rasterizer_device; // 0x0071d174

typedef int32_t (__stdcall *d3d_set_vertex_shader_constant_fn)(void *device, uint32_t reg, const void *data, uint32_t count);


// blam-cc: unaff_EDI -> rasterizer_node_matrices, stack -> upload
// Builds the weighted skinning matrix palette for up to rasterizer_maximum_skinning_nodes of a
// model's blended nodes, and optionally uploads it as vertex shader constants (register 0x1d).
void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes)
{
    int16_t count = (nodes->node_count < rasterizer_maximum_skinning_nodes)
                        ? nodes->node_count : rasterizer_maximum_skinning_nodes;
    int16_t i;

    for (i = 0; i < count; i++) {
        real_matrix4x3 *m = &((real_matrix4x3 *)nodes->matrices)[i];
        rasterizer_skinning_matrix *out = &rasterizer_skinning_palette[i];
        float scale = m->scale;

        out->rows[0][0] = scale * m->forward.i;
        out->rows[0][1] = scale * m->left.i;
        out->rows[0][2] = scale * m->up.i;
        out->rows[0][3] = m->position.x;

        out->rows[1][0] = scale * m->forward.j;
        out->rows[1][1] = scale * m->left.j;
        out->rows[1][2] = scale * m->up.j;
        out->rows[1][3] = m->position.y;

        out->rows[2][0] = scale * m->forward.k;
        out->rows[2][1] = scale * m->left.k;
        out->rows[2][2] = scale * m->up.k;
        out->rows[2][3] = m->position.z;
    }

    if (upload != 0) {
        void **vtable = *(void ***)rasterizer_device;
        ((d3d_set_vertex_shader_constant_fn)vtable[0x178 / 4])(rasterizer_device, 0x1d,
            rasterizer_skinning_palette, (uint32_t)(count * 3));
    }
}

#if 0
Original Ghidra decompilation (0x518b40):

void chimera__rasterizer_set_model_skinning(char param_1)

{
  float fVar1;
  int iVar2;
  float *pfVar3;
  int iVar4;
  ushort uVar5;
  uint uVar6;
  int *unaff_EDI;

  uVar5 = DAT_0069c67e;
  if ((short)*(ushort *)(unaff_EDI + 1) < (short)DAT_0069c67e) {
    uVar5 = *(ushort *)(unaff_EDI + 1);
  }
  if (0 < (short)uVar5) {
    iVar4 = 0;
    uVar6 = (uint)uVar5;
    pfVar3 = (float *)&DAT_007c04e8;
    do {
      fVar1 = *(float *)(*unaff_EDI + iVar4);
      iVar2 = *unaff_EDI + iVar4;
      iVar4 = iVar4 + 0x34;
      uVar6 = uVar6 - 1;
      pfVar3[-2] = fVar1 * *(float *)(iVar2 + 4);
      pfVar3[-1] = fVar1 * *(float *)(iVar2 + 0x10);
      *pfVar3 = fVar1 * *(float *)(iVar2 + 0x1c);
      pfVar3[1] = *(float *)(iVar2 + 0x28);
      pfVar3[2] = fVar1 * *(float *)(iVar2 + 8);
      pfVar3[3] = fVar1 * *(float *)(iVar2 + 0x14);
      pfVar3[4] = fVar1 * *(float *)(iVar2 + 0x20);
      pfVar3[5] = *(float *)(iVar2 + 0x2c);
      pfVar3[6] = fVar1 * *(float *)(iVar2 + 0xc);
      pfVar3[7] = fVar1 * *(float *)(iVar2 + 0x18);
      pfVar3[8] = fVar1 * *(float *)(iVar2 + 0x24);
      pfVar3[9] = *(float *)(iVar2 + 0x30);
      pfVar3 = pfVar3 + 0xc;
    } while (uVar6 != 0);
  }
  if (param_1 != '\0') {
    (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0x1d,&DAT_007c04e0,(short)uVar5 * 3);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
