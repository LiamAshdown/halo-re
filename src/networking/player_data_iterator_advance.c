// player_data_iterator_advance  (Ghidra: FUN_004d98f0; renamed, no prior name)
// address 0x4d98f0, size 110 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md summary itself flags this one as unreliable:
// "Advances a datum iterator forward a given number of steps, intended to return the datum
// handle at that position (decompiled return value appears lossy)."
// register convention: `param_1` (step count) is Ghidra's only recovered parameter; the
// iterator itself is never visibly constructed in this function's body even though
// out/functions.json lists player_data (0x0087a480) among its referenced globals, so its setup
// must have been elided the same way `unaff_`-register arguments are elsewhere in this codebase.
// UNSURE (major, load-bearing): as decompiled, EVERY path through this function returns the
// literal constant 0xffffffff -- `param_1` is read (compared to -1) but never decremented or
// otherwise consumed, so the do-while loop's only exits are "iterator exhausted" and "param_1
// was -1 to begin with", both of which fall through to an unconditional `return 0xffffffff;`.
// This cannot be the real behaviour of a function whose only caller
// (network_session_player_table_index_apply.c / network_session_player_table_index_apply, same task batch) explicitly
// tests the result against 0 and 0xffffffff before using it as a live index -- the real function
// almost certainly returns the datum handle from the loop's last successful
// `data_iterator_next` call via a register (AX) Ghidra did not track back to the `return`
// statements, and `param_1` almost certainly does decrement per iteration in the real assembly.
// Per the task's rule to preserve semantics exactly and never invent behaviour, this is
// transcribed literally as Ghidra decompiled it (including the always--1 result), not "fixed"
// to the presumed intended behaviour, since there is no disassembly evidence here for what the
// correct fix would be. Flagged for follow-up with real disassembly.
// UNSURE: `data_iterator_next`'s iterator argument is not visibly constructed anywhere in this
// function; modeled as a local iterator freshly bound to `player_data`, matching the
// initialization convention already established in
// src/game/game_engine_send_team_allegiance_message.c, since player_data is the only global
// this function references.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>

extern data_array *player_data; // 0x0087a480
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI

int32_t player_data_iterator_advance(int16_t step_count)
{
    data_iterator iter; // UNSURE: construction elided by Ghidra; see file header
    void *element;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;

    element = data_iterator_next(&iter);
    if (element == 0) {
        return -1;
    }
    do {
        if (step_count == -1) {
            return -1;
        }
        element = data_iterator_next(&iter);
    } while (element != 0);
    return -1;
}

#if 0
Original Ghidra decompilation (0x4d98f0):

undefined4 FUN_004d98f0(short param_1)

{
  int iVar1;

  iVar1 = data_iterator_next();
  if (iVar1 == 0) {
    return 0xffffffff;
  }
  do {
    if (param_1 == -1) {
      return 0xffffffff;
    }
    iVar1 = data_iterator_next();
  } while (iVar1 != 0);
  return 0xffffffff;
}
#endif
