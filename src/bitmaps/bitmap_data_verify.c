// bitmap_data_verify  (Ghidra: FUN_0043fd30; named here, not yet renamed in Ghidra/CEA)
// address 0x43fd30, size 245 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_types_notes.md, "Misnamed or misattributed functions": "0x43fd30
//   does not validate a cache chunk header: the 'bitm' dword is BitmapData::bitmap_class. It is
//   bitmap_data_verify(bitmap, require_runtime)." Checks, in order: the 'bitm' signature: the
//   flags fit k_bitmap_data_valid_flags_mask: type and format are within their enum ranges;
//   width and height are in (0, k_bitmap_maximum_dimension]; depth is valid for the type (via
//   bitmap_data_depth_valid_for_type, 0x43fe30); mipmap_count is non-negative and does not
//   exceed floor(log2(max(width, height, depth))); then, only when require_runtime is set, that the
//   bitmap is additionally a fully-resident runtime bitmap: format == bitmapdataformat_a8r8g8b8,
//   a non-null pixel pointer, mipmap_count == 0, and none of the compressed/palettized/swizzled
//   flags set.
// register convention: EDX bitmap, stack bool require_runtime, per bitmaps_types_notes.md
//   ("0x43fd30: EDX BitmapData, stack bool require_runtime").
//   // blam-cc: EDX -> bitmap, stack -> require_runtime
// log2 argument (verified against objdump, phase 4 review): ECX = max(width, height, depth)
//   (0x43fdc8..0x43fde8: edi = max(height, depth) by signed word compare, then width if larger),
//   and mipmap_count (BX) is compared to the low word of the result (cmp bx,ax; jg -> fail).
// Return (verified against objdump): the function sets only AL (xor al,al / mov al,1) and the
//   upper 24 bits of EAX are left over from earlier work; its only caller 0x43f0c6 tests AL.
//   Modelled as a uint8_t return.

#include "tags.h"
#include "bitmaps.h"
#include "fn_math.h"
#include "fn_bitmaps.h"


// blam-cc: EDX -> bitmap, stack -> require_runtime
// Validates bitmap's tag-side fields (signature, flags, type, format, width, height, depth,
// mipmap_count). When require_runtime is set, additionally requires it to already be a resident
// 32-bit-per-pixel runtime bitmap with no mip chain and none of the compressed/palettized/
// swizzled flags.
uint8_t bitmap_data_verify(BitmapData *bitmap, uint8_t require_runtime)
{
    int32_t max_dimension;
    int32_t max_levels;

    if (bitmap->bitmap_class != k_bitmap_data_signature) {
        return 0;
    }
    if ((bitmap->flags & ~(uint32_t)k_bitmap_data_valid_flags_mask) != 0) {
        return 0;
    }
    if (bitmap->type < 0 || bitmap->type >= k_bitmap_data_type_count) {
        return 0;
    }
    if (bitmap->format < 0 || bitmap->format >= k_bitmap_data_format_count) {
        return 0;
    }
    if (bitmap->width <= 0 || bitmap->width > k_bitmap_maximum_dimension) {
        return 0;
    }
    if (bitmap->height <= 0 || bitmap->height > k_bitmap_maximum_dimension) {
        return 0;
    }
    if (!bitmap_data_depth_valid_for_type(bitmap->depth, bitmap->type)) {
        return 0;
    }
    if ((int16_t)bitmap->mipmap_count < 0) {
        return 0;
    }

    // max(width, max(height, depth)); all three were range checked above, so the original's
    // signed word compares agree with these int compares.
    max_dimension = (bitmap->height > bitmap->depth) ? bitmap->height : bitmap->depth;
    if (bitmap->width > max_dimension) {
        max_dimension = bitmap->width;
    }
    max_levels = uint32_log2_floor((uint32_t)max_dimension);
    if ((int16_t)bitmap->mipmap_count > (int16_t)max_levels) {
        return 0;
    }

    if (require_runtime == 0) {
        return 1;
    }

    if (bitmap->format == k_bitmap_runtime_format &&
        *(void **)&((struct BitmapData *)bitmap)->pixel_base != 0 &&
        bitmap->mipmap_count == 0 &&
        (bitmap->flags & (_bitmap_data_compressed_bit | _bitmap_data_palettized_bit | _bitmap_data_swizzled_bit)) == 0) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x43fd30):

uint FUN_0043fd30(char param_1)

{
  ushort uVar1;
  ushort uVar2;
  short sVar3;
  uint in_EAX;
  int *in_EDX;
  int iVar4;
  undefined8 uVar5;

  if (*in_EDX == 0x6269746d) {
    uVar1 = *(ushort *)((int)in_EDX + 0xe);
    in_EAX = (uint)uVar1;
    if ((uVar1 & 0xfe00) == 0) {
      uVar2 = *(ushort *)((int)in_EDX + 10);
      in_EAX = (uint)uVar2;
      if ((((-1 < (short)uVar2) && ((short)uVar2 < 4)) && (-1 < (short)in_EDX[3])) &&
         ((((short)in_EDX[3] < 0x12 && (0 < (short)in_EDX[1])) &&
          (((short)in_EDX[1] < 0x7531 &&
           ((0 < *(short *)((int)in_EDX + 6) && (*(short *)((int)in_EDX + 6) < 0x7531)))))))) {
        uVar5 = FUN_0043fe30(in_EAX);
        in_EAX = (uint)uVar5;
        if (((char)uVar5 != '\0') &&
           (sVar3 = *(short *)((int)((ulonglong)uVar5 >> 0x20) + 0x14), -1 < sVar3)) {
          uVar5 = uint32_log2_floor();
          iVar4 = (int)((ulonglong)uVar5 >> 0x20);
          in_EAX = (uint)uVar5;
          if ((sVar3 <= (short)uVar5) &&
             ((in_EAX = CONCAT31((int3)((ulonglong)uVar5 >> 8),param_1), param_1 == '\0' ||
              ((((*(short *)(iVar4 + 0xc) == 0xb && (in_EAX = *(uint *)(iVar4 + 0x2c), in_EAX != 0))
                && (sVar3 == 0)) && ((uVar1 & 0xe) == 0)))))) {
            return CONCAT31((int3)(in_EAX >> 8),1);
          }
        }
      }
    }
  }
  return in_EAX & 0xffffff00;
}
#endif
