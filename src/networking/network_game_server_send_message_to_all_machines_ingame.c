// network_game_server_send_message_to_all_machines_ingame  (Ghidra: already named)
// address 0x4e4ef0, size 12 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md ("Clears a flag on a broadcast-message context,
// likely restricting a subsequent server broadcast to only clients currently in-game").
// register convention: the sole instruction operates on an implicit ESI pointer Ghidra could not
// attribute to a formal parameter; no caller in this batch or elsewhere in the codebase was
// found to cross-check the pointed-to type against.
//   // blam-cc: ESI -> context
// UNSURE: the shape/owner of `context`; +0x1c is written as a single byte with no other evidence
// in this batch of what type it belongs to.

#include "tags.h"
#include "memory.h"

// Clears byte +0x1c of an opaque broadcast-message context, per the function's (unresolved)
// name.
void network_game_server_send_message_to_all_machines_ingame(uint8_t *context) // blam-cc: ESI -> context
{
    context[0x1c] = 0;
}

#if 0
Original Ghidra decompilation (0x4e4ef0), from tools/pack.py 0x4e4ef0:

void network_game_server_send_message_to_all_machines_ingame(void)

{
  int unaff_ESI;

  *(undefined1 *)(unaff_ESI + 0x1c) = 0;
  return;
}
#endif
