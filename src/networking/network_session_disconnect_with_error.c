// network_session_disconnect_with_error  (Ghidra: FUN_004d97e0; renamed, no prior name)
// address 0x4d97e0, size 31 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Records an error code and triggers a
// network-session disconnect/cleanup").
// register convention: the error code arrives in AX (in_AX). // blam-cc: AX -> error_code
// UNSURE: `network_join_error_code + error_code + 0x2b` is an odd combination (a retry-limit
// slot being seeded from an error code plus a constant); preserved exactly, not simplified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern int16_t network_join_error_code; // 0x00718fa4, the pending join/disconnect error
                                        // string index; -1 means none. WORD-sized everywhere
                                        // (cmp/mov WORD PTR ds:0x718fa4), consumed and reset by
                                        // the main-menu display_error call at 0x4a9ff0.
extern uint8_t network_host_handoff_requested;       // 0x0071c2de, per network_send_join_request_packet.c
extern void chat_close(void); // 0x4aa900

// blam-cc: AX -> error_code
void network_session_disconnect_with_error(int16_t error_code)
{
    if (network_join_error_code == -1) {
        network_join_error_code = error_code + 0x2b;
    }
    network_host_handoff_requested = 1;
    chat_close();
}

#if 0
Original Ghidra decompilation (0x4d97e0):

void FUN_004d97e0(void)

{
  short in_AX;

  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = in_AX + 0x2b;
  }
  DAT_0071c2de = 1;
  chat_close();
  return;
}
#endif
