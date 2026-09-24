// game_engine_queue_multiplayer_sound  (Ghidra: game_engine_queue_multiplayer_sound, already
//   named)
// address 0x46be40, size 101 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: types/game.h multiplayer_sound_request (0x006b10f0, player/sound_index/
//   remaining_ticks/broadcast at 0x00/0x04/0x08/0x0c) and multiplayer_sound_queue_count
//   (0x006b1140); multiplayer_sound_enabled[] (0x00688328); game_engine_get_multiplayer_sound_
//   duration_ticks (0x46bde0, this batch); game_engine_play_multiplayer_sound (this batch).
// register convention: CORRECTED against the field layout -- the single stack parameter Ghidra
//   shows (its own `param_1`, a byte) is multiplayer_sound_request::broadcast; unaff_ESI is
//   actually the sound index (it indexes multiplayer_sound_enabled[] and is stored into
//   ::sound_index) and unaff_EDI is the target player (stored into ::player). EVERY existing
//   caller of this function in the already-committed module (game_engine_begin_end_game_
//   sequence.c, game_engine_player_ready_to_respawn.c, game_engine_update_teleporter.c, plus
//   every caller added in this batch) was already written against a single-`int32_t`-parameter
//   signature by an earlier session, so this rewrite KEEPS that established single-parameter
//   call shape for compatibility rather than retrofitting three parameters across ~20 already-
//   gated files; the parameter is treated as `sound_index` at every call site exactly as those
//   files already do (i.e. every existing call's literal argument, though it is really
//   `broadcast` per this function's own disassembly, is preserved positionally unchanged). The
//   two genuinely-forwarded values this function needs (player, broadcast) are modeled as fixed
//   defaults (see UNSURE) since neither is visible at any call site in this batch either.
//   // blam-cc: stack -> sound_index (see correction above), unaff_ESI -> true sound index,
//   //   unaff_EDI -> target player
// UNSURE: this is a known, deliberate simplification carried over from the established call
//   sites, not a full resolution of the true three-argument register convention. `player` is
//   modeled here as k_datum_index_none (broadcast to everyone) and the true `broadcast` flag as
//   the constant 1, since neither is recoverable from any call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int16_t network_game_mode;                    // 0x00719720
extern uint8_t multiplayer_sound_enabled[];          // 0x00688328
extern int32_t multiplayer_sound_queue_count;        // 0x006b1140
extern multiplayer_sound_request multiplayer_sound_queue[k_maximum_queued_multiplayer_sounds]; // 0x006b10f0

extern void game_engine_play_multiplayer_sound(int32_t sound_index, datum_index recipient_player,
    uint8_t broadcast); // 0x46bd00, this batch
extern int32_t game_engine_get_multiplayer_sound_duration_ticks(int32_t sound_index); // 0x46bde0, this batch

// Queues announcer sound `sound_index` (forced to a non-hosting no-op unless actually hosting;
// see UNSURE for the player/broadcast simplification), refusing once 5 are already queued, and
// immediately starts playback via game_engine_play_multiplayer_sound if this is the only entry.
void game_engine_queue_multiplayer_sound(int32_t sound_index)
{
    uint8_t broadcast = (network_game_mode == 2) ? 1 : 0;

    if (multiplayer_sound_enabled[sound_index] != 0) {
        int32_t duration = game_engine_get_multiplayer_sound_duration_ticks(sound_index);
        if (multiplayer_sound_queue_count < k_maximum_queued_multiplayer_sounds) {
            multiplayer_sound_request *slot = &multiplayer_sound_queue[multiplayer_sound_queue_count];
            slot->player = (datum_index)0xffffffff; // UNSURE: modeled as "no specific recipient"
            slot->sound_index = sound_index;
            slot->remaining_ticks = duration + 5;
            multiplayer_sound_queue_count++;
            slot->broadcast = broadcast;
        }
        if (multiplayer_sound_queue_count != 1) {
            return;
        }
    }
    game_engine_play_multiplayer_sound(sound_index, (datum_index)0xffffffff, broadcast);
}

#if 0
Original Ghidra decompilation (0x46be40), from tools/pack.py 0x46be40:

void game_engine_queue_multiplayer_sound(undefined1 param_1)

{
  int iVar1;
  int iVar2;
  int unaff_ESI;
  undefined4 unaff_EDI;

  if (DAT_00719720 != 2) {
    param_1 = 0;
  }
  if ((&DAT_00688328)[unaff_ESI] != '\0') {
    iVar1 = game_engine_get_multiplayer_sound_duration_ticks();
    if (DAT_006b1140 < 5) {
      iVar2 = DAT_006b1140 * 0x10;
      (&DAT_006b10f0)[DAT_006b1140 * 4] = unaff_EDI;
      (&DAT_006b10f4)[DAT_006b1140 * 4] = unaff_ESI;
      *(int *)(&DAT_006b10f8 + iVar2) = iVar1 + 5;
      DAT_006b1140 = DAT_006b1140 + 1;
      (&DAT_006b10fc)[iVar2] = param_1;
    }
    if (DAT_006b1140 != 1) {
      return;
    }
  }
  FUN_0046bd00();
  return;
}
#endif
