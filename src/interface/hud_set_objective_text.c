// hud_set_objective_text  (Ghidra: FUN_004adb80, renamed in the phase-4 review)
// address 0x4adb80, size 106 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4adb80..0x4adbe9. Same tag walk as hud_set_help_text (0x4adb30):
// Scenario::hud_messages (+0x5a0) -> HUDMessageText::messages (+0x24, stride 0x40). The message
// is taken only when its panel_count (+0x24) is 1 and its first element
// (message_elements.pointer +0x18, stride 2, index start_index_of_message_block +0x22) has
// type 0. The display time is HUDGlobals objective_uptime_ticks (+0x11c) plus
// objective_fade_ticks (+0x11e). The first rewrite read 0x00746f8c as a datum.
// register convention: plain cdecl, one stack argument (a short).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario; // 0x00746f8c
extern tag_instance *tag_instances;           // 0x0087bc14
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern hud_messaging_globals *hud_messaging;  // 0x006b3a40

// Shows message message_index of the scenario hud_messages tag as the objective text for the
// HUD globals objective up time plus fade time.
void hud_set_objective_text(int16_t message_index)
{
    HUDMessageText *messages_tag;
    HUDMessageTextMessage *message;
    datum_index tag_id;

    tag_id = *(datum_index *)&global_scenario->hud_messages.tag_id;
    if (tag_id == (datum_index)-1) {
        return;
    }
    messages_tag = (HUDMessageText *)tag_instances[tag_id & 0xffff].data;
    message = (HUDMessageTextMessage *)messages_tag->messages.pointer + message_index;
    if (message->panel_count != 1) {
        return;
    }
    if (((HUDMessageTextElement *)messages_tag->message_elements.pointer)[message->start_index_of_message_block].type != 0) {
        return;
    }
    hud_messaging->objective_text = message;
    hud_messaging->objective_text_ticks =
        (int16_t)(hud_globals_tag_data->objective_fade_ticks + hud_globals_tag_data->objective_uptime_ticks);
}

#if 0
Original Ghidra decompilation (0x4adb80):

void FUN_004adb80(short param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  iVar3 = DAT_0071941c;
  iVar2 = DAT_006b3a40;
  if (*(uint *)(DAT_00746f8c + 0x5a0) != 0xffffffff) {
    iVar1 = *(int *)((*(uint *)(DAT_00746f8c + 0x5a0) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar4 = param_1 * 0x40 + *(int *)(iVar1 + 0x24);
    if ((*(char *)(iVar4 + 0x24) == '\x01') &&
       (*(char *)(*(int *)(iVar1 + 0x18) + (uint)*(ushort *)(iVar4 + 0x22) * 2) == '\0')) {
      *(int *)(DAT_006b3a40 + 0x470) = iVar4;
      *(short *)(iVar2 + 0x474) = *(short *)(iVar3 + 0x11e) + *(short *)(iVar3 + 0x11c);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
