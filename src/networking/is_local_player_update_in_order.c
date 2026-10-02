// is_local_player_update_in_order  (Ghidra: is_local_player_update_in_order, already named)
// address 0x4e69b0, size 103 bytes
// name confidence: 0.85   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md; types/networking.h "player update history"
// section (the local-player ordering window of 15); game_time+0xc, the same
// tick-counter global player_update_history_log_write's callers all format as "[%d]".
// register convention: EAX -> current_update_id, ECX -> new_update_id (matching this batch's
// other in-order test, is_remote_player_update_in_order).
//   // blam-cc: EAX -> current_update_id, ECX -> new_update_id
// UNSURE: player_update_history_log_write's two leading register arguments are dropped by the
// batch decompile at both call sites here; (1, 0) is carried over from sibling call sites in this
// file group that have disassembly-confirmed values.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...); // this module, 0x4e5ea0

// True unless new_update_id is more than 15 ahead of current_update_id (forward case) or more
// than 15 behind once wrapped modulo 64 (backward/wrap case), in which case the ack is logged and
// discarded.
uint8_t is_local_player_update_in_order(int32_t current_update_id, int32_t new_update_id)
    // blam-cc: EAX -> current_update_id, ECX -> new_update_id
{
    if (current_update_id != -1) {
        if (current_update_id < new_update_id) {
            if (0xf < new_update_id - current_update_id) {
                player_update_history_log_write(1, 0,
                    "[%d]a: Threw away local player ack [%d] (%d).\n",
                    game_time->game_time);
                return 0;
            }
        } else if (0xf < (new_update_id - current_update_id) + 0x20) {
            player_update_history_log_write(1, 0,
                "[%d]b: Threw away local player ack [%d] (%d).\n",
                game_time->game_time);
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e69b0), from tools/pack.py 0x4e69b0:

undefined4 is_local_player_update_in_order(void)

{
  int in_EAX;
  int in_ECX;

  if (in_EAX != -1) {
    if (in_EAX < in_ECX) {
      if (0xf < in_ECX - in_EAX) {
        player_update_history_log_write
                  ("[%d]a: Threw away local player ack [%d] (%d).\n",
                   *(undefined4 *)(DAT_006f1d6c + 0xc));
        return 0;
      }
    }
    else if (0xf < (in_ECX - in_EAX) + 0x20) {
      player_update_history_log_write
                ("[%d]b: Threw away local player ack [%d] (%d).\n",
                 *(undefined4 *)(DAT_006f1d6c + 0xc));
      return 0;
    }
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
