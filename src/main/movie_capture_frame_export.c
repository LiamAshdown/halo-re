// movie_capture_frame_export  (Ghidra: movie_capture_frame_export, already named)
// address 0x4c9530, size 181 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: matches the given name. out/phase4/main_types_notes.md: "0x4c9530 / 0x4ca1a0: a
// 0x43-dword memset, signature filo at +0x00, flags byte +0x04, location -1 at +0x06, path built
// by path_append_component from +0x08" is types/saved_games.h file_reference_record;
// main_globals.movie_frame_bitmap (0x00719724) "points at one of these; FUN_00518180 (rasterizer
// capture) takes it on the stack" and "passes it in EAX to targa_export". movie_frame_index
// (0x00719730) and screenshot_tile_count (0x00719aac) are main.h main_globals fields.
// rasterizer_capture_and_present (0x518180) and path_append_component/path_remove_last_component
// (EBX -> component/path, ESI -> reference) reuse src/rasterizer/rasterizer_capture_and_present.c
// and src/game/savegame_index_append_slot.c's established signatures.
// register convention: cdecl, no parameters.
// UNSURE: targa_export (0x43fe60) is not established anywhere else; declared here from
// main_types_notes.md's own note that it takes the bitmap in EAX. The `flags & 1` check gating
// path_remove_last_component is unreachable on every call (the local file_reference_record is
// freshly zeroed immediately before), preserved verbatim rather than dropped as dead code.

// phase 4 review (disassembly 0x4c9530..0x4c95e4): path_append_component takes the destination in
// ESI (&request.path, record +0x08) and the component in EBX; the phase 3 file passed the
// record base as the destination (8 bytes early, over signature / flags / location).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "saved_games.h"
#include "hs.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern main_globals main_globals_data; // 0x00719700

extern void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); // 0x518180, foreign (rasterizer module)
extern void path_append_component(char *destination, const char *component); // 0x555ec0, foreign (saved_games)
    // blam-cc: ESI -> destination, EBX -> component
extern void path_remove_last_component(uint8_t *path); // 0x555f80, foreign (game module)
extern char * targa_export(BitmapData *bitmap, file_reference_record *destination); // 0x43fe60, foreign
    // blam-cc: EAX -> bitmap, EBX -> destination (0x4c95cb mov eax,[0x719724] ; lea ebx,[esp+0x10])

// Captures the current frame into movie_frame_bitmap and, once per call while capturing is
// active, exports it as "movie\\frameNNNNNN.tga" (the frame index counting up from
// movie_frame_index), only while no screenshot tiling is also in progress.
void movie_capture_frame_export(void)
{
    char path[0x200];
    BitmapData *movie_frame_bitmap = (BitmapData *)main_globals_data.movie_frame_bitmap;

    rasterizer_capture_and_present(0, movie_frame_bitmap);

    if (main_globals_data.screenshot_tile_count < 1 && movie_frame_bitmap != 0) {
        file_reference_record request;

        _snprintf(path, 0x200, "movie\\frame%06d.tga", main_globals_data.movie_frame_index);
        main_globals_data.movie_frame_index = main_globals_data.movie_frame_index + 1;

        memset(&request, 0, sizeof(request));
        request.signature = 0x66696c6f; // 'filo'
        request.location = -1;

        if ((request.flags & 1) != 0) {
            path_remove_last_component((uint8_t *)&request.path);
        }
        path_append_component(request.path, path);   // 0x4c95bb EBX = path, ESI = &request.path (+0x08)
        request.flags = request.flags | 1;

        targa_export(movie_frame_bitmap, &request);
    }
}

#if 0
Original Ghidra decompilation (0x4c9530):

void __cdecl movie_capture_frame_export(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_318;
  byte local_314;
  undefined2 local_312;
  char local_208 [516];

  FUN_00518180(DAT_00719724);
  iVar1 = DAT_00719730;
  if ((DAT_00719aac < 1) && (DAT_00719724 != 0)) {
    DAT_00719730 = DAT_00719730 + 1;
    __snprintf(local_208,0x200,"movie\\frame%06d.tga",iVar1);
    puVar2 = &local_318;
    for (iVar1 = 0x43; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    local_318 = 0x66696c6f;
    local_312 = 0xffff;
    if ((local_314 & 1) != 0) {
      path_remove_last_component();
    }
    path_append_component();
    local_314 = local_314 | 1;
    targa_export();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
