// bitmap_data_calculate_mip_level_pixel_count  (Ghidra:
// bitmap_data_calculate_mip_level_pixel_count, already named)
// address 0x43fc10, size 156 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: independently computes width/height (each >>= level, clamped to 1, then rounded up
//   to a multiple of 4 when compressed) and depth (>>= level, clamped to 1, never rounded),
//   multiplies the three, and multiplies by k_cube_map_face_count when
//   BitmapDataType_t == bitmapdatatype_cube_map; out/phase4/bitmaps_functions.md: "Computes the
//   total pixel count of a bitmap_data at the current mip level (width*height*depth, times 6
//   for cubemaps)." This is the width/height half of bitmap_data_calculate_mip_dimension
//   (0x43fbb0) and the depth half of bitmap_data_calculate_mip_depth (0x43fbe0) inlined
//   together rather than calling them, exactly as Ghidra shows.
// register convention: ESI bitmap, CL level, per bitmaps_types_notes.md ("0x43fc10: ESI
//   BitmapData, CL level").
//   // blam-cc: ECX (CL) -> level, ESI -> bitmap
// The round-up-to-a-multiple-of-4 steps below are `(x + 3) & ~3` rather than Ghidra's
// `x + ((byte)-(char)x & 3)`; the two are the same 8-bit round-up-to-4 idiom and were checked to
// agree for every x in [0, 40000] (well past k_bitmap_maximum_dimension).

#include "tags.h"
#include "bitmaps.h"

// blam-cc: ECX (CL) -> level, ESI -> bitmap
// Computes the total texel count of bitmap at mip level `level`: width * height * depth (width
// and height rounded up to a multiple of 4 when the bitmap is block compressed), times
// k_cube_map_face_count for cube maps.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level)
{
    int32_t width;
    int32_t height;
    int32_t depth;
    int32_t pixel_count;
    uint32_t compressed;

    compressed = bitmap->flags & _bitmap_data_compressed_bit;

    width = (int16_t)bitmap->width >> (level & 0x1f);
    if (width < 2) {
        width = 1;
    }
    if (compressed != 0) {
        width = (width + 3) & ~3;
    }

    height = (int16_t)bitmap->height >> (level & 0x1f);
    if (height < 2) {
        height = 1;
    }
    if (compressed != 0) {
        height = (height + 3) & ~3;
    }

    depth = (int16_t)bitmap->depth >> (level & 0x1f);
    if (depth < 2) {
        depth = 1;
    }

    pixel_count = height * width * depth;
    if (bitmap->type == bitmapdatatype_cube_map) {
        pixel_count = pixel_count * k_cube_map_face_count;
    }
    return (uint32_t)pixel_count;
}

#if 0
Original Ghidra decompilation (0x43fc10):

int bitmap_data_calculate_mip_level_pixel_count(void)

{
  short sVar1;
  short sVar2;
  short sVar3;
  int iVar4;
  byte in_CL;
  int unaff_ESI;
  ushort uVar5;

  if (*(short *)(unaff_ESI + 4) >> (in_CL & 0x1f) < 2) {
    sVar1 = 1;
  }
  else {
    sVar1 = *(short *)(unaff_ESI + 4) >> (in_CL & 0x1f);
  }
  uVar5 = *(ushort *)(unaff_ESI + 0xe) & 2;
  if (uVar5 != 0) {
    sVar1 = sVar1 + ((byte)-(char)sVar1 & 3);
  }
  if (*(short *)(unaff_ESI + 6) >> (in_CL & 0x1f) < 2) {
    sVar2 = 1;
  }
  else {
    sVar2 = *(short *)(unaff_ESI + 6) >> (in_CL & 0x1f);
  }
  if (uVar5 != 0) {
    sVar2 = sVar2 + ((byte)-(char)sVar2 & 3);
  }
  if (*(short *)(unaff_ESI + 8) >> (in_CL & 0x1f) < 2) {
    sVar3 = 1;
  }
  else {
    sVar3 = *(short *)(unaff_ESI + 8) >> (in_CL & 0x1f);
  }
  iVar4 = (int)sVar2 * (int)sVar1 * (int)sVar3;
  if (*(short *)(unaff_ESI + 10) == 2) {
    iVar4 = iVar4 * 6;
  }
  return iVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
