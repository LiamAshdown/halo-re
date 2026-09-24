// hud_add_item_message  (Ghidra: FUN_004ae400, renamed in the phase-4 review)
// address 0x4ae400, size 127 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4ae400..0x4ae47e. Finds (hud_message_find_slot 0x4ae480, ESI source) or
// recycles the message slot of {source, kind}, restarts its count when the slot was not
// active, adds count, stamps source, kind, game time, active and the next sequence number, and
// clears prompt_changed. Callers: hud_post_item_message (0x4ae350) and
// hud_receive_item_message (0x4ae200).
// register convention: EAX local player index, ECX source, BL kind; one stack argument.
//   // blam-cc: local_player_index -> EAX, source -> ECX, source_kind -> BL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern hud_messaging_globals *hud_messaging;  // 0x006b3a40
extern game_time_globals *game_time;          // 0x006f1d6c

extern hud_message_slot *hud_message_find_slot(int32_t source, hud_player_messaging_state *record,
                                               uint8_t source_kind); // 0x4ae480, blam-cc: ESI source

// blam-cc: local_player_index -> EAX, source -> ECX, source_kind -> BL
void hud_add_item_message(int16_t local_player_index, int32_t source, uint8_t source_kind, int16_t count)
{
    hud_player_messaging_state *record;
    hud_message_slot *slot;

    if (local_player_index == -1) {
        return;
    }
    record = &hud_messaging->players[local_player_index];
    slot = hud_message_find_slot(source, record, source_kind);
    if (slot->active == 0) {
        slot->count = 0;
    }
    slot->count = (int16_t)(slot->count + count);
    slot->source = source;
    slot->source_kind = source_kind;
    slot->timestamp = game_time->game_time;
    slot->active = 1;
    slot->sequence = hud_messaging->next_sequence;
    hud_messaging->next_sequence++;
    record->prompt_changed = 0;
}

#if 0
Original Ghidra decompilation (0x4ae400):

void FUN_004ae400(short param_1)

{
  char *pcVar1;
  int iVar2;
  int iVar3;
  short in_AX;
  undefined4 *puVar4;
  undefined4 in_ECX;
  undefined1 unaff_BL;
  int iVar5;

  iVar2 = DAT_006b3a40;
  if (in_AX != -1) {
    iVar5 = in_AX * 0x460 + DAT_006b3a40;
    puVar4 = (undefined4 *)FUN_004ae480(iVar5);
    if (*(char *)((int)puVar4 + 0x82) == '\0') {
      *(undefined2 *)(puVar4 + 0x22) = 0;
    }
    *(short *)(puVar4 + 0x22) = *(short *)(puVar4 + 0x22) + param_1;
    iVar3 = DAT_006f1d6c;
    puVar4[0x21] = in_ECX;
    *(undefined1 *)((int)puVar4 + 0x8a) = unaff_BL;
    *puVar4 = *(undefined4 *)(iVar3 + 0xc);
    *(undefined1 *)((int)puVar4 + 0x82) = 1;
    *(undefined1 *)((int)puVar4 + 0x83) = *(undefined1 *)(iVar2 + 0x465);
    pcVar1 = (char *)(iVar2 + 0x465);
    *pcVar1 = *pcVar1 + '\x01';
    *(undefined1 *)(iVar5 + 0x45e) = 0;
  }
  return;
}
#endif
