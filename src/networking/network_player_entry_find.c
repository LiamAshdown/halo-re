// network_player_entry_find  (Ghidra: FUN_004de900; named per this rewrite)
// address 0x4de900, size 65 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/networking.h cites this address directly under network_player_entry: "0x4de900
// find". Validates the key record via network_player_entry_validate, then scans session->players[] for a
// machine_index/machine_player_index match. Both callers in this batch (network_player_entry_
// update.c, network_player_entry_remove.c) only ever test the low byte of this function's
// return (truthy/falsy); only AL is defined in the original, so a plain 0/1 return is equivalent.
// register convention: session in param_1 (stack), key in ESI (unaff_ESI). blam-cc: ESI -> key,
// stack -> session

// VERIFIED against disassembly 0x4de900..0x4de941 (2026-09-30): validate(ESI key via EAX), scan of machine_index (+0x1c) then machine_player_index (+0x1d) over 16 rows
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch

// blam-cc: ESI -> key
char network_player_entry_find(network_game_session *session, network_player_entry *key)
{
    int32_t i;

    if (network_player_entry_validate(key) == 0) {
        return 0;
    }
    for (i = 0; i < 0x10; i++) {
        if (session->players[i].machine_index == key->machine_index &&
            session->players[i].machine_player_index == key->machine_player_index) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de900):

uint FUN_004de900(int param_1)

{
  uint uVar1;
  char *pcVar2;
  int iVar3;
  int unaff_ESI;

  uVar1 = network_player_entry_validate();
  if ((char)uVar1 == '\0') {
    return uVar1 & 0xffffff00;
  }
  iVar3 = 0;
  pcVar2 = (char *)(param_1 + 0x1bf);
  while ((pcVar2[-1] != *(char *)(unaff_ESI + 0x1c) || (*pcVar2 != *(char *)(unaff_ESI + 0x1d)))) {
    iVar3 = iVar3 + 1;
    pcVar2 = pcVar2 + 0x20;
    if (0xf < iVar3) {
      return (uint)pcVar2 & 0xffffff00;
    }
  }
  return CONCAT31((int3)((uint)pcVar2 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
