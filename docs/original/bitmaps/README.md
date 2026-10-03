# `bitmaps` — Blam bitmap data, color helpers and software DXT decoding

Retail Halo PC `halo.exe` 1.0.10, `0x43f030 .. 0x440347` (27 functions), plain C / MSVC 7.1 / x86,
LTCG. Every file in this directory is one function, rewritten from its Ghidra decompilation against
`types/bitmaps.h` and `types/tags.h`. The original decompile is kept verbatim at the bottom of each
file inside `#if 0 ... #endif` for diffing. Every function in the range was also checked
against `objdump` of `bin/halo.exe` during the phase 4 review (see "Review fixes" below).

Gate: `python tools/build_check.py bitmaps` → **27 ok, 0 failed**.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| bitmap group | `0x43f030`–`0x43f32f` | post-process a freshly loaded `bitm` tag (pixel sizes, pixel base pointers, hardware texture creation, sprite sequence clean-up); look up a `BitmapData` by index or by sequence/frame |
| color math | `0x43f330`–`0x43f87f` | RGB↔HSV, packed ARGB/RGB → float, RGB or HSV interpolation, two-ARGB interpolation with an optional tint |
| bitmap data | `0x43f880`–`0x43fe5f` | free one runtime `BitmapData`; pixel addresses for 2D, 3D and cube map bitmaps; per-mip sizes; `bitmap_data_verify` |
| export | `0x43fe60`–`0x43ff7f` | `targa_export`: writes mip 0 of a 32-bit bitmap as an uncompressed TGA (screenshots, movie capture) |
| DXT | `0x43ff80`–`0x440347` | software decoders for one texel of a DXT1/DXT3/DXT5 block, used by the rasterizer texel sampler at `0x5247b0` |

The seven color helpers sit between bitmap functions in one contiguous run, so they are treated as
part of this object file (see `out/phase4/bitmaps_types_notes.md`).

Call graph inside the module:

```
bitmap_group_postprocess 0x43f030
  bitmap_data_calculate_mip_level_pixel_count 0x43fc10   (per mip level)
  bitmap_data_verify 0x43fd30 -> bitmap_data_depth_valid_for_type 0x43fe30, uint32_log2_floor 0x4cb740
  bitmap_data_calculate_pixel_data_size 0x43fb70 -> 0x43fc10
  rasterizer 0x523fa0 / 0x524100 / 0x524270 / 0x5243c0
bitmap_data_get_pixel_address 0x43fb20
  type 0 -> bitmap_data_get_row_address 0x43f8e0
  type 1 -> bitmap_data_get_volume_pixel_address 0x43f990
  type 2 -> bitmap_data_get_cube_map_pixel_address 0x43fa90
color_interpolate_argb_with_tint 0x43f7d0 -> color_interpolate 0x43f6a0
  -> color_rgb_to_hsv 0x43f330 (x2), color_hsv_to_rgb 0x43f460 -> __ftol 0x6391b4 (CRT)
targa_export 0x43fe60 -> file_reference_create/open/write/close, bitmap_data_get_row_address
dxt3_decode_alpha_texel 0x440150, dxt5_decode_alpha_texel 0x440190
  -> dxt1_decode_block_texel 0x43ffe0 -> color_565_unpack_to_rgb888 0x43ff80 (x2)
```

## Struct layouts

Tag side structs come from `types/tags.h` and are not redefined. The tables below list only the
fields this module reads or writes. Offsets are byte offsets; `#pragma pack(push,1)` is in force.

### `Bitmap` (tags.h) — fields used

| Off | Type | Field | Use |
|---|---|---|---|
| `0x00` | `BitmapType_t` | `type` | 3 = sprites (sequence clean-up), 4 = interface bitmaps (linear flag, no hardware texture) |
| `0x30` | `uint32_t` | `processed_pixel_data.size` | bound for `pixel_data_offset + pixel data size` |
| `0x3c` | `void *` | `processed_pixel_data.pointer` | base of the pixels |
| `0x54` / `0x58` | reflexive | `bitmap_group_sequence` | stride `0x40` |
| `0x60` / `0x64` | reflexive | `bitmap_data` | stride `0x30` |

### `BitmapGroupSequence` (tags.h) — size `0x40`

| Off | Type | Field | Note |
|---|---|---|---|
| `0x20` | `uint16_t` | `first_bitmap_index` | added as a word |
| `0x22` | `uint16_t` | `bitmap_count` | the code treats it as **signed** (`test ax,ax; jle`, `idiv`) |
| `0x34` / `0x38` | reflexive | `sprites` | `BitmapGroupSprite`, stride `0x20`, `bitmap_index` at `+0x00` |

### `BitmapData` (tags.h) — size `0x30`, with runtime meanings

| Off | Type | Field | Runtime meaning |
|---|---|---|---|
| `0x00` | `uint32_t` | `bitmap_class` | must be `'bitm'` (`0x6269746d`) |
| `0x04` / `0x06` / `0x08` | `uint16_t` | `width` / `height` / `depth` | read as signed words everywhere |
| `0x0a` | `int16_t` | `type` | 0 2D, 1 3D, 2 cube map, 3 white |
| `0x0c` | `int16_t` | `format` | index into `bitmap_format_bits_per_pixel`, `< 18` |
| `0x0e` | `uint16_t` | `flags` | see `bitmap_data_flags` in `types/bitmaps.h` |
| `0x14` | `uint16_t` | `mipmap_count` | signed word, inclusive last level |
| `0x18` | `uint32_t` | `pixel_data_offset` | signed, offset into `processed_pixel_data` |
| `0x1c` | `uint32_t` | `pixel_data_size` | rewritten by `bitmap_group_postprocess` |
| `0x20` | `TagID` | `bitmap_tag_id` | rewritten with the owning tag handle |
| `0x24` | `uint32_t` | `pointer` | texture cache `datum_index`, `-1` when not resident |
| `0x28` | `uint8_t[4]` | `_pad_28` | `IDirect3DBaseTexture9 *` hardware texture |
| `0x2c` | `uint8_t[4]` | `_pad_2c` | base address of the pixels |

Flags given a runtime meaning here: `0x02` compressed (4×4 block rounding), `0x04|0x08` palettized /
swizzled (rejected by `require_runtime`), `0x10` linear (set on interface bitmaps), `0x40` runtime
allocated (GlobalFree pixels and the `BitmapData` itself), `0x80` texture-cache streamed (evict the
`+0x24` handle), `0xfe00` invalid.

### `real_hsv_color` (bitmaps.h) — size `0x0c`

| Off | Type | Field |
|---|---|---|
| `0x00` | `float` | `hue` in `[0, 1)` |
| `0x04` | `float` | `saturation` |
| `0x08` | `float` | `value` |

### `targa_header` (bitmaps.h) — size `0x12`

| Off | Type | Field | Value written |
|---|---|---|---|
| `0x00` | `uint8_t` | `id_length` | 0 |
| `0x01` | `uint8_t` | `color_map_type` | 0 |
| `0x02` | `uint8_t` | `image_type` | 2 (true color) |
| `0x03` | `uint16_t` | `color_map_first_entry` | 0 |
| `0x05` | `uint16_t` | `color_map_length` | 0 |
| `0x07` | `uint8_t` | `color_map_entry_size` | 0 |
| `0x08` / `0x0a` | `int16_t` | `x_origin` / `y_origin` | 0 |
| `0x0c` / `0x0e` | `int16_t` | `width` / `height` | from `BitmapData` |
| `0x10` | `uint8_t` | `bits_per_pixel` | 32 |
| `0x11` | `uint8_t` | `image_descriptor` | `0x28` (top-left origin, 8 alpha bits) |

### `dxt_color_block` — size `0x08`; `dxt3_block`, `dxt5_block` — size `0x10`

| Struct | Off | Type | Field |
|---|---|---|---|
| `dxt_color_block` | `0x00` / `0x02` | `uint16_t` | `color0` / `color1` (r5g6b5; 4-color mode when `color0 > color1` unsigned) |
| | `0x04` | `uint32_t` | `indices`, 2 bits per texel, texel `x + 4y` |
| `dxt3_block` | `0x00` | `uint16_t[4]` | `alpha_rows`, 4 bits per texel |
| | `0x08` | `dxt_color_block` | `color` |
| `dxt5_block` | `0x00` / `0x01` | `uint8_t` | `alpha0` / `alpha1` |
| | `0x02` | `uint8_t[6]` | two 24-bit groups of 3-bit indices (rows 0–1, rows 2–3) |
| | `0x08` | `dxt_color_block` | `color` |

Decoded texels are `ColorARGBInt` (tags.h: blue, green, red, alpha bytes).

### Globals

| Address | Declaration | Note |
|---|---|---|
| `0x006571f4` | `int8_t bitmap_format_bits_per_pixel[18]` | `.rdata`: 8 8 8 16 0 0 16 0 16 16 32 32 0 0 4 8 8 8 |
| `0x006f1874` | `uint8_t bitmap_group_debug_dump` | `.bss`, never written in the retail image |
| `0x0087bc14` | `tag_instance *tag_instances` | not owned (types/cache.h) |
| `0x006ac540` | `struct cache *texture_cache` | not owned (types/cache.h) |

## Register conventions (LTCG, confirmed from objdump)

| Function | Registers | Stack |
|---|---|---|
| `bitmap_group_postprocess` | — | `tag_id, skip_hardware_textures` |
| `bitmap_group_get_bitmap_data` | EAX tag, DX index | — |
| `bitmap_group_sequence_get_bitmap_data` | EAX tag, DI frame | `sequence_index` |
| `color_rgb_to_hsv` | ECX rgb in, EDX hsv out (returned in EAX) | — |
| `color_hsv_to_rgb` | EDI hsv in, ESI rgb out (returned in EAX) | — |
| `color_argb_int_to_real`, `color_rgb_int_to_real` | EAX out, ECX packed | — |
| `color_interpolate` | EAX color weighted by `t`, ECX color weighted by `1-t` | `dest, flags, t` |
| `color_interpolate_argb_with_tint` | EBX ARGB color1, EDX flags, ESI dest, EDI tint | `color0, t` |
| `bitmap_data_free` | ESI bitmap | — |
| `bitmap_data_get_row_address` | EDI bitmap, AX mip | `x, y` |
| `bitmap_data_get_volume_pixel_address` | ECX bitmap | `x, y, z, mip` |
| `bitmap_data_get_cube_map_pixel_address` | EDI bitmap, CX mip | `x, y, face` |
| `bitmap_data_get_pixel_address` | ECX bitmap, EAX mip | — |
| `bitmap_data_calculate_pixel_data_size` | EAX bitmap | — |
| `bitmap_data_calculate_mip_dimension`, `_mip_row_byte_size` | EDX bitmap, CL mip | — |
| `bitmap_data_calculate_mip_depth` | EAX bitmap, DL mip | — |
| `bitmap_data_calculate_mip_level_pixel_count` | ESI bitmap, CL mip | — |
| `bitmap_data_calculate_mip_level_byte_size` | EAX bitmap, CL mip | — |
| `bitmap_data_verify` | EDX bitmap; returns AL only | `require_runtime` |
| `bitmap_data_depth_valid_for_type` | AX depth; returns EAX 0/1 | `type` |
| `targa_export` | EAX bitmap, EBX file reference | — |
| `color_565_unpack_to_rgb888` | EAX pointer to the word | `out` |
| `dxt1_decode_block_texel` | EAX out | `block, x, y` |
| `dxt3_decode_alpha_texel` | EBX x, ESI y, EDI out | `block` |
| `dxt5_decode_alpha_texel` | EBX block | `out, x, y` |

In the C signatures the `BitmapData *` always comes first, then the mip level, then the stack
arguments. The C parameter order is only a label: the registers above are what the binary uses.

## Review fixes (phase 4)

- `bitmap_data_verify`: the `uint32_log2_floor` argument is `max(width, height, depth)`, not
  `max(width, height)`. The return is only AL (`xor al,al` / `mov al,1`), so the function and its
  caller now use `uint8_t`.
- `bitmap_group_sequence_get_bitmap_data`: `bitmap_count` is `uint16_t` in tags.h but is tested and
  divided as a signed word. Casts added.
- `bitmap_data_get_volume_pixel_address`: `mip_level` is a signed word (`test bp,bp; jle`), was
  `uint16_t`.
- `bitmap_data_get_cube_map_pixel_address`: x/y/face are read as words (`movsx`). The mip halving now
  uses `>> 1` (`sar`) like the other walkers.
- `color_rgb_to_hsv`, `color_hsv_to_rgb`: they return their out pointer in EAX. Now modelled.
- `targa_export`: the mip level passed to `bitmap_data_get_row_address` is 0 (`xor eax,eax` at
  `0x43ff03`), so it is no longer UNSURE.
- `bitmap_group_postprocess`: `0x524270` is the **3D** volume upload (it calls
  `bitmap_data_calculate_mip_depth`), despite its rasterizer name.
- Parameter order made bitmap-first for `bitmap_data_calculate_mip_level_pixel_count`,
  `_mip_dimension`, `_mip_row_byte_size`, `bitmap_data_get_pixel_address` and
  `bitmap_data_get_cube_map_pixel_address`, matching most of the foreign declarations.
- The local `release_fn` typedef was moved into `types/bitmaps.h` as
  `bitmap_hardware_texture_release_proc`. All 7 `bitmap_format_bits_per_pixel` externs now
  read the same.

## Known gaps

- **Foreign declarations disagree with this module** (not edited here, they belong to other modules):
  - `src/main/screenshot_render.c` still calls 0x43f880 `bitmap_group_free`, and
    `src/rasterizer/rasterizer_shutdown.c` declares it as `void bitmap_group_free(void)`.
  - `src/main/screenshot_render.c` and `src/main/movie_capture_frame_export.c` declare
    `targa_export` returning `uint8_t`. It returns `char *`, NULL on success or an error string
    at `0x65f934` / `0x65f948` / `0x65f960`.
  - `src/rasterizer/rasterizer_bitmap_sample_texel.c` declares the three DXT decoders as returning
    the texel. At `0x5247be..0x524805` they write through an out pointer (EAX for DXT1, EDI for
    DXT3, the stack for DXT5) and the caller reads the local afterwards.
  - `src/rasterizer/rasterizer_bitmap_upload_cubemap_mipmaps_by_face.c` still calls
    `FUN_0043fa90(bitmap, face, mip_level)`. The real inputs are EDI bitmap and CX mip, with x, y
    and face on the stack.
  - `src/rasterizer/*` names `0x524270` `rasterizer_bitmap_upload_cubemap_mipmaps` but it uploads
    3D textures. `0x5243c0` (`..._by_face`) is the cube map path.
  - `color_interpolate` (0x43f6a0) is declared with at least four different, mostly wrong,
    signatures in ai/effects/objects/rasterizer. The real signature is EAX/ECX colors plus
    `(dest, flags, t)` on the stack.
  - The address `0x006571f4` is called `rasterizer_bitmap_format_bits_per_pixel` in 5 rasterizer
    files and `bitmap_format_bits_per_pixel` here.
- `bitmap_group_postprocess` has no direct `call` in the image; it is reached through a pointer
  (the tag group postprocess table, most likely). The caller was not confirmed.
- `color_hsv_to_rgb` and `color_interpolate` are written with `float` intermediates. The original
  keeps `hue*6` and the blends on the x87 stack. Results match when the FPU is in 24-bit precision,
  which Direct3D sets by default, but may differ in the last ulp at 53/64-bit precision.
- `dxt3_decode_alpha_texel` passes the full ESI to `dxt1_decode_block_texel` as y but indexes the
  alpha rows with SI only. The C passes one `int32_t y` to both, so they agree only when the caller
  keeps ESI's upper half clean (the one caller, `0x5247ef`, does: `and esi,3`).
- `bitmap_group_debug_dump` (`0x006f1874`): the name is a guess. It is never written.

## Functions and rewrite confidence

`name` is confidence in the symbol name, `rw` is confidence in the C rewrite, and `U` counts
`UNSURE` markers in the file. Every function was compared line by line with objdump in the
phase 4 review.

| Address | Function | Bytes | name | rw | U |
|---|---|---|---|---|---|
| `0x43f030` | `bitmap_group_postprocess` | 534 | 0.80 | 0.80 | 0 |
| `0x43f250` | `bitmap_group_get_bitmap_data` | 53 | 0.60 | 0.85 | 0 |
| `0x43f290` | `bitmap_group_sequence_get_bitmap_data` | 153 | 0.50 | 0.85 | 0 |
| `0x43f330` | `color_rgb_to_hsv` | 290 | 0.70 | 0.85 | 0 |
| `0x43f460` | `color_hsv_to_rgb` | 296 | 0.65 | 0.80 | 0 |
| `0x43f5a0` | `color_argb_int_to_real` | 135 | 0.75 | 0.85 | 0 |
| `0x43f630` | `color_rgb_int_to_real` | 105 | 0.75 | 0.85 | 0 |
| `0x43f6a0` | `color_interpolate` | 303 | 0.55 | 0.85 | 0 |
| `0x43f7d0` | `color_interpolate_argb_with_tint` | 166 | 0.80 | 0.85 | 0 |
| `0x43f880` | `bitmap_data_free` | 94 | 0.80 | 0.85 | 0 |
| `0x43f8e0` | `bitmap_data_get_row_address` | 165 | 0.55 | 0.85 | 0 |
| `0x43f990` | `bitmap_data_get_volume_pixel_address` | 241 | 0.80 | 0.85 | 0 |
| `0x43fa90` | `bitmap_data_get_cube_map_pixel_address` | 136 | 0.80 | 0.85 | 0 |
| `0x43fb20` | `bitmap_data_get_pixel_address` | 80 | 0.60 | 0.85 | 0 |
| `0x43fb70` | `bitmap_data_calculate_pixel_data_size` | 62 | 0.55 | 0.90 | 0 |
| `0x43fbb0` | `bitmap_data_calculate_mip_dimension` | 47 | 0.60 | 0.90 | 0 |
| `0x43fbe0` | `bitmap_data_calculate_mip_depth` | 34 | 0.60 | 0.90 | 0 |
| `0x43fc10` | `bitmap_data_calculate_mip_level_pixel_count` | 156 | 0.60 | 0.90 | 0 |
| `0x43fcb0` | `bitmap_data_calculate_mip_level_byte_size` | 33 | 0.55 | 0.90 | 0 |
| `0x43fce0` | `bitmap_data_calculate_mip_row_byte_size` | 75 | 0.50 | 0.90 | 0 |
| `0x43fd30` | `bitmap_data_verify` | 245 | 0.80 | 0.85 | 0 |
| `0x43fe30` | `bitmap_data_depth_valid_for_type` | 34 | 0.80 | 0.85 | 0 |
| `0x43fe60` | `targa_export` | 277 | 0.90 | 0.90 | 0 |
| `0x43ff80` | `color_565_unpack_to_rgb888` | 85 | 0.55 | 0.90 | 0 |
| `0x43ffe0` | `dxt1_decode_block_texel` | 357 | 0.70 | 0.85 | 0 |
| `0x440150` | `dxt3_decode_alpha_texel` | 54 | 0.55 | 0.85 | 0 |
| `0x440190` | `dxt5_decode_alpha_texel` | 440 | 0.65 | 0.85 | 0 |

The 7 names first established by this rewrite (`0x43f030`, `0x43f7d0`, `0x43f880`, `0x43f990`,
`0x43fa90`, `0x43fd30`, `0x43fe30`) are registered in `symbols/agent_phase4_bitmaps.txt`, to be merged
into `symbols/functions.txt` by `tools/merge_symbols.py`. The other 20 already matched
`symbols/functions.txt` exactly. `bitmap_data_get_row_address` keeps its established name
even though it returns a pixel address and not a row address, because two rasterizer files use it.

Misattributed: `0x6391b4` (called by `color_hsv_to_rgb`) is the CRT `__ftol` helper, not module code.
`0x43f880` was misnamed `bitmap_group_free`. It frees one `BitmapData`.
