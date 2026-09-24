// game_unload_map  (Ghidra: FUN_0045afb0; renamed per symbols/review_queue.txt)
// address 0x45afb0, size 152 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: symbols/review_queue.txt 0x45afb0 "spins on FUN_004434a0 until the cache load state
//   finishes, then calls cache_file_unload and resets the tag-index globals
//   (DAT_00746f8c/90/94/98/9c/a0) to their [defaults]"; types/game.h global_scenario
//   (0x00746f8c).
// register convention: no arguments.
// reconciled: R07 0x00746f94 tag_cache_render_states_* (TYPES-GAP) -> scenario.h scenario_game_globals *global_scenario_game_globals (0x7c-byte scenario game-state block)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "scenario.h"

extern uint8_t download_in_progress;   // 0x006ac470
extern uint8_t *cache_file_slot_table; // 0x006b0b80, TYPES-GAP
extern scenario_game_globals *global_scenario_game_globals; // 0x00746f94, scenario.h
extern uint32_t unknown_0069e8d4; // TYPES-GAP
extern uint16_t unknown_0069e8d8; // TYPES-GAP
extern Scenario *global_scenario; // 0x00746f8c, cache module
extern void *unknown_00746f9c;    // TYPES-GAP
extern void *unknown_00746f98;    // TYPES-GAP
extern void *unknown_00746f90;    // TYPES-GAP
extern Globals *global_globals;   // 0x00746fa0

extern int16_t cache_file_download_status_get(float *progress_out, int32_t unaff_ecx); // 0x4434a0,
    // blam-cc: EAX -> progress_out, ECX -> unaff_ecx (src/cache/cache_file_download_status_get.c) // 0x4434a0
extern void FUN_004c8f20(void);              // UNSURE module
extern void movie_capture_frame_export(void); // 0x4c9530
extern void widget_close_all(void);           // 0x498650
extern void interface_handle_quit_request(void); // 0x499170
extern void cache_file_download_finish(void); // 0x443540
extern void cache_file_unload(void);          // 0x442430

// Spins pumping the download/movie-export loop until any pending cache download finishes, then
// unloads the current map cache file and resets the tag-index globals to their unloaded state.
void game_unload_map(void)
{
    int16_t status;

    if (download_in_progress != 0) {
        cache_file_slot_table[3] = 1;
        do {
            // objdump 0x45afd0..0x45afd8: EAX = cache_file_slot_table + 4, the float the
            // download poll writes its progress into; ECX is never set at this call site.
            status = cache_file_download_status_get((float *)(cache_file_slot_table + 4), 0);
            FUN_004c8f20();
            movie_capture_frame_export();
        } while (status == 0);
        widget_close_all();
        if (status == 2) {
            interface_handle_quit_request();
        }
        cache_file_download_finish();
    }
    if (*cache_file_slot_table != 0) {
        cache_file_unload();
        global_scenario_game_globals->structure_bsp_index = -1; // WORD +0x00
        unknown_0069e8d4 = 0xffffffff;
        unknown_0069e8d8 = 0xffff;
        global_scenario = (Scenario *)0;
        unknown_00746f9c = (void *)0;
        unknown_00746f98 = (void *)0;
        unknown_00746f90 = (void *)0;
        global_globals = (Globals *)0;
        *cache_file_slot_table = 0;
    }
}

#if 0
Original Ghidra decompilation (0x45afb0), from tools/pack.py 0x45afb0:

void FUN_0045afb0(void)

{
  short sVar1;

  if (DAT_006ac470 != '\0') {
    DAT_006b0b80[3] = '\x01';
    do {
      sVar1 = cache_file_download_status_get();
      FUN_004c8f20();
      movie_capture_frame_export();
    } while (sVar1 == 0);
    widget_close_all();
    if (sVar1 == 2) {
      interface_handle_quit_request();
    }
    cache_file_download_finish();
  }
  if (*DAT_006b0b80 != '\0') {
    cache_file_unload();
    *DAT_00746f94 = 0xffff;
    DAT_0069e8d4 = 0xffffffff;
    DAT_0069e8d8 = 0xffff;
    global_scenario = 0;
    DAT_00746f9c = 0;
    DAT_00746f98 = 0;
    DAT_00746f90 = 0;
    DAT_00746fa0 = 0;
    *DAT_006b0b80 = '\0';
  }
  return;
}
#endif
