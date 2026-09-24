// network_player_entry_update  (Ghidra: FUN_004de5f0; named per this rewrite)
// address 0x4de5f0, size 75 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/networking.h cites this address directly under network_player_entry: "0x4de5f0
// update". Finds the row via network_player_entry_find (0x4de900) then overwrites it with the
// incoming 32-byte record from `in_EAX`, after confirming the found slot's own
// machine_player_index/color_index still match (a stale-slot guard).
// UNSURE: `*(char*)(puVar3+7) == *(char*)(in_EAX+7)` compares byte +0x1c of the FOUND slot
// (puVar3 is `undefined4*`, so +7 scales to +0x1c = machine_index) against the incoming record's
// own +0x1c -- i.e. this re-checks machine_index after already finding by it, which is
// redundant; kept literally.
// register convention: session in ECX (in_ECX), incoming record in EAX (in_EAX). blam-cc:
// EAX -> incoming, ECX -> session

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_player_entry_find(network_game_session *session, network_player_entry *key); // 0x4de900, this batch

// blam-cc: EAX -> incoming, ECX -> session
uint32_t network_player_entry_update(network_player_entry *incoming, network_game_session *session)
{
    int32_t slot_index;
    network_player_entry *slot;
    uint32_t *src;
    uint32_t *dst;
    int32_t i;

    if (network_player_entry_find(session, incoming) == 0) {
        return 0;
    }
    slot_index = incoming->slot_index;
    slot = &session->players[slot_index];
    if (slot->machine_player_index == incoming->machine_player_index &&
        slot->machine_index == incoming->machine_index) {
        dst = (uint32_t *)slot;
        src = (uint32_t *)incoming;
        for (i = 0; i < 8; i++) {
            dst[i] = src[i];
        }
        return ((uint32_t)slot_index << 8) | 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de5f0):

uint FUN_004de5f0(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  int in_ECX;
  int iVar2;
  undefined4 *puVar3;

  uVar1 = FUN_004de900();
  if ((char)uVar1 != '\0') {
    uVar1 = *(char *)((int)in_EAX + 0x1f) * 0x20;
    puVar3 = (undefined4 *)(uVar1 + 0x1a2 + in_ECX);
    if ((*(char *)(uVar1 + 0x1bf + in_ECX) == *(char *)((int)in_EAX + 0x1d)) &&
       (*(char *)(puVar3 + 7) == *(char *)(in_EAX + 7))) {
      for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = *in_EAX;
        in_EAX = in_EAX + 1;
        puVar3 = puVar3 + 1;
      }
      return CONCAT31((int3)(uVar1 >> 8),1);
    }
  }
  return uVar1 & 0xffffff00;
}
#endif
