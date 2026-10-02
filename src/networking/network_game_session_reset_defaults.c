// network_game_session_reset_defaults  (Ghidra: FUN_004e1820, unnamed)
// address 0x4e1820, size 82 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md's network_game_session section: "FUN_004e1820
// copies 0x26 dwords (0x98 bytes) of 0x0087aa80 (game.h's game_engine_pending_variant) to
// server + 0x10c, i.e. session+0x104 ... The same function does
// strncpy(server + 0x8c, 0x0087aa40, 0x3f) ... and zeroes word session+0x07e and dword
// session+0x080." listen_channel->listening (channel+0xae0) and server->flags bit0 match
// types/networking.h exactly.
// register convention: EBX = server (network_server_globals *).
// blam-cc: EBX -> server

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_variant game_engine_pending_variant; // 0x0087aa80
extern char variant_defaults_source[]; // 0x0087aa40 (UNSURE name)

// Resets `server`'s session to compiled-in defaults: copies the pending game variant, the
// default server name, and clears the two fields between them, then marks the session and the
// listen channel as initialized.
// FIXED: every ret of the original is preceded by mov eax,1 and callers test it
int32_t network_game_session_reset_defaults(network_server_globals *server)
{
    memcpy(&server->session.variant, &game_engine_pending_variant, sizeof(game_variant));
    strncpy(server->session.server_name, variant_defaults_source, 0x3f);
    server->session.server_name[0x3f] = 0;
    server->session.unknown_07e = 0;
    server->session.unknown_080 = 0;
    server->flags |= 1;
    server->listen_channel->listening = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e1820):

void FUN_004e1820(void)

{
  int iVar1;
  int *unaff_EBX;
  int *piVar2;
  int *piVar3;

  piVar2 = &DAT_0087aa80;
  piVar3 = unaff_EBX + 0x43;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *piVar3 = *piVar2;
    piVar2 = piVar2 + 1;
    piVar3 = piVar3 + 1;
  }
  _strncpy((char *)(unaff_EBX + 0x23),&DAT_0087aa40,0x3f);
  *(undefined1 *)((int)unaff_EBX + 0xcb) = 0;
  *(undefined2 *)((int)unaff_EBX + 0x86) = 0;
  unaff_EBX[0x22] = 0;
  *(ushort *)((int)unaff_EBX + 6) = *(ushort *)((int)unaff_EBX + 6) | 1;
  *(undefined1 *)(*unaff_EBX + 0xae0) = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
