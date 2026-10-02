// hs_evaluate_nothing  (not a Ghidra function; the evaluate handler shared by 39 hs functions, listed below)
// address 0x47cf20, size 11 bytes
// shared by: 92 "render_effects" (boolean -> void); 156 "cheats_load" (no parameters -> void);
//   168 "ai_deselect" (no parameters -> void); 220 "ai_reconnect" (no parameters -> void);
//   260 "profile_service_clear_timers" (no parameters -> void);
//   261 "profile_service_dump_timers" (no parameters -> void); 272 "texture_cache_flush" (no parameters -> void);
//   273 "sound_cache_flush" (no parameters -> void); 275 "debug_memory" (no parameters -> void);
//   276 "debug_memory_by_file" (no parameters -> void); 277 "debug_memory_for_file" (string -> void);
//   278 "debug_tags" (no parameters -> void); 279 "profile_reset" (no parameters -> void);
//   280 "profile_dump" (string -> void); 281 "profile_activate" (string -> void);
//   282 "profile_deactivate" (string -> void); 283 "profile_graph_toggle" (string -> void);
//   287 "ai_lines" (no parameters -> void); 288 "ai_debug_sound_point_set" (no parameters -> void);
//   289 "ai_debug_vocalize" (string, string -> void); 290 "ai_debug_teleport_to" (ai -> void);
//   291 "ai_debug_speak" (string -> void); 292 "ai_debug_speak_list" (string -> void);
//   304 "attract_mode_start" (no parameters -> void); 335 "debug_sounds_distances" (string, real, real -> void);
//   336 "debug_sounds_wet" (string, real -> void); 379 "hammer_begin" (string, string, long, short, short -> void);
//   380 "hammer_stop" (no parameters -> void); 381 "network_server_dump" (no parameters -> void);
//   382 "network_client_dump" (no parameters -> void); 386 "show_player_update_stats" (no parameters -> void);
//   387 "message_metrics_clear" (no parameters -> void); 390 "structure_lens_flares_place" (no parameters -> void);
//   412 "time_code_show" (boolean -> void); 413 "time_code_start" (boolean -> void);
//   414 "time_code_reset" (no parameters -> void); 417 "rasterizer_decals_flush" (no parameters -> void);
//   418 "rasterizer_fps_accumulate" (no parameters -> void); 429 "delete_save_game_files" (no parameters -> void);
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] -> record, evaluate (+0xc) 0x47cf20, only reachable through
//   that pointer. Campaign track: a10's scripts call it.
// objdump 0x47cf20..0x47cf2b: returns 0 without evaluating anything: the evaluator of the debug-only and editor-only functions retail
//   compiled out (ai_debug_vocalize, time_code_start, hammer_begin, attract_mode_start, ...).
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

void hs_evaluate_nothing(int16_t function_index, uint32_t thread_index, char first)
{
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
