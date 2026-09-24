// network_game_message_handle_ping_timestamp  (Ghidra: FUN_004e20b0; named per this rewrite)
// address 0x4e20b0, size 83 bytes
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Handles queued message type 0x34 by recording
// elapsed time since a stored timestamp into a field of the local player's datum." Called with
// a single explicit argument (`param_1`) from src/networking/network_client_drain_queued_updates.c's
// own `case 0x34: FUN_004e20b0(param_1);`, so `param_1` there and here are the same value; its
// offset +0x9c0 falls inside types/networking.h's network_server_globals::unknown_9bc[0x3c]
// span, accessed here the same way network_game_server_handle_client_join.c accesses the
// neighbouring +0x9c4 (an explicit byte-offset cast, since the header leaves that span
// unresolved).
// register convention: EAX = message (a pointer to the queued-message record pointer, matching
// network_client_drain_queued_updates.c's `&local_108`), stack = param_1 (server, UNSURE).
//   // blam-cc: EAX -> message, stack -> param_1
// UNSURE (major): FUN_004ec670, FUN_004ec590 and datum_get are all called with zero visible
// arguments in Ghidra's own decompile; FUN_004ec670/FUN_004ec590 are message-delta functions
// outside this batch's range (0x4ec2f0+) and datum_get's real signature elsewhere in this
// codebase takes (handle, array), neither of which is recoverable at this call site. Declared
// and called with no arguments here, matching Ghidra literally, rather than inventing a
// plausible index/array pair.
// UNSURE: FUN_00449210 (foreign, < this module) is presumed to return a millisecond-shaped
// tick count purely from how its result is subtracted from a stored timestamp.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void FUN_004ec670(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern uint8_t FUN_004ec590(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern void *datum_get(void);        // 0x4d0680, called here with no visible arguments (UNSURE)
extern int32_t FUN_00449210(void);   // foreign, presumed a tick-count getter (UNSURE)

// blam-cc: EAX -> message, stack -> param_1
// If the queued message's first dword is non-zero, skips it via FUN_004ec670. Otherwise, if
// FUN_004ec590 reports true, resolves the local player's datum and stores the elapsed time
// since server+0x9c0 into datum+0xdc.
uint32_t network_game_message_handle_ping_timestamp(int32_t **message, network_server_globals *param_1)
{
    uint8_t *player;
    uint8_t decode_scratch[5]; // [esp+0x3], the one-byte-aligned tail of the 4-byte frame

    if (**message != 0) {
        FUN_004ec670(message); // blam-cc: EAX -> message (unchanged since entry)
        return 1;
    }
    if (FUN_004ec590(message, decode_scratch) == 1) { // blam-cc: EAX -> message, ECX -> scratch
        player = (uint8_t *)datum_get();
        if (player != 0) {
            int32_t stored_time = *(int32_t *)((uint8_t *)param_1 + 0x9c0);
            int32_t now = FUN_00449210();
            *(int32_t *)(player + 0xdc) = now - stored_time;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e20b0), from tools/pack.py 0x4e20b0:

undefined4 FUN_004e20b0(int param_1)

{
  int iVar1;
  char cVar2;
  undefined4 *in_EAX;
  int iVar3;
  int iVar4;

  if (*(int *)*in_EAX != 0) {
    FUN_004ec670();
    return 1;
  }
  cVar2 = FUN_004ec590();
  if (cVar2 == '\x01') {
    iVar3 = datum_get();
    if (iVar3 != 0) {
      iVar1 = *(int *)(param_1 + 0x9c0);
      iVar4 = FUN_00449210();
      *(int *)(iVar3 + 0xdc) = iVar4 - iVar1;
    }
  }
  return 1;
}
#endif
