// rasterizer_bitmap_upload_2d_mipmaps  (Ghidra: FUN_00524100)
// address 0x524100, size 363 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase2/results/rasterizer_01.json ("Uploads a packed 2D bitmap's mip chain into
// a locked Direct3D texture, level by level."). `param_1 + 0x28` is bitmap->pointer (the hardware
// texture created by rasterizer_bitmap_create_hardware_texture.c); its vtable+0x4c/+0x50 (index
// 19/20) are IDirect3DTexture9::LockRect(Level, pLockedRect, pRect, Flags) and UnlockRect(Level),
// confirmed by their 4-arg and 1-arg call shapes. The per-level copy loop reads either row by row
// (via bitmap_data_calculate_mip_dimension/_mip_row_byte_size) or in one bulk block (via
// bitmap_data_calculate_mip_level_byte_size) depending on bit 1 of `param_1 + 0xe`
// (BitmapDataFlags, presumably "compressed").
// register convention: param_1 as the recognized parameter (bitmap).
// UNSURE: bitmap_data_get_pixel_address, bitmap_data_calculate_mip_dimension,
// bitmap_data_calculate_mip_row_byte_size and bitmap_data_calculate_mip_level_byte_size all live
// outside this module (0x43fxxx) and are called here with every argument elided by Ghidra; their
// signatures are inferred as (bitmap, mip_level) from context and are not independently
// verified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h>

extern void *rasterizer_device; // 0x0071d174, UNSURE relevance here (read but not used directly)

extern void *bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level); // 0x43fb20, UNSURE signature
extern uint32_t bitmap_data_calculate_mip_dimension(BitmapData *bitmap, int32_t mip_level); // 0x43fbb0, UNSURE signature
extern uint32_t bitmap_data_calculate_mip_level_byte_size(BitmapData *bitmap, int32_t mip_level); // 0x43fcb0, UNSURE signature
extern uint32_t bitmap_data_calculate_mip_row_byte_size(BitmapData *bitmap, int32_t mip_level); // 0x43fce0, UNSURE signature
extern int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width, int16_t *out_height); // 0x523f10

typedef int32_t (*d3d_lock_rect_fn)(void *self, uint32_t level, void *out_rect, const void *rect, uint32_t flags);
typedef int32_t (*d3d_unlock_rect_fn)(void *self, uint32_t level);

// D3DLOCKED_RECT

// blam-cc: param_1 = bitmap
void rasterizer_bitmap_upload_2d_mipmaps(BitmapData *bitmap)
{
    uint8_t ok;
    int32_t mip_skip;
    int16_t out_width, out_height;
    int16_t level;
    int32_t source_mip;
    void **vtable;
    d3d_lock_rect_fn lock_rect;
    d3d_unlock_rect_fn unlock_rect;
    d3d_locked_rect locked;
    int32_t hresult;
    uint8_t *dest;
    uint8_t *source;
    uint32_t row, rows, row_size;
    uint32_t level_size;

    ok = 1;
    mip_skip = rasterizer_bitmap_compute_mipmap_skip_count(bitmap, &out_width, &out_height);

    // UNSURE: bitmap+0x2c has no established field name in types/rasterizer.h.
    if (rasterizer_device == 0 || *(uint32_t *)((uint8_t *)bitmap + 0x2c) == 0 ||
        bitmap->pointer == 0) {
        return;
    }

    source_mip = mip_skip;
    for (level = 0; ok; level++, source_mip++) {
        if (bitmap->mipmap_count - mip_skip < level) {
            return;
        }

        vtable = *(void ***)(void *)bitmap->pointer;
        lock_rect = (d3d_lock_rect_fn)vtable[0x13]; // +0x4c
        hresult = lock_rect((void *)bitmap->pointer, (uint32_t)level, &locked, 0, 0);

        if (hresult < 0 || locked.bits == 0) {
            ok = 0;
            break;
        }

        source = (uint8_t *)bitmap_data_get_pixel_address(bitmap, source_mip);

        if ((bitmap->flags & 2) == 0) {
            rows = bitmap_data_calculate_mip_dimension(bitmap, source_mip);
            row_size = bitmap_data_calculate_mip_row_byte_size(bitmap, source_mip);
            dest = (uint8_t *)locked.bits;
            for (row = 0; row < rows; row++) {
                memcpy(dest, source, row_size);
                source = source + row_size;
                dest = dest + locked.pitch;
            }
        } else {
            level_size = bitmap_data_calculate_mip_level_byte_size(bitmap, source_mip);
            memcpy((void *)locked.bits, source, level_size);
        }

        vtable = *(void ***)(void *)bitmap->pointer;
        unlock_rect = (d3d_unlock_rect_fn)vtable[0x14]; // +0x50
        hresult = unlock_rect((void *)bitmap->pointer, (uint32_t)level);
        if (hresult < 0) {
            ok = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x524100):

void FUN_00524100(int param_1)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  uint uVar5;
  uint uVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined4 *local_1c;
  int local_18;
  int local_14;
  uint uStack_10;
  int local_c;
  int local_8;
  undefined4 *puStack_4;

  bVar1 = true;
  iVar2 = rasterizer_bitmap_compute_mipmap_skip_count(&local_1c);
  if (((DAT_0071d174 != 0) && (*(int *)(param_1 + 0x2c) != 0)) && (*(int *)(param_1 + 0x28) != 0)) {
    local_18 = 0;
    local_c = iVar2;
    do {
      if (*(short *)(param_1 + 0x14) - local_c < (int)(short)local_18) {
        return;
      }
      local_14 = iVar2;
      iVar3 = (**(code **)(**(int **)(param_1 + 0x28) + 0x4c))
                        (*(int **)(param_1 + 0x28),(int)(short)local_18,&local_8,0,0);
      puVar7 = puStack_4;
      if (((iVar3 < 0) || (!bVar1)) || (puStack_4 == (undefined4 *)0x0)) {
LAB_00524244:
        bVar1 = false;
      }
      else {
        puVar4 = (undefined4 *)bitmap_data_get_pixel_address();
        local_1c = puVar4;
        if ((*(byte *)(param_1 + 0xe) & 2) == 0) {
          uStack_10 = bitmap_data_calculate_mip_dimension();
          uVar5 = bitmap_data_calculate_mip_row_byte_size();
          if (0 < (short)uStack_10) {
            uStack_10 = uStack_10 & 0xffff;
            do {
              puVar8 = puVar7;
              for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
                *puVar8 = *puVar4;
                puVar4 = puVar4 + 1;
                puVar8 = puVar8 + 1;
              }
              for (uVar6 = uVar5 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
                *(undefined1 *)puVar8 = *(undefined1 *)puVar4;
                puVar4 = (undefined4 *)((int)puVar4 + 1);
                puVar8 = (undefined4 *)((int)puVar8 + 1);
              }
              puVar4 = (undefined4 *)((int)local_1c + uVar5);
              puVar7 = (undefined4 *)((int)puVar7 + local_8);
              uStack_10 = uStack_10 - 1;
              iVar2 = local_14;
              local_1c = puVar4;
            } while (uStack_10 != 0);
          }
        }
        else {
          uVar5 = bitmap_data_calculate_mip_level_byte_size();
          for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
            *puVar7 = *puVar4;
            puVar4 = puVar4 + 1;
            puVar7 = puVar7 + 1;
          }
          for (uVar5 = uVar5 & 3; iVar2 = local_14, uVar5 != 0; uVar5 = uVar5 - 1) {
            *(undefined1 *)puVar7 = *(undefined1 *)puVar4;
            puVar4 = (undefined4 *)((int)puVar4 + 1);
            puVar7 = (undefined4 *)((int)puVar7 + 1);
          }
        }
        iVar3 = (**(code **)(**(int **)(param_1 + 0x28) + 0x50))
                          (*(int **)(param_1 + 0x28),(int)(short)local_18);
        if (iVar3 < 0) goto LAB_00524244;
      }
      local_18 = local_18 + 1;
      iVar2 = iVar2 + 1;
    } while (bVar1);
  }
  return;
}
#endif
