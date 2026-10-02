// chimera__rcon_out  (Ghidra: chimera__rcon_out, already named)
// address 0x4e50c0, size 119 bytes
// name confidence: 0.6   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md; the "chimera" callee-name-prefix hint plus this
// batch's network_server_handle_rcon_request.c, whose every call site loads a literal string
// constant into EAX and pushes a client/machine id on the stack immediately before calling this
// function -- matching this function's own body, which only ever reads its EAX-sourced text
// (never the pushed value).
// register convention: EAX -> text; the pushed stack value every caller supplies is not read by
// this function's own body.
//   // blam-cc: EAX -> text, stack -> unused_machine_id
// UNSURE: unused_machine_id -- every caller in this batch passes a machine/client id here, but
// this function's disassembly never reads it, so either the real send target is hardcoded (as
// the literal `1` passed to network_session_send_to_machine below suggests) or this batch does
// not have enough context to see where else it might be used.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rcon_out_channel_key; // 0x00871de0, UNSURE: passed to network_session_send_to_machine as the target key

extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
extern uint8_t network_session_send_to_machine(int32_t machine_index, void *key, int32_t size,
    int32_t reliable, int32_t d, int32_t e, int32_t message_kind); // this module (earlier batch), 0x4e1930, UNSURE shape

// Sends one line of rcon command output text (message type 0x37) back over the network.
void chimera__rcon_out(char *text, int32_t unused_machine_id) // blam-cc: EAX -> text, stack -> unused_machine_id
{
    char buf[81];
    void *fields[2];
    int32_t encoded_bits;

    strncpy(buf, text, 0x50);
    buf[0x50] = 0;
    fields[0] = buf;
    fields[1] = 0;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x37, 0, fields, 0, 1, 0);
    if (0 < encoded_bits) {
        network_session_send_to_machine(1, rcon_out_channel_key, encoded_bits, 1, 0, 0, 9);
    }
}

#if 0
Original Ghidra decompilation (0x4e50c0), from tools/pack.py 0x4e50c0:

void chimera__rcon_out(void)

{
  char *in_EAX;
  int iVar1;
  char *local_5c;
  undefined4 local_58;
  char local_54 [80];
  undefined1 local_4;

  _strncpy(local_54,in_EAX,0x50);
  local_5c = local_54;
  local_4 = 0;
  local_58 = 0;
  iVar1 = message_delta_encode_message(0,0x37,0,&local_5c,0,1,'\0');
  if (0 < iVar1) {
    network_session_send_to_machine(1,&DAT_00871de0,iVar1,1,0,0,9);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
