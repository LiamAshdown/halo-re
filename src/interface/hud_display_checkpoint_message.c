// hud_display_checkpoint_message  (Ghidra: FUN_004aa310, renamed)
// address 0x4aa310, size 219 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.85
// phase-4 review: checked against objdump 0x4aa310..0x4aa3ea; the string lookup takes ECX tag and
// DX index, and the sound parameters are a hud_sound_start_parameters {0, 1.0, 1.0}.
// evidence: types/interface.h hud_globals_tag_data and hud_messaging_globals (same message-slot
// clear idiom as hud_display_loading_message.c); types/tags.h HUDGlobals::checkpoint_begin_text/
// checkpoint_end_text (0x3dc/0x3de) and checkpoint_sound (TagDependency at 0x3e0, tag_id at
// 0x3ec) and item_message_text (tag_id at 0xa0, matching hud_get_message_string.c); reuses the
// same tag-lookup idiom directly instead of calling hud_get_message_string.
// UNSURE: sound_play_new (0x549af0, out of this module's range) is declared with a best-guess
// 7-argument sound-play signature based only on the literals passed at this call site.
// register convention: bool flag in DL (in_DL, unresolved register read).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern HUDGlobals *hud_globals_tag_data; // 0x0071941c
extern tag_instance *tag_instances;  // 0x0087bc14
extern hud_messaging_globals *hud_messaging; // 0x006b3a40
extern player_globals *local_player_globals; // 0x0087a478
extern uint16_t *empty_wide_string_pointer;  // 0x00692d7c, UNSURE: fallback value

extern void chimera__hud_message(int16_t local_player_index, const uint16_t *text); // 0x4ae180, blam-cc: AX local_player_index
extern uint16_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0; blam-cc: ECX, DX
extern int32_t sound_play_new(datum_index sound_tag, void *parameters, int32_t unknown_0, int32_t unknown_1,
                            int32_t unknown_2, int32_t unknown_3, int32_t unknown_4); // 0x549af0, starts a 2D impulse sound

// Clears the local player's 4 message-slot active flags, plays the HUDGlobals checkpoint sound
// when is_begin is set and one is configured, then displays the checkpoint begin/end text (only
// when is_begin is set and this build's one local player slot is in use) if a string is
// configured for it.
// blam-cc: DL -> is_begin
void hud_display_checkpoint_message(uint8_t is_begin)
{
    HUDGlobals *hud_globals = (HUDGlobals *)hud_globals_tag_data;
    int16_t message_index = is_begin ? hud_globals->checkpoint_begin_text : hud_globals->checkpoint_end_text;
    int32_t i;

    for (i = 0; i < 4; i = i + 1) {
        hud_messaging->players[0].messages[i].active = 0;
    }

    if (is_begin) {
        int32_t sound_tag_id = *(int32_t *)&hud_globals->checkpoint_sound.tag_id;
        if (sound_tag_id != -1) {
            hud_sound_start_parameters parameters;

            parameters.unknown_00 = 0;
            parameters.scale = 1.0f;
            parameters.gain = 1.0f;
            sound_play_new((datum_index)sound_tag_id, &parameters, -1, 0, 0, 0, 0);
        }
    }

    if (message_index != -1 && local_player_globals->local_players[0] != (datum_index)-1) {
        const uint16_t *text = empty_wide_string_pointer;
        int32_t string_list_tag_id = *(int32_t *)&hud_globals->item_message_text.tag_id;
        if (string_list_tag_id != -1) {
            int32_t *string_list_tag_data = (int32_t *)tag_instances[string_list_tag_id & 0xffff].data;
            if (string_list_tag_data != 0 && message_index > -1 && message_index < *string_list_tag_data) {
                text = text_string_list_get_string((datum_index)string_list_tag_id, message_index);
            }
        }
        chimera__hud_message(0, text); // objdump 0x4aa3db: xor eax,eax
    }
}

#if 0
Original Ghidra decompilation (0x4aa310):

void FUN_004aa310(void)

{
  short sVar1;
  int *piVar2;
  undefined1 *puVar3;
  undefined *puVar4;
  int iVar5;
  char in_DL;
  int iVar6;
  undefined2 local_40 [2];
  undefined4 local_3c;
  undefined4 local_38;

  iVar6 = DAT_0071941c;
  if (in_DL == '\0') {
    sVar1 = *(short *)(DAT_0071941c + 0x3de);
  }
  else {
    sVar1 = *(short *)(DAT_0071941c + 0x3dc);
  }
  puVar3 = (undefined1 *)(DAT_006b3a40 + 0x82);
  iVar5 = 4;
  do {
    *puVar3 = 0;
    puVar3 = puVar3 + 0x8c;
    iVar5 = iVar5 + -1;
  } while (iVar5 != 0);
  if ((in_DL != '\0') && (*(int *)(iVar6 + 0x3ec) != -1)) {
    local_40[0] = 0;
    local_3c = 0x3f800000;
    local_38 = 0x3f800000;
    FUN_00549af0(*(int *)(iVar6 + 0x3ec),local_40,0xffffffff,0,0,0,0);
    iVar6 = DAT_0071941c;
  }
  if ((sVar1 != -1) && (*(int *)(DAT_0087a478 + 4) != -1)) {
    puVar4 = PTR_DAT_00692d7c;
    if ((*(uint *)(iVar6 + 0xa0) != 0xffffffff) &&
       (((piVar2 = *(int **)((*(uint *)(iVar6 + 0xa0) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
         piVar2 != (int *)0x0 && (-1 < sVar1)) && ((int)sVar1 < *piVar2)))) {
      puVar4 = (undefined *)text_string_list_get_string();
    }
    chimera__hud_message(puVar4);
  }
  return;
}
#endif
