// hud_set_player_message  (Ghidra: FUN_004adfc0, renamed in the phase-4 review)
// address 0x4adfc0, size 140 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.85
// evidence: objdump 0x4adfc0..0x4ae04b. HUDGlobals +0xfc is the tag id of hud_messages (a
// hud_message_text tag; TagDependency at +0xf0), +0x20/+0x24 of that tag the messages block.
// The pointer stored at +0x454 is the message itself (hud_player_messaging_state::message; the
// header used to call it argument_count). Called by hud_update_interaction_prompt with the
// message index per interaction type. An out of range index stores nothing and only clears
// message_shown; -1 clears message_shown. Nothing happens while show_hud_help_text is on or
// when HUDGlobals has no hud_messages tag.
// register convention: EAX message index; one stack argument (local player index).
//   // blam-cc: message_index -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern hud_globals_flags *hud_flags;          // 0x00719420
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_messaging_globals *hud_messaging;  // 0x006b3a40
extern tag_instance *tag_instances;           // 0x0087bc14

// blam-cc: message_index -> EAX
// Shows HUDGlobals hud_messages message message_index as the action message line of a local
// player, clearing its substitution argument kinds.
void hud_set_player_message(int16_t message_index, int16_t local_player_index)
{
    hud_player_messaging_state *record;
    datum_index tag_id;

    if (hud_flags->help_text_shown != 0) {
        return;
    }
    tag_id = *(datum_index *)&hud_globals_tag_data->hud_messages.tag_id;
    if (tag_id == (datum_index)-1) {
        return;
    }
    record = &hud_messaging->players[local_player_index];
    if (message_index != -1) {
        HUDMessageText *messages = (HUDMessageText *)tag_instances[tag_id & 0xffff].data;
        if ((int32_t)message_index < (int32_t)messages->messages.count) {
            record->message = (HUDMessageTextMessage *)messages->messages.pointer + message_index;
            record->argument_is_string = 0;
            record->message_shown = message_index != -1;
            return;
        }
        message_index = -1;
    }
    record->message_shown = message_index != -1;
}

#if 0
Original Ghidra decompilation (0x4adfc0):

void FUN_004adfc0(short param_1)

{
  int iVar1;
  short in_AX;
  int iVar2;
  bool bVar3;

  if ((*(char *)(DAT_00719420 + 1) == '\0') && (*(uint *)(DAT_0071941c + 0xfc) != 0xffffffff)) {
    iVar2 = param_1 * 0x460 + DAT_006b3a40;
    bVar3 = in_AX == -1;
    if (!bVar3) {
      iVar1 = *(int *)((*(uint *)(DAT_0071941c + 0xfc) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if ((int)in_AX < *(int *)(iVar1 + 0x20)) {
        *(int *)(iVar2 + 0x454) = in_AX * 0x40 + *(int *)(iVar1 + 0x24);
        *(undefined1 *)(iVar2 + 0x459) = 0;
        *(bool *)(iVar2 + 0x458) = in_AX != -1;
        return;
      }
      bVar3 = true;
    }
    *(bool *)(iVar2 + 0x458) = !bVar3;
  }
  return;
}
#endif
