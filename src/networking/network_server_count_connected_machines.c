// network_server_count_connected_machines  (Ghidra: FUN_004e1880, unnamed)
// address 0x4e1880, size 38 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/networking_types_notes.md's network_machine section: "FUN_004e1880
// counts slots where *(int *)(id - 0xc) != 0 && id != -1, naming +0x00 as the channel
// pointer."
// register convention: ECX = server (network_server_globals *).
// blam-cc: ECX -> server

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

// Counts machine-table slots that have both a live channel and a connected (non -1) id.
int32_t network_server_count_connected_machines(network_server_globals *server)
{
    int32_t count;
    int32_t i;

    count = 0;
    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].channel != 0 && server->machines[i].machine_id != -1) {
            count = count + 1;
        }
    }
    return count;
}

#if 0
Original Ghidra decompilation (0x4e1880):

int FUN_004e1880(void)

{
  int iVar1;
  int in_ECX;
  short *psVar2;
  int iVar3;

  iVar1 = 0;
  psVar2 = (short *)(in_ECX + 0x3c4);
  iVar3 = 0x10;
  do {
    if ((*(int *)(psVar2 + -6) != 0) && (*psVar2 != -1)) {
      iVar1 = iVar1 + 1;
    }
    psVar2 = psVar2 + 0x30;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  return iVar1;
}
#endif
