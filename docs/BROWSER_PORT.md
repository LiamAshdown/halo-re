# Browser port

**Goal:** Halo PC 1.0.10 (halo-re) runs fully in a desktop browser (Chrome/Firefox, WebGL2 + WebAudio + WebAssembly). That means
boot to the main menu, play the campaign with working saves, and join or host multiplayer. The native Windows build (D3D9 and
GL) must keep working throughout.

**Done means:** `emcmake cmake` plus a build gives `halo.html/.js/.wasm`. Served from a static host with the user's own maps,
it boots to the menu and loads a.map ... d40.map. It holds 60 fps at 1080p on a mid-range GPU. Keyboard, mouse (pointer
lock) and gamepad work, sound plays, checkpoints and profiles survive a page reload, and a browser client can play
multiplayer against another browser client.

## Where things stand (2026-10-05)

Done:
- OpenGL renderer at parity with D3D9 (docs/OPENGL_PORT.md).
- No fixed exe base. Map memory stays at 0x40000000, which is fine because wasm32 linear memory can place it there.
- `halo::platform` layer (time, file, thread, memory, system, window, input, audio).
- Window, input, GL context and audio on SDL2. DirectInput, DirectSound and WGL are gone.

Still Windows-only, from a survey at commit e3038f5c:
- About 100 files include Windows/DirectX headers, and about 37 call Win32 directly.
- x87 inline asm: src/main/main_loop.cpp, src/math/x87.cpp, src/rasterizer/device.cpp, src/shell/{crash_reporter,hardware,system}.cpp.
- SEH (`__try/__except`): src/shell/{application,config,crash_reporter,diagnostics}.cpp.
- Winsock: src/networking/net1_runtime.cpp and src/gamespy/*.
- The GL backend uses MojoShader's GLSL 1.10 profile and desktop GL entry points.
- The main loop blocks (`MainLoop::loop`), so the browser can never get control back.

Browser bring-up (later on 2026-10-05, branch browser-port):
- The web build boots in Chrome at -O3, streams the maps from `tools/serve_web.py` and runs the main loop.
- At -O1 and above, an empty spin in `structure_bsp_loader::load` hung forever. It was found with `-DHALO_WEB_O0_SOURCES="dir/*.cpp;..."`, which compiles the listed files at -O0 for bisecting.
- Not yet checked by eye: menu rendering, input, audio and save persistence.

## Milestones

Do them in order. Each one leaves the Windows build working, gets the user's boot test (they run it and report), and is
committed on its own.

1. **Finish the platform layer.**
   - Move every remaining direct Win32 call behind `halo::platform` or SDL.
   - Remove `GetActiveWindow` from src/sound/directsound_device.cpp.
   - Cover the registry, clipboard, message boxes, CPU and hardware queries, and the crash reporter.
   - Add a `HALO_PLATFORM_NO_WIN32` guard and keep Win32-only diagnostics (crash dialog, diagnostics dialog) behind it.
   - Exit: outside src/platform/*_win32.cpp and the D3D9 backend, nothing includes `<windows.h>`.
   - 2026-10-05 (branch browser-port): 92 files no longer include win32.h; date/time text, file version, code page,
     double-click time, mapped file name, file size and ipv4 text moved to halo::platform; the GL backend has no
     windows.h. Left: Winsock (net1_runtime, net1_client_2, main/network, gamespy, application's inet_addr), the Win32
     console (ifr1_console_terminal), autopatch (wininet), and the Windows-only shell (application, system,
     diagnostics, crash_reporter, hardware probe).
   - 2026-10-05, later: done for compiling off Windows. POSIX platform layer (src/platform/*_posix.cpp: files with
     Win32 semantics and case-insensitive backslash paths, pthreads with alertable-wait completions, time, memory,
     system; SHA-1 and Windows-1252 in system_portable.cpp, checked against Windows by tools/platform_test.cpp).
     POSIX shell pieces (hardware probe, crash reporter, settings file, documents folder); types/win32.h gives BSD
     sockets under the Winsock names off Windows; the rest of the Win32-only code is guarded. CMake takes *_win32.cpp
     on Windows and *_posix.cpp elsewhere.
2. **Portable compiler surface.**
   - Replace the x87 asm (`finit/fldcw`) with a platform `fpu_reset()` that does nothing on wasm.
   - Replace SEH with platform crash hooks.
   - Make `__stdcall` and `__thiscall` empty macros on non-MSVC targets.
   - Check every `#pragma pack` struct and every pointer-size assumption (wasm32 is 32-bit like the original, so layouts hold).
   - Exit: a clang-cl or MinGW-clang build of the game with GL only compiles and boots.
   - 2026-10-05: inline assembly only in src/platform/cpu.cpp (fpu_reset, cpuid, time_stamp_counter; portable
     fallbacks) and src/math/x87.cpp (x87 trig kept on MSVC x86 for identical simulation, C library elsewhere);
     SEH behind HALO_TRY / HALO_EXCEPT (include/halo/platform/fault.hpp). Left: __stdcall/__thiscall macros, MSVC
     CRT names (_stricmp etc., ~60 files), 16-bit wchar_t (125 files use wcs*/swprintf/L"" as UTF-16: needs
     -fshort-wchar plus our own 16-bit wcs*/swprintf with MSVC %s semantics), types/*.h shadowing system headers
     (math.h, memory.h: use -iquote for types/).
   - 2026-10-05, later: done short of a real clang build. include/halo/clang_compat.h (calling conventions, CRT
     names, wchar16); halo/platform/wchar16 (MSVC-semantics 16-bit wide strings, checked against the MSVC CRT);
     redundant qualifiers, MSVC-only headers and conversions fixed; every engine C++ file passes
     `g++ -m32 -std=c++20 -fsyntax-only` with clang_compat.h force-included (MinGW, Windows mode).
     standalone_data_layout_check() verifies at startup that the 342 clustered globals keep the original offsets
     (a toolchain without MSVC's ordered sections), generated by tools/gen_layout_check.py.
3. **D3D9 out of the GL path.**
   - The GL build must not need d3d9.h, d3dx9 or the DXSDK. Move the D3D9 types the engine reads into the repo's own
     headers (types/), and keep the D3D9 backend as a Windows-only, optional source set.
   - Exit: a CMake option `HALO_RENDERER_D3D9=OFF` builds and boots in GL.
   - 2026-10-05: done as `-DHALO_D3D9=OFF` (build/gl-only): no DirectX SDK; D3DX stand-ins in d3dx_sdk.cpp;
     gl_direct3d.cpp answers the IDirect3D9 adapter/mode/format/caps queries. Renders the same frames.
4. **GLES 3.0 / WebGL2 renderer.**
   - Generate shaders with a GLSL ES 3.00 profile: patch the vendored MojoShader and record the change in README.halo.md
     and halo.patch.
   - Drop desktop-only GL: client-side vertex arrays (use VBOs), GL_QUADS, the fixed-function leftover in gl_device.cpp,
     sampler/texture formats WebGL lacks (BGRA, A8/L8 to R8 plus swizzle), DXT through EXT_texture_compression_s3tc
     with a decompress fallback, and glReadPixels formats.
   - Exit: native build with `SDL_GL_CONTEXT_PROFILE_ES` 3.0 (ANGLE) renders the same as today.
   - 2026-10-05: done. gl_essl.cpp rewrites MojoShader's GLSL 1.10 and the fixed-function stages as GLSL ES 3.00
     (one code path; desktop drivers take it through ARB_ES3_compatibility); D3DCOLOR attributes swizzled in the
     shader; no BGRA uploads/read-backs; glClearDepthf/glDepthRangef; polygon mode and border clamp desktop-only;
     ANY_SAMPLES_PASSED queries on ES. `HALO_GL_ES=1` runs on a real ES context (NVIDIA ES 3.2): frames match.
     Still to check under WebGL 2 proper: DXT needs WEBGL_compressed_texture_s3tc (add a CPU decompress fallback),
     float render targets need EXT_color_buffer_float.
5. **Emscripten build.**
   - Add an emscripten CMake toolchain path with SDL2 (`-sUSE_SDL=2`) and `-sMAX_WEBGL_VERSION=2`.
   - Size `-sINITIAL_MEMORY` so 0x40000000 + 0x1b40000 fits, or reserve map memory via the platform memory layer.
   - Threads: use `-pthread` with COOP/COEP headers, or make the async file reads synchronous.
   - Main loop: have `MainLoop::loop` run one frame per `emscripten_set_main_loop` callback. If that is too invasive at
     first, ship with `-sASYNCIFY` and replace it later. (2026-10-05: split into loop_begin / loop_frame / shutdown.)
   - Files: the game is for local use only, so a small local web server serves the user's Halo folder (maps, shaders,
     fx.bin override) and the browser streams files from it as the game opens them (HTTP range requests for the
     big .map files), never bundled. Saves and profiles go to IDBFS, synced after each write.
   - Needs the Emscripten SDK (a large download: ask the user first).
   - 2026-10-05: scaffolding written, not yet built: cmake/web.cmake (`emcmake cmake -S . -B build/web`),
     standalone/web_main.cpp (Halo folder as WasmFS fetch-backed files from the manifest, $HOME on OPFS),
     web/shell.html (Play button: audio and pointer lock need a click), the browser main loop in MainLoop::loop,
     tools/serve_web.py (range requests, manifest, COOP/COEP; tested with curl).
   - Next, with the SDK: configure and build, fix what clang reports (expected: Emscripten-only API details in
     web_main.cpp, link flags, the data image), then boot to the menu in Chrome.
   - Exit: boots to the main menu in Chrome.
6. **Playable campaign in the browser.**
   - Load times: stream maps lazily instead of preloading hundreds of MB.
   - Pointer lock and fullscreen.
   - Audio unlocks on the first user gesture.
   - Profiles and checkpoints persist across reloads.
   - Frame pacing on requestAnimationFrame.
   - Exit: a.map through d40.map are completable.
7. **Multiplayer.**
   - Browsers cannot use raw UDP. Put net1_runtime's sockets behind `halo::platform` and implement them on WebRTC data
     channels (unreliable, unordered) with a small signalling server, or on a WebSocket-to-UDP relay so retail servers
     still work.
   - GameSpy master-server lookups go through the same relay.
   - Exit: a browser client joins a retail-protocol server and a browser-hosted game.
8. **Polish and ship.**
   - Loading UI and a map-folder picker.
   - Error reporting to the console.
   - CI builds both Windows and wasm.
   - A static host page with the COOP/COEP headers.

## Ground rules

- Never push, never download third-party binaries without asking, and keep untracked private folders.
- The user boots and tests; find bugs from the code, not by repeated boots.
- Commit trailer: `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Record progress here with a dated line under each milestone as it lands.

## Checking frames without a screen capture

`HALO_GL_SNAPSHOT=30000,45000` writes the GL back buffer to gl_snapshot_<ms>.ppm beside the exe at those times after
the first frame; with the scripted start (`HALO_STANDALONE_KEYS=25000:ENTER,3000:ENTER,3000:ENTER,3000:ENTER`,
`-window -novideo`) the 45 s frame lands at the same spot of b30 every run, so renderer changes compare
pixel-for-pixel. `HALO_GL_ES=1` asks for an OpenGL ES 3.0 context.
