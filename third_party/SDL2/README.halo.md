# SDL2 (vendored)

Simple DirectMedia Layer (https://github.com/libsdl-org/SDL), release 2.32.10, zlib licence (LICENSE.txt), from
`SDL2-devel-2.32.10-VC.zip` (sha256 af347939395a58b365846aaea27391e69f9ec9d4dd650d6ac40802159b418a6e). Only the headers
and the 32-bit import library and DLL are kept; the build copies SDL2.dll next to halo_rebuilt.exe. The browser build
uses Emscripten's own SDL2 port instead.

halo::platform implements the window, events, input and the other services SDL covers on top of it
(src/platform/*_sdl.cpp). The game keeps its own WinMain, so SDL2main is not used (SDL_SetMainReady is called before
SDL_Init).

To update: replace include/ and lib/x86/ from a newer SDL2-devel-*-VC.zip and rebuild.
