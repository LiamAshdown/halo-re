// game_engine_broadcast_kill_feed_gated  (Ghidra: FUN_00460db0; renamed per its summary)
// address 0x460db0, size 122 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Conditionally broadcasts a kill-feed message id to
// every entry in the current data iteration"); the sole caller, game_engine_on_player_death.c,
// which already documents this exact call as "UNSURE: ... modelled here as (killer, victim) on
// both" -- i.e. this function is called there as FUN_00460db0(killer, victim); types/memory.h
// data_iterator, same idiom as the sibling broadcast helpers in this address range.
// register convention: a gate in ESI (unaff_ESI, tested every send exactly like the sibling
// broadcast helpers, and also forwarded as chimera__kill_feed's message_type argument);
// exclude_index/alternate_recipient/subject are this function's own stack parameters.
//   // blam-cc: ESI -> broadcast_enabled, EBX -> broadcast, stack -> exclude_index,
//   //   alternate_recipient, subject
// FIXED (register inputs, objdump): EBX (read at 0x460e07/0x460e12, "push ebx", live-in and
// never assigned in this function) is chimera__kill_feed's `broadcast` argument -- pushed first
// in both branches, i.e. the last/bottom stack argument per chimera__kill_feed's own recovered
// convention (stack -> param_1, message_type, subject, broadcast). Re-deriving the whole push
// sequence against that convention (0x460e07..0x460e16) also caught two pre-existing mistakes
// this rewrite is fixing here since they involve the same call: (1) `forwarded_message_type`
// was sourced from EBP (stack idx2, offset 0xc), but EBP is actually pushed second-from-top,
// i.e. chimera__kill_feed's `subject` argument -- `message_type` is really ESI
// (broadcast_enabled), pushed third; (2) two stack slots this rewrite invented
// (`forwarded_subject`, `forwarded_broadcast` at offsets 0x10/0x14) are never read anywhere in
// 0x460db0..0x460e30 -- dropped, since `subject` is really the offset-0xc stack word (renamed
// from `forwarded_message_type`) and `broadcast` is EBX. (3) the skip test at 0x460df4
// (`cmp edi,[esp+0x1c]`) compares the current recipient against `param_1`, not `param_1` against
// -1 as Ghidra's pseudocode showed it (`if (param_1 != -1)`) -- renamed to `exclude_index` and
// fixed to skip only the matching player, and `forwarded_param_1`'s -1 case now falls back to
// the recipient itself (EDI, matching the 0x460e0a push) rather than a literal -1.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void chimera__kill_feed(datum_index recipient, int32_t param_1, uint32_t message_type,
    datum_index subject, char broadcast); // 0x460a30, this batch

// blam-cc: ESI -> broadcast_enabled, EBX -> broadcast, stack -> exclude_index,
//   alternate_recipient, subject
// Broadcasts to every in-use player except `exclude_index` (recipient = that player's own
// handle each time), passing `alternate_recipient` (or the recipient itself when
// `alternate_recipient` is -1) as chimera__kill_feed's own param_1, gated on `broadcast_enabled`
// (also forwarded as chimera__kill_feed's message_type).
void game_engine_broadcast_kill_feed_gated(int32_t broadcast_enabled, int32_t exclude_index,
    int32_t alternate_recipient, datum_index subject, char broadcast)
{
    data_iterator iter;
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = data_iterator_next(&iter);
    while (element != 0) {
        if ((int32_t)iter.index != exclude_index) {
            int32_t forwarded_param_1 = (alternate_recipient == -1) ? (int32_t)iter.index : alternate_recipient;
            if (broadcast_enabled != -1) {
                chimera__kill_feed(iter.index, forwarded_param_1, broadcast_enabled, subject, broadcast);
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
