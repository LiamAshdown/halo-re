// network_host_round_reset  (Ghidra: FUN_004df640; named per this rewrite)
// address 0x4df640, size 65 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Resets the per-round update counters and
// completion flags on the object at in_EAX, increments its round counter, and calls network_game_session_reset_defaults
// to continue setup." Same host offsets (+0x9b8, +0x9bc-region, +0x3b0) as
// network_game_server_host_new.c; see that file's header for the field-matching evidence and the
// same "+0x3b0 lands in session.unknown_3a2[10]" UNSURE note.
// register convention: host in EAX (in_EAX). blam-cc: EAX -> host

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_game_session_reset_defaults(void); // 0x4e1820, outside this batch, elided args

// blam-cc: EAX -> host
void network_host_round_reset(network_server_globals *host)
{
    *(uint32_t *)&host->unknown_9bc[0x0c] = 0; // 0x9c8
    *(uint32_t *)&host->unknown_9bc[0x10] = 0; // 0x9cc
    *(uint32_t *)&host->unknown_9bc[0x14] = 0; // 0x9d0
    *(uint32_t *)&host->unknown_9bc[0x18] = 0; // 0x9d4
    host->unknown_9b8 = 0;
    *(uint32_t *)&host->unknown_9bc[0x08] = 0; // 0x9c4
    host->unknown_9f9 = 0;
    host->unknown_9fa = 0;
    host->unknown_9f8 = 0;
    *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1; // UNSURE, see header
    network_game_session_reset_defaults();
}

#if 0
Original Ghidra decompilation (0x4df640):

void FUN_004df640(void)

{
  int in_EAX;

  *(undefined4 *)(in_EAX + 0x9c8) = 0;
  *(undefined4 *)(in_EAX + 0x9cc) = 0;
  *(undefined4 *)(in_EAX + 0x9d0) = 0;
  *(undefined4 *)(in_EAX + 0x9d4) = 0;
  *(undefined4 *)(in_EAX + 0x9b8) = 0;
  *(undefined4 *)(in_EAX + 0x9c4) = 0;
  *(undefined1 *)(in_EAX + 0x9f9) = 0;
  *(undefined1 *)(in_EAX + 0x9fa) = 0;
  *(undefined1 *)(in_EAX + 0x9f8) = 0;
  *(int *)(in_EAX + 0x3b0) = *(int *)(in_EAX + 0x3b0) + 1;
  FUN_004e1820();
  return;
}
#endif
