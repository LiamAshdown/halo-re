# OpenGL / WebGL renderer port

Goal: draw the game through OpenGL so the same renderer can later run as WebGL 2 in a browser (Emscripten).
This is the renderer only. A browser build also needs the audio, input, memory-layout, file and networking
work listed at the end; none of that is started.

## Where the seam already is

All Direct3D 9 calls go through one interface, `halo::rasterizer::RenderDevice`
(`include/halo/rasterizer/render_device.hpp`, 76 virtual methods). `D3D9Device`
(`src/rasterizer/d3d9_device.cpp`) is the only implementation and `render_device()` returns it. The port is a second
implementation, `GlDevice`, selected at start-up, plus the places that create the device and window
(`src/rasterizer/device.cpp`, `src/shell/window.cpp`, `src/shell/application.cpp`).

The interface still speaks Direct3D: objects are opaque `void *` COM pointers (textures, surfaces, buffers,
shaders, effects), flags are D3D constants (`D3DRS_*`, `D3DTSS_*`, FVF codes, `D3DFMT_*`), and several methods
fill D3D structs (`D3DLOCKED_RECT`, `D3DVIEWPORT9`, `D3DPRESENT_PARAMETERS`). `GlDevice` has to accept those values
unchanged and translate them; the engine code above the interface is not rewritten.

## What GlDevice must cover

| Group | RenderDevice methods | GL mapping |
|---|---|---|
| Device / swap | `create_device`, `reset`, `present`, `get_back_buffer`, `set_gamma_ramp`, adapter queries | WGL (or EGL / WebGL canvas) context, default framebuffer, fake adapter info |
| Resources | `create_texture`, `create_volume_texture`, `create_cube_texture`, `create_vertex_buffer`, `create_index_buffer`, `create_offscreen_plain_surface`, `*_lock_*`, `*_unlock_*`, `buffer_get_desc`, surface and level accessors | `glTexImage*`, buffers; locks are CPU shadow copies uploaded on unlock; D3DFMT to GL format table (DXT1/3/5, A8R8G8B8, V8U8, P8, ...) |
| Render targets | `set_render_target`, `get_render_target`, `stretch_rect`, `clear` | framebuffer objects, blit, `glClear` |
| Fixed function | `set_transform`, `set_material`, `set_light`, `light_enable`, `set_render_state`, `set_texture_stage_state`, `set_sampler_state`, `set_fvf` | no fixed pipeline in GL ES / WebGL: emulated by generated shaders keyed on the current state (same idea as wined3d's fixed-function replacement) |
| Programmable | `set_vertex_shader(_constant_f)`, `set_pixel_shader(_constant_f)`, `create_*_shader`, `create_vertex_declaration`, `set_vertex_declaration`, `process_vertices` | translate D3D shader bytecode (vs 1.x, ps 1.x-2.0) to GLSL at create time; constants map to uniform arrays |
| Effects | `effect_*` (set vector/texture/technique, begin/pass/end, parameter lookup) | the 122 pixel-shader effects from `shaders\fx.bin` (an `ID3DXEffect` binary): needs an effect-file reader plus the shader translator above |
| Draw | `draw_primitive`, `draw_indexed_primitive`, the `_up` variants, `set_stream_source`, `set_indices` | vertex arrays / VAOs from the vertex declaration, `glDrawElements` |
| Queries | `create_query`, `query_issue`, `query_get_data` | occlusion queries (used by lens flares) |

## The two hard parts

1. Shader translation. D3D9 shader bytecode has to become GLSL. Writing this is a known but sizeable task.
   MojoShader (zlib licence) already does bytecode to GLSL and can parse D3DX effect files; using it would remove
   most of this work, but it is an external dependency (would need to be downloaded into `vendor/`).
2. Fixed-function emulation. The renderer uses texture stage states, lights and fog on top of shaders. WebGL 2 has
   none of that, so a state-keyed generated-shader cache is needed.

## Milestones

1. `GlDevice` skeleton behind `HALO_RENDERER=gl`: a WGL context on the existing window, device create/reset/present,
   clear. The game boots, the screen clears, nothing else draws. D3D9 stays the default.
2. Resources: textures (including DXT), vertex and index buffers, locks, render targets. UI bitmaps and text draw
   (menus use the fixed pipeline and a few shaders).
3. Fixed-function emulation: transforms, lights, fog, texture stages, blend/depth/alpha states. Menu and HUD correct.
4. Shader translation plus the effect reader. World geometry, models, lightmaps, water, decals, lens flares.
5. Parity checks: screenshot comparison against the D3D9 path on fixed scenes; fix state-tracking bugs.
6. Move the GL calls behind a small loader so the same backend builds against GLES3 / WebGL2.

## Beyond the renderer (needed for a browser build)

- Audio: DirectSound and EAX to WebAudio (via OpenAL or SDL audio).
- Input and window: Win32 message loop and DirectInput to browser events.
- Memory: the engine data sits in an image fixed at the original addresses (`/BASE:0x10000000`, no relocation);
  WebAssembly has its own 32-bit memory with no such image, so every fixed-address access must go through the
  link tables or be rebuilt.
- Files: loading the retail `.map` files (hundreds of MB) through the browser's file API.
- Threads and timing: Win32 threads and `QueryPerformanceCounter`.
- Multiplayer: browsers cannot send UDP, so a WebSocket/WebRTC to UDP proxy is required for retail servers.

## Progress log

- 2026-10-04: milestone 1 done. `HALO_RENDERER=gl` boots the game with GlDevice (WGL context on the game window, clear/present, CPU-side
  stand-ins for all resources); nothing is drawn yet (black window). All direct device vtable calls were routed through RenderDevice
  (stage/sampler/render states, water ripple draw, GetDisplayMode). `build/dbg/gl_smoke.sh <seconds>` runs the GL path and screenshots the
  screen to `build/dbg/gl_shot.png`; `build/dbg/resolve_eip.py` resolves crash addresses against the link map. Next: milestone 2 (texture and buffer
  uploads, render targets, 2D menu drawing). vendor/mojoshader holds the MojoShader source for milestone 4.

- 2026-10-04: milestones 2-3 first cut. The main menu now draws through OpenGL (`HALO_RENDERER=gl`): GL entry points loaded at run time (`gl_api.hpp`),
  textures uploaded from the CPU shadow copies (DXT, legacy formats converted to RGBA8), vertex/index buffers, vertex declarations and FVF, render/sampler
  state mapped to GL state, render targets via FBOs, vs/ps bytecode translated to GLSL 1.10 by MojoShader (built as a static lib; vendored copy patched for
  vs_1_x relative addressing without a CTAB and `mov a0` float-to-int), and generated GLSL for the fixed-function vertex and texture-stage pixel pipelines.
  Not done yet: fixed-function lighting/fog, ID3DXEffect (fx.bin) for world rendering, process_vertices, parity screenshots, GLES3 profile.
  Debug: `HALO_GL_TRACE=1` logs draws; `HALO_GL_PROGRAMS=<dir/>` dumps generated GLSL.

- 2026-10-04 (later): effects. `ID3DXEffect` runs on MojoShader's effect parser (`gl_effect.cpp`): fx.bin loads, techniques/passes bind translated shaders and apply
  render/sampler/texture state. Vendored MojoShader patches (vendor/ is untracked, re-apply on re-download): vs_1_x relative addressing without a CTAB, `mov a0` int cast,
  20-byte CTAB header, ps_1_4 `texld` and `_dz/_dw`, pass states that name a shader variable. The menu draws with no skipped draws or translation errors.
  Not verified: in-game levels (`-exec map_name` does not leave the menu in either backend in the cmake build, so world/model/water/decal parity needs a way to start a level),
  fixed-function lighting/fog, process_vertices, GLES3 profile.

- 2026-10-04: level geometry no longer black. Cause: the pixel-shader wrapper (alpha test, HALO_GL_FORCEWHITE, HALO_GL_PSDEBUG) read and wrote
  `gl_FragData[0]` while MojoShader's translation writes `gl_FragColor`, so the alpha test compared an undefined value and discarded every fragment of
  alpha-tested draws (most world/model draws). The wrapper now uses MojoShader's own `ps_oC0` name. The skinning path (vs 91, c29+3*bone) was checked and
  gives in-range clip positions. The campaign start now shows the ship interior, marines and first-person weapon; brightness against D3D9 not compared yet.
- 2026-10-04: BSP textured and lightmapped. (1) Effect passes now bind `Texture[n] = <param>` pass states (D3DX state 0xA4): the environment ps_1_x asm
  shaders have no CTAB, so this is their only texture binding (vendored MojoShader patch: `readstates` keeps the state index). (2) vs_1_x inputs are matched
  to declaration elements by their `dcl` usage like every other version (Halo's vsh.bin declares every input; the lightmap shaders read stream 1's
  NORMAL1/TEXCOORD1 from v7/v8, which element order left unbound). NORMAL1 shares the TEXCOORD7 attribute slot. Remaining: night scene lacks D3D's blue fog tint.
- 2026-10-04: effect shader constants. Halo's asm effect passes set constants with `PixelShaderConstant[n] = <param>` states; in the converted fx.bin
  (tools/convert_fx.py) each is an FXLC expression copying one float4 parameter. MojoShader dropped them (vendored patch: the state records the
  parameter named in the expression's CTAB, or by name for usage 1), and `gl_effect.cpp` now copies that parameter into the constant registers at
  BeginPass. Fixes stale lightmap multipliers and the missing atmospheric fog; the a50 start now matches D3D9.
- 2026-10-04: ps_1_x sampler types. ps_1_x declares no sampler types (Direct3D samples whatever is bound); the translation assumed 2D, so cube
  textures (the flashlight's projected cube map) sampled black. ps_1_x shaders keep their bytecode and are re-translated with a MojoShader sampler
  map when a cube/volume texture is bound (cached per shader, up to 8 combinations). Vendored MojoShader patch: ps_1_4 `texld` emits
  textureCube/texture3D for those samplers. Flashlight lights the BSP. Open: other levels unchecked, GLES3/WebGL profile (milestone 6).
- 2026-10-04: MojoShader is now tracked in third_party/mojoshader (upstream ad5dff8 plus the halo-re changes listed in its README.halo.md, full diff
  in halo.patch); the build no longer reads vendor/mojoshader.
