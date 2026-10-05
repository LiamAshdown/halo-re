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

## Milestones

Do them in order. Each one leaves the Windows build working, gets the user's boot test (they run it and report), and is
committed on its own.

1. **Finish the platform layer.**
   - Move every remaining direct Win32 call behind `halo::platform` or SDL.
   - Remove `GetActiveWindow` from src/sound/directsound_device.cpp.
   - Cover the registry, clipboard, message boxes, CPU and hardware queries, and the crash reporter.
   - Add a `HALO_PLATFORM_NO_WIN32` guard and keep Win32-only diagnostics (crash dialog, diagnostics dialog) behind it.
   - Exit: outside src/platform/*_win32.cpp and the D3D9 backend, nothing includes `<windows.h>`.
2. **Portable compiler surface.**
   - Replace the x87 asm (`finit/fldcw`) with a platform `fpu_reset()` that does nothing on wasm.
   - Replace SEH with platform crash hooks.
   - Make `__stdcall` and `__thiscall` empty macros on non-MSVC targets.
   - Check every `#pragma pack` struct and every pointer-size assumption (wasm32 is 32-bit like the original, so layouts hold).
   - Exit: a clang-cl or MinGW-clang build of the game with GL only compiles and boots.
3. **D3D9 out of the GL path.**
   - The GL build must not need d3d9.h, d3dx9 or the DXSDK. Move the D3D9 types the engine reads into the repo's own
     headers (types/), and keep the D3D9 backend as a Windows-only, optional source set.
   - Exit: a CMake option `HALO_RENDERER_D3D9=OFF` builds and boots in GL.
4. **GLES 3.0 / WebGL2 renderer.**
   - Generate shaders with a GLSL ES 3.00 profile: patch the vendored MojoShader and record the change in README.halo.md
     and halo.patch.
   - Drop desktop-only GL: client-side vertex arrays (use VBOs), GL_QUADS, the fixed-function leftover in gl_device.cpp,
     sampler/texture formats WebGL lacks (BGRA, A8/L8 to R8 plus swizzle), DXT through EXT_texture_compression_s3tc
     with a decompress fallback, and glReadPixels formats.
   - Exit: native build with `SDL_GL_CONTEXT_PROFILE_ES` 3.0 (ANGLE) renders the same as today.
5. **Emscripten build.**
   - Add an emscripten CMake toolchain path with SDL2 (`-sUSE_SDL=2`) and `-sMAX_WEBGL_VERSION=2`.
   - Size `-sINITIAL_MEMORY` so 0x40000000 + 0x1b40000 fits, or reserve map memory via the platform memory layer.
   - Threads: use `-pthread` with COOP/COEP headers, or make the async file reads synchronous.
   - Main loop: have `MainLoop::loop` run one frame per `emscripten_set_main_loop` callback. If that is too invasive at
     first, ship with `-sASYNCIFY` and replace it later.
   - Files: maps are fetched or `--preload-file`d from a user-supplied folder, never bundled (copyrighted). Saves and
     profiles go to IDBFS, synced after each write.
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
