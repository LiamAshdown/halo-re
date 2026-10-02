// network_channel_key_open  (Ghidra: FUN_004de870; named per this rewrite)
// address 0x4de870, size 75 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Opens/obtains a network channel for the given
// (player,machine) key via player_new_network, records the resulting index, and notifies network_index_cache_find_or_allocate_slot
// of the new channel." entry->machine_index/machine_player_index/slot_index match
// types/networking.h's network_player_entry.
// register convention: entry in EAX (in_EAX). blam-cc: EAX -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t network_channel_key_resolve_target(network_player_entry *entry); // 0x4ddcc0, this batch
extern int32_t player_new_network(int32_t machine_index, int16_t machine_player_index); // 0x473780, outside this batch
extern void network_index_cache_find_or_allocate_slot(int32_t index); // 0x4e9c20, outside this batch

// blam-cc: EAX -> entry
int32_t network_channel_key_open(network_player_entry *entry)
{
    int16_t key;
    int32_t index;

    if (network_channel_key_resolve_target(entry) == 0) {
        key = -1;
    } else {
        key = entry->machine_player_index;
    }
    index = player_new_network(entry->machine_index, key);
    if (index != -1) {
        entry->slot_index = (int8_t)index;
        network_index_cache_find_or_allocate_slot(index);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de870):

undefined4 FUN_004de870(void)

{
  char cVar1;
  short sVar2;
  int in_EAX;
  int iVar3;

  cVar1 = FUN_004ddcc0();
  if (cVar1 == '\0') {
    sVar2 = -1;
  }
  else {
    sVar2 = (short)*(char *)(in_EAX + 0x1d);
  }
  iVar3 = FUN_00473780((int)*(char *)(in_EAX + 0x1c),sVar2);
  if (iVar3 != -1) {
    *(char *)(in_EAX + 0x1f) = (char)iVar3;
    FUN_004e9c20(iVar3);
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
