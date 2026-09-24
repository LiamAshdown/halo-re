# render module: type recovery notes

Header: `types/render.h`. Smoke test: `out/phase4/render_smoke.c`, built with

```
cd C:\Users\Liam-\halo-re
C:\msys64\ucrt64\bin\gcc.exe -fsyntax-only -I types out/phase4/render_smoke.c
```

It passes. It asserts the size of all 10 structs, about 110 field offsets, and a set of span
identities tied to absolute addresses (the frame graph fields against their globals
0x006b9268..0x006bc310, `0x006b9260 + sizeof(frame_graph) == 0x006bc510`, the render camera /
frustum / fog blocks `0x007c3114 -> 0x007c3168 -> 0x007c32f4 -> 0x007c3344`, the rendered object
list, the frame statistics history and the render view array). If a field moves, the build fails.

The smoke file includes `tags.h`, `memory.h`, `math.h`, `rasterizer.h`, `render.h`. That is more
than the three headers the task named. The extra two are needed because the header reuses
`render_camera` / `render_frustum` / `render_fog` / `render_lighting` /
`rasterizer_dynamic_screen_vertex` from `rasterizer.h` and `real_matrix4x3` / `real_point3d` from
`math.h` instead of defining its own copies. A combined include of every `types/*.h` gives the
same 12 errors (input.h ordering) with and without `render.h`, so the new header adds no name
clashes. `out/phase4/rasterizer_smoke.c` already fails (`d3dx_macro` size, a 64 bit pointer
problem). That failure is older than this header and has nothing to do with it.

Struct names follow CEA symbol hints (hint only): `object_render_data`, `render_model_effect`,
`rendered_particle_datum`, `build_sprite_data`, `cinematic_screen_effect_globals`,
`rasterizer_frame_statistics_s`, and the `fg_*` frame graph. CEA calls the window record
`struct render_window`. Here it is called `render_view`, so `render_window` stays free for the
function at 0x50bfb0. (CEA gives that function the prototype `render_window(local_player_index,
source_camera, source_frustum, rasterizer_camera, rasterizer_frustum, rasterizer_target,
has_mirror)`.)

---

## Records reused, not redefined

| type | owner | render module evidence |
|---|---|---|
| `render_camera` 0x54 | rasterizer.h | Every copy is 0x15 dwords (0x50bdc0, 0x50bfb0, 0x50c590, 0x50c660, 0x4c9260). The fields read here are position +0x00 (0x007c3114, 26 readers), forward +0x0c, up +0x18, mirrored +0x24 (0x007c3138 in 0x50ee20; 0x50c660 writes the negation), fov +0x28 (0x50cb70, 0x50cc40), viewport/window rectangles +0x2c..+0x3a (0x50ca90, 0x50cb70, 0x50de30), z_near/z_far +0x3c/+0x40, and mirror_plane +0x44 (0x50c660 writes the plane). |
| `render_frustum` 0x18c | rasterizer.h | 0x50cc40 builds it. The field offsets are fixed by these accesses: frustum_bounds +0x00, view_to_world +0x44 (scale 1.0 at +0x44, basis, position +0x6c), world_planes +0x78..+0xd4 (read by 0x50d4c0, 0x50d5b0, 0x50d890), z_near/z_far +0xd8/+0xdc, world_vertices +0xe0, world_bounds +0x128 (cube and sphere rejection), projection +0x144 (0x50c9a0 touches +0x14c/+0x15c/+0x16c/+0x17c, 0x50dac0/0x50ddc0/0x50de30 read +0x144/+0x158/+0x164/+0x168), and projection_world_to_screen +0x184/+0x188 (0x007c32ec / 0x007c32f0). |
| `render_fog` 0x50 | rasterizer.h | 0x007c32f4. 0x50ba80 reads +0x02, +0x10, +0x18, +0x1c, +0x40. 0x50eba0 reads planar_mode +0x1c and plane +0x20. |
| `render_lighting` 0x74 | rasterizer.h | 0x50f270 steps ambient +0x00, distant light colours and directions +0x10/+0x1c/+0x28/+0x34, reflection_tint +0x4c (4 floats, 0x50f5c0), shadow_vector +0x5c (0.0015 step, renormalised) and shadow_color +0x68. It copies point_light_count +0x40 and both indices +0x44/+0x48 directly. **This pins the three fields rasterizer.h marks "hint only".** 0x50eba0 takes the NTSC luminance of shadow_color, and 0x50f830 builds the shadow basis from shadow_vector. |
| `rasterizer_window_parameters` 0x258 | rasterizer.h | 0x50bfb0 zeroes it with a 0x96 dword stosd, then fills type, window_index (0x007c310a), +0x04 (has_mirror), the camera (0x15 dwords), the frustum (99 dwords) and the fog (0x14 dwords from 0x007c32f4). |
| `rasterizer_dynamic_screen_vertex` 0x18 | rasterizer.h | This is the vertex that the contrail pairs (0x50e090, stride 0xc floats per pair), `build_sprite` (0x511700) and the frame graph (0x512700) write. The layout is x, y, z, packed colour, u, v. |
| `structure_bsp_mirror_result` 0x1c | structures.h | 0x50ba80 lets 0x553560 fill it (local_524, cluster at +0x18 = local_50c), and render_camera_mirror 0x50c660 reads the plane, +0x10 and +0x14. |
| `contrail`, `contrail_point`, `particle` | effects.h | 0x50df20 / 0x50e090 read contrail +0x04, +0x08, +0x0c, +0x10, +0x14, +0x18, +0x1c, +0x2c[4] and +0x34[4], and contrail_point +0x02, +0x03, +0x04, +0x0c, +0x1c, +0x34. 0x50fd90 reads particle +0x02, +0x04, +0x08, +0x0c, +0x0f, +0x10, +0x14, +0x18, +0x2c, +0x30, +0x3c, +0x54, +0x5c, +0x60. All of these agree with effects.h. |

---

## Structs defined in render.h

### render_view (0xac): element of 0x00719b70, owned by main
* **Producer:** render_frame_all_views 0x4c9260 fills the trailing element one field at a time: +0x00 = -1, +0x02 = 1, +0x58/+0x5c/+0x60 = origin, +0x64..+0x6c = global forward, +0x70..+0x78 = global up, +0x7c = 0, +0x80 = 2·atan(tan(40°)·0.6375), +0x84 = viewport_split_rect_compute, +0x94/+0x98 = 0x0069c65c/0x0069c660. When 0x00873d30 is clear it then copies 0x15 dwords from +0x58 to +0x04.
* **Consumers:** render_frame 0x50bea0 steps through the array by 0xac and tests +0x02 and +0x00. 0x50ba80 reads +0x00, uses +0x04 as the source camera, clamps +0x44 against +0x40, and passes +0x58 as the rasterizer camera. 0x50bdc0 and 0x50c590 copy 0x15 dwords from +0x04 and from +0x58.
* **Array length:** two elements (0x719b70..0x719cc7). The next referenced global is 0x00719ccc, so 0x719cc8..0x719ccb is unreferenced.
* **Unresolved:** +0x03 (never written).

### render_model_effect (0x28): stack block of render_object_list 0x50ee20
* 0x50ee20 copies the parent block with `mov ecx,0xa; rep movsd`. It rewrites type 2 to 0 for children and clears +0x1c/+0x20/+0x24.
* Type 1 is set when the object type is biped or vehicle (`(1 << type) & 3`) and unit +0x37c > 0. It copies unit +0x37c/+0x380, the object datum, and bounding_center (+0xa0).
* Type 2 is set when Object tag flags (+0x02) bit 1 is set.
* +0x1c is the Shader tag data of Object.modifier_shader (+0x9c). It is kept only for shader_type 1 or 5..11.
* +0x20 = object +0x1b8 (change_colors). +0x24 = object +0x134 (function_out_values).
* The block is handed to render_model 0x4d6fc0 (the sky pass 0x510c50 passes 0).
* **Unresolved:** +0x02. The meaning of unit +0x37c/+0x380 is also open: unit fields that ramp at 1/120 and 1/90, most likely the active camouflage amount and its fade.

### object_render_data (0x48): EDI block of render_object 0x50eba0
* **Size bound:** render_objects 0x50e930 holds it at esp+0x10 in a 0x50 byte frame. render_window 0x50bfb0 holds it at esp+0x10, below the window parameters at esp+0x58, and initialises +0x00 = -1, +0x04 = 0, +0x08 = 1, +0x09 = 1, +0x40 = 0, +0x44 = -1.
* +0x00: written per object by 0x50e930 and 0x50eb70.
* +0x04: written by 0x50eba0 from 0x50ea00.
* +0x08: selects the shadow branch of 0x50eba0 and 0x50ee20.
* +0x09: written by 0x50eba0 from the fog plane test. 0x50ee20 turns it into render_model flag 4.
* +0x0c..+0x3f: a real_matrix4x3. 0x50f830 passes `lea edi,[esi+0xc]` to 0x4cb970 and stores the centre at [edi+0x28] (+0x34). render_object_shadow_end 0x50f980 (ECX) reads +0x10, +0x1c, +0x28 as the axes and +0x34 as the centre.
* +0x40: 0x50f830 pushes `esi+0x40` as the out pointer of 0x530ff0. That function stores its radius argument there on success and 0 on failure. 0x50f980 reads +0x40 thirteen times.
* **Unresolved:** +0x0a..+0x0b (padding) and +0x44 (only ever set to -1).

### cached_object_render_state (0x100): element of *0x007c30ec
* **Data array:** `"cached object render states"` (0x0066ebf0), 0x100 entries of 0x100 bytes, created at 0x45aa9c.
* **0x50f150:** compares +0x04 with the object and reads +0x0c for eviction. It also reads and writes object +0x170, the cache datum stored in the object.
* **0x50f270:** writes +0x04, +0x08, +0x0c, +0x10, and +0xfc (the level of detail pixels argument). It samples into +0x88, steps or copies (0x1d dwords) +0x14 toward +0x88, and copies point light count and indices (+0x54/+0x58/+0x5c from +0xc8/+0xcc/+0xd0).
* **0x50ea00:** returns +0x14.
* **Unresolved:** +0x02.

### rendered_particle_datum (0x08): render_particles 0x50fd90
* The records live on the stack at local_2004 (0x2000 bytes, so 0x400 records).
* +0x00 = particle datum index, +0x02 = particle +0x04 (low half), +0x04 = particle +0x2c, +0x06 = 1 for this viewer's own first person particle.
* Every comparator in the sort (0x5109d0, 0x510830, 0x510500, 0x510a90, 0x510b20) orders by word +2, then word +4, then byte +6. 0x50fd90 groups runs on the same three keys into local_2404 (0x200 counts).
* **Unresolved:** +0x07.

### build_sprite_data (0xa4) and build_sprite_group (0x10)
* **render_particles 0x50fd90** builds one on its stack (locals 0x24ac..0x248c):
  * +0x00 = Particle bitmap tag id (Particle +0x10)
  * +0x04 = group particle count
  * +0x08 = Particle + 0xb0
  * +0x0c = 0
  * +0x10 = flags
  * +0x14..+0x1c = origin
  * +0x20 = 0
* **build_sprite 0x511700 (EBX):**
  * reads +0x00 (Bitmap tag: sequences +0x54/+0x58 stride 0x40, sprites +0x34/+0x38 stride 0x20, bitmap data +0x64 stride 0x30)
  * reads +0x04 and +0x0c
  * reads +0x08 → +0x2c (the framebuffer_fade_mode of the shader block)
  * tests bit 0 of +0x10
  * accumulates +0x14..+0x1c
  * bumps +0x0c and group +0x08
* **build_sprite_get_group 0x511520 (EDI data, EAX bitmap):** walks `(i+3)*0x10`, i.e. group key at +0x30 + i·0x10. Appending a group writes +0x24 + i·0x10: [0] slot, [1] lock pointer, [3] key, word [2] = 0. It fails once +0x20 reaches 8.
* **build_sprites_end 0x511620 (ESI):** divides +0x14..+0x1c by +0x0c, transforms the result through view_to_world (0x007c31ac), unlocks and draws each group, and clears flag bit 2.
* **Other builders** pass their own block in EBX / ESI: 0x455069..0x455256, 0x4592a9/0x4592ce, 0x4fb4a2/0x4fb4c1 and 0x4fe651/0x4fe665.
* **Unresolved:** +0x06, +0x0e, +0x22, group +0x0a. The meaning of flag bits 1 and 2 is unknown: bit 1 becomes draw flag 0x80, and bit 2 is set by particles and cleared at the end.

### cinematic_screen_effect_globals (0x78): *0x0071cfc4
* **Setters** (hs table 0x0065a8a8..0x0065a9a0):
  * 0x481150 start: +0x38 = 1. When the argument is false and +0x39 is clear, it also clears 0xe dwords and sets +0x39.
  * 0x481340 stop: +0x38 = 0.
  * 0x4811c0 → 0x5121d0 set_convolution: +0x00, +0x02 (DX), +0x3c, +0x40, +0x44 = ticks/30, +0x48 = +0x44 + time. It also clears +0x23, +0x24, +0x28..+0x34.
  * 0x481220 → 0x512230 set_filter: +0x4c..+0x58, +0x20, +0x21/+0x22 = 0, +0x5c/+0x60, and the same clears.
  * 0x481280 set_filter_desaturation_tint: +0x14..+0x1c.
  * 0x4812f0 → 0x5122a0 set_video: clears 0xe dwords and +0x3c..+0x60, then sets +0x24, +0x23 = 1, +0x28 / +0x34 = BitmapData pointers from GlobalsRasterizerData +0x128 / +0x138, +0x2c, +0x30 = 1.0.
  * 0x4810f0 script_screen_effect_set_value: +0x64 + i·4.
  * 0x481360 cinematic_set_near_clip_distance: +0x74.
* **Resets:** 0x449f12 and 0x51579e clear 0x1e dwords and set +0x64..+0x70 to 1.0.
* **Readers:**
  * 0x512360 update: reads +0x38 and +0x44/+0x48/+0x5c/+0x60; writes +0x04, +0x0c, +0x10 and fixes +0x14; returns EAX = the block.
  * 0x52d8a0 / 0x52e2d0 test +0x02, +0x08, +0x0c, +0x10, +0x23.
  * 0x517470 reads +0x74.
  * 0x5121a0 reads +0x64[AX].
* **Unresolved:** +0x08 (only tested for non zero; no writer other than the block clears), +0x21, +0x22, +0x26, +0x30, +0x3a.

### rasterizer_frame_statistics (0x18): 0x007c30a0
* rasterizer_frame_statistics_get_fps 0x512530 is always called with `mov ebx,0x7c30a0` (0x50be7a and the other three callers).
* It writes +0x00 = 1000/latest, +0x04 = count (word), +0x08 = count·1000/span, +0x0c = 1000/longest (minimum fps), +0x10 = 1000/shortest (maximum fps), +0x14 = dropped·100/count.
* 0x512e80 reads +0x00..+0x14 for the table row.
* **Unresolved:** +0x06.

### frame_graph (0x32b0): 0x006b9260
* The stride comes from `imul ecx,ecx,0x32b0` in fg_add_sample 0x512d90.
* **fg_init 0x512700** writes:
  * +0x00..+0x07 (bounds: 0x1e, 0x40, 0x96, computed right)
  * the three label rectangles +0x08/+0x10/+0x18 (bottom 0x1e0, right 0x280)
  * 0x200 vertices at +0x20 (stride 0x18: x, y = 150.0, colour 0xffffffff)
  * 5 border vertices at +0x3020 (colour 0xffffff00)
  * +0x3098 = 60.0
  * +0x30a0..+0x30ac = 0
  * +0x30b0 = 0x00535046, the bytes of "FPS"
* **fg_add_sample:** shifts +0x30a0..+0x30ac, writes +0x309c (average), clamps against +0x3098, scrolls the vertex y values (+0x3c, stride 0x18) and sets the last one (+0x300c).
* **fg_render 0x5129a0 and its tail at 0x512b80:** draw +0x20 (0x1ff line strip primitives) and +0x3020 (4 primitives), then print +0x30b0 in rectangle +0x08, +0x3098 in +0x10 and +0x309c in +0x18.
* **Unresolved:** +0x30b4..+0x32af is never referenced. It is declared as the rest of `char name[0x200]` because 0x30b0 + 0x200 is exactly the stride, but the length of `name` is an inference.

---

## Globals: unresolved or unreferenced bytes inside the owned ranges
* 0x006b8d94..0x006b8dbf: no reference in the binary.
* 0x006b8dc2: the high half of the rendered object count. Ghidra shows it in a CONCAT22, but it is never used on its own.
* 0x006b91c4..0x006b91c7: no reference.
* 0x006b923c: sky animation times. The array is bounded at 9 floats by the frame graph at 0x006b9260. The real maximum is the Sky animations block limit, which is not recovered.
* 0x007c3088..0x007c309f, 0x007c30b8..0x007c30c3, 0x007c30ca..0x007c30cf, 0x007c30e9..0x007c30eb, 0x007c30f0..0x007c30ff: no reference (checked against a full objdump).
* 0x0071cfd0 / 0x0071cfd8: 64 bit values read by 0x512e80 but never written anywhere.
* 0x007c30e8: read by 0x50ee20 but never written. Probably a console global; the name is UNSURE.

---

## Misattributed functions, library code and non-functions in this range

| address | Ghidra / phase-2 name | finding |
|---|---|---|
| 0x6b4c00 | render_window | **Not a function.** 0x6b4c00 lies in `.data` (0x676000..), the file bytes are zero, and nothing in the binary references it. The entry comes from the OpenSauce list (CE address, drift). No types. |
| 0x50fd3c | render_window_call_hook_weather_particle_systems_render | **Not a function.** 0x50fd3c is the `cmp ecx,0xffffffff` in the middle of FUN_0050fcd0. Ghidra cut that function at 108 bytes, but its body runs to about 0x50fd8a. This is also an OpenSauce CE address. |
| 0x512b80 | object_render_state_refresh | **Not a function.** It is the tail of fg_render 0x5129a0 (0x5129a0 + 480 = 0x512b80): the device state setup, then the FPS label and the two `%d` values. The CEA function of that name is 0x50f270. |
| 0x510410..0x510ba0 (10 functions) | sort_introsort_loop, FUN_00510500, FUN_005107e0, FUN_00510830, FUN_005108f0, FUN_00510980, FUN_005109d0, FUN_00510a90, FUN_00510b20, FUN_00510ba0 | **Library code:** an MSVC STL `std::sort` instantiation for 8 byte records (introsort loop, unguarded partition, sort_heap, insertion sort, median, make_heap, med3, adjust_heap, push_heap, rotate). No types defined for the sort itself. `rendered_particle_datum` is defined because render_particles 0x50fd90 builds and groups the array. |
| 0x50fd90 | contrail_render_all_active | Misnamed. This is render_particles: it iterates particle_data 0x0087abd0 (0x70 elements) and builds sprites. It does not touch contrails. |
| 0x50df20 / 0x50e090 / 0x50e000 | contrail_* | Correct subsystem: render_contrails(render_type_flags) and its segment builder. The types are in effects.h. |
| 0x511b40 | contrail_draw_segment_blended | Called only from the particle system renderer (0x455069, 0x4551f4). It is a sprite helper (probably CEA build_sprite_rotational), not contrail code. |
| 0x50e930 / 0x50eb70 / 0x50eba0 / 0x50ee20 / 0x50f830 / 0x50f980 | FUN_* / shadow_compute_bounding_box_and_register | render_objects, render_object_shadows, render_object, render_object_list, render_object_shadow_begin, render_object_shadow_end. |
| 0x50f150 / 0x50f270 / 0x50ea00 | shadow_cache_get_or_allocate_entry, FUN_0050f270, render_get_cluster_ambient_light_sample | The object render state cache, not a shadow cache. Probable names: object_get_cached_render_state, object_render_state_refresh, object_get_cached_render_lighting. |
| 0x511d80, 0x511da0, 0x511df0, 0x511e80, 0x511ef0, 0x5120f0, 0x512120, 0x512150, 0x512190 | render_device_is_ready, FUN_* | Rasterizer-side helpers in the render range. They use only rasterizer.h types and globals: device and 0x0071d16c, the dynamic index slot table 0x006dd9e0 (stride 0xc = rasterizer_dynamic_index_slot), D3DRS_LIGHTING (0x89), lens_flare_current_key 0x00746fb0, lens_flare_vertex_specular 0x0069e708, the effect slot pointer 0x0071d278, and a thunk to 0x537800. Nothing to define here. 0x511da0 is render_initialize: it carves the 0x10 byte tint block (0x0071cfc0) and calls rasterizer_initialize_direct3d. |
| 0x512530, 0x512700, 0x5129a0, 0x512d90, 0x512e80 | rasterizer_frame_statistics_* | CEA files these under rasterizer/ and render/fg_*. Their types are defined here because they live in this range and no other header owns them. |
| 0x5121a0..0x512360 | FUN_*, screen_effect_update | cinematic screen effect state (CEA files it under rasterizer/rasterizer_screen_effect*). types/rasterizer.h lists 0x0071cfc4 with "UNSURE owner", so the type is defined here. |

Code in the range that is **not** in the function list: the structure pass callbacks 0x511f70, 0x511f90, 0x511fe0, 0x512010, 0x512020, 0x512040, 0x512070, 0x512080 and 0x5120c0, which render_window 0x50bfb0 hands to 0x552de0. There is also the object collector callback 0x50e8d0, which 0x50eac0 hands to 0x554420. rasterizer.h already credits 0x511f90 with setting rasterizer_lightmap_bitmap.

---

## Findings that affect other headers (not changed here)

* **types/interface.h `first_person_light_parameters` (0x20)** is the first 0x20 bytes of `render_model_effect` (0x28). Its +0x1c "zero" is the modifier shader pointer, which the first person path leaves 0. It is missing +0x20 change_colors and +0x24 function_values. 0x50ee20 copies all 10 dwords (`mov ecx,0xa; rep movsd` at 0x50ee88).
* **types/rasterizer.h `render_lighting`:** reflection_tint +0x4c, shadow_vector +0x5c and shadow_color +0x68 are now pinned by 0x50f270, so they no longer need to be marked "hint only".
* **types/rasterizer.h 0x0069c65c "rasterizer_letterbox_height"** is the default near clip distance. 0x511df0 seeds 0x0069c65c/0x0069c660 with 0.0625/1024 and 0x0069c664/0x0069c668 with 0.01171875/1024. 0x4c9260 copies the first pair into render_view.rasterizer_camera z_near/z_far. 0x517470 replaces 0x0069c65c with the cinematic near clip distance.
* **types/rasterizer.h 0x007c310a** is typed uint8_t. It is an int16: render_frame stores it with a WORD (`mov WORD PTR ds:0x7c310a,di` at 0x50bf20). The three BYTE readers (0x4f13fd, 0x4f16ce, 0x513b4e) only need the low byte.
* **types/rasterizer.h 0x0071d16c "rasterizer_fullscreen":** render_device_is_ready 0x511d80 treats it as the "device initialised" flag (it ANDs the flag with the device pointer).
* **types/objects.h `object.unknown_170`** (+0x170, -1 at create) is the datum index of the cached_object_render_state of the object (0x50f150 reads and writes it).
* **types/effects.h `weather_instance.delta_time`** is copied from 0x007c3110, which is the time since the previous frame. This agrees with sky animation, which accumulates the same global. render_frame computes its frame time from 0x007c310c (game_time_globals.leftover_time), not from 0x007c3110.

---

## Register conventions seen while pinning fields (for the rewriters; confirm per function)
Verified in objdump:
* render_frame 0x50bea0: EBX = screenshot index (Point2DInt*, never loaded inside the function), plus five stack arguments (views, count, screenshot page index, time since tick, time since frame). The combined tile is page·0x00696568 + index.
* 0x50ba80: EAX = screenshot index (Point2DInt*) or 0, plus one stack view.
* 0x50bdc0: EAX = nonplayer type (0 or 1), plus one stack view.
* 0x50cc40: ECX = camera, ESI = frustum out, EAX = bounds or 0, plus one stack bool.
* 0x50eb70: EAX = object_render_data.
* 0x50eba0: EDI = object_render_data.
* 0x50f830: EAX = object_render_data, plus one stack level of detail value.
* 0x50f980: ECX = object_render_data.
* 0x530ff0: EAX = matrix, EBX = colour, stack (radius, out pointer).
* 0x511700: EBX = build_sprite_data, AX = sequence, ECX = sprite, plus eight stack arguments.
* 0x511620: ESI = build_sprite_data.
* 0x511520: EDI = build_sprite_data, EAX = BitmapData*.
* 0x512530: EBX = rasterizer_frame_statistics*, plus one stack byte.
* 0x512d90: ECX = graph index, plus one stack float.
* fg_render 0x5129a0: BL = render_graph, AL = render_infos.
* 0x512360: returns EAX = cinematic_screen_effect_globals*.
