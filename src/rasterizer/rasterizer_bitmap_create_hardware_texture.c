// rasterizer_bitmap_create_hardware_texture  (Ghidra: FUN_00523fa0)
// address 0x523fa0, size 341 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase2/results/rasterizer_01.json's own summary for this address ("Locks the
// appropriate Direct3D surface/volume/cube-face...") does not match what the vtable offsets
// actually resolve to once traced against the standard D3D9 vtable layout -- the three branches
// call the DEVICE's CreateTexture/CreateVolumeTexture/CreateCubeTexture (not a per-resource
// Lock), and the created object is stored into bitmap->pointer, so this rewrite follows the
// vtable evidence over the phase2 summary. This one's three branches match
// IDirect3DDevice9::CreateTexture (vtable+0x5c, index 23, 8 args), ::CreateVolumeTexture
// (+0x60, index 24, 9 args) and ::CreateCubeTexture (+0x64/100, index 25, 7 args) exactly by
// argument count, gated on BitmapDataType_t (`unaff_ESI + 0xa`, 0 2D, 1 3D, 2 cube) and
// `DAT_007c10fc` (d3d_caps9.texture_caps: bit 0x2000 volume-texture support, bit 0x800
// cube-texture support, bit 0x10000 mipmappable cube maps, per the header's own "bitmap lock and
// upload paths" note). `DAT_0065e040` is a BitmapDataFormat_t -> D3DFORMAT lookup table. The
// created object is written to `bitmap->pointer` (+0x28), the same tag-data repurposing already
// documented for BitmapData::pointer elsewhere in this codebase.
// register convention: bitmap in ESI (live-in; no stack parameters).
// UNSURE: the exact CreateTexture Usage argument (hardcoded 0 for 2D/volume) and the format
// lookup table's element type are not independently verified here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0

extern void *rasterizer_device; // 0x0071d174
extern int32_t rasterizer_bitmap_format_to_d3dformat[]; // 0x0065e040, UNSURE element count

extern int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width, int16_t *out_height);

typedef int32_t (*d3d_create_texture_fn)(void *self, uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, int32_t format, uint32_t pool, void *out_texture, void *shared_handle);
typedef int32_t (*d3d_create_volume_texture_fn)(void *self, uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, int32_t format, uint32_t pool, void *out_texture, void *shared_handle);
typedef int32_t (*d3d_create_cube_texture_fn)(void *self, uint32_t edge_length, uint32_t levels, uint32_t usage, int32_t format, uint32_t pool, void *out_texture, void *shared_handle);

// Creates the hardware texture/volume texture/cube texture object for `bitmap` (matching its
// type field) and stores it in bitmap->pointer. Returns 1 on success (including "nothing to do"
// cases: no device, no format mapping, or an unsupported capability), 0 on a real failure.
// blam-cc: ESI = bitmap
uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap)
{
    uint8_t ok;
    int32_t format;
    int32_t hresult;
    int16_t width, height;
    int32_t mip_skip;
    int32_t levels;

    ok = 1;
    if (rasterizer_device == 0) {
        bitmap->pointer = 0;
        return 1;
    }

    format = rasterizer_bitmap_format_to_d3dformat[bitmap->format];
    if (format == -1) {
        bitmap->pointer = 0;
        return 1;
    }

    if (bitmap->type == 0) {
        mip_skip = rasterizer_bitmap_compute_mipmap_skip_count(bitmap, &width, &height);
        d3d_create_texture_fn create_texture = (d3d_create_texture_fn)(*(void ***)rasterizer_device)[0x17]; // +0x5c
        hresult = create_texture(rasterizer_device, width, height,
            (uint32_t)((bitmap->mipmap_count - mip_skip) + 1), 0, format, 1, &bitmap->pointer, 0);
        if (hresult < 0) {
            ok = 0;
        }
    } else if (bitmap->type == 1) {
        if ((rasterizer_caps.texture_caps & 0x2000) == 0) {
            bitmap->pointer = 0;
        } else {
            levels = ((int8_t)(rasterizer_caps.texture_caps >> 8) < 0) ? bitmap->mipmap_count + 1 : 1;
            d3d_create_volume_texture_fn create_volume_texture =
                (d3d_create_volume_texture_fn)(*(void ***)rasterizer_device)[0x18]; // +0x60
            hresult = create_volume_texture(rasterizer_device, bitmap->width, bitmap->height,
                bitmap->depth, (uint32_t)levels, 0, format, 1, &bitmap->pointer, 0);
            if (hresult < 0) {
                ok = 0;
            }
        }
    } else if (bitmap->type == 2) {
        if ((rasterizer_caps.texture_caps & 0x800) == 0) {
            bitmap->pointer = 0;
        } else {
            levels = (rasterizer_caps.texture_caps & 0x10000) == 0 ? 1 : bitmap->mipmap_count + 1;
            d3d_create_cube_texture_fn create_cube_texture =
                (d3d_create_cube_texture_fn)(*(void ***)rasterizer_device)[0x19]; // +0x64
            hresult = create_cube_texture(rasterizer_device, bitmap->width, (uint32_t)levels, 0,
                format, 1, &bitmap->pointer, 0);
            if (hresult < 0) {
                ok = 0;
            }
        }
    }

    if (bitmap->pointer == 0) {
        bitmap->pointer = 0;
        return 0;
    }
    if (ok == 0) {
        bitmap->pointer = 0;
    }
    return ok;
}

#if 0
Original Ghidra decompilation (0x523fa0):

char FUN_00523fa0(void)

{
  short sVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  int unaff_ESI;
  char local_9;
  short local_8 [2];
  short local_4;

  piVar2 = DAT_0071d174;
  local_9 = '\x01';
  if (DAT_0071d174 == (int *)0x0) {
    *(undefined4 *)(unaff_ESI + 0x28) = 0;
    return '\x01';
  }
  iVar4 = *(int *)(&DAT_0065e040 + *(short *)(unaff_ESI + 0xc) * 4);
  if (iVar4 != -1) {
    sVar1 = *(short *)(unaff_ESI + 10);
    if (sVar1 == 0) {
      iVar3 = rasterizer_bitmap_compute_mipmap_skip_count(local_8);
      iVar4 = (**(code **)(*piVar2 + 0x5c))
                        (piVar2,(int)local_4,(int)local_8[0],
                         (*(short *)(unaff_ESI + 0x14) - iVar3) + 1,0,iVar4,1,unaff_ESI + 0x28,0);
      if (iVar4 < 0) {
        local_9 = '\0';
      }
    }
    else if (sVar1 == 1) {
      if ((DAT_007c10fc & 0x2000) == 0) {
        *(undefined4 *)(unaff_ESI + 0x28) = 0;
      }
      else {
        if ((char)(DAT_007c10fc >> 8) < '\0') {
          iVar3 = *(short *)(unaff_ESI + 0x14) + 1;
        }
        else {
          iVar3 = 1;
        }
        iVar4 = (**(code **)(*DAT_0071d174 + 0x60))
                          (DAT_0071d174,(int)*(short *)(unaff_ESI + 4),
                           (int)*(short *)(unaff_ESI + 6),(int)*(short *)(unaff_ESI + 8),iVar3,0,
                           iVar4,1,unaff_ESI + 0x28,0);
        if (iVar4 < 0) {
          local_9 = '\0';
        }
      }
    }
    else if (sVar1 == 2) {
      if ((DAT_007c10fc & 0x800) == 0) {
        *(undefined4 *)(unaff_ESI + 0x28) = 0;
      }
      else {
        if ((DAT_007c10fc & 0x10000) == 0) {
          iVar3 = 1;
        }
        else {
          iVar3 = *(short *)(unaff_ESI + 0x14) + 1;
        }
        iVar4 = (**(code **)(*DAT_0071d174 + 100))
                          (DAT_0071d174,(int)*(short *)(unaff_ESI + 4),iVar3,0,iVar4,1,
                           unaff_ESI + 0x28,0);
        if (iVar4 < 0) {
          local_9 = '\0';
        }
      }
    }
    if (*(int *)(unaff_ESI + 0x28) == 0) {
      *(undefined4 *)(unaff_ESI + 0x28) = 0;
      return '\0';
    }
    if (local_9 == '\0') {
      *(undefined4 *)(unaff_ESI + 0x28) = 0;
    }
    return local_9;
  }
  *(undefined4 *)(unaff_ESI + 0x28) = 0;
  return '\x01';
}
#endif
