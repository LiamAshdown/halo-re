// game_engine_broadcast_kill_feed_or_direct  (Ghidra: FUN_00460d10; renamed per its summary)
// address 0x460d10, size 159 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Broadcasts a single kill-feed message id to every
// entry in the current data iteration"); types/memory.h data_iterator; the identical explicit
// `data_iterator iter` idiom already committed in game_engine_on_player_death.c.
// register convention: a recipient-or-broadcast-all selector in EAX (in_EAX: -1 means "every
// player", anything else is a single specific recipient handle); a gate in ESI (unaff_ESI,
// tested every send exactly like the sibling broadcast helpers); param_1 and subject are this
// function's own stack parameters; broadcast is EBX.
//   // blam-cc: EAX -> recipient_or_all, ESI -> broadcast_enabled, EBX -> broadcast, stack ->
//   param_1, subject
// FIXED (register inputs, objdump): EBX carries broadcast (read at 0x460d2d and again at
// 0x460d81/0x460d90, always pushed as chimera__kill_feed's last stack argument). Working out
// EBX also exposed two other mistakes in the old "UNSURE" forwarded arguments: what was modeled
// as a separate "forwarded_message_type" stack parameter is really the same ESI register as
// broadcast_enabled (pushed unchanged at every call site, right after the gate test), and what
// was modeled as a third guessed stack parameter ("forwarded_subject") is actually this
// function's own genuine second stack argument (loaded once into EBP at 0x460d14 and forwarded
// unchanged); both are corrected here rather than left as separate phantom parameters.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0


// blam-cc: EAX -> recipient_or_all, ESI -> broadcast_enabled, EBX -> broadcast, stack -> param_1,
//   subject
// When `recipient_or_all` is -1, broadcasts to every in-use player (recipient = that player's
// own handle each time); otherwise sends once, directly to `recipient_or_all`. Both paths pass
// `param_1` (or -1 when `param_1` is itself -1) as chimera__kill_feed's own extra argument, and
// both are gated on `broadcast_enabled`, which doubles as chimera__kill_feed's message_type.
void game_engine_broadcast_kill_feed_or_direct(datum_index recipient_or_all, int32_t broadcast_enabled,
    char broadcast, int32_t hash_key, datum_index subject)
{
    int32_t forwarded_param_1 = (hash_key == -1) ? -1 : hash_key;

    if (recipient_or_all == (datum_index)0xffffffff) {
        data_iterator iter;
        void *element;

        iter.data = player_data;
        iter.next_index = 0;
        iter.index = (datum_index)0xffffffff;
        iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

        element = data_iterator_next(&iter);
        while (element != 0) {
            if (broadcast_enabled != -1) {
                chimera__kill_feed(iter.index, forwarded_param_1, broadcast_enabled, subject,
                    broadcast);
            }
            element = data_iterator_next(&iter);
        }
    } else if (broadcast_enabled != -1) {
        chimera__kill_feed(recipient_or_all, hash_key, broadcast_enabled, subject, broadcast);
    }
}

#if 0
Original Ghidra decompilation (0x460d10), from tools/pack.py 0x460d10:

void FUN_00460d10(int param_1)

{
  int in_EAX;
  int iVar1;
  int unaff_ESI;
  int local_8;
  
  if (in_EAX == -1) {
    local_8 = -1;
    iVar1 = data_iterator_next();
    while (iVar1 != 0) {
      iVar1 = param_1;
      if (param_1 == -1) {
        iVar1 = local_8;
      }
      if (unaff_ESI != -1) {
        chimera__kill_feed(iVar1);
      }
      iVar1 = data_iterator_next();
    }
  }
  else if (unaff_ESI != -1) {
    chimera__kill_feed(param_1);
    return;
  }
  return;
}
#endif
