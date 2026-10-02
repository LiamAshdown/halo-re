// network_server_handle_rcon_request  (Ghidra: already named)
// address 0x4e4f00, size 438 bytes
// name confidence: 0.75   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("validates the password, executes the command if
// valid, and reports the result back to the requesting client"); disassembly (objdump -d -M
// intel) confirms EAX -> client record (+0xc machine id) and EDX -> message, and that
// FUN_004ec590 takes the message in EAX and an output buffer in ECX.
// register convention: EAX -> client, EDX -> message.
//   // blam-cc: EAX -> client, EDX -> message
// UNSURE: this whole function reads a message-delta-decoded record whose exact field layout is
// not resolved (types/networking.h documents the message-delta protocol as only partly
// resolved); the password/command split below (a 20-byte password region immediately followed
// by a 64-byte command region) matches the relative order and rough sizes of Ghidra's own
// locals (local_4c/local_4b zeroed as one ~74-byte run, then local_43[64]) but the exact byte
// boundary is not independently confirmed. UNSURE: FUN_004c69a0's signature (foreign, executes
// the decoded command); UNSURE: the "+1" on the machine id in the success-message call, kept
// exactly as Ghidra shows it even though every other branch here logs the plain id.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

typedef struct rcon_request_decode {
    char password[20];
    char command[64];
} rcon_request_decode;

extern char sv_rcon_password_value[9]; // 0x0071c410, see sv_rcon_password.c

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern uint8_t console_process_rcon_command(char *command); // foreign, UNSURE shape; executes a console command string
extern void chimera__rcon_out(char *text, int32_t machine_id); // this batch, 0x4e50c0
extern void *global_white_argb; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Server-side handler for an incoming rcon-request message: decodes it, validates the password
// against sv_rcon_password_value, executes the command if both the server has rcon enabled and
// the password matches, and reports the outcome back to the requesting client via
// chimera__rcon_out plus a server console log line.
void network_server_handle_rcon_request(network_player_entry *client, void *message) // blam-cc: EAX -> client, EDX -> message
{
    int16_t machine_id = client->machine_index;
    rcon_request_decode decode;

    if (*(int32_t *)*(int32_t *)message != 0) {
        message_delta_decode_compound_field_staged(message);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring meaningless rcon_request message from client #%d", machine_id);
        return;
    }
    memset(&decode, 0, sizeof(decode));
    if (message_delta_decode_compound_field(message, &decode) == 0) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Could not decode rcon message from client #%d", machine_id);
        return;
    }
    if (sv_rcon_password_value[0] == 0) {
        chimera__rcon_out((char *)"rcon command ignored (rcon is disabled)", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (rcon is disabled)", machine_id);
        return;
    }
    if (strcmp(sv_rcon_password_value, decode.password) != 0) {
        chimera__rcon_out((char *)"rcon command ignored (bad password)", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (bad password)", machine_id);
        return;
    }
    if (decode.command[0] == 0) {
        chimera__rcon_out((char *)"rcon command ignored (empty)", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (empty command)", machine_id);
        return;
    }
    if (console_process_rcon_command(decode.command) != 0) {
        chimera__rcon_out((char *)"rcon command finished", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Successfully executed rcon command from client #%d.", machine_id + 1);
        return;
    }
    chimera__rcon_out((char *)"rcon command failed", machine_id);
    chimera__console_out((ColorARGB *)global_white_argb, (char *)"Failure executing rcon command from client #%d.", machine_id);
}

#if 0
Original Ghidra decompilation (0x4e4f00), from tools/pack.py 0x4e4f00:

void network_server_handle_rcon_request(void)

{
  byte bVar1;
  char cVar2;
  int in_EAX;
  char *pcVar3;
  byte *pbVar4;
  char *pcVar5;
  int iVar6;
  undefined4 *in_EDX;
  int iVar7;
  byte *pbVar8;
  undefined4 *puVar9;
  bool bVar10;
  byte local_4c;
  undefined4 local_4b;
  undefined1 local_44;
  char local_43 [64];
  undefined1 local_3;

  iVar7 = (int)*(short *)(in_EAX + 0xc);
  if (*(int *)*in_EDX != 0) {
    FUN_004ec670();
    chimera__console_out("Ignoring meaningless rcon_request message from client #%d",iVar7);
    return;
  }
  local_4c = 0;
  puVar9 = &local_4b;
  for (iVar6 = 0x12; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar9 = 0;
    puVar9 = puVar9 + 1;
  }
  *(undefined1 *)puVar9 = 0;
  cVar2 = FUN_004ec590();
  if (cVar2 == '\0') {
    chimera__console_out("Could not decode rcon message from client #%d",iVar7);
    return;
  }
  local_44 = 0;
  local_3 = 0;
  pcVar5 = &DAT_0071c410;
  do {
    pcVar3 = pcVar5;
    pcVar5 = pcVar3 + 1;
  } while (*pcVar3 != '\0');
  if (pcVar3 == &DAT_0071c410) {
    chimera__rcon_out(iVar7);
    chimera__console_out("Ignoring rcon request from client #%d (rcon is disabled)",iVar7);
    return;
  }
  pbVar8 = &local_4c;
  pbVar4 = &DAT_0071c410;
  do {
    bVar1 = *pbVar4;
    bVar10 = bVar1 < *pbVar8;
    if (bVar1 != *pbVar8) {
LAB_004e4fb4:
      iVar6 = (1 - (uint)bVar10) - (uint)(bVar10 != 0);
      goto LAB_004e4fb9;
    }
    if (bVar1 == 0) break;
    bVar1 = pbVar4[1];
    bVar10 = bVar1 < pbVar8[1];
    if (bVar1 != pbVar8[1]) goto LAB_004e4fb4;
    pbVar4 = pbVar4 + 2;
    pbVar8 = pbVar8 + 2;
  } while (bVar1 != 0);
  iVar6 = 0;
LAB_004e4fb9:
  if (iVar6 != 0) {
    chimera__rcon_out(iVar7);
    chimera__console_out("Ignoring rcon request from client #%d (bad password)",iVar7);
    return;
  }
  pcVar5 = local_43;
  do {
    cVar2 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar2 != '\0');
  if (pcVar5 != local_43 + 1) {
    cVar2 = FUN_004c69a0();
    if (cVar2 != '\0') {
      chimera__rcon_out(iVar7);
      chimera__console_out("Successfully executed rcon command from client #%d.",iVar7 + 1);
      return;
    }
    chimera__rcon_out(iVar7);
    chimera__console_out("Failure executing rcon command from client #%d.",iVar7);
    return;
  }
  chimera__rcon_out(iVar7);
  chimera__console_out("Ignoring rcon request from client #%d (empty command)",iVar7);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
