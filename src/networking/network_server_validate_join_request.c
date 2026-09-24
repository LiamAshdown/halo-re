// network_server_validate_join_request  (Ghidra: FUN_004e0850, unnamed)
// address 0x4e0850, size 141 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Validates a connecting client's build/version
// and channel state against accepted ranges and returns a numeric status/error code, checking
// for a free machine slot on success." `&DAT_0087bc1c + count*0x14` lands on
// network_pending_connections[count-1].first_payload_word (network_pending_connection is 0x14
// bytes, first_payload_word at +0x10, base 0x0087bc20 = 0x0087bc1c + 0x14 * 1); +0x1a8/+0x1a5
// (relative to server, session at +0x008) are session.player_count (session+0x1a0) and
// session.maximum_players (session+0x19d); +6 and +0x3c4 are ::flags and
// ::machines[0].machine_id, matching every sibling function in this batch.
// register convention: EDX = server (network_server_globals *).
// blam-cc: EDX -> server
// UNSURE: `local_4` is left uninitialized in the original when
// network_pending_connection_count is not positive (a genuine read of an indeterminate stack
// value); preserved as an uninitialized local rather than defaulting it to 0.
// UNSURE: the numeric return codes (0, 4, 5, 6, 7) have no established meaning beyond what
// this function's own branches imply (0 = ok, 5 = build too new, 6 = no room, 7 = session not
// ready, 4 = build too old).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_pending_connection_count; // 0x006f16d0
extern network_pending_connection network_pending_connections[30]; // 0x0087bc20

// Reads the build/version word from the most recently queued pending connection and checks it
// against an accepted range, then -- if in range -- checks the session has room and is ready,
// finally searching for a free machine slot.
int32_t network_server_validate_join_request(network_server_globals *server)
{
    int32_t build; // UNSURE: uninitialized when network_pending_connection_count <= 0

    if (network_pending_connection_count > 0) {
        build = network_pending_connections[network_pending_connection_count - 1].first_payload_word;
    }

    if (build > 0x9663f) {
        if (build > 0x96640) {
            return 5;
        }
        if (server->session.player_count < (int16_t)server->session.maximum_players) {
            int32_t i;

            if ((server->flags & 1) == 0) {
                return 7;
            }
            for (i = 0; i < 16; i = i + 1) {
                if (server->machines[i].machine_id == -1) {
                    return 0;
                }
            }
        }
        return 6;
    }
    return 4;
}

#if 0
Original Ghidra decompilation (0x4e0850):

undefined4 FUN_004e0850(void)

{
  int iVar1;
  int in_EDX;
  short *psVar2;
  int local_4;

  if (0 < DAT_006f16d0) {
    local_4 = *(int *)(&DAT_0087bc1c + DAT_006f16d0 * 0x14);
  }
  if (0x9663f < local_4) {
    if (0x96640 < local_4) {
      return 5;
    }
    if (*(short *)(in_EDX + 0x1a8) < (short)*(char *)(in_EDX + 0x1a5)) {
      if ((*(byte *)(in_EDX + 6) & 1) == 0) {
        return 7;
      }
      iVar1 = 0;
      psVar2 = (short *)(in_EDX + 0x3c4);
      do {
        if (*psVar2 == -1) {
          return 0;
        }
        iVar1 = iVar1 + 1;
        psVar2 = psVar2 + 0x30;
      } while (iVar1 < 0x10);
    }
    return 6;
  }
  return 4;
}
#endif
