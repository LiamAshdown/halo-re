// network_server_reset_game_stats  (Ghidra: FUN_004a1670, renamed)
// renamed from FUN_004a1670 in the naming pass
// address 0x4a1670, size 38 bytes, callers=0 in this build
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: functions.md: "Resets the hosted multiplayer game's statistics/state fields to zero
// on the active host session structure." types/networking.h names 0x0071c2d4 network_server.
// register convention: none (void); returns 1 in the low byte (the upper 24 bits carry whatever
// network_server happened to be, per Ghidra's CONCAT31 -- preserved for fidelity, though no
// caller in this session reads more than the low byte).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern network_server_globals *network_server; // 0x0071c2d4

// UNSURE: offsets 0x9c8..0x9d5 fall past types/networking.h's own documented fields of
// network_server_globals (size 0xa10, so still in range but not individually named there).
uint32_t network_server_reset_game_stats(void)
{
    if (network_server != (network_server_globals *)0) {
        uint8_t *raw = (uint8_t *)network_server;

        *(int32_t *)(raw + 0x9c8) = 0;
        *(int32_t *)(raw + 0x9cc) = 0;
        *(int32_t *)(raw + 0x9d0) = 0;
        *(int32_t *)(raw + 0x9d4) = 0;
        raw[0x9d5] = 1;
    }
    return ((uint32_t)network_server << 8) | 1;
}

#if 0
Original Ghidra decompilation (0x4a1670):

undefined4 FUN_004a1670(void)

{
  int iVar1;

  iVar1 = DAT_0071c2d4;
  if (DAT_0071c2d4 != 0) {
    *(undefined4 *)(DAT_0071c2d4 + 0x9c8) = 0;
    *(undefined4 *)(iVar1 + 0x9cc) = 0;
    *(undefined4 *)(iVar1 + 0x9d0) = 0;
    *(undefined4 *)(iVar1 + 0x9d4) = 0;
    *(undefined1 *)(iVar1 + 0x9d5) = 1;
  }
  return CONCAT31((int3)((uint)iVar1 >> 8),1);
}
#endif
