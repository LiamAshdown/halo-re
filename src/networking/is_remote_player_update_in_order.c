// is_remote_player_update_in_order  (Ghidra: is_remote_player_update_in_order, already named)
// address 0x4e6a20, size 121 bytes
// name confidence: 0.85   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md; types/networking.h "player update history"
// section (the remote-player ordering window of 3, previous id at player+0x15c);
// player_update_history_log_printf_filtered.c (this batch's own review-pass rewrite establishes
// its real signature as (player *target_player /* EAX */, int32_t unused_arg, const char *format, ...)).
// register convention: EAX -> plr, EDX -> new_update_id.
//   // blam-cc: EAX -> plr, EDX -> new_update_id
// UNSURE (load-bearing): the batch decompile drops player_update_history_log_printf_filtered's
// EAX-passed target_player entirely at both call sites here, showing only the literal unused_arg
// (1), the format string and one vararg. plr (this function's own EAX) is the only player pointer
// in scope and is passed through unchanged, matching the register-passthrough idiom this file
// group uses elsewhere (e.g. player_update_client_local_player_update_from_network.c's decode
// context, handle_remote_player_action_update.c's control_source).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern game_time_globals *game_time; // 0x006f1d6c, +0x0c is the current game tick

extern void player_update_history_log_printf_filtered(player *target_player, int32_t unused_arg,
    const char *format, ...); // this module, 0x4e5f20

// True unless new_update_id (a byte-wide wrapping sequence number) is more than 3 ahead of
// plr's previously recorded remote-player update id, or wrapped more than 3 behind it.
uint8_t is_remote_player_update_in_order(player *plr, uint32_t new_update_id)
    // blam-cc: EAX -> plr, EDX -> new_update_id
{
    int32_t previous_id;
    int32_t delta;
    uint8_t new_id_byte;

    previous_id = *(int32_t *)((uint8_t *)plr + 0x15c);
    if (previous_id == -1) {
        return 1;
    }
    new_id_byte = (uint8_t)new_update_id;
    delta = new_id_byte - previous_id;
    if (previous_id < new_id_byte) {
        if (3 < delta) {
            player_update_history_log_printf_filtered(plr, 1,
                "[%d]a: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n",
                game_time->game_time);
            return 0;
        }
    } else if (3 < delta + 8) {
        player_update_history_log_printf_filtered(plr, 1,
            "[%d]b: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n",
            game_time->game_time);
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e6a20), from tools/pack.py 0x4e6a20:

undefined4 is_remote_player_update_in_order(void)

{
  int iVar1;
  int in_EAX;
  uint in_EDX;
  int iVar2;

  iVar1 = *(int *)(in_EAX + 0x15c);
  if (iVar1 == -1) {
    return 1;
  }
  iVar2 = (in_EDX & 0xff) - iVar1;
  if (iVar1 < (int)(in_EDX & 0xff)) {
    if (3 < iVar2) {
      player_update_history_log_printf_filtered
                (1,
                 "[%d]a: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n"
                 ,*(undefined4 *)(DAT_006f1d6c + 0xc));
      return 0;
    }
  }
  else if (3 < iVar2 + 8) {
    player_update_history_log_printf_filtered
              (1,
               "[%d]b: Threw away remote player position update [%d] (%d), previous ack [%d] (%d).\n"
               ,*(undefined4 *)(DAT_006f1d6c + 0xc));
    return 0;
  }
  return 1;
}
#endif
