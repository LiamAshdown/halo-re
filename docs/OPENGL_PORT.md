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
