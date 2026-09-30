// network_server_any_machine_awaiting_flag  (Ghidra: FUN_004e14e0, unnamed)
// address 0x4e14e0, size 53 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Reports whether any machine slot is still
// awaiting a particular connection flag, used to gate further handshake work." Traced
// literally: returns 1 as soon as every one of the 16 slots is either not connected (id out of
// 0..15) or has k_network_machine_version_mismatch set, and returns 0 the moment it finds a
// connected, non-mismatched slot.
// register convention: ECX = server (network_server_globals *).
// blam-cc: ECX -> server

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

// Returns 1 if no machine slot is both connected (id 0..15) and free of
// k_network_machine_version_mismatch; returns 0 as soon as one is found.
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t network_server_any_machine_awaiting_flag(network_server_globals *server)
{
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        int16_t id;

        id = server->machines[i].machine_id;
        if (id >= 0 && id <= 15 && (server->machines[i].flags & k_network_machine_version_mismatch) == 0) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e14e0):

undefined1 FUN_004e14e0(void)

{
  int in_ECX;
  short *psVar1;
  int iVar2;

  iVar2 = 0;
  psVar1 = (short *)(in_ECX + 0x3c4);
  while (((*psVar1 < 0 || (0xf < *psVar1)) || ((*(byte *)(psVar1 + 1) >> 3 & 1) != 0))) {
    iVar2 = iVar2 + 1;
    psVar1 = psVar1 + 0x30;
    if (0xf < iVar2) {
      return 1;
    }
  }
  return 0;
}
#endif
