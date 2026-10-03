# `rasterizer` - the Direct3D 9 back end

This directory covers the retail Halo PC `halo.exe` 1.0.10, module range `0x5132b0 .. 0x537d60`
(228 Ghidra functions). The code is plain C, MSVC 7.1, x86. Each file holds one function,
rewritten against `types/rasterizer.h` from its Ghidra decompilation, or from the raw
disassembly where Ghidra lost registers, stack slots or call arguments. The original decompile
is kept verbatim at the bottom of each file inside `#if 0 ... #endif`.

Coverage is the whole module: 224 files.
- Four of the 228 Ghidra "functions" are not functions, so they have no file (see Known gaps).
- 0x518ce0 and 0x518d40 were one function that Ghidra split in two. They are one file.

Checks:
- Gate: `python tools/build_check.py rasterizer` gives **224 ok, 0 failed**.
- Header smoke test: `gcc -fsyntax-only -m32 -I types out/phase4/rasterizer_smoke.c` is clean.
  It checks every struct size, the table spans against neighbouring globals, and the tag
  offsets that the draws rely on.

Calling conventions: most functions take some or all of their arguments in registers (MSVC 7.1
whole-program optimisation). Every file states them in a `// blam-cc:` line above the
definition, and the C parameter order follows the file header. Register-passed externs repeat
the same `// blam-cc:` line at every declaration.

## What the module contains

| Family | Range | What it is |
|---|---|---|
| lens flares | `0x5132b0`-`0x513cf0` | Ghidra calls these `decal_*`. They cover 11:11:10 normal packing, per-window visibility bytes, BSP marker instancing, occlusion sample smoothing, the rotation of a flare (`lens_flare_compute_rotation`) and the per-reflection quad builder `lens_flare_render_all` |
| bitmaps and text | `0x514560`-`0x514cb0` | scratch bump allocator, mip count and pixel data size, the 512x512 font glyph atlas, and the 8/16-bit debug text drawers |
| transparent geometry queue | `0x5151c0`-`0x515740` | the 384 (+32) `transparent_geometry_group` pools, their depth sort and drawn bits |
| device and window | `0x515930`-`0x518450` | game window, display modes, present parameters, `rasterizer_initialize_direct3d`, device reset, begin/end frame, fog constants, `rasterizer_capture_and_present` (back buffer capture then Present), shutdown |
| texture binding | `0x518680`-`0x518a60` | `SetTexture` and `ID3DXEffect::SetTexture` binders, and the tag bitmap resolvers |
| lighting and skinning constants | `0x518b40`-`0x519200` | skinning palette upload, point light constants, the c15..c25 lighting block (`rasterizer_prepare_lighting_constants`), frustum z / clip planes, shader stage configuration |
| render target capture, active camouflage | `0x519b00`-`0x51a660` | copying render target 1 into 2, and the camouflage refraction pass |
| decals | `0x51a6a0`-`0x51b0e0` | decal vertex cache, decal pass state, and the per-cluster decal draw |
| detail objects | `0x51b150`-`0x51b890` | the detail object vertex buffer: fill, states and draw |
| dynamic geometry | `0x51bb90`-`0x51c830` | dynamic vertex/index caches, the four indexed draw paths, and their dispatcher |
| UI quads, light passes | `0x51c9a0`-`0x51e9f0` | `rasterizer_ui_quad_draw` (every HUD/UI textured quad, handed a `ui_quad_render_state`), the per-light environment pass (the "light cone" family), and the fog overlay |
| shader_environment passes | `0x51eb20`-`0x522300` | fog layer, self-illumination, reflection, dynamic mirror, projected light and lightmap specular passes, all through `ID3DXEffect` Begin/Pass/End |
| transparent glass, gamma | `0x522300`-`0x523ec0` | transparent group creation, the gamma ramp, and the three ShaderTransparentGlass procedures (tint, reflection, diffuse) in pixel shader and fixed-function variants |
| bitmap upload | `0x523f10`-`0x524980` | hardware texture creation, 2D/3D/cube mip uploads, texel sampling, vertex buffer creation |
| sun glow | `0x525030`-`0x525ab0` | the index buffer constructor, then the sun glow: project the sun, capture the rectangle around it, ping-pong blur between targets 6 and 7 with effect 76, add the result back with effect 77 |
| model lighting and shader_model | `0x526700`-`0x52b630` | fixed-function lights, the shader_environment technique table, node parts, model draw state setup/restore, shader_model technique selection, its three draw paths (limited, fixed function, pixel shader), the environment draw dispatcher, and the transparent group builder |
| motion sensor | `0x52b690`-`0x52bc40` | begin, per-blip draw, and end of the motion sensor sweep |
| plasma, render targets | `0x52c4a0`-`0x52ce10` | the transparent plasma draw; the render target table (initialize, dispose, set active, bind to a stage or to an effect texture) |
| screen effects and flash | `0x52ce50`-`0x52ed00` | the video / screen effect (effect 114) with its UV transform and fixed-function fallback, and the six full-screen flash blends |
| shader and declaration loading | `0x52f780`-`0x530800` | `shaders\fx.bin` and `shaders\vsh.bin` loading, effect constant handles, vertex declarations (D3DVERTEXELEMENT9 arrays), vertex buffer creation and slot management |
| object shadows | `0x530830`-`0x531570` | the dynamic object shadow. Begin (target 3 and the projection), draw model parts into the silhouette, blur 3 into 4, and draw the shadow onto the structure triangles the BSP walk hands back |
| text, chicago, transparent groups | `0x531ab0`-`0x534e50` | widescreen text scaling and text begin/end, the two ShaderTransparentChicago draws, the transparent group vertex/part/group draws (dispatching on `shader_type`), and the misc vertex buffer |
| lens flare batching | `0x536b70`-`0x537b40` | the lens flare sprite batcher (material, slots, flush), projection to screen, and occlusion queries |
| chicago combiners | `0x537bb0`-`0x537d60` | fixed-function texture stage setup for the chicago and chicago-extended maps |

The key global tables are:

| Table | Address | Size | Contents |
|---|---|---|---|
| `rasterizer_effects` | 0x69d410 | 122 x 0x20 | `ID3DXEffect` per pixel shader effect |
| `rasterizer_vertex_shaders` | 0x69e350 | 64 x 8 | vertex shaders |
| `rasterizer_vertex_declarations` | 0x6e1a90 | 20 x 0xc | vertex declarations |
| `rasterizer_render_targets` | 0x69d358 | 9 x 0x14 | render targets |
| `rasterizer_window` | 0x7c1220 | 0x258 | camera, frustum, fog and screen flash of the window being drawn |

Effect indices named in the code:

| Index | Effect |
|---|---|
| 44 | plasma |
| 45 | shadow blur |
| 47 | shadow on structure |
| 49..73 | multitexture UI quads |
| 76 / 77 | sun glow |
| 111 | meter |
| 114 | screen effect |
| 115 | screen flash |
| 116..121 | environment, self-illumination, change colour, multipurpose, reflection, plain |

## Struct layouts

All of the structs below are in `types/rasterizer.h`, which is under `#pragma pack(push,1)`.
The tables are generated from the header, and every size is asserted in
`out/phase4/rasterizer_smoke.c`. Pointer fields are `uint32_t`, with the pointee named in the
note, so that the 32-bit sizes still hold under a 64-bit host compiler.

The module also uses records defined in other headers; they are not redefined here:
- `ui_quad_render_state` (0x8c) and `hud_quad_vertex` (0x18) in `types/interface.h`. The same
  0x8c-byte record is what `rasterizer_ui_quad_draw` and `rasterizer_draw_text_begin` receive.
- `d3d_display_mode` and `win32_rect` in `types/interface.h`.
- The shader tags `ShaderModel`, `ShaderTransparentChicago`, `ShaderTransparentChicagoExtended`
  and `ShaderTransparentChicagoMap`, the glass, meter and plasma shaders, `LensFlare`,
  `BitmapData` and `GlobalsRasterizerData`, all in `types/tags.h`.
- `real_matrix4x3`, `real_vector3d` and `real_point3d` in `types/math.h`.

#### rasterizer_vertex_buffer (size 0x14)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | type | int16_t | rasterizer_vertex_type |
| 0x02 | unknown_02 | int16_t | alignment, never written |
| 0x04 | count | int32_t | vertex count |
| 0x08 | unknown_08 | int16_t | zeroed by the constructor |
| 0x0a | unknown_0a | int16_t | zeroed by the constructor |
| 0x0c | data | uint32_t | void* source vertices (tag data) |
| 0x10 | hardware_buffer | uint32_t | void* IDirect3DVertexBuffer9, from the 0x530570 wrapper |

#### rasterizer_index_buffer (size 0x10)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | type | int16_t | TriangleBufferType |
| 0x02 | unknown_02 | int16_t | alignment |
| 0x04 | count | int32_t | primitive count |
| 0x08 | data | uint32_t | void* source indices (tag data) |
| 0x0c | hardware_buffer | uint32_t | void* IDirect3DIndexBuffer9 |

#### rasterizer_vertex_declaration (size 0x0c)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | declaration | uint32_t | void* IDirect3DVertexDeclaration9 |
| 0x04 | fvf | uint32_t | always 0 in this build |
| 0x08 | usage | uint32_t | D3DUSAGE bits for buffers of this type |

#### rasterizer_vertex_buffer_slot (size 0x14)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | hardware_buffer | uint32_t | void* IDirect3DVertexBuffer9, NULL when free |
| 0x04 | vertex_type | int32_t | rasterizer_vertex_type (EAX of 0x530570) |
| 0x08 | length | uint32_t | bytes |
| 0x0c | fvf | uint32_t |  |
| 0x10 | managed | uint8_t | UNSURE: nonzero entries are skipped on recreate |
| 0x11 | unknown_11[3] | uint8_t | never read |

#### rasterizer_dynamic_vertex_cache (size 0x0c)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | used | int32_t | vertices handed out this frame |
| 0x04 | capacity | int32_t | vertices |
| 0x08 | buffer_handle | int32_t | 1-based rasterizer_vertex_buffer_slot handle, 0 none |

#### rasterizer_dynamic_vertex_slot (size 0x10)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | vertex_type | int16_t | rasterizer_vertex_type |
| 0x02 | unknown_02 | int16_t | alignment |
| 0x04 | first_vertex | int32_t |  |
| 0x08 | vertex_count | int32_t |  |
| 0x0c | locked_vertices | uint32_t | void* Lock result, NULL on failure |

#### rasterizer_dynamic_index_slot (size 0x0c)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | first_index | int32_t |  |
| 0x04 | index_count | int32_t |  |
| 0x08 | unknown_08 | int32_t | no writer found in this module |

#### rasterizer_effect_slot (size 0x20)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | effect | uint32_t | void* ID3DXEffect |
| 0x04 | vertex_shader_index | int32_t | index into rasterizer_vertex_shaders (0x0069e350) |
| 0x08 | texture_handles[4] | uint32_t | void* D3DXHANDLE Texture0..Texture3 |
| 0x18 | constant_handles | uint32_t | void** GlobalAlloc array of named constant handles |
| 0x1c | unknown_1c | uint32_t | no reader found |

#### rasterizer_vertex_shader (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | shader | uint32_t | void* IDirect3DVertexShader9 |
| 0x04 | enabled | int32_t | static initialised data; 0 skips the chunk |

#### rasterizer_render_target (size 0x14)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | width | uint32_t |  |
| 0x04 | height | uint32_t |  |
| 0x08 | format | uint32_t | D3DFORMAT; 0x15 A8R8G8B8 |
| 0x0c | surface | uint32_t | void* IDirect3DSurface9 |
| 0x10 | texture | uint32_t | void* IDirect3DTexture9 |

#### d3d_present_parameters (size 0x38)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | back_buffer_width | uint32_t | (used) |
| 0x04 | back_buffer_height | uint32_t | (used) |
| 0x08 | back_buffer_format | uint32_t | (used) |
| 0x0c | back_buffer_count | uint32_t | (used) |
| 0x10 | multisample_type | uint32_t |  |
| 0x14 | multisample_quality | uint32_t |  |
| 0x18 | swap_effect | uint32_t | (used) 1 or 3 |
| 0x1c | device_window | uint32_t | void* (used) |
| 0x20 | windowed | int32_t | (used) |
| 0x24 | enable_auto_depth_stencil | int32_t | (used) 1 |
| 0x28 | auto_depth_stencil_format | uint32_t | (used) 0x4b |
| 0x2c | flags | uint32_t | (used) |
| 0x30 | fullscreen_refresh_rate | uint32_t | (used) |
| 0x34 | presentation_interval | uint32_t | (used) 1 means vsync, 0x80000000 immediate |

#### d3d_caps9 (size 0x130)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | device_type | uint32_t |  |
| 0x04 | adapter_ordinal | uint32_t |  |
| 0x08 | caps | uint32_t |  |
| 0x0c | caps2 | uint32_t | (used) gamma paths |
| 0x10 | caps3 | uint32_t |  |
| 0x14 | presentation_intervals | uint32_t |  |
| 0x18 | cursor_caps | uint32_t |  |
| 0x1c | dev_caps | uint32_t | (used, high word at 0x007c10de) |
| 0x20 | primitive_misc_caps | uint32_t |  |
| 0x24 | raster_caps | uint32_t | (used) 0x007c10e4: 0x04000000 DEPTHBIAS and 0x02000000 SLOPESCALEDEPTHBIAS gate render states 0xc3/0xaf in 0x5194e0..0x5195d0 |
| 0x28 | z_cmp_caps | uint32_t |  |
| 0x2c | src_blend_caps | uint32_t | (used) |
| 0x30 | dest_blend_caps | uint32_t |  |
| 0x34 | alpha_cmp_caps | uint32_t |  |
| 0x38 | shade_caps | uint32_t |  |
| 0x3c | texture_caps | uint32_t | (used) 0x007c10fc, bitmap lock and upload paths |
| 0x40 | texture_filter_caps | uint32_t | (used) |
| 0x44 | cube_texture_filter_caps | uint32_t |  |
| 0x48 | volume_texture_filter_caps | uint32_t |  |
| 0x4c | texture_address_caps | uint32_t | (used) 0x007c110c bit 3 BORDER |
| 0x50 | volume_texture_address_caps | uint32_t |  |
| 0x54 | line_caps | uint32_t |  |
| 0x58 | max_texture_width | uint32_t |  |
| 0x5c | max_texture_height | uint32_t |  |
| 0x60 | max_volume_extent | uint32_t |  |
| 0x64 | max_texture_repeat | uint32_t |  |
| 0x68 | max_texture_aspect_ratio | uint32_t |  |
| 0x6c | max_anisotropy | uint32_t | (used) |
| 0x70 | max_vertex_w | float |  |
| 0x74 | guard_band_left | float |  |
| 0x78 | guard_band_top | float |  |
| 0x7c | guard_band_right | float |  |
| 0x80 | guard_band_bottom | float |  |
| 0x84 | extents_adjust | float |  |
| 0x88 | stencil_caps | uint32_t |  |
| 0x8c | fvf_caps | uint32_t |  |
| 0x90 | texture_op_caps | uint32_t |  |
| 0x94 | max_texture_blend_stages | uint32_t |  |
| 0x98 | max_simultaneous_textures | uint32_t | (used) 0x007c1158, shader_environment path pick |
| 0x9c | vertex_processing_caps | uint32_t |  |
| 0xa0 | max_active_lights | uint32_t | (used) 0x007c1160, rasterizer_light_disable_all |
| 0xa4 | max_user_clip_planes | uint32_t |  |
| 0xa8 | max_vertex_blend_matrices | uint32_t |  |
| 0xac | max_vertex_blend_matrix_index | uint32_t |  |
| 0xb0 | max_point_size | float |  |
| 0xb4 | max_primitive_count | uint32_t |  |
| 0xb8 | max_vertex_index | uint32_t |  |
| 0xbc | max_streams | uint32_t | (used) 0x007c117c; 0x51c310 binds a second stream only when it is above 1 |
| 0xc0 | max_stream_stride | uint32_t |  |
| 0xc4 | vertex_shader_version | uint32_t |  |
| 0xc8 | max_vertex_shader_const | uint32_t |  |
| 0xcc | pixel_shader_version | uint32_t | (used) 0x007c118c, 74 comparisons in the module |
| 0xd0 | pixel_shader_1x_max_value | float |  |
| 0xd4 | dev_caps2 | uint32_t |  |
| 0xd8 | max_npatch_tessellation_level | float |  |
| 0xdc | reserved5 | uint32_t |  |
| 0xe0 | master_adapter_ordinal | uint32_t |  |
| 0xe4 | adapter_ordinal_in_group | uint32_t |  |
| 0xe8 | number_of_adapters_in_group | uint32_t |  |
| 0xec | decl_types | uint32_t |  |
| 0xf0 | num_simultaneous_rts | uint32_t |  |
| 0xf4 | stretch_rect_filter_caps | uint32_t |  |
| 0xf8 | vs20_caps[4] | uint32_t | D3DVSHADERCAPS2_0 |
| 0x108 | ps20_caps[5] | uint32_t | D3DPSHADERCAPS2_0 |
| 0x11c | vertex_texture_filter_caps | uint32_t |  |
| 0x120 | max_vshader_instructions_executed | uint32_t |  |
| 0x124 | max_pshader_instructions_executed | uint32_t |  |
| 0x128 | max_vertex_shader30_instruction_slots | uint32_t |  |
| 0x12c | max_pixel_shader30_instruction_slots | uint32_t |  |

#### d3d_gamma_ramp (size 0x600)

| offset | field | type | note |
|---|---|---|---|
| 0x000 | red[0x100] | uint16_t |  |
| 0x200 | green[0x100] | uint16_t |  |
| 0x400 | blue[0x100] | uint16_t |  |

#### d3d_locked_rect (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | pitch | int32_t |  |
| 0x04 | bits | uint32_t | void* |

#### d3d_surface_desc (size 0x20)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | format | uint32_t |  |
| 0x04 | type | uint32_t |  |
| 0x08 | usage | uint32_t |  |
| 0x0c | pool | uint32_t |  |
| 0x10 | multisample_type | uint32_t |  |
| 0x14 | multisample_quality | uint32_t |  |
| 0x18 | width | uint32_t | (used) |
| 0x1c | height | uint32_t | (used) |

#### d3d_viewport (size 0x18)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | x | uint32_t |  |
| 0x04 | y | uint32_t |  |
| 0x08 | width | uint32_t |  |
| 0x0c | height | uint32_t |  |
| 0x10 | min_z | float |  |
| 0x14 | max_z | float |  |

#### d3dx_macro (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | name | const char* |  |
| 0x04 | definition | const char* |  |

#### d3d_vertex_element9 (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | stream | uint16_t |  |
| 0x02 | offset | uint16_t |  |
| 0x04 | type | uint8_t | D3DDECLTYPE |
| 0x05 | method | uint8_t | D3DDECLMETHOD |
| 0x06 | usage | uint8_t | D3DDECLUSAGE |
| 0x07 | usage_index | uint8_t |  |

#### win32_wndclassexa (size 0x30)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | size | uint32_t | 0x30 |
| 0x04 | style | uint32_t |  |
| 0x08 | window_procedure | uint32_t | void* WNDPROC |
| 0x0c | class_extra | int32_t |  |
| 0x10 | window_extra | int32_t |  |
| 0x14 | instance | uint32_t | void* HINSTANCE |
| 0x18 | icon | uint32_t | void* HICON |
| 0x1c | cursor | uint32_t | void* HCURSOR |
| 0x20 | background_brush | uint32_t | void* HBRUSH |
| 0x24 | menu_name | uint32_t | const char* |
| 0x28 | class_name | uint32_t | const char* |
| 0x2c | small_icon | uint32_t | void* HICON |

#### rasterizer_screen_vertex (size 0x1c)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | x | float |  |
| 0x04 | y | float |  |
| 0x08 | z | float |  |
| 0x0c | rhw | float |  |
| 0x10 | diffuse | uint32_t |  |
| 0x14 | u | float |  |
| 0x18 | v | float |  |

#### rasterizer_dynamic_screen_vertex (size 0x18)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | x | float |  |
| 0x04 | y | float |  |
| 0x08 | z | float |  |
| 0x0c | color | uint32_t | D3DCOLOR ARGB |
| 0x10 | u | float |  |
| 0x14 | v | float |  |

#### rasterizer_node_matrices (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | matrices | uint32_t | real_matrix4x3* |
| 0x04 | node_count | int16_t |  |
| 0x06 | unknown_06 | int16_t | not read |

#### rasterizer_display_mode (size 0x10)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | width | int32_t | present parameters +0x00 |
| 0x04 | height | int32_t | present parameters +0x04 |
| 0x08 | refresh_rate | int32_t | 60 when os_platform < 3 or none enumerated |
| 0x0c | vsync | uint8_t | presentation_interval == 1 |
| 0x0d | unknown_0d[3] | uint8_t | never written |

#### render_camera (size 0x54)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | position | real_point3d | (used) |
| 0x0c | forward | real_vector3d | (used) |
| 0x18 | up | real_vector3d |  |
| 0x24 | mirrored | uint8_t |  |
| 0x25 | unknown_25[3] | uint8_t |  |
| 0x28 | vertical_field_of_view | float |  |
| 0x2c | viewport_bounds | Rectangle2D | (used) top, left, bottom, right |
| 0x34 | window_bounds | Rectangle2D |  |
| 0x3c | z_near | float |  |
| 0x40 | z_far | float | (used) |
| 0x44 | mirror_plane | real_plane3d |  |

#### render_frustum (size 0x18c)

| offset | field | type | note |
|---|---|---|---|
| 0x000 | frustum_bounds[4] | float | real_rectangle2d; structures.h keeps a separate screen bounds pair in front of its copy |
| 0x010 | world_to_view | real_matrix4x3 | (used) |
| 0x044 | view_to_world | real_matrix4x3 | (used) |
| 0x078 | world_planes[6] | real_plane3d |  |
| 0x0d8 | z_near | float |  |
| 0x0dc | z_far | float |  |
| 0x0e0 | world_vertices[5] | real_point3d |  |
| 0x11c | world_midpoint | real_point3d |  |
| 0x128 | world_bounds | real_rectangle3d |  |
| 0x140 | projection_valid | uint8_t |  |
| 0x141 | unknown_141[3] | uint8_t |  |
| 0x144 | projection[4][4] | float | (used) |
| 0x184 | projection_world_to_screen | real_vector2d |  |

#### render_fog (size 0x50)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | flags | uint16_t | render_fog_flags |
| 0x02 | unknown_02 | uint16_t |  |
| 0x04 | atmospheric_color | ColorRGB | color_rgb_float_to_int in rasterizer_begin_frame |
| 0x10 | atmospheric_maximum_density | float | forced to 1.0 when not positive |
| 0x14 | atmospheric_minimum_distance | float | defaults to camera z_far |
| 0x18 | atmospheric_maximum_distance | float | defaults to twice camera z_far |
| 0x1c | planar_mode | int16_t | 0 none, 2 builds the plane through 0x44d9e0 (0x5176d0 reads and clears it as a word) |
| 0x1e | unknown_1e | int16_t |  |
| 0x20 | plane | real_plane3d | planar fog plane; defaults to the camera plane |
| 0x30 | planar_color | ColorRGB |  |
| 0x3c | planar_maximum_density | float |  |
| 0x40 | planar_maximum_distance | float |  |
| 0x44 | planar_maximum_depth | float |  |
| 0x48 | unknown_48 | uint32_t | no reader in this module |
| 0x4c | unknown_4c | uint32_t | no reader in this module |

#### render_screen_flash (size 0x18)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | type | int16_t | 0 none |
| 0x02 | unknown_02 | int16_t |  |
| 0x04 | intensity | float |  |
| 0x08 | color | ColorARGB |  |

#### rasterizer_window_parameters (size 0x258)

| offset | field | type | note |
|---|---|---|---|
| 0x000 | type | int16_t | (used) 1 is the main 3D pass; most draw paths test type == 1 (0x007c1220) |
| 0x002 | window_index | int16_t | (used) 0x007c1222; lens flare instances carry it and only draw in the matching window; -1 for the loading screen path at 0x50bdc0 |
| 0x004 | unknown_04 | uint8_t | (used) 0x007c1224; gates the second environment technique path 0x520b90/0x520e50 |
| 0x005 | clear_target | uint8_t | (used) 0 asks 0x52ccc0 to clear the new target |
| 0x006 | unknown_06 | uint16_t |  |
| 0x008 | camera | render_camera |  |
| 0x05c | frustum | render_frustum |  |
| 0x1e8 | fog | render_fog |  |
| 0x238 | screen_flash | render_screen_flash |  |
| 0x250 | unknown_250 | uint32_t | no reader in this module |
| 0x254 | unknown_254 | uint32_t | no reader in this module |

#### rasterizer_frame_time (size 0x10)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | time | double | (used, as a double) |
| 0x08 | unknown_08 | uint32_t | copied, never read here |
| 0x0c | unknown_0c | uint32_t | copied, never read here |

#### rasterizer_light (size 0x38)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | definition | uint32_t | void* Light tag data; +0x70 == -1 means no cube map, flags bit 0x10 first_person_flashlight |
| 0x04 | position | real_point3d |  |
| 0x10 | forward | real_vector3d | the spot direction |
| 0x1c | up | real_vector3d | second axis of the projected light basis |
| 0x28 | color | ColorRGB | NTSC luminance taken in 0x521750 |
| 0x34 | radius | float | 1/(r*r) in the fog plane cache, 2.5*r as the D3D range, r * Light.specular_radius_multiplier |

#### rasterizer_point_light_constants (size 0x30)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | position | real_point3d | rasterizer_light.position |
| 0x0c | inverse_radius_squared | float | 1 / (radius * radius) |
| 0x10 | forward | real_vector3d | rasterizer_light.forward |
| 0x1c | falloff_scale | float | 1 / (cos_falloff - cos_cutoff), 0 for omni lights |
| 0x20 | color | ColorRGB | rasterizer_light.color |
| 0x2c | falloff_offset | float | -scale * cos_cutoff, 1.0 for omni lights |

#### rasterizer_projected_light_constants (size 0x50)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | position | real_point3d |  |
| 0x0c | inverse_radius | float | 0.5 / radius |
| 0x10 | basis[3][4] | float | negated forward, cross product and up axes, each followed by 1.0 |
| 0x40 | cone_axis | real_vector3d | forward scaled by 1 / (r - r/2) |
| 0x4c | cone_offset | float |  |

#### render_distant_light (size 0x18)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | color | ColorRGB |  |
| 0x0c | direction | real_vector3d |  |

#### render_lighting (size 0x74)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | ambient_color | ColorRGB | (used) |
| 0x0c | distant_light_count | int16_t | (used) at most 2 |
| 0x0e | unknown_0e | int16_t |  |
| 0x10 | distant_lights[2] | render_distant_light | (used) |
| 0x40 | point_light_count | int16_t | (used) only the first two are uploaded (0x518ce0) |
| 0x42 | unknown_42 | int16_t |  |
| 0x44 | point_light_indices[2] | int32_t | rasterizer_light index, passed in EAX to 0x518c10 |
| 0x4c | reflection_tint | ColorARGB | hint only |
| 0x5c | shadow_vector | real_vector3d | hint only |
| 0x68 | shadow_color | ColorRGB | hint only |

#### rasterizer_skinning_matrix (size 0x30)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | rows[3][4] | float |  |

#### rasterizer_geometry_group_parameters (size 0x28)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | mode | int16_t | 0 plain, 1 blended environment pass (0x533850, 0x522c60), 2 batched pass drawn once per key |
| 0x02 | unknown_02 | int16_t |  |
| 0x04 | blend_factor | float | 1 - x in 0x522c60 when mode == 1; FUN_00526f50 tests it against 0 for mode 1 |
| 0x08 | distortion_factor | float | lerp factor into GlobalsRasterizerData camouflage values in FUN_00519f70 |
| 0x0c | sort_key | int32_t | copied into group.sort_key when mode != 0 |
| 0x10 | position | real_point3d | copied into group.position when mode != 0 |
| 0x1c | shader | uint32_t | void* Shader tag data of the overlay pass (0x52b050 tests shader_type 0xb and +0x2c) |
| 0x20 | unknown_20 | uint32_t |  |
| 0x24 | function_values | uint32_t | float* indexed [n - 1] by the overlay shader in 0x52b050 and 0x533850 |

#### rasterizer_model_draw_context (partial: at least 0xcc)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | flags | uint32_t | rasterizer_model_draw_flags; bits 8..23 also feed set_model_skinning |
| 0x04 | unknown_04 | uint32_t | copied into group +0x04 |
| 0x08 | node_matrices | uint32_t | real_matrix4x3* set_model_skinning reads scale/forward/left/up/position |
| 0x0c | node_count | int16_t |  |
| 0x0e | unknown_0e | int16_t |  |
| 0x10 | lighting | render_lighting | group.lighting points here |
| 0x84 | unknown_84[2] | uint32_t | group +0x74 points here; two dwords copied by FUN_00519f70 |
| 0x8c | group_parameters | rasterizer_geometry_group_parameters |  |
| 0xb4 | center | real_point3d | fog distance point in FUN_00526f50 |
| 0xc0 | unknown_c0 | uint32_t |  |
| 0xc4 | unknown_c4 | float | copied into group +0x3c |
| 0xc8 | unknown_c8 | float | copied into group +0x40 |

#### transparent_geometry_group (size 0xa8)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | flags | uint32_t | transparent_geometry_group_flags (from the caller or the model draw context) |
| 0x04 | unknown_04 | uint32_t | model draw context +0x04, else 0 |
| 0x08 | sort_key | int32_t | compare tiebreak; draw batches runs of equal keys |
| 0x0c | shader | uint32_t | void* Shader tag data; NULL means a callback group |
| 0x10 | shader_permutation | uint16_t | passed as the bitmap index to set_texture |
| 0x12 | unknown_12 | uint16_t | never written |
| 0x14 | parameters | rasterizer_geometry_group_parameters |  |
| 0x3c | unknown_3c | float | 1.0 unless copied from the context; pushed as a shader constant by 0x533850 and 0x519f70 |
| 0x40 | unknown_40 | float | 1.0 unless copied from the context; same readers |
| 0x44 | dynamic_index_slot | int32_t | rasterizer_dynamic_index_slot index; a negative value is minus a primitive kind (3 or 4 are quads) |
| 0x48 | index_buffer | uint32_t | rasterizer_index_buffer* static indices; for a callback group (shader NULL) this is the callback procedure |
| 0x4c | first_index | int32_t | callback argument for a callback group |
| 0x50 | primitive_count | int32_t |  |
| 0x54 | dynamic_vertex_slot | int32_t | rasterizer_dynamic_vertex_slot index, -1 none |
| 0x58 | vertex_buffer | uint32_t | rasterizer_vertex_buffer* static vertices; +0x14 is the lightmap stream when it points into a BSP material |
| 0x5c | lightmap_bitmap | uint32_t | void* BitmapData; its +0x28 texture gates the lightmap pass |
| 0x60 | node_matrices | uint32_t | real_matrix4x3* skinning source, NULL uses the identity at 0x0069673c |
| 0x64 | node_count | int16_t |  |
| 0x66 | unknown_66 | int16_t | never written |
| 0x68 | node_part_indices | uint32_t | uint8_t* 0x0071d19c when node_parts_bit is set |
| 0x6c | node_part_count | int32_t | 0x0071d1a0 |
| 0x70 | lighting | uint32_t | render_lighting* |
| 0x74 | lighting_extra | uint32_t | uint32_t* the two dwords at model draw context +0x84 |
| 0x78 | depth | float | -(camera.forward . (position - camera.position)); the primary sort key, +0.25 for some shaders |
| 0x7c | position | real_point3d |  |
| 0x88 | tint | ColorARGB | zero unless the caller passes one (0x522300) |
| 0x98 | sorted_index | int32_t | slot at allocation, sorted position after transparent_geometry_group_sort 0x5156d0 |
| 0x9c | previous_group_index | int16_t | drawn first when not -1; FUN_0052b180 hands out its address for the caller to link |
| 0x9e | next_group_index | int16_t | drawn after when not -1 |
| 0xa0 | unknown_a0 | int32_t | nonzero skips the draw unless forced; context sort_key in mode 1 |
| 0xa4 | unknown_a4 | uint8_t | never written |
| 0xa5 | first_person | uint8_t | 0x007c0478; compare sorts these last |
| 0xa6 | unknown_a6[2] | uint8_t | never written |

#### transparent_geometry_group_link (size 0x0c)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | previous_group_index | uint32_t | int16_t* &group->previous_group_index |
| 0x04 | next_group_index | uint32_t | int16_t* &group->next_group_index |
| 0x08 | group_index | int16_t | transparent_geometry_group_index_from_pointer, -1 none |
| 0x0a | unknown_0a | int16_t | not written |

#### lens_flare_instance (size 0x28)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | definition | uint32_t | void* LensFlare tag data |
| 0x04 | position | real_point3d |  |
| 0x10 | packed_direction | uint32_t | vector3d_pack_normal_11_11_10 0x5132d0 |
| 0x14 | packed_up | uint32_t | perpendicular axis, same packing |
| 0x18 | color | uint32_t | ARGB; alpha 0 culls, alpha byte scales brightness |
| 0x1c | object_index | int16_t | -1 for a BSP marker, otherwise compared with lens_flare_object_visibility.object_index |
| 0x1e | visibility_high | int16_t | object flares: visibility slot; BSP flares: bit 15 set and the high half of the marker offset |
| 0x20 | visibility_low | int16_t | BSP flares: low half of marker index + 8 |
| 0x22 | window_flags | uint8_t | lens_flare_instance_window_flags; 0x007c310a at create |
| 0x23 | intensity | uint8_t | lerp factor between the two brightness bounds |
| 0x24 | sample_count | int32_t | occlusion samples, FUN_00513ba0; <= 0 skips |

#### lens_flare_object_visibility (size 0x0a)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | object_index | int16_t |  |
| 0x02 | visibility[8] | uint8_t | indexed by window index |

#### lens_flare_vertex (size 0x20)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | x | float |  |
| 0x04 | y | float |  |
| 0x08 | z | float |  |
| 0x0c | rhw | float |  |
| 0x10 | diffuse | uint32_t |  |
| 0x14 | specular | uint32_t | 0x0069e708 |
| 0x18 | u | float |  |
| 0x1c | v | float |  |

#### lens_flare_batch_key (size 0x10)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | bitmap_tag_index | int32_t | low half passed to 0x5187e0 / 0x518a60 |
| 0x04 | second_bitmap_tag_index | int32_t | -1 selects the single texture path |
| 0x08 | bitmap_index | int32_t |  |
| 0x0c | shader_stage_config | uint16_t | handed to rasterizer_set_shader_stage_config |
| 0x0e | unknown_0e | uint16_t | compared but never set on its own |

#### lens_flare_batch (size 0x18018)

| offset | field | type | note |
|---|---|---|---|
| 0x00000 | vertices[0xc00] | lens_flare_vertex |  |
| 0x18000 | vertex_count | int32_t | 0x0075efc0 |
| 0x18004 | key | lens_flare_batch_key | 0x0075efc4 |
| 0x18014 | last_used | uint32_t | 0x0075efd4, LRU stamp from 0x00746fa8 |

#### rasterizer_detail_object_draw (size 0x18)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | first_instance | int32_t | index of 6 byte instance records (see below) |
| 0x04 | quad_count | int32_t | two triangles each; the fill caps a frame at 0x1000 |
| 0x08 | cell_x | int16_t | instance x = cell_x * 8 + byte / 255 * 8 |
| 0x0a | cell_y | int16_t |  |
| 0x0c | base_z | float | instance z = (base_z + plane . (x, y, z, 1)) * 8 |
| 0x10 | first_vertex | int32_t | DrawPrimitive start vertex, written by the fill |
| 0x14 | z_reference | uint32_t | float[4]* plane the instance bytes are projected on |

#### rasterizer_detail_object_instance (size 0x06)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | x | uint8_t |  |
| 0x01 | y | uint8_t |  |
| 0x02 | z | uint8_t |  |
| 0x03 | type_and_sprite | uint8_t |  |
| 0x04 | packed_normal | uint16_t |  |

#### rasterizer_detail_object_vertex (size 0x14)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | position | real_point3d |  |
| 0x0c | normal | uint32_t | widened packed normal |
| 0x10 | sprite | uint32_t | 0x01SSTTCC: sprite index, type index, corner |

#### rasterizer_detail_object_batch (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | draws | uint32_t | rasterizer_detail_object_draw* |
| 0x04 | draw_count | int16_t |  |
| 0x06 | collection_palette_index | int16_t | Scenario.detail_object_collection_palette index |

#### rasterizer_detail_object_batches (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | batches | uint32_t | rasterizer_detail_object_batch* |
| 0x04 | batch_count | int16_t |  |
| 0x06 | unknown_06 | int16_t |  |

#### font_glyph_cache_entry (size 0x08)

| offset | field | type | note |
|---|---|---|---|
| 0x00 | character | uint32_t | void* FontCharacter tag data, NULL when free |
| 0x04 | x | int16_t | atlas position plus the one texel border |
| 0x06 | y | int16_t |  |

#### font_glyph_cache (size 0x1010)

| offset | field | type | note |
|---|---|---|---|
| 0x0000 | initialized | uint8_t |  |
| 0x0001 | unknown_0001 | uint8_t |  |
| 0x0002 | oldest_slot | uint16_t | ring read index, masked 0x1ff |
| 0x0004 | next_slot | uint16_t | ring write index |
| 0x0006 | cursor_x | int16_t |  |
| 0x0008 | cursor_y | int16_t |  |
| 0x000a | row_height | int16_t | tallest glyph of the current row plus 2 |
| 0x000c | atlas | uint32_t | void* BitmapData (GlobalAlloc 0x30) with pixels at +0x2c |
| 0x0010 | entries[0x200] | font_glyph_cache_entry |  |

## Known gaps

**Code that Ghidra never defined as a function.** None of it is in the 228-function list, and it
needs a function defined in Ghidra before it can be rewritten:
- `0x51e2a0`, `0x51e570`, `0x51e8f0`: the three variants of the `0x007c0490` environment
  procedure, stored by `rasterizer_select_hardware_codepaths` 0x516810.
- `0x51fad0`, `0x51fd80`: the fixed-function self-illumination procedures stored in
  `0x007c048c`.
- `0x5276c0`, `0x527ae0`, `0x528050`: the shader_model / environment draws that
  `rasterizer_shader_environment_select_draw_functions` 0x52b630 installs in `0x007c0470`.
  About 5 KB of code in `0x5276c0 .. 0x528ae0`.

**Ghidra "functions" that are not functions (no files):**
- `0x51acd0` and `0x51ad91` are switch cases of `rasterizer_decals_draw_cluster` 0x51aa50.
- `0x52b340` is the tail of `rasterizer_transparent_geometry_group_build` 0x52b180.
- `0x518d40` (Ghidra and OpenSauce call it `render_objects_transparent`) is the target of a
  `jmp` inside `rasterizer_prepare_lighting_constants` 0x518ce0. The two halves are one
  function and one file.

**Files still below 0.5 rewrite confidence (24):** `lens_flare_update_visibility` 0x513780 (0.45), `lens_flare_update_samples` 0x513ba0 (0.4), `lens_flare_render_all` 0x513cf0 (0.25), `bitmap_compute_mipmap_count` 0x5145a0 (0.4), `chimera__draw_8_bit_text` 0x5148b0 (0.2), `chimera__draw_16_bit_text` 0x514ab0 (0.2), `rasterizer_render_loading_screen` 0x5157e0 (0.3), `rasterizer_display_mode_differs` 0x515d10 (0.4), `rasterizer_device_reset` 0x515d90 (0.3), `rasterizer_select_hardware_codepaths` 0x516810 (0.35), `chimera__cinematic_screen_effect` 0x517470 (0.45), `chimera__rasterizer_set_frustum_z_func` 0x518f40 (0.35), `rasterizer_editbox_log_dump` 0x5196b0 (0.35), `rasterizer_ksml_ui_shutdown` 0x5198a0 (0.35), `rasterizer_resource_file_verify_signature` 0x519980 (0.3), `rasterizer_glass_diffuse_draw` 0x523690 (0.45), `rasterizer_glass_diffuse_draw_fixed_function` 0x523d10 (0.2), `rasterizer_bitmap_upload_2d_mipmaps` 0x524100 (0.35), `rasterizer_bitmap_upload_cubemap_mipmaps` 0x524270 (0.2), `rasterizer_bitmap_upload_cubemap_mipmaps_by_face` 0x5243c0 (0.15), `rasterizer_bitmap_sample_texel` 0x524590 (0.25), `rasterizer_vertex_buffer_create` 0x524980 (0.15).
- These are transliterations of the Ghidra output. Their register arguments and call sites are
  consistent with their definitions, but their bodies were not checked instruction by
  instruction.

**Uninitialised reads kept from the original:**
- The effect index of the reflection and mirror passes for reflection types above 2.
- The vertex shader variant of the glass reflection for vertex types other than 0/2/4.
- c12.x and c12.z of the glass reflection constants.
- The w components of the lighting block (c21..c25), which are zeroed in the C.
- The two unwritten pixel constant registers c4/c5 of the multitexture UI quad path.

**Undefined behaviour kept or avoided:**
- `rasterizer_misc_vertex_buffer_create` 0x534e50 goes on to Lock a null buffer when creation
  fails. The C returns early instead.
- `rasterizer_shader_transparent_chicago_extended_set_texture_stages` 0x537d60 copies map
  pointers into a four-entry stack array without a bound check.

**Types:**
- `rasterizer_model_draw_context` is partial; the caller that fills it is in the render module.
- `rasterizer_gamma_settings` is an opaque byte typedef.
- The three flag bytes at `ui_quad_render_state + 8`, which pick the u or v axis per map, have
  no name in `types/interface.h` (they are `unknown_08` there).

**Externs in other modules that disagree with the definitions here.** These are outside this
module's scope and are listed for their owners:
- `src/cache/model_load_vertex_buffers.c` declares `rasterizer_index_buffer_create` 0x525030
  with the phase 2 signature. The real one is EAX count, DX type, stack (out, source).
- `src/cache/texture_cache_get.c` notes the bitmap in EBX. Every rasterizer caller and the
  prologue `mov esi,eax` at 0x444556 pass it in EAX.
- `src/interface/first_person_weapon_update_screen_effects.c` still uses the phase 2 names for
  0x52d8a0 and 0x52e2d0.
- 29 more externs use `FUN_xxxxxxxx` names for functions that are named here: the 11:11:10
  normal packer, the dynamic caches, the detail objects, the motion sensor trio,
  `rasterizer_ui_quad_draw`, and the model draw states.

**Names that are still hints only** (name confidence below 0.5) are marked in the table below.
Notably:
- the "light cone" family (0x51d6a0 / 0x51da20 / 0x51dc50), which draws lit environment
  geometry per light;
- the lens flare batch functions 0x536b70..0x537130.

## Phase 4 review changes

### First half (`0x5132b0 .. 0x524980`, earlier review)

- **Unwritten functions.** Wrote the 9 that the first rewriter left out: 0x514ed0, 0x5169c0,
  0x5176d0, 0x517b90, 0x519b00, 0x519f70, 0x51aa50, 0x51b3f0, 0x51b890.
- **Rewrites from the raw code.** Replaced these wrong or placeholder bodies:
  - the texture binders;
  - the static/dynamic draw family;
  - the detail object trio;
  - nine shader_environment passes;
  - the glass reflection;
  - the lens flare marker instancing;
  - the decal vertex cache.

### Second half (`0x525030 .. 0x537d60`, this review)

**Written.** Wrote the functions the two rewriters left as placeholders or unwritten:
- the transparent group draw 0x533850;
- the motion sensor trio 0x52b690 / 0x52bad0 / 0x52bc40;
- the plasma draw 0x52c4a0;
- the sun glow render 0x525ab0;
- render target initialize 0x52ca20;
- the screen effect UV transform, render and fixed-function render (0x52ce50, 0x52d8a0, 0x52e2d0);
- the three shader_model draws (0x528be0, 0x529230, 0x529e00);
- the two chicago draws (0x531ed0, 0x532a40).

**Rewrites of misidentified families.** Rewrote from the raw code four families that the
rewriters had misidentified:
- **0x530830 / 0x530ff0 / 0x531350 / 0x531570** are the dynamic object shadow, not a "motion
  sensor HUD". Their callers are the shadow code 0x50f830 and the model draw 0x4d72a0, plus the
  BSP polygon callback 0x511f50.
- **0x537bb0 / 0x537d60** are the chicago fixed-function combiners, not "object lights vertex
  shader constants".
- **0x531b80 / 0x531e90** bracket the chicago debug text draws, not a first-person model.
- **0x51c9a0** is the HUD/UI quad draw for every interface quad builder, not a lens flare element.

**Corrected in place against the raw code:**
- the sun glow capture and blur (ping-pong direction, target activation, COLORWRITE, constants);
- the transparent group build (link offset, mode test, allocation);
- the environment draw dispatch and draw-function selection;
- the geometry part draw and group vertex draw arguments;
- the index buffer create (EAX count, DX type, pool);
- the light set (cube map test, cross product order);
- the environment technique table (effect per stage, table sizes);
- `GetTechniqueByName`, which takes one argument;
- the screen effect shader init (effect 114);
- the shader initialisation return value (a success byte, not a masked handle), plus the elided
  `c_light_enhancement` handle;
- `D3DXFVFFromDeclarator` for declaration 15;
- the occlusion test (no ESI argument; a real screen quad; `floor`);
- `rasterizer_capture_and_present` (EAX tile, stack bitmap, four-argument Present);
- the lighting block 0x518ce0 (merged with its tail 0x518d40);
- `lens_flare_compute_rotation` (ESI flare, DI mode, no phantom arguments);
- the gamma ramp (both flags must be set to skip; `pow` is `_CIpow` on the x87 stack) and the
  brightness exponent (x^(ln 0.5 / ln (128/255)) * 255);
- the misc vertex buffer pool;
- the render target effect texture index width;
- the text begin state (vertex declaration 8, and the blend mode in CX).

**Consistency.**
- One name and one type per address across the module: 33 extern renames, 42 declarations
  canonicalised, and every placeholder call site given its real arguments.
- Renames were appended to `symbols/agent_phase4_rasterizer.txt`.
- `d3dx_macro`, `d3d_vertex_element9`, `rasterizer_dynamic_screen_vertex` and
  `transparent_geometry_group_link` were folded into `types/rasterizer.h`.
- The duplicate `d3d_display_mode` / `win32_rect` and the opaque
  `lens_flare_render_element_context` were dropped in favour of `types/interface.h`.

## Functions

The table lists all 224 functions by address.
- **rewrite conf** is the confidence given in each file header: 0.9 and up means checked
  instruction by instruction against the raw code, and below 0.5 means a Ghidra transliteration.
- **UNSURE** counts the open notes in the file header.
- **raw code checked in phase 4** marks the 101 files that were written or corrected
  against the disassembly during the review.

| address | function | size | name conf | rewrite conf | UNSURE | raw code checked in phase 4 |
|---|---|---|---|---|---|---|
| 0x5132b0 | color_channel_real_to_byte | 27 | 0.5 | 0.85 | 0 |  |
| 0x5132d0 | vector3d_pack_normal_11_11_10 | 291 | 0.6 | 0.55 | 1 |  |
| 0x513400 | vector3d_unpack_normal_11_11_10 | 132 | 0.5 | 0.7 | 0 |  |
| 0x513490 | bsp_compressed_rendered_vertex_unpack_normal (was lens_flare_unpack_direction; orphan pass 4 review) | 34 | 0.7 | 0.9 | 0 |  |
| 0x5134c0 | bsp_compressed_lightmap_vertex_unpack_normal (was lens_flare_unpack_up; orphan pass 4 review) | 33 | 0.7 | 0.9 | 0 |  |
| 0x5134f0 | lens_flare_get_visibility_byte | 73 | 0.5 | 0.6 | 0 |  |
| 0x513540 | lens_flare_compute_rotation | 556 | 0.5 | 0.9 | 0 | yes |
| 0x513780 | lens_flare_update_visibility | 277 | 0.5 | 0.45 | 5 |  |
| 0x5138a0 | lens_flare_add_instance | 342 | 0.55 | 0.5 | 3 |  |
| 0x513a00 | structure_cluster_add_lens_flares | 408 | 0.5 | 0.9 | 3 | yes |
| 0x513ba0 | lens_flare_update_samples | 334 | 0.45 | 0.4 | 6 |  |
| 0x513cf0 | lens_flare_render_all | 2147 | 0.55 | 0.25 | 21 |  |
| 0x514560 | chimera__rasterizer_memory_alloc | 59 | 0.6 | 0.75 | 0 |  |
| 0x5145a0 | bitmap_compute_mipmap_count | 287 | 0.5 | 0.4 | 6 |  |
| 0x5146c0 | bitmap_compute_texture_data_size | 343 | 0.55 | 0.7 | 1 |  |
| 0x514820 | text_font_system_initialize | 143 | 0.55 | 0.6 | 0 | yes |
| 0x5148b0 | chimera__draw_8_bit_text | 511 | 0.55 | 0.2 | 5 |  |
| 0x514ab0 | chimera__draw_16_bit_text | 511 | 0.55 | 0.2 | 10 |  |
| 0x514cb0 | font_glyph_cache_clear_all | 44 | 0.55 | 0.75 | 0 |  |
| 0x514ed0 | font_glyph_cache_allocate_and_upload | 745 | 0.5 | 0.75 | 1 | yes |
| 0x5151c0 | transparent_geometry_pool_initialize | 109 | 0.55 | 0.75 | 3 |  |
| 0x515230 | transparent_geometry_group_allocate | 46 | 0.55 | 0.85 | 0 |  |
| 0x515260 | transparent_geometry_group_allocate_secondary | 43 | 0.55 | 0.85 | 0 |  |
| 0x515290 | transparent_geometry_group_get_next_sorted | 54 | 0.4 | 0.7 | 0 |  |
| 0x5152d0 | transparent_geometry_group_index_from_pointer | 53 | 0.55 | 0.85 | 0 |  |
| 0x515310 | transparent_geometry_group_test_drawn_bit | 94 | 0.4 | 0.7 | 0 |  |
| 0x515370 | transparent_geometry_group_set_drawn_bit | 129 | 0.4 | 0.7 | 0 |  |
| 0x515400 | transparent_geometry_group_get_vertex_type_reference | 33 | 0.35 | 0.55 | 0 |  |
| 0x515430 | chimera__rasterizer_dispose_free_memory | 100 | 0.6 | 0.75 | 0 |  |
| 0x5154a0 | transparent_geometry_group_draw_all | 251 | 0.6 | 0.5 | 1 |  |
| 0x5155b0 | transparent_geometry_group_compare | 287 | 0.5 | 0.55 | 1 |  |
| 0x5156d0 | transparent_geometry_group_sort | 109 | 0.55 | 0.8 | 0 |  |
| 0x515740 | decal_and_font_system_reset | 148 | 0.55 | 0.55 | 1 |  |
| 0x5157e0 | rasterizer_render_loading_screen | 321 | 0.4 | 0.3 | 4 |  |
| 0x515930 | rasterizer_create_game_window | 490 | 0.7 | 0.7 | 1 |  |
| 0x515b20 | rasterizer_resize_game_window | 271 | 0.55 | 0.65 | 7 |  |
| 0x515c30 | rasterizer_get_capture_surface | 43 | 0.4 | 0.6 | 0 |  |
| 0x515c70 | rasterizer_get_refresh_rate | 42 | 0.4 | 0.5 | 0 |  |
| 0x515ca0 | display_mode_get_current | 97 | 0.5 | 0.55 | 0 |  |
| 0x515d10 | rasterizer_display_mode_differs | 124 | 0.4 | 0.4 | 4 |  |
| 0x515d90 | rasterizer_device_reset | 547 | 0.55 | 0.3 | 3 |  |
| 0x515fc0 | rasterizer_build_present_parameters | 261 | 0.55 | 0.55 | 6 |  |
| 0x5160d0 | rasterizer_set_default_render_states | 1845 | 0.6 | 0.7 | 1 |  |
| 0x516810 | rasterizer_select_hardware_codepaths | 166 | 0.5 | 0.35 | 4 |  |
| 0x5168c0 | rasterizer_parse_vidmode_commandline | 252 | 0.65 | 0.6 | 3 |  |
| 0x5169c0 | rasterizer_initialize_direct3d | 2707 | 0.6 | 0.6 | 12 | yes |
| 0x517470 | chimera__cinematic_screen_effect | 139 | 0.45 | 0.45 | 6 |  |
| 0x517500 | rasterizer_reset_device_if_needed | 177 | 0.5 | 0.55 | 3 |  |
| 0x5175c0 | rasterizer_begin_frame | 266 | 0.5 | 0.6 | 4 |  |
| 0x5176d0 | rasterizer_set_fog_constants | 1201 | 0.6 | 0.8 | 0 | yes |
| 0x517b90 | rasterizer_end_frame | 1339 | 0.6 | 0.75 | 4 | yes |
| 0x5180d0 | rasterizer_service_deferred_windowed_ops | 89 | 0.4 | 0.6 | 0 |  |
| 0x518130 | rasterizer_unbind_stream_and_textures | 78 | 0.45 | 0.6 | 0 |  |
| 0x518180 | rasterizer_capture_and_present | 704 | 0.55 | 0.85 | 1 | yes |
| 0x518450 | rasterizer_shutdown | 370 | 0.6 | 0.55 | 2 |  |
| 0x5185d0 | chimera__rasterizer_set_framebuffer_blend_function | 171 | 0.6 | 0.6 | 4 |  |
| 0x518680 | rasterizer_bind_texture_d3d9 | 57 | 0.5 | 0.95 | 0 | yes |
| 0x5186c0 | rasterizer_bind_texture_d3dx | 54 | 0.5 | 0.95 | 0 | yes |
| 0x518700 | chimera__rasterizer_set_texture_direct_d3dx | 100 | 0.6 | 0.9 | 0 | yes |
| 0x518770 | chimera__rasterizer_set_texture_direct_d3d9 | 100 | 0.6 | 0.9 | 0 | yes |
| 0x5187e0 | rasterizer_validate_and_rebind_texture | 118 | 0.4 | 0.9 | 0 | yes |
| 0x518860 | rasterizer_resolve_and_cache_submap_b | 251 | 0.4 | 0.9 | 0 | yes |
| 0x518960 | chimera__rasterizer_set_texture | 245 | 0.6 | 0.9 | 0 | yes |
| 0x518a60 | rasterizer_resolve_and_cache_submap_c | 217 | 0.4 | 0.85 | 0 | yes |
| 0x518b40 | chimera__rasterizer_set_model_skinning | 195 | 0.55 | 0.75 | 0 |  |
| 0x518c10 | rasterizer_light_set_point_constants | 195 | 0.4 | 0.6 | 0 |  |
| 0x518ce0 | rasterizer_prepare_lighting_constants | 603 | 0.5 | 0.9 | 1 | yes |
| 0x518f40 | chimera__rasterizer_set_frustum_z_func | 704 | 0.55 | 0.35 | 15 |  |
| 0x519200 | rasterizer_set_shader_stage_config | 704 | 0.6 | 0.75 | 2 |  |
| 0x5194e0 | rasterizer_apply_decal_zbias | 77 | 0.45 | 0.75 | 2 |  |
| 0x519530 | chimera__transparent_decal_zbias | 77 | 0.5 | 0.75 | 2 |  |
| 0x519580 | rasterizer_clear_decal_zbias | 67 | 0.55 | 0.85 | 0 |  |
| 0x5195d0 | rasterizer_decal_zbias_active | 21 | 0.5 | 0.85 | 0 |  |
| 0x5195f0 | rasterizer_round_up_resolution_height | 182 | 0.55 | 0.85 | 0 |  |
| 0x5196b0 | rasterizer_editbox_log_dump | 484 | 0.6 | 0.35 | 7 |  |
| 0x5198a0 | rasterizer_ksml_ui_shutdown | 213 | 0.55 | 0.35 | 6 |  |
| 0x519980 | rasterizer_resource_file_verify_signature | 107 | 0.4 | 0.3 | 5 |  |
| 0x5199f0 | rasterizer_load_file_and_verify | 195 | 0.6 | 0.7 | 0 |  |
| 0x519ac0 | rasterizer_transparent_decals_enabled | 59 | 0.55 | 0.6 | 0 |  |
| 0x519b00 | rasterizer_render_target_capture_frame | 1123 | 0.5 | 0.8 | 0 | yes |
| 0x519f70 | rasterizer_transparent_geometry_group_draw_active_camouflage | 1768 | 0.7 | 0.7 | 2 | yes |
| 0x51a6a0 | rasterizer_decals_initialize | 201 | 0.9 | 0.9 | 4 | yes |
| 0x51a770 | rasterizer_decal_vertex_cache_lock | 150 | 0.5 | 0.9 | 1 | yes |
| 0x51a810 | rasterizer_decal_pass_begin | 562 | 0.4 | 0.5 | 2 |  |
| 0x51aa50 | rasterizer_decals_draw_cluster | 826 | 0.55 | 0.75 | 1 | yes |
| 0x51b0e0 | rasterizer_end_decal_pass | 105 | 0.45 | 0.6 | 1 |  |
| 0x51b150 | rasterizer_detail_objects_expand_quad_vertices | 543 | 0.6 | 0.9 | 0 | yes |
| 0x51b370 | rasterizer_detail_object_vertex_buffer_create | 117 | 0.7 | 0.95 | 0 | yes |
| 0x51b3f0 | rasterizer_detail_objects_begin | 758 | 0.6 | 0.9 | 2 | yes |
| 0x51b6f0 | rasterizer_detail_objects_vertex_buffer_fill | 406 | 0.6 | 0.85 | 2 | yes |
| 0x51b890 | rasterizer_detail_objects_draw | 761 | 0.6 | 0.85 | 1 | yes |
| 0x51bb90 | rasterizer_decal_index_buffer_initialize | 284 | 0.5 | 0.5 | 3 |  |
| 0x51bcd0 | rasterizer_dynamic_geometry_dispose | 137 | 0.5 | 0.8 | 0 |  |
| 0x51bd60 | rasterizer_dynamic_index_cache_reserve | 109 | 0.7 | 0.8 | 0 |  |
| 0x51bdd0 | rasterizer_dynamic_vertex_cache_reserve | 106 | 0.7 | 0.75 | 0 |  |
| 0x51be40 | rasterizer_dynamic_vertex_cache_lock | 113 | 0.6 | 0.55 | 1 |  |
| 0x51bec0 | rasterizer_dynamic_vertex_draw | 455 | 0.35 | 0.9 | 0 | yes |
| 0x51c090 | rasterizer_dynamic_index_cache_draw | 295 | 0.5 | 0.9 | 0 | yes |
| 0x51c1c0 | chimera__rasterizer_draw_dynamic_triangles_static_vertices | 324 | 0.55 | 0.9 | 0 | yes |
| 0x51c310 | chimera__rasterizer_draw_dynamic_triangles_static_vertices2 | 383 | 0.5 | 0.9 | 0 | yes |
| 0x51c490 | rasterizer_dynamic_vertex_draw_indexed | 338 | 0.4 | 0.9 | 0 | yes |
| 0x51c5f0 | rasterizer_dynamic_geometry_chain_draw | 312 | 0.3 | 0.9 | 0 | yes |
| 0x51c730 | rasterizer_dynamic_geometry_draw_dispatch | 82 | 0.5 | 0.9 | 0 | yes |
| 0x51c790 | rasterizer_dynamic_vertex_process_and_get_handle | 151 | 0.3 | 0.85 | 1 | yes |
| 0x51c830 | rasterizer_transparent_object_append | 361 | 0.5 | 0.55 | 1 |  |
| 0x51c9a0 | rasterizer_ui_quad_draw | 3292 | 0.6 | 0.8 | 2 | yes |
| 0x51d6a0 | rasterizer_light_cone_set_texture_stage_states | 889 | 0.4 | 0.7 | 1 |  |
| 0x51da20 | rasterizer_light_cone_set_orientation_constants | 553 | 0.4 | 0.5 | 4 | yes |
| 0x51dc50 | rasterizer_light_cone_draw | 660 | 0.35 | 0.85 | 1 | yes |
| 0x51def0 | rasterizer_fog_screen_overlay_set_states | 934 | 0.4 | 0.75 | 2 |  |
| 0x51e9f0 | rasterizer_force_bilinear_filtering | 295 | 0.45 | 0.85 | 0 |  |
| 0x51eb20 | rasterizer_water_fade_compute_and_set_states | 818 | 0.35 | 0.6 | 4 |  |
| 0x51ee60 | rasterizer_water_ripple_draw | 456 | 0.3 | 0.85 | 1 | yes |
| 0x51f030 | rasterizer_underwater_tint_set_states | 729 | 0.45 | 0.85 | 2 |  |
| 0x51f310 | rasterizer_underwater_tint_jitter_update | 200 | 0.4 | 0.7 | 2 | yes |
| 0x51f3e0 | rasterizer_shader_environment_self_illumination_draw | 1764 | 0.7 | 0.8 | 1 | yes |
| 0x520020 | rasterizer_shader_decal_pass_set_states | 708 | 0.35 | 0.9 | 0 |  |
| 0x5202f0 | rasterizer_shader_environment_reflection_draw | 1172 | 0.7 | 0.85 | 2 | yes |
| 0x520790 | rasterizer_shader_environment_technique_multipurpose_set_states | 369 | 0.35 | 0.9 | 2 |  |
| 0x520910 | rasterizer_shader_environment_set_lightmap | 91 | 0.6 | 0.9 | 1 | yes |
| 0x520970 | rasterizer_shader_environment_technique_draw | 539 | 0.35 | 0.85 | 1 | yes |
| 0x520b90 | rasterizer_shader_environment_technique_self_illumination_set_states | 699 | 0.35 | 0.9 | 0 |  |
| 0x520e50 | rasterizer_shader_environment_dynamic_mirror_draw | 1144 | 0.65 | 0.85 | 1 | yes |
| 0x5212d0 | rasterizer_shader_environment_technique_ps2_set_states | 726 | 0.35 | 0.9 | 0 |  |
| 0x5215b0 | rasterizer_projected_light_constants_build_cube_map | 403 | 0.55 | 0.7 | 1 |  |
| 0x521750 | rasterizer_projected_light_constants_build | 418 | 0.55 | 0.65 | 1 |  |
| 0x521900 | rasterizer_shader_environment_projected_light_draw | 946 | 0.65 | 0.85 | 0 | yes |
| 0x521cc0 | rasterizer_dynamic_light_technique_ps2_set_states | 720 | 0.3 | 0.9 | 1 |  |
| 0x521f90 | rasterizer_shader_environment_lightmap_specular_draw | 866 | 0.6 | 0.85 | 1 | yes |
| 0x522300 | rasterizer_transparent_geometry_group_new | 541 | 0.5 | 0.55 | 2 | yes |
| 0x522520 | chimera__registry_check_4 | 399 | 0.9 | 0.8 | 4 |  |
| 0x5226c0 | chimera__registry_check_3 | 210 | 0.85 | 0.85 | 0 |  |
| 0x5227a0 | chimera__gamma | 233 | 0.85 | 0.85 | 0 | yes |
| 0x522890 | rasterizer_gamma_brightness_to_exponent | 158 | 0.45 | 0.85 | 1 | yes |
| 0x522930 | rasterizer_glass_tint_draw | 802 | 0.4 | 0.5 | 3 | yes |
| 0x522c60 | rasterizer_glass_reflection_draw | 2596 | 0.65 | 0.8 | 3 | yes |
| 0x523690 | rasterizer_glass_diffuse_draw | 743 | 0.4 | 0.45 | 3 |  |
| 0x523980 | rasterizer_glass_tint_draw_fixed_function | 521 | 0.4 | 0.55 | 0 | yes |
| 0x523b90 | rasterizer_glass_reflection_draw_fixed_function | 372 | 0.4 | 0.7 | 0 | yes |
| 0x523d10 | rasterizer_glass_diffuse_draw_fixed_function | 428 | 0.4 | 0.2 | 2 |  |
| 0x523ec0 | rasterizer_glass_draw_procedures_select | 74 | 0.45 | 0.85 | 0 | yes |
| 0x523f10 | rasterizer_bitmap_compute_mipmap_skip_count | 133 | 0.5 | 0.75 | 1 |  |
| 0x523fa0 | rasterizer_bitmap_create_hardware_texture | 341 | 0.35 | 0.55 | 2 |  |
| 0x524100 | rasterizer_bitmap_upload_2d_mipmaps | 363 | 0.4 | 0.35 | 7 |  |
| 0x524270 | rasterizer_bitmap_upload_cubemap_mipmaps | 322 | 0.4 | 0.2 | 7 |  |
| 0x5243c0 | rasterizer_bitmap_upload_cubemap_mipmaps_by_face | 456 | 0.35 | 0.15 | 2 |  |
| 0x524590 | rasterizer_bitmap_sample_texel | 983 | 0.5 | 0.25 | 9 |  |
| 0x524980 | rasterizer_vertex_buffer_create | 1670 | 0.55 | 0.15 | 2 |  |
| 0x525030 | rasterizer_index_buffer_create | 256 | 0.6 | 0.95 | 0 | yes |
| 0x525130 | rasterizer_sun_glow_project_point | 483 | 0.4 | 0.7 | 2 |  |
| 0x525320 | rasterizer_sun_glow_capture | 1021 | 0.55 | 0.9 | 1 | yes |
| 0x525720 | rasterizer_sun_glow_blur | 898 | 0.55 | 0.9 | 0 | yes |
| 0x525ab0 | rasterizer_sun_glow_render | 3148 | 0.6 | 0.8 | 1 | yes |
| 0x526700 | rasterizer_light_disable_all | 93 | 0.55 | 0.85 | 0 |  |
| 0x526760 | rasterizer_light_set | 452 | 0.5 | 0.55 | 3 | yes |
| 0x526930 | rasterizer_shader_environment_build_technique_table | 953 | 0.65 | 0.9 | 1 | yes |
| 0x526cf0 | chimera__rasterizer_set_up_node_parts | 607 | 0.6 | 0.75 | 0 |  |
| 0x526f50 | rasterizer_model_draw_prepare_states | 1456 | 0.45 | 0.8 | 9 | yes |
| 0x527500 | rasterizer_shader_model_select_technique | 408 | 0.65 | 0.9 | 1 | yes |
| 0x528ae0 | rasterizer_geometry_draw_fixed_function | 251 | 0.55 | 0.95 | 0 | yes |
| 0x528be0 | rasterizer_shader_model_draw_limited | 1614 | 0.55 | 0.85 | 0 | yes |
| 0x529230 | rasterizer_shader_model_draw_fixed_function | 3020 | 0.55 | 0.8 | 1 | yes |
| 0x529e00 | rasterizer_shader_model_draw_pixel_shader | 4674 | 0.55 | 0.75 | 5 | yes |
| 0x52b050 | rasterizer_shader_environment_draw_dispatch | 296 | 0.4 | 0.85 | 1 | yes |
| 0x52b180 | rasterizer_transparent_geometry_group_build | 515 | 0.5 | 0.85 | 1 | yes |
| 0x52b530 | rasterizer_model_draw_restore_states | 255 | 0.4 | 0.75 | 2 | yes |
| 0x52b630 | rasterizer_shader_environment_select_draw_functions | 84 | 0.55 | 0.95 | 0 | yes |
| 0x52b690 | rasterizer_motion_sensor_begin | 1081 | 0.6 | 0.85 | 1 | yes |
| 0x52bad0 | rasterizer_motion_sensor_blip_draw | 362 | 0.6 | 0.85 | 0 | yes |
| 0x52bc40 | rasterizer_motion_sensor_end | 2141 | 0.6 | 0.85 | 0 |  |
| 0x52c4a0 | rasterizer_shader_transparent_plasma_draw | 1402 | 0.7 | 0.85 | 1 |  |
| 0x52ca20 | rasterizer_render_target_initialize | 547 | 0.5 | 0.9 | 1 | yes |
| 0x52cc50 | rasterizer_render_target_dispose | 111 | 0.45 | 0.85 | 0 |  |
| 0x52ccc0 | rasterizer_render_target_set_active | 258 | 0.45 | 0.8 | 1 | yes |
| 0x52cdd0 | rasterizer_render_target_bind_texture_stage | 50 | 0.45 | 0.8 | 0 |  |
| 0x52ce10 | rasterizer_render_target_bind_effect_texture | 51 | 0.45 | 0.75 | 0 |  |
| 0x52ce50 | rasterizer_screen_effect_compute_uv_transform | 2286 | 0.6 | 0.85 | 2 | yes |
| 0x52d740 | rasterizer_screen_effect_init_shaders | 341 | 0.85 | 0.9 | 0 | yes |
| 0x52d8a0 | rasterizer_screen_effect_render | 2599 | 0.6 | 0.75 | 2 | yes |
| 0x52e2d0 | rasterizer_screen_effect_render_fixed_function | 2406 | 0.6 | 0.85 | 0 | yes |
| 0x52ec40 | rasterizer_screen_flash_init_shaders | 181 | 0.8 | 0.85 | 0 |  |
| 0x52ed00 | rasterizer_screen_flash_render | 2656 | 0.8 | 0.55 | 1 | yes |
| 0x52f780 | rasterizer_dx9_shaders_init_effect | 499 | 0.8 | 0.75 | 1 |  |
| 0x52f980 | rasterizer_dx9_pixel_shader_effect_load | 123 | 0.55 | 0.7 | 1 |  |
| 0x52fa00 | rasterizer_dx9_pixel_shaders_load_all | 168 | 0.6 | 0.7 | 0 |  |
| 0x52fab0 | rasterizer_dx9_shaders_initialize | 1192 | 0.7 | 0.85 | 1 | yes |
| 0x52ff60 | rasterizer_dx9_pixel_shaders_release | 281 | 0.6 | 0.75 | 0 |  |
| 0x530090 | rasterizer_dx9_shaders_release_all | 48 | 0.55 | 0.8 | 0 |  |
| 0x5300d0 | rasterizer_dx9_effects_initialize | 70 | 0.55 | 0.75 | 1 |  |
| 0x530120 | rasterizer_shader_technique_for_name | 143 | 0.75 | 0.7 | 1 | yes |
| 0x5301b0 | rasterizer_dx9_vertex_declarations_create | 897 | 0.6 | 0.7 | 0 | yes |
| 0x530540 | rasterizer_dx9_vertex_declarations_release | 47 | 0.65 | 0.85 | 0 |  |
| 0x530570 | rasterizer_dx9_create_vertex_buffer | 122 | 0.2 | 0.75 | 1 | yes |
| 0x5305f0 | rasterizer_vertex_buffer_slot_allocate | 160 | 0.55 | 0.75 | 1 | yes |
| 0x530690 | rasterizer_vertex_buffer_slot_recreate_lost | 69 | 0.55 | 0.8 | 0 |  |
| 0x5306e0 | rasterizer_dx9_vertex_shaders_load_all | 205 | 0.6 | 0.7 | 0 |  |
| 0x5307b0 | rasterizer_dx9_vertex_shaders_initialize | 66 | 0.6 | 0.75 | 0 |  |
| 0x530800 | rasterizer_dx9_vertex_shaders_reload | 41 | 0.55 | 0.8 | 0 |  |
| 0x530830 | rasterizer_object_shadow_blur | 1973 | 0.55 | 0.85 | 0 | yes |
| 0x530ff0 | rasterizer_object_shadow_begin | 850 | 0.55 | 0.85 | 1 | yes |
| 0x531350 | rasterizer_object_shadow_model_draw | 536 | 0.55 | 0.9 | 0 | yes |
| 0x531570 | rasterizer_object_shadow_structure_draw | 1337 | 0.5 | 0.85 | 0 | yes |
| 0x531ab0 | chimera__widescreen_text_scaling | 201 | 0.45 | 0.9 | 0 |  |
| 0x531b80 | rasterizer_draw_text_begin | 781 | 0.5 | 0.85 | 0 | yes |
| 0x531e90 | rasterizer_draw_text_end | 50 | 0.5 | 0.95 | 0 | yes |
| 0x531ed0 | rasterizer_shader_transparent_chicago_draw | 2882 | 0.7 | 0.8 | 1 | yes |
| 0x532a40 | rasterizer_shader_transparent_chicago_extended_draw | 3051 | 0.7 | 0.8 | 1 |  |
| 0x533660 | rasterizer_transparent_geometry_group_draw_vertices | 202 | 0.75 | 0.95 | 0 | yes |
| 0x533730 | rasterizer_geometry_part_draw | 281 | 0.5 | 0.95 | 0 | yes |
| 0x533850 | rasterizer_transparent_geometry_group_draw | 5552 | 0.7 | 0.7 | 3 | yes |
| 0x534e50 | rasterizer_misc_vertex_buffer_create | 296 | 0.45 | 0.6 | 1 | yes |
| 0x536b70 | rasterizer_lens_flare_batch_apply_material | 149 | 0.4 | 0.55 | 1 |  |
| 0x536c10 | rasterizer_lens_flare_batch_draw_slot | 109 | 0.45 | 0.7 | 0 |  |
| 0x536c80 | rasterizer_lens_flare_batch_flush_all | 46 | 0.5 | 0.85 | 0 |  |
| 0x536cb0 | rasterizer_lens_flare_batch_find_slot | 195 | 0.4 | 0.65 | 0 |  |
| 0x536d80 | rasterizer_lens_flare_project_to_screen | 491 | 0.5 | 0.7 | 0 | yes |
| 0x536f70 | rasterizer_lens_flare_occlusion_queries_create | 113 | 0.55 | 0.85 | 0 |  |
| 0x536ff0 | rasterizer_lens_flare_occlusion_sample_add | 314 | 0.4 | 0.7 | 0 |  |
| 0x537130 | rasterizer_lens_flare_batching_select_mode | 1054 | 0.4 | 0.7 | 3 | yes |
| 0x537550 | rasterizer_lens_flare_quad_add | 686 | 0.5 | 0.6 | 0 |  |
| 0x537800 | rasterizer_lens_flare_occlusion_test_issue | 826 | 0.5 | 0.9 | 0 | yes |
| 0x537b40 | rasterizer_lens_flare_occlusion_query_get_result | 104 | 0.55 | 0.6 | 2 |  |
| 0x537bb0 | rasterizer_shader_transparent_chicago_set_texture_stages | 416 | 0.6 | 0.9 | 0 | yes |
| 0x537d60 | rasterizer_shader_transparent_chicago_extended_set_texture_stages | 526 | 0.6 | 0.9 | 1 | yes |
