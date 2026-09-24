// network_channel_key_close  (Ghidra: FUN_004de8c0; named per this rewrite)
// address 0x4de8c0, size 55 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Releases the channel previously obtained for
// the given (player,machine) key via player_new_local, recording the returned index at in_EAX+0x1f
// if valid." Mirrors network_channel_key_open.c.
// UNSURE: `unaff_EBX` (player_new_local's implicit result) is preserved as an out-parameter written
// by that call, since Ghidra shows no explicit return capture.
// register convention: entry in EAX (in_EAX). blam-cc: EAX -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_channel_key_resolve_target(network_player_entry *entry); // 0x4ddcc0, this batch
extern void player_new_local(int32_t machine_index, int16_t machine_player_index, int32_t *out_index); // 0x473940, outside this batch, elided out-param

// blam-cc: EAX -> entry
int32_t network_channel_key_close(network_player_entry *entry)
{
    int16_t key;
    int32_t index;

    if (network_channel_key_resolve_target(entry) == 0) {
        key = -1;
    } else {
        key = entry->machine_player_index;
    }
    index = -1;
    player_new_local(entry->machine_index, key, &index); // UNSURE: out-param elided, see header
    if (index != -1) {
        entry->slot_index = (int8_t)index;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4de8c0):

undefined4 FUN_004de8c0(void)

{
  char cVar1;
  short sVar2;
  int in_EAX;
  int unaff_EBX;

  cVar1 = FUN_004ddcc0();
  if (cVar1 == '\0') {
    sVar2 = -1;
  }
  else {
    sVar2 = (short)*(char *)(in_EAX + 0x1d);
  }
  FUN_00473940((int)*(char *)(in_EAX + 0x1c),sVar2);
  if (unaff_EBX != -1) {
    *(char *)(in_EAX + 0x1f) = (char)unaff_EBX;
    return 1;
  }
  return 0;
}
#endif
