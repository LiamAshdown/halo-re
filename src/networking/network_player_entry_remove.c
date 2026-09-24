// network_player_entry_remove  (Ghidra: FUN_004de640; named per this rewrite)
// address 0x4de640, size 130 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/networking.h cites this address directly under network_player_entry: "0x4de640
// remove". After the network_player_entry_find pre-check, re-scans for the same
// (machine_index, machine_player_index) key and resets that row to its documented empty state
// (matching network_game_session_reset.c's field-by-field evidence), decrementing player_count.
// register convention: session in EBX (unaff_EBX), key in EAX (in_EAX). blam-cc: EAX -> key,
// EBX -> session

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_player_entry_find(network_game_session *session, network_player_entry *key); // 0x4de900, this batch
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch

// blam-cc: EAX -> key, EBX -> session
uint32_t network_player_entry_remove(network_player_entry *key, network_game_session *session)
{
    int32_t i;
    network_player_entry *slot;

    if (network_player_entry_find(session, key) == 0) {
        return 0;
    }
    for (i = 0; ; i++) {
        if (network_player_entry_validate(key) != 0 && session->players[i].machine_index == key->machine_index &&
            session->players[i].machine_player_index == key->machine_player_index) {
            break;
        }
        if (i > 0xf) {
            return 0;
        }
    }
    slot = &session->players[i];
    slot->unknown_1e = -1;
    slot->machine_player_index = -1;
    slot->unknown_1a = -1;
    slot->slot_index = -1;
    slot->name[0] = 0;
    slot->color_index = -1;
    slot->machine_index = -1;
    session->player_count = session->player_count - 1;
    return ((uint32_t)i << 8) | 1;
}

#if 0
Original Ghidra decompilation (0x4de640):

uint FUN_004de640(void)

{
  undefined2 *puVar1;
  char extraout_AL;
  int in_EAX;
  uint uVar2;
  uint3 extraout_var;
  int unaff_EBX;
  char *pcVar3;
  int iVar4;

  uVar2 = FUN_004de900();
  if ((char)uVar2 == '\0') {
    return uVar2 & 0xffffff00;
  }
  iVar4 = 0;
  pcVar3 = (char *)(unaff_EBX + 0x1be);
  while( true ) {
    network_player_entry_validate();
    if (((extraout_AL != '\0') && (*pcVar3 == *(char *)(in_EAX + 0x1c))) &&
       (pcVar3[1] == *(char *)(in_EAX + 0x1d))) break;
    iVar4 = iVar4 + 1;
    pcVar3 = pcVar3 + 0x20;
    if (0xf < iVar4) {
      return (uint)extraout_var << 8;
    }
  }
  puVar1 = (undefined2 *)(iVar4 * 0x20 + 0x1a2 + unaff_EBX);
  *(undefined1 *)(puVar1 + 0xe) = 0xff;
  *(undefined1 *)((int)puVar1 + 0x1d) = 0xff;
  *(undefined1 *)((int)puVar1 + 0x1f) = 0xff;
  *(undefined1 *)((int)puVar1 + 0x1f) = 0xff;
  *puVar1 = 0;
  puVar1[0xc] = 0xffff;
  puVar1[0xd] = 0xffff;
  *(short *)(unaff_EBX + 0x1a0) = *(short *)(unaff_EBX + 0x1a0) + -1;
  return CONCAT31((int3)((uint)puVar1 >> 8),1);
}
#endif
