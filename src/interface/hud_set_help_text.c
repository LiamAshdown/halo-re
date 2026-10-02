// hud_set_help_text  (Ghidra: FUN_004adb30, renamed in the phase-4 review)
// address 0x4adb30, size 73 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4adb30..0x4adb78. No direct callers (reached through the hs function
// table, like the hud_set_objective_text / hud_set_timer_time / pause_hud_timer neighbours).
// global_scenario +0x5a0 is the tag id of Scenario::hud_messages (a hud_message_text tag) and
// +0x24 of that tag the messages block pointer (HUDMessageTextMessage, stride 0x40). The first
// rewrite read 0x00746f8c as a datum itself (it is the Scenario pointer) and called the argument
// a local player index. The message pointer lands in hud_messaging_globals::help_text.
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
extern hud_globals_flags *hud_flags;          // 0x00719420
extern hud_messaging_globals *hud_messaging;  // 0x006b3a40

// Points the HUD help text at message message_index of the scenario hud_messages tag; ignored
// unless show_hud_help_text is on and the scenario has a hud_messages tag.
void hud_set_help_text(int16_t message_index)
{
    datum_index tag_id;

    if (hud_flags->help_text_shown == 0) {
        return;
    }
    tag_id = *(datum_index *)&global_scenario->hud_messages.tag_id;
    if (tag_id == (datum_index)-1) {
        return;
    }
    hud_messaging->help_text =
        (HUDMessageTextMessage *)((HUDMessageText *)tag_instances[tag_id & 0xffff].data)->messages.pointer +
        message_index;
}

#if 0
Original Ghidra decompilation (0x4adb30):

void FUN_004adb30(short param_1)

{
  if ((*(char *)(DAT_00719420 + 1) != '\0') && (*(uint *)(DAT_00746f8c + 0x5a0) != 0xffffffff)) {
    *(int *)(DAT_006b3a40 + 0x46c) =
         param_1 * 0x40 +
         *(int *)(*(int *)((*(uint *)(DAT_00746f8c + 0x5a0) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                 + 0x24);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
