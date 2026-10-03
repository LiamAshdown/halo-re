#pragma once
// Blam rasterizer module (halo.exe 1.0.10 retail, 0x5132b0..0x537d60, 228 Ghidra functions).
// This is the Direct3D 9 back end of the renderer. It owns the device and its caps, the window
// parameters of the frame being drawn, the dynamic vertex and index caches, the transparent
// geometry group queue and its depth sort, the lens flare occlusion and sprite batching system,
// the debug font atlas, the vertex declaration / vertex buffer / effect tables, the render target
// pool, the gamma ramps and a long list of fixed-function and pixel-shader draw paths.
//
// Offsets in comments are byte offsets from the struct base. Where the binary itself carries a
// layout it is preferred over the decompiler and the fact is called out:
//
//   - Every tag side layout already exists in types/tags.h and is not redefined here. The ones
//     this module reads were re-derived from its arithmetic and agree: BitmapData (0x30; the
//     font atlas constructor 0x514820 fills one field by field, bitmap_compute_mipmap_count
//     0x5145a0 reads width/height/depth/flags at +4/+6/+8/+0xe, and every texture bind reads the
//     hardware texture at +0x28, which tags.h still has as _pad_28), LensFlare (+0x10
//     occlusion_radius, +0x14 occlusion_offset_direction, +0x18/+0x1c near/far fade, +0x30
//     flags, +0xc4 reflections.count), Light (+0x1c cos_falloff_angle, +0x20 cos_cutoff_angle,
//     +0x24 specular_radius_multiplier, +0x70 primary_cube_map.tag_id, +0x88
//     secondary_cube_map.tag_id), Shader (+0x24 shader_type, +0x28 the first flags word of the
//     derived shader), ScenarioStructureBSPCluster (+0x40/+0x42 lens flare marker range, stride
//     0x68 at ScenarioStructureBSP +0x138), ScenarioStructureBSPLensFlareMarker (0x10),
//     ScenarioStructureBSPMaterial (+0xb0 rendered and +0xc4 lightmap vertex buffers),
//     ModelGeometryPart (+0x44 triangle buffer, +0x54 vertex buffer), FontCharacter (+4/+6
//     bitmap size, +0xc hardware_character_index, +0x10 pixels offset) and
//     GlobalsRasterizerData (Globals +0x134 rasterizer_data, read into 0x0071d164).
//
//   - rasterizer_vertex_buffer and rasterizer_index_buffer are the runtime views of the tag
//     blocks named above; rasterizer_vertex_buffer_create 0x524980 and
//     rasterizer_index_buffer_create 0x525030 are their constructors and fill every field.
//
//   - The four Direct3D 9 records below (d3d_caps9, d3d_present_parameters, d3d_gamma_ramp and
//     the element strides) are the public SDK layouts. They are pinned to this binary by the
//     field accesses listed on each one, e.g. PixelShaderVersion at caps +0xcc is the global
//     0x007c118c that the whole module compares against 0xffff0101 (ps_1_1).
//
//   - The render window parameters (0x258 bytes) are copied in one rep movsd of 0x96 dwords by
//     rasterizer_begin_frame 0x5175c0; the camera part is 0x15 dwords (FUN_0050bdc0 copies it),
//     the fog part starts at +0x1e8 (rasterizer_begin_frame passes lea edx,[ebp+0x1e8] to
//     0x5176d0, which copies 0x14 dwords), and the frustum part is 0x18c bytes, which is also the
//     size types/structures.h found for the per-cluster frustum of structure_bsp_visible_cluster.
//     Sub-field names inside camera and frustum follow the OpenSauce layout as a hint only; the
//     ones marked (used) are pinned by accesses in this module.
//
//   - Every pointer field is held as uint32_t with the pointee type written first in its
//     comment (the effects.h convention), so the 32 bit sizes survive a 64 bit host compiler.
//
//   - Several table sizes come from the address of the next global referenced by the module
//     rather than from a literal; those are marked (bounded by next global).
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here:
//   types/memory.h   datum_index
//   types/math.h     real_point3d, real_vector3d, real_vector2d, real_plane3d, real_matrix4x3,
//                    real_rectangle3d
//   types/tags.h     every tag structure named above, plus ColorRGB, ColorARGB and Rectangle2D
//
// Functions in this address range that are misnamed, misattributed or not real functions are
// listed at the end of out/phase4/rasterizer_types_notes.md.

#include <stddef.h>   /* NULL for the inline helpers below; the mingw-only check32 gate does not get it via windows.h */

#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// module wide constants
// ---------------------------------------------------------------------------
typedef enum rasterizer_constants {
    k_rasterizer_vertex_type_count = 20,                 // 0x3c dwords of vertex declarations
    k_rasterizer_maximum_skinning_nodes = 63,            // 0x0069c67e is initialised to 0x3f
    k_rasterizer_maximum_node_parts = 22,                // set_up_node_parts 0x526cf0 clamps to 0x16
    k_rasterizer_maximum_lights = 128,                   // light list add checks count < 0x80
    k_rasterizer_maximum_transparent_groups = 384,       // 0x180; pool 0xfc00 = 384 * 0xa8
    k_rasterizer_maximum_secondary_groups = 32,          // 0x20; pool 0x1500 = 32 * 0xa8
    k_rasterizer_scratch_memory_size = 0x18000,          // bump allocator 0x514560, test < 0x18001
    k_rasterizer_dynamic_vertex_slots = 0x400,           // both slot tables, test < 0x3ff
    k_rasterizer_dynamic_index_budget = 0x8000,          // 0x51bd60 indices per frame
    k_rasterizer_dynamic_index_buffer_size = 0x30000,    // CreateIndexBuffer in 0x51bb90
    k_rasterizer_draw_chunk_size = 10000,                // every chunked draw loop
    k_rasterizer_vertex_buffer_slots = 0x100,            // 0x5305f0 table capacity
    k_rasterizer_pixel_shader_effects = 122,             // 0x7a chunks in shaders\fx.bin
    k_rasterizer_vertex_shaders = 64,                    // 0x40 chunks in shaders\vsh.bin
    k_rasterizer_render_targets = 9,                     // index < 9 in 0x52ccc0/0x52cdd0
    k_rasterizer_screen_effect_techniques = 11,          // 0x52d740
    k_rasterizer_screen_flash_techniques = 6,            // 0x52ec40
    k_lens_flare_maximum_instances = 0x400,              // decal_add_to_active_list 0x5138a0
    k_lens_flare_object_visibility_slots = 0x380,        // 0x8c0 dwords / 10 bytes
    k_lens_flare_marker_visibility_size = 0x10008,       // 0x4002 dwords
    k_lens_flare_batch_slots = 5,                        // (0x7d7038 - 0x75efc0) / 0x18018
    k_lens_flare_batch_vertices = 0xc00,                 // 0x18000 / 0x20, flushed when full
    k_lens_flare_occlusion_queries = 0x400,              // 0x536f70
    k_font_glyph_cache_slots = 0x200,                    // 0x514cb0 loop count, ring mask 0x1ff
    k_font_atlas_size = 0x200,                           // 512 by 512 BitmapData in 0x514820
    k_gamma_ramp_entries = 0x100
} rasterizer_constants;

// ---------------------------------------------------------------------------
// rasterizer_vertex_type  (index into the vertex declaration table 0x006e1a90, the vertex size
// table 0x0065de00 and the per type dynamic cache 0x006d98e8)
// The first six values are VertexType from types/tags.h. The remaining names follow the
// OpenSauce list (hint only); the element strides are read straight out of .rdata at 0x0065de00:
// 38 20 14 08 44 20 18 24 18 10 10 14 20 08 20 20 24 1c 20 28.
// ---------------------------------------------------------------------------
typedef enum rasterizer_vertex_type {
    _rasterizer_vertex_type_environment_uncompressed = 0,           // 0x38
    _rasterizer_vertex_type_environment_compressed = 1,             // 0x20
    _rasterizer_vertex_type_environment_lightmap_uncompressed = 2,  // 0x14
    _rasterizer_vertex_type_environment_lightmap_compressed = 3,    // 0x08
    _rasterizer_vertex_type_model_uncompressed = 4,                 // 0x44; 0x800 dynamic
    _rasterizer_vertex_type_model_compressed = 5,                   // 0x20
    _rasterizer_vertex_type_dynamic_unlit = 6,                      // 0x18; 0x2000 dynamic
    _rasterizer_vertex_type_dynamic_lit = 7,                        // 0x24; 2 dynamic
    _rasterizer_vertex_type_dynamic_screen = 8,                     // 0x18; 0x4000 dynamic
    _rasterizer_vertex_type_debug = 9,                              // 0x10
    _rasterizer_vertex_type_decal = 10,                             // 0x10
    _rasterizer_vertex_type_detail_object = 11,                     // 0x14
    _rasterizer_vertex_type_environment_uncompressed_ff = 12,       // 0x20
    _rasterizer_vertex_type_environment_lightmap_uncompressed_ff = 13, // 0x08
    _rasterizer_vertex_type_model_uncompressed_ff = 14,             // 0x20
    _rasterizer_vertex_type_model_processed = 15,                   // 0x20; 0x2000 dynamic
    _rasterizer_vertex_type_unlit_zsprite = 16,                     // 0x24
    _rasterizer_vertex_type_screen_transformed_lit = 17,            // 0x1c
    _rasterizer_vertex_type_screen_transformed_lit_specular = 18,   // 0x20; the lens flare
                                                                    //      sprite vertex below
    _rasterizer_vertex_type_environment_single_stream_ff = 19       // 0x28
} rasterizer_vertex_type;

// ---------------------------------------------------------------------------
// rasterizer_vertex_buffer  (runtime view of a tag vertex buffer)
// rasterizer_vertex_buffer_create 0x524980 writes every field and zeroes all ten words on
// failure. ScenarioStructureBSPMaterial carries two back to back at +0xb0 and +0xc4 (the
// lightmap stream is the second, which is why rasterizer_transparent_geometry_group_draw_vertices
// 0x533660 passes group.vertex_buffer + 0x14 as the second stream), and ModelGeometryPart one at
// +0x54. The draw helpers 0x51c1c0/0x51c310/0x51c790 read type, count and hardware_buffer.
// ---------------------------------------------------------------------------
typedef struct rasterizer_vertex_buffer {
    int16_t type;                   // 0x00 rasterizer_vertex_type
    int16_t unknown_02;             // 0x02 alignment, never written
    int32_t count;                  // 0x04 vertex count
    int16_t unknown_08;             // 0x08 zeroed by the constructor
    int16_t unknown_0a;             // 0x0a zeroed by the constructor
    void *data;                     // 0x0c source vertices (tag data)
    void *hardware_buffer;          // 0x10 IDirect3DVertexBuffer9, from the 0x530570 wrapper
} rasterizer_vertex_buffer;         // size 0x14

// ---------------------------------------------------------------------------
// rasterizer_index_buffer  (runtime view of a tag triangle buffer)
// rasterizer_index_buffer_create 0x525030: type in DX, count in EAX; byte size is count*6 for
// type 0 (triangle list) and count*2+4 for type 1 (strip). The raw code stores the source
// pointer at +8 and the created IDirect3DIndexBuffer9 at +0xc (mov [ebp+0xc],eax; mov
// [ebp+8],edx at 0x525122); 0x51c5f0 passes +0xc to SetIndices.
// ---------------------------------------------------------------------------
typedef struct rasterizer_index_buffer {
    int16_t type;                   // 0x00 TriangleBufferType
    int16_t unknown_02;             // 0x02 alignment
    int32_t count;                  // 0x04 primitive count
    const void *data;               // 0x08 source indices (tag data)
    void *hardware_buffer;          // 0x0c IDirect3DIndexBuffer9
} rasterizer_index_buffer;          // size 0x10

// ---------------------------------------------------------------------------
// rasterizer_vertex_declaration  (element of the table at 0x006e1a90)
// rasterizer_dx9_vertex_declarations_create 0x5301b0 zeroes 0x3c dwords, creates 19 entries
// with CreateVertexDeclaration (device +0x158) into +0x00 of each, and sets the usage words
// (0x18, 0x218 or 8 depending on the pixel shader version). Every draw helper ORs usage & 0x10
// (D3DUSAGE_SOFTWAREPROCESSING) into SetSoftwareVertexProcessing, and 0x530570 passes fvf and
// usage straight to CreateVertexBuffer.
// ---------------------------------------------------------------------------
typedef struct rasterizer_vertex_declaration {
    void *declaration;              // 0x00 IDirect3DVertexDeclaration9
    uint32_t fvf;                   // 0x04 always 0 in this build
    uint32_t usage;                 // 0x08 D3DUSAGE bits for buffers of this type
} rasterizer_vertex_declaration;    // size 0x0c

// ---------------------------------------------------------------------------
// rasterizer_vertex_buffer_slot  (element of the table at 0x007bf060, 0x100 entries)
// FUN_005305f0 (Ghidra: vertex_shader_cache_get_or_create, really a vertex buffer allocator)
// creates the buffer through 0x530570 and stores eax/ebx/edi/esi into +0/+4/+8/+0xc; it returns
// the slot index plus one, so a handle h addresses the buffer as *(0x007bf04c + h*0x14).
// FUN_00530690 recreates every buffer whose length is set but whose +0x10 byte is clear, which
// is the device reset path for default-pool buffers.
// ---------------------------------------------------------------------------
typedef struct rasterizer_vertex_buffer_slot {
    void *hardware_buffer;          // 0x00 IDirect3DVertexBuffer9, NULL when free
    int32_t vertex_type;            // 0x04 rasterizer_vertex_type (EAX of 0x530570)
    uint32_t length;                // 0x08 bytes
    uint32_t fvf;                   // 0x0c
    uint8_t managed;                // 0x10 UNSURE: nonzero entries are skipped on recreate
    uint8_t unknown_11[3];          // 0x11 never read
} rasterizer_vertex_buffer_slot;    // size 0x14

// ---------------------------------------------------------------------------
// rasterizer_dynamic_vertex_cache  (element of the per vertex type table at 0x006d98e8)
// rasterizer_decal_index_buffer_initialize 0x51bb90 sets capacity and buffer_handle for all 20
// types (0x800 for type 4, 0x2000 for 6 and 15, 2 for 7, 0x4000 for 8, 0 otherwise);
// rasterizer_begin_frame 0x5175c0 zeroes used; FUN_0051bdd0 bumps it.
// ---------------------------------------------------------------------------
typedef struct rasterizer_dynamic_vertex_cache {
    int32_t used;                   // 0x00 vertices handed out this frame
    int32_t capacity;               // 0x04 vertices
    int32_t buffer_handle;          // 0x08 1-based rasterizer_vertex_buffer_slot handle, 0 none
} rasterizer_dynamic_vertex_cache;  // size 0x0c

// ---------------------------------------------------------------------------
// rasterizer_dynamic_vertex_slot  (element of the table at 0x006d99d8, count at 0x006dd9d8)
// FUN_0051bdd0 fills type/first/count, FUN_0051be40 locks the range and stores the pointer.
// transparent_geometry_group.dynamic_vertex_slot indexes this table.
// ---------------------------------------------------------------------------
typedef struct rasterizer_dynamic_vertex_slot {
    int16_t vertex_type;            // 0x00 rasterizer_vertex_type
    int16_t unknown_02;             // 0x02 alignment
    int32_t first_vertex;           // 0x04
    int32_t vertex_count;           // 0x08
    void *locked_vertices;          // 0x0c Lock result, NULL on failure
} rasterizer_dynamic_vertex_slot;   // size 0x10

// ---------------------------------------------------------------------------
// rasterizer_dynamic_index_slot  (element of the table at 0x006dd9e0, count at 0x006e09e0)
// FUN_0051bd60 fills first/count out of the shared 0x30000 byte index buffer 0x006e09e8;
// 0x51c1c0 hands the element address to IDirect3DIndexBuffer9::GetDesc (+0x34) as scratch.
// ---------------------------------------------------------------------------
typedef struct rasterizer_dynamic_index_slot {
    int32_t first_index;            // 0x00
    int32_t index_count;            // 0x04
    void *locked_indices;           // 0x08 out pointer of the index buffer Lock in
                                    //    rasterizer_dynamic_index_slot_lock, which returns it; the sibling
                                    //    rasterizer_dynamic_vertex_slot keeps locked_vertices
} rasterizer_dynamic_index_slot;    // size 0x0c

// ---------------------------------------------------------------------------
// rasterizer_effect_slot  (element of the table at 0x0069d410, 122 entries)
// rasterizer_dx9_pixel_shader_effect_load 0x52f980 creates the effect into +0,
// rasterizer_dx9_shaders_init_effect 0x52f780 resolves the Texture0..3 handles, and
// rasterizer_dx9_shaders_initialize 0x52fab0 GlobalAllocs the per effect constant handle arrays
// that 0x5202f0/0x520e50/0x531ed0 read through +0x18.
// ---------------------------------------------------------------------------
typedef struct rasterizer_effect_slot {
    void *effect;                   // 0x00 ID3DXEffect
    int32_t vertex_shader_index;    // 0x04 index into rasterizer_vertex_shaders (0x0069e350)
    void *texture_handles[4];       // 0x08 D3DXHANDLE Texture0..Texture3
    void **constant_handles;        // 0x18 GlobalAlloc array of named constant handles
    uint32_t unknown_1c;            // 0x1c no reader found
} rasterizer_effect_slot;           // size 0x20

// ---------------------------------------------------------------------------
// rasterizer_vertex_shader  (element of the table at 0x0069e350, 64 entries up to 0x0069e550)
// rasterizer_dx9_vertex_shaders_load_all 0x5306e0 skips every entry whose +4 is zero and
// creates the rest from shaders\vsh.bin with CreateVertexShader (device +0x16c).
// ---------------------------------------------------------------------------
typedef struct rasterizer_vertex_shader {
    void *shader;                   // 0x00 IDirect3DVertexShader9
    int32_t enabled;                // 0x04 static initialised data; 0 skips the chunk
} rasterizer_vertex_shader;         // size 0x08

// ---------------------------------------------------------------------------
// rasterizer_render_target  (element of the table at 0x0069d358, 9 entries)
// FUN_0052ca20 fills entry 0 from the back buffer desc, entry 2 at half size, and creates a
// texture plus its level 0 surface for every entry whose surface is NULL; FUN_0052cc50
// releases surface and texture of all nine; FUN_0052ccc0 binds surface, FUN_0052cdd0 and
// FUN_0052ce10 bind texture. Indices 6 and 7 are the projected light shadow buffers.
// ---------------------------------------------------------------------------
typedef struct rasterizer_render_target {
    uint32_t width;                 // 0x00
    uint32_t height;                // 0x04
    uint32_t format;                // 0x08 D3DFORMAT; 0x15 A8R8G8B8
    void *surface;                  // 0x0c IDirect3DSurface9
    void *texture;                  // 0x10 IDirect3DTexture9
} rasterizer_render_target;         // size 0x14

// ---------------------------------------------------------------------------
// Direct3D 9 SDK records, pinned by the accesses listed
// ---------------------------------------------------------------------------

// D3DPRESENT_PARAMETERS. rasterizer_build_present_parameters 0x515fc0 writes all 14 dwords
// (format 0x16 X8R8G8B8, depth format 0x4b D24S8, hwnd from 0x007461c4);
// display_mode_get_current 0x515ca0 reads width/height/refresh/interval.
typedef struct d3d_present_parameters {
    uint32_t back_buffer_width;     // 0x00 (used)
    uint32_t back_buffer_height;    // 0x04 (used)
    uint32_t back_buffer_format;    // 0x08 (used)
    uint32_t back_buffer_count;     // 0x0c (used)
    uint32_t multisample_type;      // 0x10
    uint32_t multisample_quality;   // 0x14
    uint32_t swap_effect;           // 0x18 (used) 1 or 3
    uint32_t device_window;         // 0x1c void* (used)
    int32_t windowed;               // 0x20 (used)
    int32_t enable_auto_depth_stencil; // 0x24 (used) 1
    uint32_t auto_depth_stencil_format; // 0x28 (used) 0x4b
    uint32_t flags;                 // 0x2c (used)
    uint32_t fullscreen_refresh_rate; // 0x30 (used)
    uint32_t presentation_interval; // 0x34 (used) 1 means vsync, 0x80000000 immediate
} d3d_present_parameters;           // size 0x38

// D3DLIGHT9 as handed to IDirect3DDevice9::SetLight by rasterizer_light_set 0x526760.
typedef struct d3d_light9 {
    uint32_t type;                  // 0x00 D3DLIGHTTYPE: 1 point, 2 spot
    float diffuse[4];               // 0x04 r, g, b, a
    float specular[4];              // 0x14
    float ambient[4];               // 0x24
    real_point3d position;          // 0x34
    real_vector3d direction;        // 0x40
    float range;                    // 0x4c
    float falloff;                  // 0x50
    float attenuation0;             // 0x54
    float attenuation1;             // 0x58
    float attenuation2;             // 0x5c
    float theta;                    // 0x60
    float phi;                      // 0x64
} d3d_light9;                       // size 0x68

// D3DCAPS9 as filled by IDirect3D9::GetDeviceCaps (+0x38, called in rasterizer_initialize_direct3d
// 0x5169c0 with the literal address 0x007c10c0). Fields read by this module are marked.
typedef struct d3d_caps9 {
    uint32_t device_type;           // 0x00
    uint32_t adapter_ordinal;       // 0x04
    uint32_t caps;                  // 0x08
    uint32_t caps2;                 // 0x0c (used) gamma paths
    uint32_t caps3;                 // 0x10
    uint32_t presentation_intervals; // 0x14
    uint32_t cursor_caps;           // 0x18
    uint32_t dev_caps;              // 0x1c (used, high word at 0x007c10de)
    uint32_t primitive_misc_caps;   // 0x20
    uint32_t raster_caps;           // 0x24 (used) 0x007c10e4: 0x04000000 DEPTHBIAS and
                                    //      0x02000000 SLOPESCALEDEPTHBIAS gate render states
                                    //      0xc3/0xaf in 0x5194e0..0x5195d0
    uint32_t z_cmp_caps;            // 0x28
    uint32_t src_blend_caps;        // 0x2c (used)
    uint32_t dest_blend_caps;       // 0x30
    uint32_t alpha_cmp_caps;        // 0x34
    uint32_t shade_caps;            // 0x38
    uint32_t texture_caps;          // 0x3c (used) 0x007c10fc, bitmap lock and upload paths
    uint32_t texture_filter_caps;   // 0x40 (used)
    uint32_t cube_texture_filter_caps; // 0x44
    uint32_t volume_texture_filter_caps; // 0x48
    uint32_t texture_address_caps;  // 0x4c (used) 0x007c110c bit 3 BORDER
    uint32_t volume_texture_address_caps; // 0x50
    uint32_t line_caps;             // 0x54
    uint32_t max_texture_width;     // 0x58
    uint32_t max_texture_height;    // 0x5c
    uint32_t max_volume_extent;     // 0x60
    uint32_t max_texture_repeat;    // 0x64
    uint32_t max_texture_aspect_ratio; // 0x68
    uint32_t max_anisotropy;        // 0x6c (used)
    float max_vertex_w;             // 0x70
    float guard_band_left;          // 0x74
    float guard_band_top;           // 0x78
    float guard_band_right;         // 0x7c
    float guard_band_bottom;        // 0x80
    float extents_adjust;           // 0x84
    uint32_t stencil_caps;          // 0x88
    uint32_t fvf_caps;              // 0x8c
    uint32_t texture_op_caps;       // 0x90
    uint32_t max_texture_blend_stages; // 0x94
    uint32_t max_simultaneous_textures; // 0x98 (used) 0x007c1158, shader_environment path pick
    uint32_t vertex_processing_caps; // 0x9c
    uint32_t max_active_lights;     // 0xa0 (used) 0x007c1160, rasterizer_light_disable_all
    uint32_t max_user_clip_planes;  // 0xa4
    uint32_t max_vertex_blend_matrices; // 0xa8
    uint32_t max_vertex_blend_matrix_index; // 0xac
    float max_point_size;           // 0xb0
    uint32_t max_primitive_count;   // 0xb4
    uint32_t max_vertex_index;      // 0xb8
    uint32_t max_streams;           // 0xbc (used) 0x007c117c; 0x51c310 binds a second stream
                                    //      only when it is above 1
    uint32_t max_stream_stride;     // 0xc0
    uint32_t vertex_shader_version; // 0xc4
    uint32_t max_vertex_shader_const; // 0xc8
    uint32_t pixel_shader_version;  // 0xcc (used) 0x007c118c, 74 comparisons in the module
    float pixel_shader_1x_max_value; // 0xd0
    uint32_t dev_caps2;             // 0xd4
    float max_npatch_tessellation_level; // 0xd8
    uint32_t reserved5;             // 0xdc
    uint32_t master_adapter_ordinal; // 0xe0
    uint32_t adapter_ordinal_in_group; // 0xe4
    uint32_t number_of_adapters_in_group; // 0xe8
    uint32_t decl_types;            // 0xec
    uint32_t num_simultaneous_rts;  // 0xf0
    uint32_t stretch_rect_filter_caps; // 0xf4
    uint32_t vs20_caps[4];          // 0xf8 D3DVSHADERCAPS2_0
    uint32_t ps20_caps[5];          // 0x108 D3DPSHADERCAPS2_0
    uint32_t vertex_texture_filter_caps; // 0x11c
    uint32_t max_vshader_instructions_executed; // 0x120
    uint32_t max_pshader_instructions_executed; // 0x124
    uint32_t max_vertex_shader30_instruction_slots; // 0x128
    uint32_t max_pixel_shader30_instruction_slots; // 0x12c
} d3d_caps9;                        // size 0x130

// D3DGAMMARAMP. chimera__registry_check_4 0x522520 captures the desktop ramp into 0x006e0b18
// (GetDeviceGammaRamp, or a linear ramp written into red and green halves 0x006e0b18/0x006e0d18),
// chimera__gamma 0x5227a0 builds the game ramp at 0x006e1118, and chimera__registry_check_3
// 0x5226c0 restores the captured one.
typedef struct d3d_gamma_ramp {
    uint16_t red[0x100];            // 0x000
    uint16_t green[0x100];          // 0x200
    uint16_t blue[0x100];           // 0x400
} d3d_gamma_ramp;                   // size 0x600

// D3DLOCKED_RECT, the out block of IDirect3DTexture9::LockRect (+0x4c) and
// IDirect3DCubeTexture9::LockRect (+0x50) in the bitmap upload paths 0x524100..0x5243c0 and of
// the texel sampler 0x524590.
typedef struct d3d_locked_rect {
    int32_t pitch;                  // 0x00
    uint8_t *bits;                  // 0x04 first byte of the locked rectangle
} d3d_locked_rect;                  // size 0x08

// D3DSURFACE_DESC, filled by IDirect3DSurface9::GetDesc (+0x30) in rasterizer_end_frame 0x517b90.
typedef struct d3d_surface_desc {
    uint32_t format;                // 0x00
    uint32_t type;                  // 0x04
    uint32_t usage;                 // 0x08
    uint32_t pool;                  // 0x0c
    uint32_t multisample_type;      // 0x10
    uint32_t multisample_quality;   // 0x14
    uint32_t width;                 // 0x18 (used)
    uint32_t height;                // 0x1c (used)
} d3d_surface_desc;                 // size 0x20

// D3DVIEWPORT9, handed to SetViewport (+0xbc) by rasterizer_end_frame 0x517b90 and
// rasterizer_initialize_direct3d 0x5169c0.
typedef struct d3d_viewport {
    uint32_t x;                     // 0x00
    uint32_t y;                     // 0x04
    uint32_t width;                 // 0x08
    uint32_t height;                // 0x0c
    float min_z;                    // 0x10
    float max_z;                    // 0x14
} d3d_viewport;                     // size 0x18

// D3DXMACRO, the NULL-terminated preprocessor define list rasterizer_dx9_shaders_initialize
// 0x52fab0 builds at 0x007c0460 ({"PS_2_0_TARGET", "ps_2_a" or "ps_2_0"}, {0, 0}).
typedef struct d3dx_macro {
    const char *name;               // 0x00
    const char *definition;         // 0x04
} d3dx_macro;                       // size 0x08

// D3DVERTEXELEMENT9, the .rdata element arrays (0x0065e168..0x0065e3c0) that
// rasterizer_dx9_vertex_declarations_create 0x5301b0 passes to CreateVertexDeclaration (+0x158)
// and D3DXFVFFromDeclarator; each array ends with D3DDECL_END {0xff, 0, 17, 0, 0, 0}.
typedef struct d3d_vertex_element9 {
    uint16_t stream;                // 0x00
    uint16_t offset;                // 0x02
    uint8_t type;                   // 0x04 D3DDECLTYPE
    uint8_t method;                 // 0x05 D3DDECLMETHOD
    uint8_t usage;                  // 0x06 D3DDECLUSAGE
    uint8_t usage_index;            // 0x07
} d3d_vertex_element9;              // size 0x08

// d3d_display_mode (D3DDISPLAYMODE, filled by IDirect3D9::GetAdapterDisplayMode (+0x20) in
// 0x5169c0 and copied verbatim into rasterizer_desktop_display_mode 0x007c11f0) and win32_rect
// (GetWindowRect in 0x5169c0, 0x515930 and 0x515b20) are defined in types/interface.h with the
// same layout and are not redefined here; files that use them include interface.h.

// Win32 WNDCLASSEXA, registered by rasterizer_create_game_window 0x515930.
typedef struct win32_wndclassexa {
    uint32_t size;                  // 0x00 0x30
    uint32_t style;                 // 0x04
    uint32_t window_procedure;      // 0x08 void* WNDPROC
    int32_t class_extra;            // 0x0c
    int32_t window_extra;           // 0x10
    uint32_t instance;              // 0x14 void* HINSTANCE
    uint32_t icon;                  // 0x18 void* HICON
    uint32_t cursor;                // 0x1c void* HCURSOR
    uint32_t background_brush;      // 0x20 void* HBRUSH
    uint32_t menu_name;             // 0x24 const char*
    uint32_t class_name;            // 0x28 const char*
    uint32_t small_icon;            // 0x2c void* HICON
} win32_wndclassexa;                // size 0x30

// FVF 0x144 (XYZRHW | DIFFUSE | TEX1) screen quad vertex, stride 0x1c, written four at a time
// into rasterizer_render_target_vertex_buffer by rasterizer_end_frame 0x517b90.
typedef struct rasterizer_screen_vertex {
    float x;                        // 0x00
    float y;                        // 0x04
    float z;                        // 0x08
    float rhw;                      // 0x0c
    uint32_t diffuse;               // 0x10
    float u;                        // 0x14
    float v;                        // 0x18
} rasterizer_screen_vertex;         // size 0x1c

// _rasterizer_vertex_type_dynamic_screen (8) vertex, stride 0x18: position, packed ARGB and one
// texture coordinate, drawn with DrawPrimitiveUP (+0x14c) as four vertex triangle fans by the
// motion sensor draws 0x52bad0 / 0x52bc40 (the vertex shader maps x, y through c13..c16).
typedef struct rasterizer_dynamic_screen_vertex {
    float x;                        // 0x00
    float y;                        // 0x04
    float z;                        // 0x08
    uint32_t color;                 // 0x0c D3DCOLOR ARGB
    float u;                        // 0x10
    float v;                        // 0x14
} rasterizer_dynamic_screen_vertex; // size 0x18

// The EDI block of chimera__rasterizer_set_model_skinning 0x518b40: a node matrix array and
// its count, the same pair rasterizer_model_draw_context keeps at +0x08/+0x0c and
// transparent_geometry_group at +0x60/+0x64.
typedef struct rasterizer_node_matrices {
    uint32_t matrices;              // 0x00 real_matrix4x3*
    int16_t node_count;             // 0x04
    int16_t unknown_06;             // 0x06 not read
} rasterizer_node_matrices;         // size 0x08 (only the first 6 bytes are read)

// Draw contexts whose layout is not recovered yet (TYPES-GAP). They are passed by pointer and
// read through raw byte offsets in the files named; each is declared as a byte so that
// pointer arithmetic on them stays in bytes.
typedef uint8_t rasterizer_gamma_settings;               // 0x522890, brightness at +0x100

// ---------------------------------------------------------------------------
// rasterizer_display_mode  (the EDI out block of display_mode_get_current 0x515ca0, compared
// field by field by FUN_00515d10)
// ---------------------------------------------------------------------------
typedef struct rasterizer_display_mode {
    int32_t width;                  // 0x00 present parameters +0x00
    int32_t height;                 // 0x04 present parameters +0x04
    int32_t refresh_rate;           // 0x08 60 when os_platform < 3 or none enumerated
    uint8_t vsync;                  // 0x0c presentation_interval == 1
    uint8_t unknown_0d[3];          // 0x0d never written
} rasterizer_display_mode;          // size 0x10

// ---------------------------------------------------------------------------
// render_camera  (render window parameters +0x08, 0x15 dwords)
// FUN_0050bdc0 copies exactly 0x15 dwords into it. Pinned: position (0x007c1228, 26 uses),
// forward (0x007c1234, the depth axis of every transparent sort key), viewport_bounds
// (0x007c1254..0x007c125b, the Rectangle2D whose right-left and bottom-top size the screen
// effect and cinematic quads) and z_far (0x007c1268, the default atmospheric fog distance in
// 0x5176d0).
// ---------------------------------------------------------------------------
typedef struct render_camera {
    real_point3d position;          // 0x00 (used)
    real_vector3d forward;          // 0x0c (used)
    real_vector3d up;               // 0x18
    uint8_t mirrored;               // 0x24
    uint8_t unknown_25[3];          // 0x25
    float vertical_field_of_view;   // 0x28
    Rectangle2D viewport_bounds;    // 0x2c (used) top, left, bottom, right
    Rectangle2D window_bounds;      // 0x34
    float z_near;                   // 0x3c
    float z_far;                    // 0x40 (used)
    real_plane3d mirror_plane;      // 0x44
} render_camera;                    // size 0x54

// ---------------------------------------------------------------------------
// render_frustum  (render window parameters +0x5c)
// Pinned: world_to_view (0x007c128c, rasterizer_light_shadow_project_point 0x525130 and
// rasterizer_lens_flare_project_to_screen 0x536d80 transform through it), view_to_world
// forward/left (0x007c12c4/0x007c12d0, the lens flare rotation basis in 0x513540) and
// projection (0x007c13c0..0x007c13ff, the perspective divide in the same two functions).
// ---------------------------------------------------------------------------
typedef struct render_frustum {
    float frustum_bounds[4];        // 0x000 real_rectangle2d; structures.h keeps a separate
                                    //       screen bounds pair in front of its copy
    real_matrix4x3 world_to_view;   // 0x010 (used)
    real_matrix4x3 view_to_world;   // 0x044 (used)
    real_plane3d world_planes[6];   // 0x078
    float z_near;                   // 0x0d8
    float z_far;                    // 0x0dc
    real_point3d world_vertices[5]; // 0x0e0
    real_point3d world_midpoint;    // 0x11c
    real_rectangle3d world_bounds;  // 0x128
    uint8_t projection_valid;       // 0x140
    uint8_t unknown_141[3];         // 0x141
    float projection[4][4];         // 0x144 (used)
    real_vector2d projection_world_to_screen; // 0x184
} render_frustum;                   // size 0x18c

// ---------------------------------------------------------------------------
// render_fog  (render window parameters +0x1e8; also the 0x14 dword EDX block of 0x5176d0)
// Every field is read by 0x5176d0, FUN_00526f50 or rasterizer_fog_set_render_states 0x51def0.
// ---------------------------------------------------------------------------
typedef enum render_fog_flags {
    _render_fog_no_planar_bit = 0x04        // flags bit 2: 0x5176d0 and 0x526f50 skip planar fog
} render_fog_flags;

typedef struct render_fog {
    uint16_t flags;                 // 0x00 render_fog_flags
    uint16_t unknown_02;            // 0x02
    ColorRGB atmospheric_color;     // 0x04 color_rgb_float_to_int in rasterizer_begin_frame
    float atmospheric_maximum_density; // 0x10 forced to 1.0 when not positive
    float atmospheric_minimum_distance; // 0x14 defaults to camera z_far
    float atmospheric_maximum_distance; // 0x18 defaults to twice camera z_far
    int16_t planar_mode;            // 0x1c 0 none, 2 builds the plane through 0x44d9e0 (0x5176d0
                                    //      reads and clears it as a word)
    int16_t unknown_1e;             // 0x1e
    real_plane3d plane;             // 0x20 planar fog plane; defaults to the camera plane
    ColorRGB planar_color;          // 0x30
    float planar_maximum_density;   // 0x3c
    float planar_maximum_distance;  // 0x40
    float planar_maximum_depth;     // 0x44
    uint32_t unknown_48;            // 0x48 no reader in this module
    float sky_fog_screen_blend;     // 0x4c (R41) scenario_sky_fog_state_update 0x53e8c0 stores
                                    //      the sky fog screen blend (source +0x28) clamped to
                                    //      [0,1]: 0 at 0x53eb20, 1.0 at 0x53eb3f, the value at
                                    //      0x53eb51. No reader in this module
} render_fog;                       // size 0x50

// ---------------------------------------------------------------------------
// render_screen_flash  (render window parameters +0x238)
// rasterizer_screen_flash_render 0x52ed00: type selects the Flash technique (1 Lighten ...),
// the color channels are each scaled by intensity.
// ---------------------------------------------------------------------------
typedef struct render_screen_flash {
    int16_t type;                   // 0x00 0 none
    int16_t unknown_02;             // 0x02
    float intensity;                // 0x04
    ColorARGB color;                // 0x08
} render_screen_flash;              // size 0x18

// ---------------------------------------------------------------------------
// rasterizer_window_parameters  (0x007c1220, 0x258 bytes)
// rasterizer_begin_frame 0x5175c0 copies the caller block in one 0x96 dword rep movsd, then
// hands +0x1e8 to 0x5176d0 and selects the render target from type (1 or 2) and +5.
// ---------------------------------------------------------------------------
typedef struct rasterizer_window_parameters {
    int16_t type;                   // 0x000 (used) 1 is the main 3D pass; most draw paths
                                    //       test type == 1 (0x007c1220)
    int16_t window_index;           // 0x002 (used) 0x007c1222; lens flare instances carry it
                                    //       and only draw in the matching window; -1 for the
                                    //       loading screen path at 0x50bdc0
    uint8_t has_mirror;             // 0x004 0x04 render_window stores its has_mirror argument here; the dynamic
                                    //    mirror draw, environment self illumination technique and glass shader test
                                    //    it together with type == 1
                                    //       technique path 0x520b90/0x520e50
    uint8_t clear_target;           // 0x005 (used) 0 asks 0x52ccc0 to clear the new target
    uint16_t unknown_06;            // 0x006
    render_camera camera;           // 0x008
    render_frustum frustum;         // 0x05c
    render_fog fog;                 // 0x1e8
    render_screen_flash screen_flash; // 0x238
    uint32_t unknown_250;           // 0x250 no reader in this module
    uint32_t unknown_254;           // 0x254 no reader in this module
} rasterizer_window_parameters;     // size 0x258

// ---------------------------------------------------------------------------
// rasterizer_frame_time  (0x007c1200, the ECX block of chimera__cinematic_screen_effect 0x517470)
// Only the first field is read back, always as a QWORD (fld/fadd/fmul/fdivr QWORD PTR 0x007c1200,
// 30 reads): it is the double precision animation time fed to periodic_function_evaluate and
// 0x540060 by the light volume, light halo, environment and self-illumination paths.
// ---------------------------------------------------------------------------
typedef struct rasterizer_frame_time {
    double time;                    // 0x00 (used, as a double)
    uint32_t unknown_08;            // 0x08 copied, never read here
    uint32_t unknown_0c;            // 0x0c copied, never read here
} rasterizer_frame_time;            // size 0x10

// ---------------------------------------------------------------------------
// rasterizer_light  (element of the list at 0x007c1484, count 0x007c1480, 128 entries up to
// 0x007c3084)
// Built on the stack by object_lights_update_all 0x4f0cf0 (objects/lights code) and appended
// with a 0xe dword copy followed by rasterizer_light_set 0x526760. Readers: FUN_00518c10 (fog
// plane cache slot), FUN_005215b0/0x521750 (projected light constants), rasterizer_light_set
// (D3DLIGHT9), 0x51da20 (light volume constants).
// ---------------------------------------------------------------------------
typedef struct rasterizer_light {
    uint32_t definition;            // 0x00 void* Light tag data; +0x70 == -1 means no cube map,
                                    //      flags bit 0x10 first_person_flashlight
    real_point3d position;          // 0x04
    real_vector3d forward;          // 0x10 the spot direction
    real_vector3d up;               // 0x1c second axis of the projected light basis
    ColorRGB color;                 // 0x28 NTSC luminance taken in 0x521750
    float radius;                   // 0x34 1/(r*r) in the fog plane cache, 2.5*r as the D3D
                                    //      range, r * Light.specular_radius_multiplier
} rasterizer_light;                 // size 0x38

// ---------------------------------------------------------------------------
// rasterizer_point_light_constants  (the 0x30 byte slot FUN_00518c10 writes, two per object)
// FUN_00518ce0 builds two of these on its stack and uploads 0xb vec4s at vertex shader constant
// register 0xf. A light index of -1 zeroes the slot.
// ---------------------------------------------------------------------------
typedef struct rasterizer_point_light_constants {
    real_point3d position;          // 0x00 rasterizer_light.position
    float inverse_radius_squared;   // 0x0c 1 / (radius * radius)
    real_vector3d forward;          // 0x10 rasterizer_light.forward
    float falloff_scale;            // 0x1c 1 / (cos_falloff - cos_cutoff), 0 for omni lights
    ColorRGB color;                 // 0x20 rasterizer_light.color
    float falloff_offset;           // 0x2c -scale * cos_cutoff, 1.0 for omni lights
} rasterizer_point_light_constants; // size 0x30

// ---------------------------------------------------------------------------
// rasterizer_projected_light_constants  (0x006e0a10, five vec4)
// FUN_005215b0 (cube map path) or FUN_00521750 (plain falloff) fill it for one light, and the
// light draw passes 0x521900/0x521f90 upload it.
// ---------------------------------------------------------------------------
typedef struct rasterizer_projected_light_constants {
    real_point3d position;          // 0x00
    float inverse_radius;           // 0x0c 0.5 / radius
    float basis[3][4];              // 0x10 negated forward, cross product and up axes, each
                                    //      followed by 1.0
    real_vector3d cone_axis;        // 0x40 forward scaled by 1 / (r - r/2)
    float cone_offset;              // 0x4c
} rasterizer_projected_light_constants; // size 0x50

// ---------------------------------------------------------------------------
// render_lighting  (the 0x74 byte lighting record an object draw carries)
// Pinned by the 0x1d dword copy out of transparent_geometry_group.lighting in FUN_00519f70,
// by FUN_00518ce0 / render_objects_transparent 0x518d40, which read ambient_color, the
// distant light count and pairs, and the point light count, and by the fact that
// rasterizer_model_draw_context places it at +0x10 with the next field at +0x84. The trailing
// three fields are pinned too (R42): render_objects_lighting_update 0x50f270 steps all three
// (0x50f5c0 for the tint), 0x50eba0 takes the luminance of shadow_color, and 0x50f830 builds the
// shadow basis from shadow_vector (lea ebx,[eax+0x5c] at 0x50f876).
// ---------------------------------------------------------------------------
typedef struct render_distant_light {
    ColorRGB color;                 // 0x00
    real_vector3d direction;        // 0x0c
} render_distant_light;             // size 0x18

typedef struct render_lighting {
    ColorRGB ambient_color;         // 0x00 (used)
    int16_t distant_light_count;    // 0x0c (used) at most 2
    int16_t unknown_0e;             // 0x0e
    render_distant_light distant_lights[2]; // 0x10 (used)
    int16_t point_light_count;      // 0x40 (used) only the first two are uploaded (0x518ce0)
    int16_t unknown_42;             // 0x42
    int32_t point_light_indices[2]; // 0x44 rasterizer_light index, passed in EAX to 0x518c10
    ColorARGB reflection_tint;      // 0x4c stepped by 0x50f270 (0x50f5c0)
    real_vector3d shadow_vector;    // 0x5c 0x50f830 builds the shadow basis from it
    ColorRGB shadow_color;          // 0x68 0x50eba0 takes its luminance
} render_lighting;                  // size 0x74

// ---------------------------------------------------------------------------
// rasterizer_skinning_matrix  (element of the palette at 0x007c04e0, 63 entries up to 0x007c10b0)
// chimera__rasterizer_set_model_skinning 0x518b40 transposes each real_matrix4x3 (scaled by
// its scale) into three rows with the translation in the fourth column and uploads the palette
// at vertex shader constant register 0x1d; chimera__rasterizer_set_up_node_parts 0x526cf0
// gathers rows out of it by node index byte.
// ---------------------------------------------------------------------------
typedef struct rasterizer_skinning_matrix {
    float rows[3][4];               // 0x00
} rasterizer_skinning_matrix;       // size 0x30

// ---------------------------------------------------------------------------
// rasterizer_geometry_group_parameters  (0x28 bytes shared by the model draw context at +0x8c and
// by every transparent_geometry_group at +0x14)
// FUN_0052b180 copies the ten dwords verbatim from context to group.
// ---------------------------------------------------------------------------
typedef struct rasterizer_geometry_group_parameters {
    int16_t mode;                   // 0x00 0 plain, 1 blended environment pass (0x533850,
                                    //      0x522c60), 2 batched pass drawn once per key
    int16_t unknown_02;             // 0x02
    float blend_factor;             // 0x04 1 - x in 0x522c60 when mode == 1; FUN_00526f50
                                    //      tests it against 0 for mode 1
    float distortion_factor;        // 0x08 lerp factor into GlobalsRasterizerData camouflage
                                    //      values in FUN_00519f70
    int32_t sort_key;               // 0x0c copied into group.sort_key when mode != 0
    real_point3d position;          // 0x10 copied into group.position when mode != 0
    struct Shader *shader;          // 0x1c Shader tag data of the overlay pass (0x52b050
                                    //      tests shader_type 0xb and +0x2c)
    uint32_t change_colors;         // 0x20 ColorRGB*; with function_values the render_animation pair copied to
                                    //      group.lighting_extra by 0x52b050
    const float *function_values;   // 0x24 indexed [n - 1] by the overlay shader in 0x52b050
                                    //      and 0x533850
} rasterizer_geometry_group_parameters; // size 0x28

// ---------------------------------------------------------------------------
// rasterizer_model_draw_context  (0x0071d1f0 while a model is being drawn)
// Installed by FUN_00526f50 (render states before model geometry) and cleared by FUN_0052b530;
// read by FUN_0052b050, FUN_0052b180, rasterizer_model_draw_environment_shader_environment
// 0x52b340 and FUN_00519f70, which also builds one on its stack. The caller that fills it lives
// in the render module, so only the fields read here are known and the true size may be larger.
// ---------------------------------------------------------------------------
typedef enum rasterizer_model_draw_flags {
    _model_draw_flag_2_bit = 0x00000002,        // draw immediately through the static group
    _model_draw_flag_4_bit = 0x00000004,        // no planar fog
    _model_draw_flag_8_bit = 0x00000008,        // depth test disabled (copied to the group)
    _model_draw_flag_40_bit = 0x00000040,       // planar fog only below the plane
    _model_draw_frustum_z_bit = 0x00000080,     // sign bit of the low byte: frustum z override
    _model_draw_node_parts_bit = 0x00000100,    // skinned by node part list
    _model_draw_fixed_function_fog_bit = 0x00000200 // pre ps_1_1 fog uses the node matrix
} rasterizer_model_draw_flags;

typedef struct rasterizer_model_draw_context {
    uint32_t flags;                 // 0x00 rasterizer_model_draw_flags; bits 8..23 also feed
                                    //      set_model_skinning
    uint32_t object_index;          // 0x04 0x04 render_model stores object_index here; the model pixel shader seeds a
                                    //    per object pseudo random value from it and group_build copies it to the
                                    //    transparent group
    uint32_t node_matrices;         // 0x08 real_matrix4x3* set_model_skinning reads scale/forward/left/up/position
    int16_t node_count;             // 0x0c
    int16_t unknown_0e;             // 0x0e
    render_lighting lighting;       // 0x10 group.lighting points here
    uint32_t change_colors;         // 0x84 ColorRGB (*)[4]: render.h render_animation.change_colors
                                    //      (render_model 0x4d6fc0 arg3 -> [ebp-0x58], 0x4d716c;
                                    //      0x006b7f60 when NULL). R43
    uint32_t function_values;       // 0x88 float (*)[4]: render_animation.function_values (arg4
                                    //      -> [ebp-0x54], 0x4d717e; 0x006b7f08 when NULL). The
                                    //      pair +0x84/+0x88 IS a render_animation (render.h sorts
                                    //      after this header, so both stay uint32 here); group
                                    //      +0x74 points at it and FUN_00519f70 copies both dwords;
                                    //      0x53fe50 reads function_values through context+0x84
                                    //      +4 (0x528eaa)
    rasterizer_geometry_group_parameters group_parameters; // 0x8c
    real_point3d center;            // 0xb4 fog distance point in FUN_00526f50
    float bounding_radius;          // 0xc0 render_model arg7 ([ebp+0x20] -> [ebp-0x1c], 0x4d7156)
    float base_map_u_scale;         // 0xc4 model tag +0x30 (0x4d7188); copied into group +0x3c
    float base_map_v_scale;         // 0xc8 model tag +0x34 (0x4d719b); copied into group +0x40
} rasterizer_model_draw_context;    // partial: at least 0xcc

// ---------------------------------------------------------------------------
// transparent_geometry_group  (0xa8 byte records)
// Pools: 0x0071d14c (384 records, GlobalAlloc 0xfc00 in transparent_geometry_pool_initialize
// 0x5151c0), 0x0071d150 (32 records, 0x1500), sort indices 0x0071d15c (384 int16, 0x300), plus
// two static records at 0x006e0a70 and 0x006e1828 used for immediate draws (their +0x98 is set
// to -1 at 0x006e0b08 and 0x006e18c0). Constructors: rasterizer_transparent_geometry_group_new
// 0x522300, FUN_0052b180, FUN_0051c830, FUN_00536ff0 and
// rasterizer_model_draw_environment_shader_environment 0x52b340; all five set every field below
// except the ones marked. Readers: transparent_geometry_group_compare 0x5155b0,
// rasterizer_transparent_geometry_group_draw 0x533850, _draw_vertices 0x533660,
// rasterizer_geometry_part_draw 0x533730, FUN_00519f70, FUN_00515400 and the decal variants
// 0x522930..0x523d10.
// ---------------------------------------------------------------------------
typedef enum transparent_geometry_group_flags {
    _group_flag_0_bit = 0x00000001,             // set when a tint is supplied (0x522300)
    _group_immediate_bit = 0x00000002,          // draw now through the static record
    _group_flag_4_bit = 0x00000004,
    _group_flag_8_bit = 0x00000008,
    _group_flag_10_bit = 0x00000010,
    _group_flag_20_bit = 0x00000020,
    _group_sort_first_bit = 0x00000080,         // compare sorts these ahead; also frustum z
    _group_node_parts_bit = 0x00000100,
    _group_fixed_function_fog_bit = 0x00000200
} transparent_geometry_group_flags;

typedef struct transparent_geometry_group {
    uint32_t flags;                 // 0x00 transparent_geometry_group_flags (from the caller or
                                    //      the model draw context)
    uint32_t object_index;          // 0x04 0x04 copied from rasterizer_model_draw_context.object_index (render_model)
                                    //    by rasterizer_transparent_geometry_group_build, 0 for non model groups
    int32_t sort_key;               // 0x08 compare tiebreak; draw batches runs of equal keys
    struct Shader *shader;          // 0x0c Shader tag data; NULL means a callback group
    uint16_t shader_permutation;    // 0x10 passed as the bitmap index to set_texture
    uint16_t unknown_12;            // 0x12 never written
    rasterizer_geometry_group_parameters parameters; // 0x14
    float base_map_u_scale;         // 0x3c group_build 0x52b180 copies context->base_map_u_scale; chicago draws
                                    //    multiply u_scale by it; active camo copies it back to
                                    //    context.base_map_u_scale
                                    //      shader constant by 0x533850 and 0x519f70
    float base_map_v_scale;         // 0x40 group_build copies context->base_map_v_scale; chicago draws multiply
                                    //    v_scale; glass reflection uses both as bump scale constants
    int32_t dynamic_index_slot;     // 0x44 rasterizer_dynamic_index_slot index; a negative
                                    //      value is minus a primitive kind (3 or 4 are quads)
    union {
        struct rasterizer_index_buffer *index_buffer; // 0x48 static indices
        void (*callback)(int32_t argument, int32_t count); // 0x48 callback group (shader NULL): the procedure
    };
    int32_t first_index;            // 0x4c callback argument for a callback group
    int32_t primitive_count;        // 0x50
    int32_t dynamic_vertex_slot;    // 0x54 rasterizer_dynamic_vertex_slot index, -1 none
    struct rasterizer_vertex_buffer *vertex_buffer; // 0x58 static vertices; +0x14 is the lightmap
                                    //      stream when it points into a BSP material
    struct BitmapData *lightmap_bitmap; // 0x5c BitmapData; its +0x28 texture gates the lightmap pass
    uint32_t node_matrices;         // 0x60 real_matrix4x3* skinning source, NULL uses the identity at 0x0069673c
    int16_t node_count;             // 0x64
    int16_t unknown_66;             // 0x66 never written
    uint8_t *node_part_indices;     // 0x68 0x0071d19c when node_parts_bit is set
    int32_t node_part_count;        // 0x6c 0x0071d1a0
    struct render_lighting *lighting; // 0x70
    struct render_animation *lighting_extra; // 0x74 (render.h): the {change_colors,
                                    //      function_values} pair at model draw context +0x84
                                    //      (R43); 0x53fe50 reads function_values at +4 through it
                                    //      (0x53242a, 0x534347)
    float depth;                    // 0x78 -(camera.forward . (position - camera.position));
                                    //      the primary sort key, +0.25 for some shaders
    real_point3d position;          // 0x7c
    ColorARGB tint;                 // 0x88 zero unless the caller passes one (0x522300)
    int32_t sorted_index;           // 0x98 slot at allocation, sorted position after
                                    //      transparent_geometry_group_sort 0x5156d0
    int16_t previous_group_index;   // 0x9c drawn first when not -1; FUN_0052b180 hands out its
                                    //      address for the caller to link
    int16_t next_group_index;       // 0x9e drawn after when not -1
    int32_t parent_sort_key;        // 0xa0 group_build: model sort_key for secondary (attached, model mode 1) groups
                                    //    else 0; draw skips nonzero unless attached and draws secondaries whose key
                                    //    == group->sort_key
                                    //      sort_key in mode 1
    uint8_t unknown_a4;             // 0xa4 never written
    uint8_t first_person;           // 0xa5 0x007c0478; compare sorts these last
    uint8_t unknown_a6[2];          // 0xa6 never written
} transparent_geometry_group;       // size 0xa8

// The EAX out block of rasterizer_transparent_geometry_group_build 0x52b180: where the caller
// can later link other groups before or after the new one. 0x52b180 writes +0, +4 and the word
// at +8 (0 / 0 / -1 when it builds nothing); FUN_004d72a0 keeps these 0x10 bytes apart in a
// local array and tests the index word for -1.
typedef struct transparent_geometry_group_link {
    uint32_t previous_group_index;  // 0x00 int16_t* &group->previous_group_index
    uint32_t next_group_index;      // 0x04 int16_t* &group->next_group_index
    int16_t group_index;            // 0x08 transparent_geometry_group_index_from_pointer, -1 none
    int16_t linked_part_index;      // 0x0a (R43) not written by 0x52b180; render_model_draw_parts
                                    //      0x4d72a0 stores the part's signed linked part index byte
                                    //      (part +0x07, else +0x06 tested) there (0x4d7497). In
                                    //      0x4d72a0's own 0x10-byte local records +0x0c is the part
                                    //      index (0x4d7492)
} transparent_geometry_group_link;  // size 0x0c

// ---------------------------------------------------------------------------
// lens flares
// The functions Ghidra calls decal_* at 0x5134f0..0x513cf0 are the lens flare system: the
// record handed to decal_add_to_active_list 0x5138a0 starts with LensFlare tag data, is culled
// against LensFlare.far_fade_distance, and FUN_00513a00 builds one per
// ScenarioStructureBSPLensFlareMarker of a cluster. Visibility is measured with occlusion
// queries (0x537800 issues, 0x537b40 polls) and smoothed into one byte per window per flare by
// decal_shadow_value_update 0x513780; decal_render_active_list 0x513cf0 draws each reflection as
// a screen space quad through the sprite batcher 0x536b70..0x537550.
// ---------------------------------------------------------------------------
typedef enum lens_flare_instance_window_flags {
    _lens_flare_window_index_mask = 0x7f,   // window_flags low bits: the render window index
    _lens_flare_window_flag_80_bit = 0x80   // with reflection flag 8 selects batch mode 2
} lens_flare_instance_window_flags;

typedef struct lens_flare_instance {
    uint32_t definition;            // 0x00 void* LensFlare tag data
    real_point3d position;          // 0x04
    uint32_t packed_direction;      // 0x10 vector3d_pack_normal_11_11_10 0x5132d0
    uint32_t packed_up;             // 0x14 perpendicular axis, same packing
    uint32_t color;                 // 0x18 ARGB; alpha 0 culls, alpha byte scales brightness
    int16_t object_index;           // 0x1c -1 for a BSP marker, otherwise compared with
                                    //      lens_flare_object_visibility.object_index
    int16_t visibility_high;        // 0x1e object flares: visibility slot; BSP flares: bit 15
                                    //      set and the high half of the marker offset
    int16_t visibility_low;         // 0x20 BSP flares: low half of marker index + 8
    uint8_t window_flags;           // 0x22 lens_flare_instance_window_flags; 0x007c310a at
                                    //      create
    uint8_t intensity;              // 0x23 lerp factor between the two brightness bounds
    int32_t sample_count;           // 0x24 occlusion samples, FUN_00513ba0; <= 0 skips
} lens_flare_instance;              // size 0x28

// Element of 0x006bc510 (0x380 entries): the per window visibility bytes of object flares.
// decal_add_to_active_list resets the slot when the object index changes.
typedef struct lens_flare_object_visibility {
    int16_t object_index;           // 0x00
    uint8_t visibility[8];          // 0x02 indexed by window index
} lens_flare_object_visibility;     // size 0x0a

// Sprite vertex drawn with DrawPrimitiveUP stride 0x20 (XYZRHW | DIFFUSE | SPECULAR | TEX1),
// written six per quad by rasterizer_lens_flare_quad_add 0x537550.
typedef struct lens_flare_vertex {
    float x;                        // 0x00
    float y;                        // 0x04
    float z;                        // 0x08
    float rhw;                      // 0x0c
    uint32_t diffuse;               // 0x10
    uint32_t specular;              // 0x14 0x0069e708
    float u;                        // 0x18
    float v;                        // 0x1c
} lens_flare_vertex;                // size 0x20

// The four dword material key compared by FUN_00536cb0 and applied by FUN_00536b70.
typedef struct lens_flare_batch_key {
    int32_t bitmap_tag_index;       // 0x00 low half passed to 0x5187e0 / 0x518a60
    int32_t second_bitmap_tag_index; // 0x04 -1 selects the single texture path
    int32_t bitmap_index;           // 0x08
    uint16_t shader_stage_config;   // 0x0c handed to rasterizer_set_shader_stage_config
    uint16_t unknown_0e;            // 0x0e compared but never set on its own
} lens_flare_batch_key;             // size 0x10

// Element of the batch array whose vertices start at 0x00746fc0 (5 entries, stride 0x18018).
typedef struct lens_flare_batch {
    lens_flare_vertex vertices[0xc00]; // 0x00000
    int32_t vertex_count;           // 0x18000 0x0075efc0
    lens_flare_batch_key key;       // 0x18004 0x0075efc4
    uint32_t last_used;             // 0x18014 0x0075efd4, LRU stamp from 0x00746fa8
} lens_flare_batch;                 // size 0x18018

// ---------------------------------------------------------------------------
// detail objects
// rasterizer_detail_objects_draw 0x51b890 and the vertex fill 0x51b6f0 take the same list: an
// array of per collection batches, each naming a Scenario.detail_object_collection_palette entry
// (+0x3c0, 0x30 byte elements, tag id at +0xc) and a run of draws out of the detail object
// vertex buffer 0x0071d1c8 (detail_object vertices, stride 0x14, 0x78000 bytes).
// rasterizer_detail_objects_begin 0x51b3f0 sets the states. The builder of the list is outside
// this module; only the fields read here are known.
// ---------------------------------------------------------------------------
typedef struct rasterizer_detail_object_draw {
    int32_t first_instance;         // 0x00 index of 6 byte instance records (see below)
    int32_t quad_count;             // 0x04 two triangles each; the fill caps a frame at 0x1000
    int16_t cell_x;                 // 0x08 instance x = cell_x * 8 + byte / 255 * 8
    int16_t cell_y;                 // 0x0a
    float base_z;                   // 0x0c instance z = (base_z + plane . (x, y, z, 1)) * 8
    int32_t first_vertex;           // 0x10 DrawPrimitive start vertex, written by the fill
    uint32_t z_reference;           // 0x14 float[4]* plane the instance bytes are projected on
} rasterizer_detail_object_draw;    // size 0x18

// A detail object instance as packed in the BSP (6 bytes, read by 0x51b150): x, y, z offsets
// in 1/255 of a cell, a byte whose high nibble picks the collection type (modulo the type
// count) and low nibble the sprite (modulo the sprite count of the type), and a 16 bit packed normal
// that is widened into the fourth dword of the vertex.
typedef struct rasterizer_detail_object_instance {
    uint8_t x;                      // 0x00
    uint8_t y;                      // 0x01
    uint8_t z;                      // 0x02
    uint8_t type_and_sprite;        // 0x03
    uint16_t packed_normal;         // 0x04
} rasterizer_detail_object_instance; // size 0x06

// The detail_object vertex (type 11, stride 0x14), six per instance (corners 0,1,2,0,2,3).
typedef struct rasterizer_detail_object_vertex {
    real_point3d position;          // 0x00
    uint32_t normal;                // 0x0c widened packed normal
    uint32_t sprite;                // 0x10 0x01SSTTCC: sprite index, type index, corner
} rasterizer_detail_object_vertex;  // size 0x14

typedef struct rasterizer_detail_object_batch {
    uint32_t draws;                 // 0x00 rasterizer_detail_object_draw*
    int16_t draw_count;             // 0x04
    int16_t collection_palette_index; // 0x06 Scenario.detail_object_collection_palette index
} rasterizer_detail_object_batch;   // size 0x08

typedef struct rasterizer_detail_object_batches {
    uint32_t batches;               // 0x00 rasterizer_detail_object_batch*
    int16_t batch_count;            // 0x04
    int16_t unknown_06;             // 0x06
} rasterizer_detail_object_batches; // size 0x08 (only the first 6 bytes are read)

// ---------------------------------------------------------------------------
// font_glyph_cache  (0x006d8828, 0x404 dwords, zeroed by text_font_system_initialize 0x514820)
// A 512 by 512 atlas BitmapData (bitmap class 'bitm', depth 1, format 9, flags 0x41) that
// font_glyph_cache_allocate_and_upload 0x514ed0 packs FontCharacter glyphs into row by row,
// evicting the oldest ring entries; the glyph remembers its slot in
// FontCharacter.hardware_character_index (+0xc) and the frame stamp 0x0069c694 at +0xe.
// ---------------------------------------------------------------------------
typedef struct font_glyph_cache_entry {
    uint32_t character;             // 0x00 void* FontCharacter tag data, NULL when free
    int16_t x;                      // 0x04 atlas position plus the one texel border
    int16_t y;                      // 0x06
} font_glyph_cache_entry;           // size 0x08

typedef struct font_glyph_cache {
    uint8_t initialized;            // 0x0000
    uint8_t unknown_0001;           // 0x0001
    uint16_t oldest_slot;           // 0x0002 ring read index, masked 0x1ff
    uint16_t next_slot;             // 0x0004 ring write index
    int16_t cursor_x;               // 0x0006
    int16_t cursor_y;               // 0x0008
    int16_t row_height;             // 0x000a tallest glyph of the current row plus 2
    uint32_t atlas;                 // 0x000c void* BitmapData (GlobalAlloc 0x30) with pixels at +0x2c
    font_glyph_cache_entry entries[0x200]; // 0x0010
} font_glyph_cache;                 // size 0x1010

// ---------------------------------------------------------------------------
// module globals
//
// device and caps
// global 0x0071d174: void *rasterizer_device                   IDirect3DDevice9 (4143 vtable calls)
// global 0x0071d178: void *rasterizer_direct3d                 IDirect3D9; its GetDeviceCaps
//                                                             (+0x38) writes 0x007c10c0
// global 0x007c10b0: uint8_t rasterizer_device_lost            set on D3DERR_DEVICELOST in the
//                                                             present path 0x518180
// global 0x007c10c0: d3d_caps9 rasterizer_caps                 see the field notes above
// global 0x007c04a0: d3d_present_parameters rasterizer_present_parameters
// global 0x0071d16c: uint8_t rasterizer_fullscreen              set when neither -window (0x0071d1a8)
//                                                             nor 0x0071d1ac is set (0x5169c0)
//                                                             NOT a "device initialised" flag (R82
//                                                             rejected): the only writers,
//                                                             0x516a0f / 0x516a17, run from the
//                                                             command-line options before any
//                                                             device exists, and 0x511d80 returns
//                                                             this flag && device (0x0071d174).
// global 0x0071d16e: uint8_t rasterizer_pending_clear          0x5180d0
// global 0x0071d16f: uint8_t rasterizer_in_scene               set after BeginScene (0x517500), cleared
//                                                             after EndScene (rasterizer_end_frame 0x517b90)
// global 0x0069c6a0: uint32_t rasterizer_device_type           D3DDEVTYPE, 2 with -useref
// global 0x0069c682: int16_t rasterizer_texture_stage_count     4, or 2 below 4 simultaneous textures
// global 0x0071d180: uint32_t rasterizer_adapter                adapter ordinal in use
// global 0x007c11f0: d3d_display_mode rasterizer_desktop_display_mode
// global 0x0069c680: uint8_t rasterizer_software_vertex_processing  ORed as usage 0x10
// global 0x0069c688: uint8_t rasterizer_caps_flag_688           set by 0x5169c0 command line
// global 0x0069c689: uint8_t rasterizer_caps_flag_689           disables the render target pool
// global 0x0069c68a: uint8_t rasterizer_caps_flag_68a           selects the second pool layout
// global 0x0069c6ac: int16_t rasterizer_shader_stage_config     cache of 0x519200
// global 0x0069c708: uint8_t rasterizer_default_material[0x44]  D3DMATERIAL9, 0x526700
// global 0x007c3084: int32_t rasterizer_fixed_function_light_count
//
// window
// global 0x0069c630: uint8_t rasterizer_frame_started           UNSURE name; set to 1 once by the
//                                                             frame start at 0x517421, gates the render
//                                                             target 1 composite in rasterizer_end_frame
// global 0x0069c632: int16_t rasterizer_vertex_buffer_lock_state  2, 4, 5
// global 0x0069c634: Rectangle2D-like int16 pair game_window_top_left (see types/networking.h)
// global 0x0069c638: int16 pair game_window_bottom_right
// 0x0069c65c..0x0069c668: two {near, far} clip-distance pairs (R11). .data holds
//   00 00 80 3d 00 00 80 44 00 00 40 3c 00 00 80 44 = (0.0625, 1024) and (0.01171875, 1024);
//   render_cinematic_screen_effect_update 0x511df0 re-seeds any that are 0.0 (0x511e05..0x511e62).
// global 0x0069c65c: float rasterizer_default_z_near           0.0625; the default near clip
//                                                             distance. 0x4c9260 copies the
//                                                             first pair into camera z_near /
//                                                             z_far; chimera__cinematic_screen_
//                                                             effect 0x517470 overwrites it with
//                                                             the cinematic near clip (render.h
//                                                             cinematic_screen_effect_globals
//                                                             +0x74) when that is positive.
//                                                             CORRECTED (R11): formerly
//                                                             "rasterizer_letterbox_height".
// global 0x0069c660: float rasterizer_default_z_far            1024.0
// global 0x0069c664: uint32_t rasterizer_frustum_z_values[2]    the second pair, (0.01171875,
//                                                             1024.0) as raw float bits, handed
//                                                             to set_frustum_z_func
// global 0x0069c66c: void *rasterizer_capture_surfaces[4]       0x0069c66c..0x0069c678
// global 0x0069c67e: int16_t rasterizer_maximum_skinning_nodes  0x3f
// global 0x0069c694: int32_t rasterizer_frame_index             glyph cache stamp
//
// frame
// global 0x007c1200: rasterizer_frame_time rasterizer_time
// global 0x007c1220: rasterizer_window_parameters rasterizer_window
// global 0x007c1480: int32_t rasterizer_light_count
// global 0x007c1484: rasterizer_light rasterizer_lights[128]
// global 0x007c04e0: rasterizer_skinning_matrix rasterizer_skinning_palette[63]
//                                                             (bounded by next global)
// global 0x0071d19c: uint8_t *rasterizer_node_part_indices     set_up_node_parts
// global 0x0071d1a0: int32_t rasterizer_node_part_count
// global 0x006e0a10: rasterizer_projected_light_constants rasterizer_projected_light
// global 0x006e0a60: uint8_t rasterizer_projected_light_has_cube_map
// global 0x006e0a64: int32_t rasterizer_projected_light_cube_map  tag index
// global 0x0071d1d8: float rasterizer_projected_light_luminance
// global 0x0071d164: GlobalsRasterizerData *rasterizer_globals_data
// global 0x0069c6a8: uint8_t rasterizer_fog_enabled            latched copy of 0x006893fc (0x5176d0)
// global 0x0069c6fc: int16_t rasterizer_projected_light_shader_variant  0 or 1 (0x521750)
// global 0x006e0a04: uint8_t unknown_006e0a04                   UNSURE: odd environment effects
// global 0x006e0a08: BitmapData *rasterizer_environment_lightmap  set from EAX by 0x51f310
// global 0x006e0a0c: uint8_t rasterizer_environment_lightmap_missing  0x520910
// global 0x006e0a68: uint8_t rasterizer_lightmap_bitmap_missing   set by 0x511f90 (render module)
// global 0x006e0a6c: BitmapData *rasterizer_lightmap_bitmap       set by 0x511f90
// global 0x0071d1b1: uint8_t rasterizer_render_target_capture_requested
// global 0x0071d1b2: uint8_t rasterizer_render_target_capture_done
// global 0x0071d1fe: uint8_t rasterizer_camouflage_fade_active, 0x0071d200 float rasterizer_camouflage_fade
// global 0x0071d1d0: rasterizer_effect_slot *rasterizer_active_environment_effect  &effects[36] or NULL
// global 0x00686b04: const ColorRGB *global_white_color         -> 0x0065513c (1, 1, 1)
//
// dynamic geometry
// global 0x0071d13c: void *rasterizer_scratch_memory           0x18000 byte bump pool
// global 0x0071d140: uint32_t rasterizer_scratch_memory_used
// global 0x006d98e8: rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[20]
// global 0x006d99d8: rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[0x400]
// global 0x006dd9d8: int32_t rasterizer_dynamic_vertex_slot_count
// global 0x006dd9e0: rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[0x400]
// global 0x006e09e0: int32_t rasterizer_dynamic_index_slot_count
// global 0x006e09e4: int32_t rasterizer_dynamic_index_count
// global 0x006e09e8: void *rasterizer_dynamic_index_buffer     IDirect3DIndexBuffer9 0x30000
// global 0x0071d1cc: uint8_t rasterizer_dynamic_index_overflow
// global 0x0071d1cd: uint8_t rasterizer_dynamic_vertex_overflow
// global 0x007bf060: rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[0x100]
// global 0x0071d258: int32_t rasterizer_vertex_buffer_slot_high_water
// global 0x0071d25c: int32_t rasterizer_vertex_buffer_slot_count
// global 0x006e1a90: rasterizer_vertex_declaration rasterizer_vertex_declarations[20]
// global 0x0065de00: int16_t rasterizer_vertex_sizes[20]       .rdata, see the enum
// global 0x0065e034: uint32_t rasterizer_triangle_buffer_primitive_types[2]  .rdata {4, 5}
// global 0x0071d270: void *rasterizer_misc_vertex_buffer       0x10000 bytes, 0x534e50
// global 0x0071d1bc: void *rasterizer_decal_vertex_cache       0x51a6a0
// global 0x0071d1c8: void *rasterizer_detail_object_vertex_buffer  0x78000 bytes, 0x51b370
// global 0x0071d1c0: uint8_t *rasterizer_decal_vertex_cache_handle  game state block of the decal
//                                                             vertex cache (+0x2c shift, +0x3c data_array)
// global 0x0071d1bc: void *rasterizer_decal_vertex_cache (see 0x51a6a0; the IDirect3DVertexBuffer9)
// global 0x006d98d8: int16_t rasterizer_decal_blend_mode, 0x006d98dc int16_t rasterizer_decal_layer,
//                    0x006d98e0 uint32_t rasterizer_decal_bitmap_tag, 0x006d98e4 int16_t
//                    rasterizer_decal_bitmap_frame (0x51a810, 0x51aa50)
// global 0x006d9878: float rasterizer_screen_quad_vertices[4][6]  type 8 quad of 0x519b00
//
// transparent geometry
// global 0x0071d14c: transparent_geometry_group *transparent_geometry_groups       384
// global 0x0071d150: transparent_geometry_group *transparent_geometry_groups_secondary  32
// global 0x0071d154: int32_t transparent_geometry_group_count
// global 0x0071d158: int32_t transparent_geometry_group_secondary_count
// global 0x0071d15c: int16_t *transparent_geometry_group_sorted_indices
// global 0x006d983c: uint32_t transparent_geometry_group_drawn_bits[12]
// global 0x006d9838: int16_t transparent_geometry_group_draw_cursor  0x5154a0
// global 0x006e0a70: transparent_geometry_group transparent_geometry_group_immediate
// global 0x006e1828: transparent_geometry_group transparent_geometry_group_environment_immediate
// global 0x006e1d58: int32_t transparent_geometry_group_last_drawn_key
// global 0x0071d1ce: uint8_t transparent_geometry_group_overflow_a  (0x51c830)
// global 0x0071d1dc: uint8_t transparent_geometry_group_overflow_b  (0x522300)
// global 0x0071d204: uint8_t transparent_geometry_group_overflow_c  (0x52b180)
// global 0x0071d27c: uint8_t transparent_geometry_group_overflow_d  (0x536ff0)
// global 0x0071d1f0: rasterizer_model_draw_context *rasterizer_active_model_context
// global 0x0071d1f8: int16_t rasterizer_active_model_mode        0, 1 or 2
//
// lens flares
// global 0x006ce818: lens_flare_instance lens_flare_instances[0x400]
// global 0x0071d134: int32_t lens_flare_instance_count
// global 0x0071d138: uint8_t lens_flare_instance_overflow
// global 0x006bc510: lens_flare_object_visibility lens_flare_object_visibility[0x380]
// global 0x006be810: uint8_t lens_flare_marker_visibility[0x10008]
// global 0x00746fa8: uint32_t lens_flare_batch_clock
// global 0x00746fb0: lens_flare_batch_key lens_flare_current_key
// global 0x00746fc0: lens_flare_batch lens_flare_batches[5]
// global 0x007bf040: lens_flare_batch_key lens_flare_applied_key
// global 0x0069e708: uint32_t lens_flare_vertex_specular
// global 0x006e1dc0: uint8_t lens_flare_occlusion_queries_supported
// global 0x006e1dc8: void *lens_flare_occlusion_queries[0x400]  IDirect3DQuery9, type 9
//
// text
// global 0x006d8828: font_glyph_cache font_glyph_cache
// global 0x006d986c: int16_t rasterizer_bound_bitmap_size_a[2]  0x518960
// global 0x006d9870: int16_t rasterizer_bound_bitmap_size_b[2]  0x518860
// global 0x006d9874: int16_t rasterizer_bound_bitmap_size_c[2]  0x518a60
//
// shaders and effects
// global 0x0069d410: rasterizer_effect_slot rasterizer_effects[122]
// global 0x0069e350: rasterizer_vertex_shader rasterizer_vertex_shaders[64]
// global 0x0071d254: void *rasterizer_effect_pool              effect defines object
// global 0x007c0460: void *rasterizer_effect_defines
// global 0x0071d210: void *screen_effect_techniques[11]         VideoOn .. VideoOffConvolvedFilterDesaturation
// global 0x0071d23c: void *screen_flash_techniques[6]           FlashLighten .. FlashTint
// global 0x006e1780: int32_t environment_techniques_multipurpose[24]
// global 0x006e17f4: int32_t environment_techniques_no[12]
// global 0x006e18d8: int32_t environment_techniques_self_illumination[24]
// global 0x006e1938: int32_t environment_techniques_plain[12]  (0x006e1950 is its second half
//                                                             on ps_1_1..1_3)
// global 0x006e1968: int32_t environment_techniques_reflection[24]
// global 0x006e19d0: int32_t environment_techniques_change_color[24]
// global 0x007c0470: void *shader_environment_draw_simple       0x52b630
// global 0x007c0474: void *shader_environment_draw              0x52b630
// global 0x007c0480: void *rasterizer_glass_draw_procedures[3]  ShaderTransparentGlass diffuse, tint,
//                                                             reflection (0x523ec0)
// global 0x007c048c: void *environment self-illumination procedure  0x51f3e0 or the undefined
//                                                             0x51fad0/0x51fd80 (0x516810)
// global 0x007c0490: void *environment procedure              the undefined 0x51e2a0/0x51e570/0x51e8f0
// global 0x007c0494: void *environment per light procedure    0x51dc50 or 0x44ad80
// global 0x006e1d08: float rasterizer_ui_text_constants[20]     0x531ab0
//
// render targets and gamma
// global 0x0069d350: int16_t rasterizer_active_render_target    0xffff none
// global 0x0069d358: rasterizer_render_target rasterizer_render_targets[9]
// global 0x0071d208: void *rasterizer_render_target_index_buffer
// global 0x0071d20c: void *rasterizer_render_target_vertex_buffer
// global 0x006e0b18: d3d_gamma_ramp rasterizer_desktop_gamma_ramp
// global 0x006e1118: d3d_gamma_ramp rasterizer_game_gamma_ramp
// global 0x0071d1e0: int32_t rasterizer_gamma_exponent
// global 0x0071d1e8: uint8_t rasterizer_gamma_disabled          -nogamma
// global 0x0071d1ec: int32_t rasterizer_gamma_captured
//
// Globals this module reads but does not own:
// global 0x00746f9c: void *structure_bsp                       ScenarioStructureBSP tag data
// global 0x00746fa0: void *global_globals                      Globals tag data
// global 0x0087bc14: tag_instance *tag_instances               types/cache.h
// global 0x007c310a: int16_t render_window_index               render module (render.h). R12:
//                                                             written as a WORD (0x50bf20) and
//                                                             read as one (0x50c017); the byte
//                                                             readers 0x4f13fd, 0x4f16ce and
//                                                             0x513b4e only need the low byte
// global 0x0071cfc4: cinematic_screen_effect_globals *cinematic_screen_effect_globals
//                                                             render.h (0x78 bytes; R80). 0x51578b
//                                                             clears 0x1e dwords of it; +0x74 is
//                                                             near_clip_distance, which 0x517470
//                                                             copies into 0x0069c65c. Allocated in
//                                                             the game-state block by 0x5169c0.
// global 0x006893e4..0x00689464: uint8_t debug toggles          console globals
// global 0x00721ef0: int32_t os_platform                       types/cache.h
// ---------------------------------------------------------------------------

#pragma pack(pop)
