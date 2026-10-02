// network_machine_find_by_id  (Ghidra: FUN_004e0810, unnamed)
// address 0x4e0810, size 46 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/networking_types_notes.md "network_machine": "0x4e0810 walks
// machine_id from server+0x3c4 with a stride of 0x60 for 16 iterations and returns
// server + 0x3b8 + i*0x60, which pins both the base and the stride."
// register convention: ESI = server (network_server_globals *), EDI = machine_id (int32_t).
// blam-cc: ESI -> server, EDI -> machine_id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

// Returns the machines[] slot whose machine_id equals `machine_id`, or NULL if none matches.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
network_machine *network_machine_find_by_id(network_server_globals *server, int32_t machine_id)
{
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            return &server->machines[i];
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e0810):

int FUN_004e0810(void)

{
  int iVar1;
  short *psVar2;
  int unaff_ESI;
  int unaff_EDI;

  iVar1 = 0;
  psVar2 = (short *)(unaff_ESI + 0x3c4);
  do {
    if (*psVar2 == unaff_EDI) {
      return iVar1 * 0x60 + 0x3b8 + unaff_ESI;
    }
    iVar1 = iVar1 + 1;
    psVar2 = psVar2 + 0x30;
  } while (iVar1 < 0x10);
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
