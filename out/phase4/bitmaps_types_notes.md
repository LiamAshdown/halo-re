# bitmaps module: type notes (types/bitmaps.h)

Range 0x43f030..0x440347, 27 functions. Evidence is the objdump of `bin/halo.exe` (the Ghidra
packs were used for orientation only). Syntax gate: `out/phase4/bitmaps_smoke.c`, which also
asserts every struct size with a duplicate-case switch; it passes with
`C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/bitmaps_smoke.c`.

## Tag structs reused from types/tags.h (not redefined)

| struct | fields confirmed | functions |
|---|---|---|
| `Bitmap` (0x6c) | `type` 0x00 (== 3 sprites, == 4 interface bitmaps), `processed_pixel_data.size` 0x30, `.pointer` 0x3c, `bitmap_group_sequence` 0x54/0x58, `bitmap_data` 0x60/0x64 | 0x43f030, 0x43f250, 0x43f290 |
| `BitmapGroupSequence` (0x40) | `first_bitmap_index` 0x20, `bitmap_count` 0x22, `sprites` 0x34/0x38 | 0x43f030 (zeroes 0x20/0x22 in sprite groups; last sequence with count 0 and no sprites fails the load), 0x43f290 |
| `BitmapGroupSprite` (0x20) | `bitmap_index` 0x00 | 0x43f290 (`[DI*0x20 + sprites.pointer]`) |
| `BitmapData` (0x30) | `bitmap_class` 0x00, `width` 0x04, `height` 0x06, `depth` 0x08, `type` 0x0a, `format` 0x0c, `flags` 0x0e, `mipmap_count` 0x14, `pixel_data_offset` 0x18, `pixel_data_size` 0x1c, `bitmap_tag_id` 0x20, `pointer` 0x24, `_pad_28`, `_pad_2c` | every function from 0x43f030 to 0x43fe60 |

BitmapData runtime words (documented in the bitmaps.h header comment, consistent with
types/cache.h notes and types/rasterizer.h):
- 0x24 texture cache handle (`-1` when not resident): 0x43f030 writes -1; 0x43f880 evicts it
  from the texture cache (0x006ac540 in EDI, handle in EBX, `cache_evict_entry` 0x4d1c20) and
  writes -1.
- 0x28 hardware texture (`IDirect3DBaseTexture9 *`): 0x43f030 zeroes it and calls 0x523fa0 when
  still 0; 0x43f880 calls vtable +0x08 (Release) and zeroes it.
- 0x2c pixel base address: 0x43f030 sets `processed_pixel_data.pointer + pixel_data_offset` when
  `pixel_data_offset + pixel_data_size(bitmap) <= processed_pixel_data.size`; the pixel address
  routines add their byte offset to it; 0x43f880 GlobalFrees it when flag 0x40 is set.
- 0x1c and 0x20 are rewritten by 0x43f030 (size recomputed from the mip chain, tag index stored).

## New types in types/bitmaps.h

### `bitmaps_constants`, `bitmaps_signatures`
- `k_bitmap_data_format_count` 18, `k_bitmap_data_type_count` 4, `k_bitmap_maximum_dimension`
  30000, `k_bitmap_runtime_format` 11: the compare immediates in bitmap_data_verify 0x43fd30
  (`cmp cx,0x12`, `cmp ax,4`, `cmp bp,0x7530`, `cmp [edx+0xc],0xb`).
- `k_bitmap_maximum_depth` 256: 0x43fe30 `cmp ax,0x100`; depth > 1 only allowed for type 1 (3D).
- `k_cube_map_face_count` 6: `* 6` in 0x43fc10 and 0x43fa90.
- `k_bitmap_compressed_block_dimension` 4: `x + (-x & 3)` rounding in 0x43f8e0/0x43f990/0x43fa90
  (minimum dimension `((flags & 2) != 0) * 3 + 1`), 0x43fbb0, 0x43fc10, 0x43fce0.
- `k_dxt_block_dimension`, `k_dxt_block_texel_count`: `(x + y*4)` shifts in 0x43ffe0/0x440190,
  `rep stosd` of 0x10 dwords in 0x43ffe0.
- `k_targa_header_size` 0x12: `mov esi,0x12` before file_reference_write in 0x43fe60.
- `k_bitmap_data_signature` 'bitm': `cmp [edx],0x6269746d` in 0x43fd30.

### `bitmap_data_flags` (masks over BitmapData::flags)
- 0x02 compressed: every mip dimension routine. 0x04/0x08 palettized/swizzled: 0x43fd30
  `test [flags],0xe` in the require-runtime path. 0x10 linear: set by 0x43f030 for interface
  bitmap groups. 0x40 runtime allocated and 0x80 texture cache: 0x43f880 (`test al,al; jns` and
  `test [esi+0xe],0x40`). 0x100 external: not read here; texture_cache_page_allocate 0x444875.
- `k_bitmap_data_valid_flags_mask` 0x1ff: 0x43fd30 `test eax,0xfffffe00`.

### `color_interpolation_flags`
- bit 0: 0x43f6a0 `test bl,1` chooses the HSV path; bit 1: `shr ebx,1; and ebx,1` compared to
  `|h0 - h1| > 0.5` (double 0.5 at 0x00672cf0). Same bits as tags.h ColorInterpolationFlags.

### `real_hsv_color` (0x0c)
- 0x00 hue, 0x04 saturation, 0x08 value: written by color_rgb_to_hsv 0x43f330 (EDX out, ECX in
  ColorRGB), read by color_hsv_to_rgb 0x43f460 (`fld [edi]` * 6.0, `[edi+4]`, `[edi+8]`), built
  on the stack by color_interpolate at [esp+0xc]/[+0x10]/[+0x14].

### `targa_header` (0x12, packed)
- Every byte written by targa_export 0x43fe60 at [esp+0x10..+0x21]: 0x02 = 2, 0x0c/0x0e =
  BitmapData width/height, 0x10 = 0x20, 0x11 = 0x28, the rest zero. Standard TGA header.

### `dxt_color_block` (0x08), `dxt3_block` (0x10), `dxt5_block` (0x10)
- color block: 0x43ffe0 reads words +0x00/+0x02 (`cmp dx,[esi]` unsigned for the mode) and the
  dword +0x04 (`mov edx,[ebp+4]`).
- dxt3: 0x440150 `lea eax,[ebp+8]` color, `mov ax,[ebp+edx*2]` alpha row y (SI), nibble x (BL).
- dxt5: 0x440190 bytes +0x00/+0x01 alpha endpoints, 24-bit groups at +0x02..+0x04 (rows 0-1)
  and +0x05..+0x07 (rows 2-3), color at +0x08 (`lea eax,[ebx+8]`).

## Globals
- 0x006571f4 `int8_t bitmap_format_bits_per_pixel[18]` (.rdata): 8 8 8 16 0 0 16 0 16 16 32 32
  0 0 4 8 8 8. 18 entries fixed by the format < 0x12 check; the following bytes are unrelated.
  src/rasterizer/bitmap_compute_texture_data_size.c and rasterizer_bitmap_sample_texel.c declare
  the same address as `rasterizer_bitmap_format_bits_per_pixel`; the two names should be
  reconciled (7 of the 12 readers are in this module).
- 0x006f1874 `uint8_t bitmap_group_debug_dump` (.bss): the only reference in the image is the
  read at 0x43f1ee, and no pointer to it exists in the data sections, so it is always 0 in the
  retail build. Name is a guess.
- Not owned: 0x0087bc14 tag_instances, 0x006ac540 texture_cache (both types/cache.h).

## Register conventions seen while pinning fields (LTCG, per function)
- 0x43f030 bitmap_group_postprocess: cdecl (tag index, bool skip_hardware_textures).
- 0x43f250: EAX bitmap tag index, DX bitmap_data index.
- 0x43f290: EAX bitmap tag index, stack sequence index, DI frame index.
- 0x43f330: ECX ColorRGB in, EDX real_hsv_color out. 0x43f460: EDI hsv in, ESI ColorRGB out.
- 0x43f5a0 / 0x43f630: EAX float out, ECX packed color.
- 0x43f6a0: EAX color weighted by t, ECX color weighted by 1-t, stack (dest, flags, t).
- 0x43f7d0: EBX and stack arg 1 are ColorARGB, EDX flags, ESI ColorRGB dest, EDI optional
  ColorRGB tint, stack arg 2 t.
- 0x43f880: ESI BitmapData. 0x43f8e0: EDI BitmapData, AX mip level, stack (x, y).
- 0x43f990: ECX BitmapData, stack (x, y, z, mip level). 0x43fa90: EDI BitmapData, CX mip level,
  stack (x, y, face). 0x43fb20: ECX BitmapData, EAX mip level.
- 0x43fb70 / 0x43fcb0: EAX BitmapData (0x43fcb0 also CL level). 0x43fbb0 / 0x43fce0: EDX
  BitmapData, CL level. 0x43fbe0: EAX BitmapData, DL level. 0x43fc10: ESI BitmapData, CL level.
- 0x43fd30: EDX BitmapData, stack bool require_runtime. 0x43fe30: AX depth, stack type.
- 0x43fe60: EAX BitmapData, EBX file_reference_record.
- 0x43ff80: EAX pointer to the r5g6b5 word, stack ColorARGBInt out. 0x43ffe0: EAX ColorARGBInt out, stack
  (block, x, y). 0x440150: EDI texel out, stack block, BL x, SI y. 0x440190: EBX block, stack
  (texel out, x, y).

## Unresolved offsets
- None inside the structs defined here. The new structs have every byte named from a read or a
  write.
- BitmapData 0x10 registration_point and 0x16 pad are not touched by this module (tags.h
  layout kept as is).
- Bitmap fields 0x02..0x2f and 0x44..0x53 are not touched by this module.

## Misnamed or misattributed functions
- 0x43f880 `bitmap_group_free` frees one **BitmapData**, not a group: it reads the flags at
  +0x0e, the cache handle at +0x24, the texture at +0x28, the pixels at +0x2c, and GlobalFrees
  the ESI pointer itself. Better name: `bitmap_data_free`. Its caller 0x43f010, just below the
  range, indexes a reflexive `(block->pointer + index*0x30)` and passes the element in ESI.
- 0x43fd30 does not validate a cache chunk header; the 'bitm' dword is BitmapData::bitmap_class.
  It is `bitmap_data_verify(bitmap, require_runtime)`. 0x43fe30 is its depth/type check.
- 0x43f030 is `bitmap_group_postprocess` (per the phase 2 JSON); it also creates the hardware
  textures through the rasterizer upload routines 0x523fa0/0x524100/0x524270/0x5243c0 when its
  second argument is false.
- 0x43f7d0 combines two ColorARGB colors (alpha at +0, RGB at +4) with an optional tint: a
  better name is `color_interpolate_argb_with_tint`.
- The seven color helpers 0x43f330..0x43f7d0 would live in a color math file by Blam naming, but
  they sit between bitmap functions (0x43f290 and 0x43f880) in one contiguous run, which points
  at one object file, so they are kept in this module and their one new type (real_hsv_color) is
  defined here.
- 0x6391b4, called by 0x43f460, is the CRT float-to-int helper (library code), not part of this
  module.
