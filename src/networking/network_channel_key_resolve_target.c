// network_channel_key_resolve_target  (Ghidra: FUN_004ddcc0; named per this rewrite)
// address 0x4ddcc0, size 81 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Decides whether the current channel-key entry
// (ESI) should resolve to a specific machine slot or a wildcard/broadcast target, based on the
// network-game state and DAT_0071c2d4/DAT_0071c2d8 globals." +0x1c on the ESI object matches
// network_player_entry.machine_index (see network_game_session_reset.c and
// network_player_entry_add.c for the evidence this whole 0x4de390..0x4de950 cluster shares).
// UNSURE (significant): `*DAT_0071c2d8` dereferences network_client as if it were an `int *`
// (its own first field, unknown_000, is only a uint16_t per types/networking.h); this rewrite
// preserves that exact dereference via a raw cast rather than asserting a specific meaning.
// register convention: entry in ESI (unaff_ESI). blam-cc: ESI -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_server_globals *network_server; // 0x0071c2d4
extern network_client_globals *network_client; // 0x0071c2d8
extern int16_t network_game_mode; // 0x00719720

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch

// blam-cc: ESI -> entry
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t network_channel_key_resolve_target(network_player_entry *entry)
{
    if (entry == 0 || network_player_entry_validate(entry) == 0) {
        if (network_game_mode != 3) {
            return 1;
        }
        return entry->machine_index == 0;
    }
    if ((network_server == 0 || ((network_server->flags >> 2) & 1) == 0) &&
        (*(int32_t *)network_client != -1 &&
         (int32_t)*(int32_t *)network_client == (int32_t)entry->machine_index)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ddcc0):

bool FUN_004ddcc0(void)

{
  char cVar1;
  int unaff_ESI;

  if ((unaff_ESI == 0) || (cVar1 = network_player_entry_validate(), cVar1 == '\0')) {
    if (DAT_00719720 != 3) {
      return true;
    }
    return *(char *)(unaff_ESI + 0x1c) == '\0';
  }
  if (((DAT_0071c2d4 == 0) || ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) &&
     ((*DAT_0071c2d8 != -1 && ((int)*DAT_0071c2d8 == (int)*(char *)(unaff_ESI + 0x1c))))) {
    return true;
  }
  return false;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
