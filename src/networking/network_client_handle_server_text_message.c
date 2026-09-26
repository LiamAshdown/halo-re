// network_client_handle_server_text_message  (Ghidra: already named)
// address 0x4e5140, size 116 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Client-side handler for a broadcast
// text-message network packet that decodes and prints the message text to the console");
// mirrors network_server_handle_rcon_request.c (this batch), which resolves FUN_004ec590's
// register convention (message pointer in EAX, output buffer in ECX) from its own disassembly.
// register convention: EDX -> message (a decoded network message record whose first dword this
// function checks for a "meaningless" sentinel, exactly as network_server_handle_rcon_request.c
// does with its own in_EAX/in_EDX pair).
//   // blam-cc: EDX -> message
// UNSURE: FUN_004ec590/FUN_004ec670's exact signatures (message-delta protocol, documented in
// types/networking.h as only partly resolved); the decode buffer's shape (a leading char plus 19
// dwords, per Ghidra's own locals) is kept as an opaque byte array sized to match.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern char *network_log_path_format; // 0x0065efec, the shared "%s" format string (see network_log_path_resolve.c)

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern void *console_color_006851fc; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Decodes an incoming broadcast text-message packet and prints its text to the console, unless
// the message's leading dword marks it as a "meaningless" (already-handled) duplicate.
void network_client_handle_server_text_message(void *message) // blam-cc: EDX -> message
{
    uint8_t decode_buf[80]; // Ghidra: char + 19 dwords, zeroed before the decode call

    if (*(int32_t *)*(int32_t *)message == 0) {
        memset(decode_buf, 0, sizeof(decode_buf));
        if (message_delta_decode_compound_field(message, decode_buf) != 0) {
            int32_t text_len = strlen((char *)decode_buf);
            if (text_len != 0) {
                chimera__console_out((ColorARGB *)console_color_006851fc, network_log_path_format, decode_buf, text_len);
            }
        }
    } else {
        message_delta_decode_compound_field_staged(message);
    }
}

#if 0
Original Ghidra decompilation (0x4e5140), from tools/pack.py 0x4e5140:

void network_client_handle_server_text_message(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  undefined4 *in_EDX;
  undefined4 *puVar4;
  char local_54;
  undefined4 local_53 [19];
  undefined1 local_4;

  if (*(int *)*in_EDX == 0) {
    local_54 = '\0';
    puVar4 = local_53;
    for (iVar3 = 0x14; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    cVar1 = FUN_004ec590();
    if (cVar1 != '\0') {
      pcVar2 = &local_54;
      local_4 = 0;
      do {
        cVar1 = *pcVar2;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      if ((int)pcVar2 - (int)local_53 != 0) {
        chimera__console_out(&DAT_0065efec,&local_54,(int)pcVar2 - (int)local_53);
        return;
      }
    }
  }
  else {
    FUN_004ec670();
  }
  return;
}
#endif
