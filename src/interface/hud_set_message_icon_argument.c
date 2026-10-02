// hud_set_message_icon_argument  (Ghidra: FUN_004ae050, renamed in the phase-4 review)
// address 0x4ae050, size 88 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4ae050..0x4ae0a7. Stores the dword argument into arguments[slot] and
// clears bit slot of argument_is_string. hud_messaging_update (0x4ae550) hands such an
// argument to hud_draw_message_icon (0x4ad970) in ESI, so the dword is a
// hud_messaging_information pointer (the weapon HUD icon of hud_update_interaction_prompt).
// Stores only while the action message is shown, show_hud_help_text is off and a message is
// set. The shift mask is the byte `1 << slot` of the binary (slots 0..7).
// register convention: EAX local player index, ESI slot; one stack argument.
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
void hud_set_message_icon_argument(int16_t local_player_index, int16_t slot,
                                   const hud_messaging_information *information)
{
    hud_player_messaging_state *record = &hud_messaging->players[local_player_index];

    if (record->message_shown == 0 || hud_flags->help_text_shown != 0 || record->message == 0) {
        return;
    }
    record->arguments[slot] = (int32_t)information;
    record->argument_is_string &= (uint8_t)~(uint8_t)(1 << slot);
}

#if 0
Original Ghidra decompilation (0x4ae050):

void FUN_004ae050(undefined4 param_1)

{
  short in_AX;
  int iVar1;
  short unaff_SI;

  iVar1 = in_AX * 0x460 + DAT_006b3a40;
  if (((*(char *)(in_AX * 0x460 + 0x458 + DAT_006b3a40) != '\0') &&
      (*(char *)(DAT_00719420 + 1) == '\0')) && (*(int *)(iVar1 + 0x454) != 0)) {
    *(undefined4 *)(iVar1 + 0x434 + unaff_SI * 4) = param_1;
    *(byte *)(iVar1 + 0x459) = *(byte *)(iVar1 + 0x459) & ~('\x01' << ((byte)unaff_SI & 0x1f));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
