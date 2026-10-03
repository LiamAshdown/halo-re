// Blam bitmaps module (halo.exe 1.0.10 retail, 0x43f030..0x440347, 27 Ghidra functions).
// Bitmap group post-processing after a map load, bitmap_data lookup by index and by
// sequence/frame, mip level size and pixel address arithmetic for 2D, 3D and cube map
// bitmaps, bitmap_data validation and teardown, a Targa exporter, the RGB/HSV color helpers
// and the software DXT1/DXT3/DXT5 texel decoders.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - Every tag side layout already exists in types/tags.h and is reused, never redefined. The
//     arithmetic in this module agrees with it everywhere:
//       Bitmap 0x6c                 type 0x00 (compared to 3 sprites and 4 interface bitmaps),
//                                   processed_pixel_data.size 0x30 and .pointer 0x3c,
//                                   bitmap_group_sequence 0x54/0x58, bitmap_data 0x60/0x64
//       BitmapGroupSequence 0x40    first_bitmap_index 0x20, bitmap_count 0x22,
//                                   sprites.count 0x34, sprites.pointer 0x38 (stride 0x40)
//       BitmapGroupSprite 0x20      bitmap_index 0x00 (stride 0x20)
//       BitmapData 0x30             bitmap_class 0x00 ('bitm'), width 0x04, height 0x06,
//                                   depth 0x08, type 0x0a, format 0x0c, flags 0x0e,
//                                   mipmap_count 0x14, pixel_data_offset 0x18,
//                                   pixel_data_size 0x1c, bitmap_tag_id 0x20 (stride 0x30)
//
//   - Three BitmapData words hold something different once a map is resident. They are named
//     here rather than changed in tags.h, since tags.h describes the on-disk layout and
//     types/cache.h / types/rasterizer.h already read them the same way:
//       0x24 pointer   the texture cache datum_index, -1 when not resident. bitmap_group
//                      postprocess 0x43f030 writes -1, bitmap_data_free 0x43f880 passes it to
//                      cache_evict_entry and writes -1 back.
//       0x28 _pad_28   the IDirect3DBaseTexture9 * hardware texture. 0x43f030 zeroes it and
//                      creates one (0x523fa0) when it is still 0; 0x43f880 calls its vtable
//                      slot +0x08 (Release) and zeroes it.
//       0x2c _pad_2c   the base address of the pixels: processed_pixel_data.pointer +
//                      pixel_data_offset after 0x43f030, a texture cache page when streamed,
//                      or a GlobalAlloc buffer for a runtime bitmap. Every pixel address
//                      routine (0x43f8e0, 0x43f990, 0x43fa90) adds its byte offset to it.
//     And 0x43f030 rewrites 0x1c pixel_data_size (sum over mip levels of the pixel count times
//     bits per pixel / 8) and 0x20 bitmap_tag_id (the owning bitmap tag index) for every entry.
//
//   - bitmap_format_bits_per_pixel is read straight out of .rdata at 0x006571f4. The table is
//     18 signed bytes: bitmap_data_verify 0x43fd30 rejects format >= 0x12, and the bytes after
//     index 17 (0x00657206: ff 00 fd fc ...) are not bit counts.
//
//   - targa_header is built field by field on the stack by targa_export 0x43fe60 and written
//     with a size of 0x12 (mov esi,0x12 before file_reference_write). It is the standard TGA
//     file header, packed.
//
//   - The DXT blocks are the Direct3D DXT1/DXT3/DXT5 (BC1/BC2/BC3) block layouts; each field
//     below is pinned by the byte offsets the decoders 0x43ffe0, 0x440150 and 0x440190 read.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/tags.h        Bitmap, BitmapGroupSequence, BitmapGroupSprite, BitmapData,
//                       BitmapDataType_t, BitmapDataFormat_t, BitmapType_t, BitmapDataFlags,
//                       ColorRGB (color_interpolate operands and result), ColorARGB (the two
//                       colors of color_interpolate_argb 0x43f7d0, alpha at +0x00 and RGB at
//                       +0x04), ColorARGBInt (the texel the DXT decoders write: blue, green,
//                       red, alpha bytes, as color_565_unpack_to_rgb888 0x43ff80 lays them out)
//   types/memory.h      datum_index (bitmap tag handles; only the low 16 bits index tag_instances)
//   types/cache.h       tag_instance (0x0087bc14, tag data at +0x14), texture_cache (0x006ac540)
//   types/saved_games.h file_reference_record (the EBX argument of targa_export)
//
// Functions in this address range that are misnamed or belong elsewhere are listed at the end
// of out/phase4/bitmaps_types_notes.md.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum bitmaps_constants {
    k_bitmap_data_format_count = 18,          // bitmap_data_verify: format < 0x12; table size
    k_bitmap_data_type_count = 4,             // bitmap_data_verify: type < 4 (2d, 3d, cube, white)
    k_bitmap_maximum_dimension = 30000,       // bitmap_data_verify: width, height <= 0x7530
    k_bitmap_maximum_depth = 256,             // bitmap_data_depth_valid_for_type 0x43fe30: depth <= 0x100
    k_bitmap_compressed_block_dimension = 4,  // compressed mip sizes round up to a multiple of 4,
                                              // and the minimum mip dimension becomes 4, not 1
    k_cube_map_face_count = 6,                // mip pixel count * 6 for cube maps (0x43fc10, 0x43fa90)
    k_bitmap_runtime_format = 11,             // bitmap_data_verify(require_runtime): format must be
                                              // bitmapdataformat_a8r8g8b8
    k_dxt_block_dimension = 4,                // texel index = x + y*4 in every DXT decoder
    k_dxt_block_texel_count = 16,             // dxt1_decode_block_texel with no block clears 16 dwords
    k_targa_header_size = 0x12                // targa_export writes exactly 0x12 header bytes
} bitmaps_constants;

typedef enum bitmaps_signatures {
    k_bitmap_data_signature = 0x6269746d      // 'bitm' at BitmapData+0x00, checked by 0x43fd30
} bitmaps_signatures;

// ---------------------------------------------------------------------------
// BitmapData::flags (0x0e), as masks. Names follow tags.h BitmapDataFlags; the bits this module
// gives a runtime meaning to are called out.
// ---------------------------------------------------------------------------
typedef enum bitmap_data_flags {
    _bitmap_data_power_of_two_dimensions_bit = 0x001,
    _bitmap_data_compressed_bit = 0x002,      // (used) block compressed: every mip width/height
                                              //        test in 0x43f8e0..0x43fce0
    _bitmap_data_palettized_bit = 0x004,      // (used) 0x43fd30 require_runtime rejects flags & 0xe
    _bitmap_data_swizzled_bit = 0x008,        // (used) same test
    _bitmap_data_linear_bit = 0x010,          // (used) 0x43f030 sets it on every bitmap_data of an
                                              //        interface_bitmaps group
    _bitmap_data_v16u16_bit = 0x020,
    _bitmap_data_runtime_allocated_bit = 0x040, // (used) tags.h unused. bitmap_data_free 0x43f880
                                              //        GlobalFrees the pixels at +0x2c and then the
                                              //        BitmapData itself; set by the runtime
                                              //        constructors (screenshot 0x40/0x41, font atlas 0x41)
    _bitmap_data_texture_cache_bit = 0x080,   // (used) tags.h make_it_actually_work. The pixels are
                                              //        streamed through the texture cache: 0x43f880
                                              //        evicts the +0x24 handle (tested as the sign
                                              //        bit of the flags byte)
    _bitmap_data_external_bit = 0x100,        // pixels live in bitmaps.map (types/cache.h)
    k_bitmap_data_valid_flags_mask = 0x1ff    // (used) 0x43fd30 rejects any bit in 0xfe00
} bitmap_data_flags;

// ---------------------------------------------------------------------------
// color_interpolate (0x43f6a0) flags argument. Same bits as the tag field
// ColorInterpolationFlags (blend_in_hsv, more_colors) in types/tags.h.
// ---------------------------------------------------------------------------
typedef enum color_interpolation_flags : int {
    _color_interpolation_hsv_bit = 0x1,       // blend in HSV space, else a plain RGB lerp
    _color_interpolation_long_hue_path_bit = 0x2 // with hsv: take the hue path longer than 0.5
                                              // (the smaller hue gets +1.0 when |h0 - h1| <= 0.5)
} color_interpolation_flags;

// ---------------------------------------------------------------------------
// real_hsv_color
// Output of color_rgb_to_hsv 0x43f330 (EDX) and input of color_hsv_to_rgb 0x43f460 (EDI);
// color_interpolate builds one on its stack at [esp+0xc]. All three components are in [0, 1];
// hue is scaled by 6.0 (0x00672c44) to pick the sextant.
// ---------------------------------------------------------------------------
typedef struct real_hsv_color {
    float hue;                     // 0x00 rgb_to_hsv wraps it into [0, 1); hsv_to_rgb reads it * 6
    float saturation;              // 0x04 (max - min) / max; 0 makes hsv_to_rgb return grey
    float value;                   // 0x08 max(r, g, b)
} real_hsv_color;                  // size 0x0c

// ---------------------------------------------------------------------------
// targa_header
// Built on the stack by targa_export 0x43fe60 at [esp+0x10]: five dwords zeroed, then
// image_type 2, width and height from BitmapData +0x04/+0x06, 32 bits per pixel and
// descriptor 0x28 (8 alpha bits, top-left origin). The rows that follow are width*4 bytes
// each (movsx esi,[edi+4]; shl esi,2).
// ---------------------------------------------------------------------------
typedef enum targa_constants {
    k_targa_image_type_true_color = 2,        // uncompressed true color
    k_targa_bits_per_pixel = 32,
    k_targa_image_descriptor_top_left_8_alpha = 0x28
} targa_constants;

typedef struct targa_header {
    uint8_t id_length;             // 0x00 0
    uint8_t color_map_type;        // 0x01 0
    uint8_t image_type;            // 0x02 k_targa_image_type_true_color
    uint16_t color_map_first_entry; // 0x03 0
    uint16_t color_map_length;     // 0x05 0
    uint8_t color_map_entry_size;  // 0x07 0
    int16_t x_origin;              // 0x08 0
    int16_t y_origin;              // 0x0a 0
    int16_t width;                 // 0x0c BitmapData width
    int16_t height;                // 0x0e BitmapData height
    uint8_t bits_per_pixel;        // 0x10 32
    uint8_t image_descriptor;      // 0x11 0x28
} targa_header;                    // size 0x12

// ---------------------------------------------------------------------------
// DXT blocks (4x4 texels each)
// dxt_color_block is the whole of a DXT1 block and the second half of DXT3/DXT5 blocks.
// dxt1_decode_block_texel 0x43ffe0 unpacks color0 (+0x00) and color1 (+0x02) with
// color_565_unpack_to_rgb888, picks the 4 color mode when color0 > color1 (unsigned compare
// of the two words) and the 3 color + transparent mode otherwise, then selects entry
// (indices >> ((x + y*4) * 2)) & 3 with indices read as a dword at +0x04.
// ---------------------------------------------------------------------------
typedef struct dxt_color_block {
    uint16_t color0;               // 0x00 r5g6b5
    uint16_t color1;               // 0x02 r5g6b5
    uint32_t indices;              // 0x04 2 bits per texel, texel 0 in the low bits
} dxt_color_block;                 // size 0x08

// dxt3_decode_alpha_texel 0x440150: color block at +0x08 (lea eax,[ebp+8]); alpha is the
// nibble (alpha_rows[y] >> (x*4)) & 0xf, widened to 8 bits as (a << 4) | a.
typedef struct dxt3_block {
    uint16_t alpha_rows[4];        // 0x00 one word per row, 4 bits per texel
    dxt_color_block color;         // 0x08
} dxt3_block;                      // size 0x10

// dxt5_decode_alpha_texel 0x440190: alpha0/alpha1 at +0x00/+0x01 build the 8 entry
// alpha palette (7 interpolated steps when alpha0 > alpha1, else 5 steps plus 0 and 0xff);
// rows 0 and 1 take their 3 bit indices from the 24 bits at +0x02..+0x04, rows 2 and 3 from
// +0x05..+0x07; the color block is at +0x08 (lea eax,[ebx+8]).
typedef struct dxt5_block {
    uint8_t alpha0;                // 0x00
    uint8_t alpha1;                // 0x01
    uint8_t alpha_indices[6];      // 0x02 two 24 bit little-endian groups, 3 bits per texel
    dxt_color_block color;         // 0x08
} dxt5_block;                      // size 0x10

// ---------------------------------------------------------------------------
// BitmapData +0x28 hardware texture (IDirect3DBaseTexture9 *). bitmap_data_free 0x43f880 calls
// vtable slot 2 (+0x08, IUnknown::Release, stdcall, this pushed) and ignores the result.
// ---------------------------------------------------------------------------
typedef uint32_t (*bitmap_hardware_texture_release_proc)(void *texture);

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------
// global 0x006571f4: int8_t bitmap_format_bits_per_pixel[18]   .rdata, indexed by BitmapData
//                    format: 8 8 8 16 0 0 16 0 16 16 32 32 0 0 4 8 8 8. Read with movsx by
//                    0x43f030, 0x43f8e0, 0x43f990, 0x43fa90, 0x43fb70, 0x43fcb0, 0x43fce0 and
//                    by the rasterizer at 0x5146e9, 0x51835f, 0x52433d, 0x5244a7, 0x524792
//                    (src/rasterizer declares it as rasterizer_bitmap_format_bits_per_pixel).
// global 0x006f1874: uint8_t bitmap_group_debug_dump   .bss, never written by any instruction
//                    in the image. bitmap_group_postprocess 0x43f030 tests it and then runs two
//                    empty loops over the bitmap_data and sprite blocks (debug output compiled
//                    out of the retail build). Name is a guess (UNSURE).
//
// Globals this module reads but does not own:
//   0x0087bc14 tag_instance *tag_instances   (types/cache.h)
//   0x006ac540 cache *texture_cache          (types/cache.h), passed to cache_evict_entry in EDI

#pragma pack(pop)
