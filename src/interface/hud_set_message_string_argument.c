// hud_set_message_string_argument  (Ghidra: FUN_004ae0b0, renamed in the phase-4 review)
// address 0x4ae0b0, size 91 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4ae0b0..0x4ae10a. Stores {int16 string index, byte} into arguments[slot]
// and sets bit slot of argument_is_string, under the same gate as
// hud_set_message_icon_argument (0x4ae050). hud_messaging_update draws such an argument as
// string index of the scenario custom_object_names list (Scenario +0x580) when the byte is
// set, of HUDGlobals alternate_icon_text (+0xc0) otherwise, and index -1 as "<unknown>".
// register convention: EAX local player index, ESI slot; two stack arguments.
//   // blam-cc: local_player_index -> EAX, slot -> ESI

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

extern hud_globals_flags *hud_flags;          // 0x00719420
extern hud_messaging_globals *hud_messaging;  // 0x006b3a40

// blam-cc: local_player_index -> EAX, slot -> ESI
void hud_set_message_string_argument(int16_t local_player_index, int16_t slot, int16_t string_index,
                                     uint8_t from_scenario_names)
{
    hud_player_messaging_state *record = &hud_messaging->players[local_player_index];
    uint8_t *argument;

    if (record->message_shown == 0 || hud_flags->help_text_shown != 0 || record->message == 0) {
        return;
    }
    argument = (uint8_t *)&record->arguments[slot];
    *(int16_t *)argument = string_index;
    argument[2] = from_scenario_names;
    record->argument_is_string |= (uint8_t)(1 << slot);
}

#if 0
Original Ghidra decompilation (0x4ae0b0):

void FUN_004ae0b0(undefined2 param_1,undefined1 param_2)

{
  short in_AX;
  int iVar1;
  short unaff_SI;

  iVar1 = in_AX * 0x460 + DAT_006b3a40;
  if (((*(char *)(in_AX * 0x460 + 0x458 + DAT_006b3a40) != '\0') &&
      (*(char *)(DAT_00719420 + 1) == '\0')) && (*(int *)(iVar1 + 0x454) != 0)) {
    *(undefined2 *)(iVar1 + 0x434 + unaff_SI * 4) = param_1;
    *(undefined1 *)(iVar1 + 0x436 + unaff_SI * 4) = param_2;
    *(byte *)(iVar1 + 0x459) = *(byte *)(iVar1 + 0x459) | '\x01' << ((byte)unaff_SI & 0x1f);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
