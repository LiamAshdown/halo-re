# rasterizer module: type recovery notes

Header: `types/rasterizer.h` (960 lines). Smoke test: `out/phase4/rasterizer_smoke.c`, built with

```
cd C:\Users\Liam-\halo-re
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/rasterizer_smoke.c
```

It passes (also clean under `-std=c99 -Wall`, apart from the pedantic typedef-redefinition
warnings every header produces). The smoke file includes `tags.h`, `memory.h`, `math.h`,
`rasterizer.h`: `math.h` is needed because the header reuses `real_point3d`, `real_vector3d`,
`real_vector2d`, `real_plane3d`, `real_matrix4x3` and `real_rectangle3d` rather than
redefining them (same choice as `effects_smoke.c`). It carries 203 assertions: 36 struct
sizes; `offsetof` checks, most of them tied to the absolute global addresses the module reads
(for example `0x007c1220 + offsetof(rasterizer_window_parameters, fog.planar_maximum_depth) ==
0x007c144c`); span identities for every table (`0x007c1484 + 128 * 0x38 == 0x007c3084`,
`0x00746fc0 + 5 * 0x18018 == 0x007bf038`, ...); and 26 checks on the tag layouts in
`types/tags.h` that the attributions depend on. The header also compiles together with every
other `types/*.h` except `networking.h` (which has its own include-order dependencies); no name
collides. `effects`, `structures`, `cache` and `objects` smoke files re-verified unchanged.

Pointer fields are `uint32_t` with the pointee type first in the comment (the `effects.h`
convention), because the host gcc is 64-bit and the size assertions would otherwise fail.

---

## Binary evidence the layouts hang on

| fact | where |
|---|---|
| window parameters are 0x258 bytes | `rasterizer_begin_frame` 0x5175c0: `rep movsd` of 0x96 dwords into 0x007c1220 |
| camera is 0x54 bytes at +8 | FUN_0050bdc0 (render module) copies 0x15 dwords into its local at +8 before calling 0x5175c0 |
| fog is 0x50 bytes at +0x1e8 | 0x5175c0 `lea edx,[ebp+0x1e8]; call 0x5176d0`; 0x5176d0 copies 0x14 dwords |
| frustum is 0x18c bytes at +0x5c | 0x5c + 0x18c == 0x1e8; projection at frustum +0x144 == 0x007c13c0, the 4x4 both projectors divide by; `structure_bsp_visible_cluster` in types/structures.h independently found 0x18c |
| D3DCAPS9 at 0x007c10c0 | IDirect3D9::GetDeviceCaps (+0x38) called with that literal in 0x5169c0; PixelShaderVersion then lands on 0x007c118c, MaxStreams on 0x007c117c, MaxSimultaneousTextures on 0x007c1158, MaxActiveLights on 0x007c1160, RasterCaps on 0x007c10e4 |
| D3DPRESENT_PARAMETERS at 0x007c04a0 | 0x515fc0 writes all 14 dwords; interval at +0x34 == 0x007c04d4 |
| 128 lights of 0x38 | the appenders in `object_lights_update_all` 0x4f0cf0 test `count < 0x80` and copy 0xe dwords; 0x007c1484 + 0x1c00 == 0x007c3084, the next global |
| 63 skinning matrices | 0x0069c67e initialised to 0x3f; 0x007c04e0 + 63*0x30 == 0x007c10b0 |
| transparent group 0xa8 | pool GlobalAllocs 0xfc00 / 0x1500 == 384 / 32 records; every indexer multiplies by 0xa8 (or 0x2a dwords) |
| lens flare instance 0x28 | `decal_add_to_active_list` 0x5138a0 copies 10 dwords into 0x006ce818 + n*0x28, cap 0x400 |
| lens flare batch 0x18018, 5 slots | 0x536c10 indexes 0x6006 dwords; loop bound 0x7d7038 in 0x536c80 |
| font glyph cache 0x1010 | 0x514820 zeroes 0x404 dwords at 0x006d8828; 0x514cb0 walks 0x200 8-byte entries from 0x006d8838 |
| vertex strides | .rdata 0x0065de00, 20 int16 values |
| vertex buffer / index buffer | constructors 0x524980 / 0x525030 fill every field; the raw code of 0x525030 (not the decompiler) shows data at +8 and the D3D buffer at +0xc |
| render_lighting is 0x74 | 0x519f70 copies 0x1d dwords out of `group.lighting`; the model draw context puts it at +0x10 with the next field at +0x84 |

---

## Struct by struct

### rasterizer_vertex_buffer (0x14) / rasterizer_index_buffer (0x10)
- `rasterizer_vertex_buffer_create` 0x524980: type +0, count +4, zeroes +8/+0xa, data +0xc,
  hardware buffer +0x10 (from the 0x530570 CreateVertexBuffer wrapper).
- `rasterizer_index_buffer_create` 0x525030: type +0 (0 list = count*6 bytes, 1 strip =
  count*2+4), count +4, data +8, hardware +0xc.
- Readers: 0x51c1c0, 0x51c310, 0x51c5f0, 0x51c790, 0x533660.
- Tag correspondence: ScenarioStructureBSPMaterial +0xb0 (rendered) and +0xc4 (lightmap, hence
  `vertex_buffer + 0x14` as the second stream in 0x533660); ModelGeometryPart +0x44 (triangles),
  +0x54 (vertices).
- Unresolved: `+0x08/+0x0a` of the vertex buffer are only zeroed.

### rasterizer_vertex_declaration (0xc), rasterizer_vertex_buffer_slot (0x14)
- 0x5301b0 creates 19 declarations and writes usage words; 0x530540 releases.
- 0x5305f0 writes slot +0/+4/+8/+0xc from eax/ebx/edi/esi (raw code 0x530673..0x530685),
  returns index+1; 0x530690 reads +8/+0xc/+0x10. Handle lookup elsewhere is
  `*(0x007bf04c + h*0x14)` (Ghidra shows it as `DAT_007bf04c + h*10` on a 2-byte type).
- Unresolved: declaration `fvf` is always 0; slot `+0x10` byte meaning (skipped on recreate)
  is a guess ("managed").

### rasterizer_dynamic_vertex_cache (0xc), _vertex_slot (0x10), _index_slot (0xc)
- 0x51bb90 sets capacity/handle per type, 0x5175c0 zeroes `used`, 0x51bdd0 bumps it and fills
  a vertex slot, 0x51be40 locks it, 0x51bd60 fills an index slot, 0x51bec0/0x51c090/0x51c490
  draw from them.
- Unresolved: index slot `+0x08` has no writer here.

### rasterizer_effect_slot (0x20), rasterizer_vertex_shader (8)
- 0x52f980 (+0 effect), 0x52f780 (+8..+0x14 Texture0..3), 0x52fab0 (+0x18 constant handle
  arrays), 0x5202f0/0x520e50/0x531ed0 (+4 vertex shader index into 0x0069e350, +0x18 reads).
- 0x5306e0 skips entries whose +4 is 0.
- Unresolved: effect slot `+0x1c`.

### rasterizer_render_target (0x14)
- 0x52ca20 (width/height/format from GetDesc, texture creation into +0x10 and GetSurfaceLevel
  into +0xc), 0x52cc50, 0x52ccc0, 0x52cdd0, 0x52ce10. Only indices 6 and 7 have a known use
  (projected light shadow buffers, 0x525ab0); no enum of the nine is given.

### d3d_present_parameters (0x38), d3d_caps9 (0x130), d3d_gamma_ramp (0x600)
- SDK layouts, pinned by the addresses above; `(used)` marks the fields this module reads.
- Gamma: 0x522520 captures into 0x006e0b18, 0x5227a0 builds 0x006e1118, 0x5226c0 restores.

### rasterizer_display_mode (0x10)
- 0x515ca0 writes it, 0x515d10 compares it. `+0xd..+0xf` never written.

### rasterizer_window_parameters (0x258) and its parts
- `render_camera`: position/forward (0x007c1228/0x007c1234, 25+ readers: every depth key),
  viewport_bounds (0x007c1254, the screen effect / letterbox / render target code), z_far
  (0x007c1268, 0x5176d0). up/mirrored/fov/window bounds/z_near/mirror plane are OpenSauce names,
  unverified here.
- `render_frustum`: world_to_view (0x525130, 0x536d80), view_to_world forward/left (0x513540),
  projection (0x525130, 0x536d80), rows read by 0x518f40. The rest are OpenSauce names; the
  total size is proved.
- `render_fog`: every field read by 0x5176d0 / 0x526f50 / 0x51def0 / 0x5175c0.
  Unresolved: `+0x48`, `+0x4c`.
- `render_screen_flash`: 0x52ed00 reads type, intensity and the four channels.
- Header fields: `type` (0x007c1220, tested `== 1` all over), `window_index` (0x007c1222, lens
  flare filter), `unknown_04` (0x007c1224, gate of 0x520b90/0x520e50), `clear_target` (+5, read
  by 0x5175c0 only). Unresolved: `+0x06`, `+0x250`, `+0x254`, and the meaning of `unknown_04`.

### rasterizer_frame_time (0x10)
- 0x517470 copies four dwords from ECX into 0x007c1200; only the first is ever read (time
  argument of periodic functions in 0x51da20, 0x51f3e0, 0x529e00). `+4..+0xc` unresolved.

### rasterizer_light (0x38)
- Written by object_lights_update_all 0x4f0cf0 (objects code, outside this module) with a 0xe
  dword copy; read by 0x518c10, 0x5215b0, 0x521750, 0x526760, 0x51da20. Every byte named.

### rasterizer_point_light_constants (0x30), rasterizer_projected_light_constants (0x50)
- 0x518c10 writes every dword of the first; 0x5215b0 / 0x521750 write the second.

### render_lighting (0x74), render_distant_light (0x18)
- 0x518ce0 / 0x518d40 read ambient, the distant pair and the point light count.
- Unresolved: `point_light_indices` (+0x44) is inferred from 0x518c10 taking a light index in
  EAX, whose loader is register passed; `reflection_tint`, `shadow_vector`, `shadow_color`
  (+0x4c..+0x73) are OpenSauce names with no reader in this module.

### rasterizer_skinning_matrix (0x30)
- 0x518b40 writes, 0x526cf0 gathers. Every byte named.

### rasterizer_geometry_group_parameters (0x28) and rasterizer_model_draw_context (partial, >= 0xcc)
- 0x52b180 copies the 10 dwords from context +0x8c to group +0x14; 0x526f50 reads mode (+0x8c)
  and +0x90 and center (+0xb4); 0x52b050 reads the shader (+0xa8) and function values
  (+0xb0); 0x52b340 reads +0x08, +0x0c, +0x10, +0x84, +0x98, +0xc4, +0xc8; 0x519f70 builds one
  on its stack, which fixes +0x10 (0x74 bytes), +0x84 (8 bytes), +0xb4 and +0xc4/+0xc8.
- The context is filled by the render module, so its tail beyond +0xcc is not known.
- Unresolved: context `+0x04`, `+0x84` (two dwords), `+0xc0`, `+0xc4`, `+0xc8`; parameters
  `+0x20`. `blend_factor`/`distortion_factor` are named from their only arithmetic.

### transparent_geometry_group (0xa8)
- Constructors that write every field: 0x522300, 0x52b180, 0x52b340, 0x51c830, 0x536ff0.
- Readers: 0x5155b0 (flags bit 7, shader type 5/6/7/8 and derived flags +0x29, depth +0x78,
  sort key +8, first_person +0xa5), 0x5156d0 and 0x515290 (+0x98), 0x533660 (+0x44..+0x58),
  0x533730 (flags, +0x14, +0x44, +0x50, +0x70), 0x533850 (everything, including the +0x48
  callback path when shader is NULL and the +0x9c/+0x9e chain), 0x519f70 (+0x1c, +0x3c, +0x40,
  +0x70, +0x74, +0x7c), 0x515400 (+0x54, +0x58), 0x522930..0x523d10 (+0x10, +0x14, +0x18,
  +0x54, +0x58, +0x5c).
- Unresolved: `+0x04` (copied from the context only), `+0x12`, `+0x3c`/`+0x40` meaning,
  `+0x66`, `+0xa0` meaning, `+0xa4`, `+0xa6`; flag bits 0, 2, 3, 5 have no semantic name.

### lens_flare_instance (0x28), lens_flare_object_visibility (0xa), lens_flare_vertex (0x20), lens_flare_batch_key (0x10), lens_flare_batch (0x18018)
- Instance: FUN_00513a00 builds one on its stack per BSP marker (raw code 0x513b2c..0x513b72
  writes +0x00..+0x23), 0x5138a0 culls and rewrites +0x1e/+0x20, 0x513ba0 writes +0x24,
  0x513780 and 0x5134f0 resolve +0x1e/+0x20/+0x22 into a visibility byte, 0x513cf0 reads all.
- The visibility byte for a BSP flare is `0x006be810 + (window & 0x7f) + marker_index + 8`
  (the +8 is literal, `lea ecx,[eax+8]` at 0x5139a4); for an object flare it is
  `0x006bc512 + slot*10 + (window & 0x7f)`.
- Batch: 0x536b70 (key), 0x536c10/0x536c80 (count, stamp), 0x536cb0 (key match, LRU),
  0x537550 (vertices), 0x537130 (reset of all of them).
- Unresolved: instance `window_flags` bit 7 meaning; batch key `+0x0e`.

### font_glyph_cache (0x1010), font_glyph_cache_entry (8)
- 0x514820, 0x514cb0, 0x514ed0. Every byte named except `+0x0001`.

---

## Misnamed, misattributed or not-a-function

| address | current name | finding |
|---|---|---|
| 0x5134f0, 0x513540, 0x513780, 0x5138a0, 0x513a00, 0x513ba0, 0x513cf0 | decal_* / FUN_ | **Lens flare** system, not decals: the record starts with LensFlare tag data, culls on `LensFlare.far_fade_distance`, 0x513a00 walks `ScenarioStructureBSPCluster` lens flare markers, and visibility comes from occlusion queries. Suggested names: lens_flare_get_visibility_byte, lens_flare_compute_rotation, lens_flare_update_visibility, lens_flare_add_instance, structure_cluster_add_lens_flares, lens_flare_update_samples, lens_flare_render_all |
| 0x515740 | decal_and_font_system_reset | Clears the lens flare visibility tables, the glyph cache, and loads GlobalsRasterizerData into 0x0071d164 |
| 0x51acd0 | rasterizer_dispose_call_from_rasterizer | Not a function: `push 2; jmp 0x51ae89`, a case label of the decal type switch in FUN_0051aa50 |
| 0x51ad91 | caseD_0 | Not a function: another case label of the same switch |
| 0x530570 | rasterizer_dx9_vertex_shader_create | Wraps **CreateVertexBuffer** (device +0x68) and returns the buffer (raw code 0x5305dc); CreateVertexShader is +0x16c |
| 0x5305f0 | vertex_shader_cache_get_or_create | Vertex buffer slot allocator (0x007bf060 table) |
| 0x530690 | vertex_shader_cache_recompile_dirty | Recreates lost vertex buffers after a device reset |
| 0x51bd60 | rasterizer_dynamic_vertex_cache_reserve | Reserves **indices** in the shared index buffer |
| 0x51bdd0 | rasterizer_dynamic_index_cache_reserve | Reserves **vertices** in the per type vertex buffer |
| 0x51be40 | rasterizer_dynamic_index_cache_lock | Locks a **vertex** slot range |
| 0x51c830 | rasterizer_transparent_object_append | A transparent_geometry_group constructor |
| 0x536ff0 | rasterizer_lens_flare_occlusion_sample_add | A transparent_geometry_group constructor for callback groups (shader NULL, procedure at +0x48) |
| 0x522520 / 0x5226c0 / 0x5227a0 | chimera__registry_check_4 / _3 / chimera__gamma | Gamma capture / restore / apply (Chimera names are hints only) |
| 0x5132b0..0x5134c0 | color / normal packing helpers | Generic math helpers placed in this range; no types of their own |
| 0x519980 / 0x5199f0 | resource verify / load | Generic whole-file load plus trailing signature check used for fx.bin and vsh.bin |
| global 0x007c117c | "vendor id" (phase 2) / `rasterizer_vertex_processing` (types/cache.h) | D3DCAPS9.MaxStreams; types/cache.h should be corrected |

Library code: none in this range (the `_qsort` called by 0x5156d0 lives in the CRT).

## Functions whose structs were left untyped

- 0x530830, 0x530ff0, 0x531350, 0x531570 ("motion sensor HUD"): the register-passed state and
  contact records are not typed. Evidence that this is the motion sensor is weak; it needs the
  callers first.
- 0x531b80 / 0x531e90 (first person model render): reads a struct with three parts at +0xc.. and
  two floats at +0x40/+0x44; not typed.
- 0x537bb0 / 0x537d60: read an array of 0xdc byte records at object +0x58 (count +0x54), or
  +0x64/+0x60 in the older layout; not typed here.
- 0x51f3e0 (light halo) reads object offsets 0x19c..0x230 and 0x52c4a0 reads 0x60..0x7c of its
  argument: both belong to structures of other modules.
- 0x51aa50 walks the effects decal grid 0x006b0ad8 and draws the `decal` records of
  types/effects.h; nothing new to define.
- 0x0071cfc4 (letterbox height at +0x74, 0x78 bytes cleared by 0x515740) and 0x0071cfc0
  (0x10 bytes) are owned elsewhere (cinematic code, UNSURE) and are not defined here.
- The screen effect parameter block that 0x52d8a0 / 0x52e2d0 get from FUN_00512360 is
  `weapon_screen_effect_parameters` in types/interface.h (0x38); not redefined.
