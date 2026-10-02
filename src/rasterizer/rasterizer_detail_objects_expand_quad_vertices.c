// rasterizer_detail_objects_expand_quad_vertices  (Ghidra: FUN_0051b150, unnamed)
// address 0x51b150, size 543 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: called only by rasterizer_detail_objects_vertex_buffer_fill 0x51b6f0 with EAX = quad
//   count, ECX = destination vertices, EDX = the draw's first 6 byte instance and (collection,
//   draw) on the stack. For every instance it builds one detail_object vertex (0x14 bytes, see
//   types/rasterizer.h) and writes it six times, bumping the low byte of the last dword by the
//   corner index 0,1,2,0,2,3. Type = (byte3 >> 4) % collection.types.count, sprite =
//   type.first_sprite_index + (byte3 & 0xf) % type.sprite_count.
//   Spot-check fix (phase 4 review): the earlier rewrite (rasterizer_decal_expand_quad_vertices)
//   carried Ghidra's float typing of the last two vertex dwords, so the widened normal and the
//   sprite word were converted to float and the corner bumps went through float/int round
//   trips. Both are raw dword stores in the binary (0x51b2c6..0x51b34f); rewritten from there.
// register convention: EAX quad_count, ECX vertices, EDX instances, stack (collection, draw).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: EAX -> quad_count, ECX -> vertices, EDX -> instances, stack -> (collection, draw)
void rasterizer_detail_objects_expand_quad_vertices(int32_t quad_count, uint32_t *vertices, const uint8_t *instances,
                                                    const DetailObjectCollection *collection,
                                                    const rasterizer_detail_object_draw *draw)
{
    rasterizer_detail_object_vertex *vertex = (rasterizer_detail_object_vertex *)vertices;
    const rasterizer_detail_object_instance *instance = (const rasterizer_detail_object_instance *)instances;

    for (; quad_count > 0; quad_count--, instance++) {
        const float *plane = (const float *)draw->z_reference;
        float fx = (float)instance->x * (1.0f / 255.0f);
        float fy = (float)instance->y * (1.0f / 255.0f);
        float fz = (float)instance->z * (1.0f / 255.0f);
        float height = fz * plane[2] + fy * plane[1] + fx * plane[0] + plane[3];
        uint32_t w = instance->packed_normal;
        uint32_t normal;
        int32_t type_index;
        const DetailObjectCollectionObjectType *type;
        uint32_t sprite;
        uint32_t packed;
        rasterizer_detail_object_vertex corner;
        int32_t i;

        corner.position.x = (float)(draw->cell_x << 3) + fx * 8.0f;
        corner.position.y = (float)(draw->cell_y << 3) + fy * 8.0f;
        corner.position.z = draw->base_z * 8.0f + height * 8.0f;

        // widen the packed normal exactly as 0x51b21e..0x51b25c does
        normal = ((w & 0xfffff800) | 0xffff0000) << 3;
        normal = (normal | (w & 0x7e0)) << 2;
        normal = (normal | (w & 0xffffe01f)) << 3;
        normal |= (((w >> 1) & 0xe) | (w & 0x600)) >> 1;
        corner.normal = normal;

        type_index = (int16_t)((int32_t)(instance->type_and_sprite >> 4) % (int32_t)collection->types.count);
        type = &((const DetailObjectCollectionObjectType *)collection->types.pointer)[type_index];
        sprite = (uint8_t)((int32_t)(instance->type_and_sprite & 0xf) % (int32_t)type->sprite_count +
                           type->first_sprite_index);
        packed = ((((sprite & 0xff) | 0x100) << 8) | ((uint32_t)type_index & 0xff)) << 8;

        for (i = 0; i < 6; i++) {
            static const uint8_t k_corner[6] = { 0, 1, 2, 0, 2, 3 };

            vertex[i] = corner;
            vertex[i].sprite = packed + k_corner[i];
        }
        vertex += 6;
    }
}

#if 0
Original Ghidra decompilation (0x51b150):

void FUN_0051b150(int param_1,int param_2)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int in_EAX;
  uint uVar5;
  float *in_ECX;
  int in_EDX;
  byte *pbVar6;
  float fVar7;
  int iVar8;
  float fVar9;
  int local_1c;

  if (0 < in_EAX) {
    pbVar6 = (byte *)(in_EDX + 2);
    local_1c = in_EAX;
    do {
      pfVar1 = *(float **)(param_2 + 0x14);
      uVar5 = (uint)*(ushort *)(pbVar6 + 2);
      fVar4 = (float)((int)*(short *)(param_2 + 8) << 3) + (float)pbVar6[-2] * 0.003921569 * 8.0;
      fVar2 = (float)((int)*(short *)(param_2 + 10) << 3) + (float)pbVar6[-1] * 0.003921569 * 8.0;
      fVar9 = (float)((((uVar5 & 0xfffff800 | 0xffff0000) << 3 | uVar5 & 0x7e0) << 2 |
                      uVar5 & 0xffffe01f) << 3 |
                     (*(ushort *)(pbVar6 + 2) >> 1 & 0xe | uVar5 & 0x600) >> 1);
      fVar3 = *(float *)(param_2 + 0xc) * 8.0 +
              ((float)pbVar6[-2] * 0.003921569 * *pfVar1 +
               (float)pbVar6[-1] * 0.003921569 * pfVar1[1] +
               (float)*pbVar6 * 0.003921569 * pfVar1[2] + pfVar1[3]) * 8.0;
      uVar5 = (uint)(short)((int)(uint)(pbVar6[1] >> 4) % *(int *)(param_1 + 0x44));
      iVar8 = uVar5 * 0x60 + *(int *)(param_1 + 0x48);
      fVar7 = (float)((((byte)((char)((ulonglong)(pbVar6[1] & 0xf) %
                                     (ulonglong)(longlong)(int)(uint)*(byte *)(iVar8 + 0x23)) +
                              *(char *)(iVar8 + 0x22)) | 0x100) << 8 | uVar5 & 0xff) << 8);
      *in_ECX = fVar4;
      in_ECX[1] = fVar2;
      in_ECX[2] = fVar3;
      in_ECX[3] = fVar9;
      in_ECX[4] = fVar7;
      in_ECX[5] = fVar4;
      in_ECX[6] = fVar2;
      in_ECX[7] = fVar3;
      in_ECX[8] = fVar9;
      in_ECX[9] = fVar7;
      in_ECX[9] = (float)((int)in_ECX[9] + 1);
      in_ECX[10] = fVar4;
      in_ECX[0xb] = fVar2;
      in_ECX[0xc] = fVar3;
      in_ECX[0xd] = fVar9;
      in_ECX[0xe] = fVar7;
      in_ECX[0xe] = (float)((int)in_ECX[0xe] + 2);
      in_ECX[0xf] = fVar4;
      in_ECX[0x10] = fVar2;
      in_ECX[0x11] = fVar3;
      in_ECX[0x12] = fVar9;
      in_ECX[0x13] = fVar7;
      in_ECX[0x14] = fVar4;
      in_ECX[0x15] = fVar2;
      in_ECX[0x16] = fVar3;
      in_ECX[0x17] = fVar9;
      in_ECX[0x18] = fVar7;
      in_ECX[0x18] = (float)((int)in_ECX[0x18] + 2);
      in_ECX[0x19] = fVar4;
      in_ECX[0x1a] = fVar2;
      in_ECX[0x1b] = fVar3;
      in_ECX[0x1c] = fVar9;
      in_ECX[0x1d] = fVar7;
      in_ECX[0x1d] = (float)((int)in_ECX[0x1d] + 3);
      in_ECX = in_ECX + 0x1e;
      pbVar6 = pbVar6 + 6;
      local_1c = local_1c + -1;
    } while (local_1c != 0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
