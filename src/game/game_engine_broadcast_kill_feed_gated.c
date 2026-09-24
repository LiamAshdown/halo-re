// game_engine_broadcast_kill_feed_gated  (Ghidra: FUN_00460db0; renamed per its summary)
// address 0x460db0, size 122 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Conditionally broadcasts a kill-feed message id to
// every entry in the current data iteration"); the sole caller, game_engine_on_player_death.c,
// which already documents this exact call as "UNSURE: ... modelled here as (killer, victim) on
// both" -- i.e. this function is called there as FUN_00460db0(killer, victim); types/memory.h
// data_iterator, same idiom as the sibling broadcast helpers in this address range.
// register convention: a gate in ESI (unaff_ESI, tested every send exactly like the sibling
// broadcast helpers); param_1/param_2 are this function's own stack parameters.
//   // blam-cc: unaff_ESI -> broadcast_enabled, stack -> param_1, param_2
// UNSURE: same unrecoverable trailing chimera__kill_feed arguments (message_type, subject,
// broadcast) as the sibling broadcast helpers in this address range; modeled as forwarded
// parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void chimera__kill_feed(datum_index recipient, int32_t param_1, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30, this batch

// blam-cc: unaff_ESI -> broadcast_enabled, stack -> param_1, param_2
// While `param_1` is not -1, broadcasts to every in-use player (recipient = that player's own
// handle each time), passing `param_2` (or -1 when `param_2` is itself -1) as chimera__kill_feed's
// own extra argument, gated on `broadcast_enabled`.
void game_engine_broadcast_kill_feed_gated(int32_t broadcast_enabled, int32_t param_1, int32_t param_2,
    uint32_t forwarded_message_type, datum_index forwarded_subject, char forwarded_broadcast) // UNSURE: last 3 params
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;

    element = data_iterator_next(&iter);
    while (element != 0) {
        if (param_1 != -1) {
            int32_t forwarded_param_1 = (param_2 == -1) ? -1 : param_2;
            if (broadcast_enabled != -1) {
                chimera__kill_feed(iter.index, forwarded_param_1, forwarded_message_type,
                    forwarded_subject, forwarded_broadcast);
            }
        }
        element = data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x460db0), from tools/pack.py 0x460db0:

void FUN_00460db0(int param_1,int param_2)

{
  int iVar1;
  int unaff_ESI;
  int local_8;
  
  local_8 = -1;
  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if (param_1 != -1) {
      iVar1 = param_2;
      if (param_2 == -1) {
        iVar1 = local_8;
      }
      if (unaff_ESI != -1) {
        chimera__kill_feed(iVar1);
      }
    }
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
