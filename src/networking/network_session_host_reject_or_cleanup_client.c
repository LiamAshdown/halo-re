// network_session_host_reject_or_cleanup_client  (Ghidra: FUN_00575ff0; named per this rewrite)
// address 0x575ff0, size 162 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md summary: "Looks up an entry in a small
// client/machine table and performs one of two cleanup calls on a network channel handle
// depending on whether the entry was found." Calls ban_list_check_and_reject_player (already
// named in this module) and searches a 16-entry, 0x60-byte-stride table off network_server
// (0x0071c2d4) -- the same stride as types/networking.h's network_machine -- for an entry
// matching a channel handle pinned in ESI.
// register convention: a channel/connection value pinned in ESI (unaff_ESI, unresolved register
// read) throughout; no recognized stack parameters.
// UNSURE: FUN_0061aa50, FUN_0061b110, FUN_0061b350 and FUN_0061b3f0 are foreign (GameSpy/CD-key)
// helpers outside this module; their exact roles are not established here. The 0x414 base offset
// into network_server does not line up cleanly with network_machine[0] at +0x3b8, so the exact
// field being scanned is not pinned either.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern void *autopatch_download_mutex_or_similar; // 0x0069fdfc, UNSURE: exact type/name

extern void FUN_0061b110(void *handle); // foreign, UNSURE
extern void FUN_0061aa50(void *handle);  // foreign, UNSURE
extern char ban_list_check_and_reject_player(void); // 0x4e3820, this module (elided args)
extern void network_server_notify_or_resend_challenge(network_server_globals *server); // this module range, UNSURE args
extern void FUN_0061b350(void *handle); // foreign, UNSURE
extern void FUN_0061b3f0(void *handle); // foreign, UNSURE

// blam-cc: ESI -> channel (unresolved)
// Looks up a matching entry in the server's machine table for the pinned channel value; if the
// connecting player is banned, rejects it immediately. Otherwise tears down the server-side
// bookkeeping for the (found or not-found) slot and releases the channel through one of two
// cleanup paths depending on whether a match was found.
int32_t network_session_host_reject_or_cleanup_client(int32_t channel)
{
    char banned;
    int32_t i;
    uint8_t *entry;

    FUN_0061b110(autopatch_download_mutex_or_similar);
    FUN_0061aa50(autopatch_download_mutex_or_similar);
    banned = ban_list_check_and_reject_player();
    if (banned == 0) {
        return 1;
    }
    entry = (uint8_t *)network_server + 0x414;
    for (i = 0; i < 0x10; i++) {
        if (*(int32_t *)entry == channel) {
            break;
        }
        entry = entry + 0x60;
    }
    network_server_notify_or_resend_challenge(network_server);
    if (channel != -1) {
        FUN_0061b350(autopatch_download_mutex_or_similar);
        return 0;
    }
    FUN_0061b3f0(autopatch_download_mutex_or_similar);
    return 0;
}

#if 0
Original Ghidra decompilation (0x575ff0):

undefined4 FUN_00575ff0(void)

{
  char cVar1;
  int iVar2;
  int *piVar3;
  int unaff_ESI;

  FUN_0061b110(DAT_0069fdfc);
  FUN_0061aa50(DAT_0069fdfc);
  cVar1 = ban_list_check_and_reject_player();
  if (cVar1 == '\0') {
    return 1;
  }
  iVar2 = 0;
  piVar3 = (int *)(DAT_0071c2d4 + 0x414);
  do {
    if (*piVar3 == unaff_ESI) break;
    iVar2 = iVar2 + 1;
    piVar3 = piVar3 + 0x18;
  } while (iVar2 < 0x10);
  FUN_004e0af0(DAT_0071c2d4);
  if (unaff_ESI != -1) {
    FUN_0061b350(DAT_0069fdfc);
    return 0;
  }
  FUN_0061b3f0(DAT_0069fdfc);
  return 0;
}
#endif
