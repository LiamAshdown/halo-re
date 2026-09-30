// bitmap_group_postprocess  (Ghidra: FUN_0043f030, renamed)
// address 0x43f030, size 534 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: out/phase4/bitmaps_types_notes.md ("0x43f030 is bitmap_group_postprocess (per the
// phase 2 JSON); it also creates the hardware textures through the rasterizer upload routines
// 0x523fa0/0x524100/0x524270/0x5243c0 when its second argument is false.") and its "Register
// conventions" line ("0x43f030 bitmap_group_postprocess: cdecl (tag index, bool
// skip_hardware_textures)."); types/tags.h Bitmap/BitmapData/BitmapGroupSequence layout;
// types/bitmaps.h BitmapData runtime words (pointer = texture cache handle, _pad_28 = hardware
// texture, _pad_2c = pixel base) and bitmap_format_bits_per_pixel. Also cross-referenced against
// src/main/screenshot_render.c's own note that "0x43f880 takes the bitmap in ESI", confirming
// this module's bitmap_data_free (this batch) is called per BitmapData entry elsewhere, the same
// pattern this function walks to postprocess every entry.
// register convention: cdecl, stack -> datum_index tag_id, uint8_t skip_hardware_textures.
//   // blam-cc: stack -> tag_id, skip_hardware_textures
// Verified against objdump (phase 4 review): the switch at 0x43f14d sends type 0 (2D) to
// 0x524100 (stack), type 1 (3D) to 0x524270 (EBX = bitmap) and type 2 (cube map) to 0x5243c0
// (stack). 0x524270 calls bitmap_data_calculate_mip_depth (0x43fbe0, call site 0x524312), so it
// is the volume texture upload; its current rasterizer name (..._cubemap_mipmaps) is a misnomer
// owned by src/rasterizer and is kept here only so the extern matches symbols/functions.txt.
// The hardware texture is created (0x523fa0, ESI = bitmap) only when +0x28 is still 0.
// bitmap_group_debug_dump (0x006f1874) is never written in the retail image; the block it
// guards only runs two empty counting loops (debug output compiled out), kept for shape.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "rasterizer.h"
#include "bitmaps.h"
#include "fn_rasterizer.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count]; // 0x006571f4 (types/bitmaps.h)
extern uint8_t bitmap_group_debug_dump; // 0x006f1874, .bss, always 0 in the retail image

extern uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level); // 0x43fc10, this module (src/bitmaps)
extern uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap); // 0x43fb70, this module (src/bitmaps)
extern uint8_t bitmap_data_verify(BitmapData *bitmap, uint8_t require_runtime); // 0x43fd30, this module (src/bitmaps)


// blam-cc: stack -> tag_id, skip_hardware_textures
uint8_t bitmap_group_postprocess(datum_index tag_id, uint8_t skip_hardware_textures)
{
    Bitmap *bitmap = (Bitmap *)tag_instances[(uint16_t)tag_id].data;
    BitmapData *bitmap_data_array = (BitmapData *)bitmap->bitmap_data.pointer;
    int32_t bitmap_data_count = (int32_t)bitmap->bitmap_data.count;
    BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;
    int32_t sequence_count = (int32_t)bitmap->bitmap_group_sequence.count;
    uint8_t success = 1;
    int32_t i;

    // Recompute each bitmap_data's derived runtime fields: total pixel data size across its mip
    // chain, the texture cache / hardware texture / pixel base pointers reset for a fresh load,
    // the interface-bitmap linear flag, and the pixel base address once the tag's processed
    // pixel data is verified to actually hold enough bytes for it.
    for (i = 0; i < bitmap_data_count; i++) {
        BitmapData *entry = bitmap_data_array + i;
        uint32_t total_pixel_count = 0;
        int32_t bit_total;
        uint8_t verified;
        int32_t pixel_data_offset;

        entry->bitmap_tag_id.index = (uint16_t)tag_id;
        entry->bitmap_tag_id.id = (uint16_t)(tag_id >> 16);

        if ((int16_t)entry->mipmap_count >= 0) {
            int16_t level;
            for (level = 0; level <= (int16_t)entry->mipmap_count; level++) {
                total_pixel_count += bitmap_data_calculate_mip_level_pixel_count(entry, level);
            }
        }

        bit_total = (int32_t)bitmap_format_bits_per_pixel[entry->format] * (int32_t)total_pixel_count;
        entry->pixel_data_size = (uint32_t)((bit_total + ((bit_total >> 31) & 7)) >> 3);

        entry->pointer = (uint32_t)k_datum_index_none;
        entry->pixel_base = 0;
        *(void **)&entry->hardware_texture = 0;

        if (bitmap->type == bitmaptype_interface_bitmaps) {
            entry->flags |= _bitmap_data_linear_bit;
        }

        verified = bitmap_data_verify(entry, 0);
        pixel_data_offset = (int32_t)entry->pixel_data_offset;
        if (!verified || pixel_data_offset < 0 ||
            (int32_t)bitmap->processed_pixel_data.size <
                (int32_t)bitmap_data_calculate_pixel_data_size(entry) + pixel_data_offset) {
            success = 0;
        } else {
            entry->pixel_base = (uint8_t *)bitmap->processed_pixel_data.pointer + pixel_data_offset;
        }
    }

    // Create/upload the hardware textures, unless the caller asked to skip that or an earlier
    // bitmap_data already failed verification.
    if (!skip_hardware_textures && success && bitmap_data_count > 0) {
        for (i = 0; i < bitmap_data_count; i++) {
            BitmapData *entry = bitmap_data_array + i;
            if (bitmap->type != bitmaptype_interface_bitmaps) {
                if (*(void **)&entry->hardware_texture == 0) {
                    rasterizer_bitmap_create_hardware_texture(entry);
                }
                switch (entry->type) {
                case bitmapdatatype_2d_texture:
                    rasterizer_bitmap_upload_2d_mipmaps(entry);
                    break;
                case bitmapdatatype_3d_texture:
                    rasterizer_bitmap_upload_cubemap_mipmaps(entry); // 3D volume upload despite the name, see header
                    break;
                case bitmapdatatype_cube_map:
                    rasterizer_bitmap_upload_cubemap_mipmaps_by_face(entry);
                    break;
                default:
                    break;
                }
            }
        }
    }

    // Sprite groups don't carry usable first_bitmap_index/bitmap_count per sequence; clear them.
    for (i = 0; i < sequence_count; i++) {
        if (bitmap->type == bitmaptype_sprites &&
            (sequences[i].first_bitmap_index != 0 || sequences[i].bitmap_count != 0)) {
            sequences[i].first_bitmap_index = 0;
            sequences[i].bitmap_count = 0;
        }
    }

    // The last sequence must resolve to something: either a bitmap_count, or at least one sprite.
    if (sequence_count > 0) {
        BitmapGroupSequence *last = &sequences[sequence_count - 1];
        if (last->bitmap_count == 0 && last->sprites.count == 0) {
            success = 0;
        }
    }

    if (bitmap_group_debug_dump) {
        // bitmap_group_debug_dump is never set anywhere in the retail image (see
        // types/bitmaps.h), so this whole block is dead code in practice. Its two loops are
        // what remains of debug print calls the compiler stripped out; they only count and
        // never touch memory or the return value, so nothing is invented here.
        int32_t count_a;
        for (count_a = 0; count_a < bitmap_data_count; count_a++) {
            // no-op counter (debug output elided by the compiler)
        }
        for (i = 0; i < sequence_count; i++) {
            if (bitmap->type == bitmaptype_sprites) {
                int32_t sprite_count = (int32_t)sequences[i].sprites.count;
                int32_t j;
                for (j = 0; j < sprite_count; j++) {
                    // no-op counter (debug output elided by the compiler)
                }
            }
        }
    }

    return success;
}

#if 0
Original Ghidra decompilation (0x43f030):

undefined4 FUN_0043f030(uint param_1,char param_2)

{
  short *psVar1;
  char cVar2;
  int iVar3;
  short sVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  char local_5;

  psVar1 = *(short **)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar6 = 0;
  local_5 = '\x01';
  sVar5 = 0;
  if (0 < *(int *)(psVar1 + 0x30)) {
    do {
      iVar7 = iVar6 * 0x30 + *(int *)(psVar1 + 0x32);
      sVar4 = 0;
      iVar6 = 0;
      *(uint *)(iVar7 + 0x20) = param_1;
      if (-1 < *(short *)(iVar7 + 0x14)) {
        do {
          iVar3 = bitmap_data_calculate_mip_level_pixel_count();
          iVar6 = iVar6 + iVar3;
          sVar4 = sVar4 + 1;
        } while (sVar4 <= *(short *)(iVar7 + 0x14));
      }
      *(int *)(iVar7 + 0x1c) =
           (int)((char)(&DAT_006571f4)[*(short *)(iVar7 + 0xc)] * iVar6 +
                ((char)(&DAT_006571f4)[*(short *)(iVar7 + 0xc)] * iVar6 >> 0x1f & 7U)) >> 3;
      *(undefined4 *)(iVar7 + 0x24) = 0xffffffff;
      *(undefined4 *)(iVar7 + 0x2c) = 0;
      *(undefined4 *)(iVar7 + 0x28) = 0;
      if (*psVar1 == 4) {
        *(byte *)(iVar7 + 0xe) = *(byte *)(iVar7 + 0xe) | 0x10;
      }
      cVar2 = FUN_0043fd30(0);
      if (((cVar2 == '\0') || (iVar6 = *(int *)(iVar7 + 0x18), iVar6 < 0)) ||
         (iVar3 = bitmap_data_calculate_pixel_data_size(), *(int *)(psVar1 + 0x18) < iVar3 + iVar6))
      {
        local_5 = '\0';
      }
      else {
        *(int *)(iVar7 + 0x2c) = *(int *)(psVar1 + 0x1e) + iVar6;
      }
      sVar5 = sVar5 + 1;
      iVar6 = (int)sVar5;
    } while (iVar6 < *(int *)(psVar1 + 0x30));
  }
  if (((param_2 == '\0') && (local_5 != '\0')) && (sVar5 = 0, 0 < *(int *)(psVar1 + 0x30))) {
    iVar6 = 0;
    do {
      iVar6 = iVar6 * 0x30 + *(int *)(psVar1 + 0x32);
      if (*psVar1 != 4) {
        if (*(int *)(iVar6 + 0x28) == 0) {
          FUN_00523fa0();
        }
        sVar4 = *(short *)(iVar6 + 10);
        if (sVar4 == 0) {
          FUN_00524100(iVar6);
        }
        else if (sVar4 == 1) {
          FUN_00524270();
        }
        else if (sVar4 == 2) {
          FUN_005243c0(iVar6);
        }
      }
      sVar5 = sVar5 + 1;
      iVar6 = (int)sVar5;
    } while (iVar6 < *(int *)(psVar1 + 0x30));
  }
  iVar6 = *(int *)(psVar1 + 0x2a);
  sVar5 = 0;
  if (0 < iVar6) {
    iVar6 = 0;
    do {
      iVar6 = iVar6 * 0x40;
      if ((*psVar1 == 3) &&
         ((*(short *)(*(int *)(psVar1 + 0x2c) + iVar6 + 0x20) != 0 ||
          (*(short *)(*(int *)(psVar1 + 0x2c) + iVar6 + 0x22) != 0)))) {
        *(undefined2 *)(*(int *)(psVar1 + 0x2c) + 0x20 + iVar6) = 0;
        *(undefined2 *)(*(int *)(psVar1 + 0x2c) + 0x22 + iVar6) = 0;
      }
      sVar5 = sVar5 + 1;
      iVar6 = (int)sVar5;
    } while (iVar6 < *(int *)(psVar1 + 0x2a));
  }
  iVar7 = *(int *)(psVar1 + 0x2a);
  if (((0 < iVar7) &&
      (iVar6 = iVar7 * 0x40 + -0x40 + *(int *)(psVar1 + 0x2c),
      *(short *)(iVar7 * 0x40 + -0x1e + *(int *)(psVar1 + 0x2c)) == 0)) &&
     (*(int *)(iVar6 + 0x34) == 0)) {
    local_5 = '\0';
  }
  iVar6 = CONCAT31((int3)((uint)iVar6 >> 8),DAT_006f1874);
  if (DAT_006f1874 != '\0') {
    iVar6 = 0;
    if (0 < *(int *)(psVar1 + 0x30)) {
      do {
        iVar6 = iVar6 + 1;
      } while ((int)(short)iVar6 < *(int *)(psVar1 + 0x30));
    }
    sVar5 = 0;
    if (0 < iVar7) {
      iVar6 = 0;
      do {
        if (((*psVar1 == 3) &&
            (iVar6 = *(int *)(iVar6 * 0x40 + *(int *)(psVar1 + 0x2c) + 0x34), 0 < iVar6)) &&
           (sVar4 = 0, 0 < iVar6)) {
          do {
            sVar4 = sVar4 + 1;
          } while (sVar4 < iVar6);
        }
        sVar5 = sVar5 + 1;
        iVar6 = (int)sVar5;
      } while (iVar6 < iVar7);
    }
  }
  return CONCAT31((int3)((uint)iVar6 >> 8),local_5);
}
#endif
