// player_profile_save_495fb0  (Ghidra: player_profile_save_495fb0, already named)
// address 0x495fb0, size 171 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: cea-pdb hint via the shared "profile not saved since it was a default profile"
// string (the same hint player_profile_save.c carries; Ghidra assigned both addresses variants
// of that one name). out/phase4/interface_functions.md "Saves the active player profile to disk
// unless it is the default (unsaved) profile, in which case it [only refreshes the cache]."
// register convention: a byte flag in AL (in_AL). // blam-cc: AL -> flag
// UNSURE: the "ui\shell\strings\temp_strings" unicode_string_list truncation (reading the tag's
// own count/reflexive-pointer/length fields and null-terminating near the end of its last
// string) is reproduced as raw offsets; the tag's true field layout is not in types/tags.h under
// that name and is not recovered here. DAT_00712f07 (the AL flag this function stashes) is not
// documented anywhere.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t unknown_00712f07;         // 0x00712f07, UNSURE
extern int32_t saved_player_profile_slots_handle;    // 0x00714dd4
extern tag_instance *tag_instances;      // 0x0087bc14
extern uint8_t profile_globals_block[];  // 0x00712dd8

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void hud_message_broadcast_to_local_players(const uint16_t *text); // 0x495f50, blam-cc: ESI text
extern const uint16_t empty_string[];          // 0x00660c34, L""
extern const uint16_t missing_string_text[]; // 0x00671fac, L"<missing string>"
extern void player_profile_refresh_settings_cache(int16_t player_index); // 0x496060, BX (0 at both calls here)
extern void console_out_printf(uint8_t unknown, const char *format, ...); // 0x4c6860
extern void player_profile_write_data(int32_t slot, void *profile_data); // 0x53a950

// blam-cc: AL -> flag
// Stashes `flag`, and, if a real profile is active: truncates the last entry of the
// "ui\shell\strings\temp_strings" unicode_string_list tag if it has more than one string,
// broadcasts a HUD message to local players, then either writes the profile to disk (for a real
// profile) or prints a "not saved" message (for the default profile) before refreshing the
// settings cache either way.
void player_profile_save_495fb0(uint8_t flag)
{
    datum_index string_list_tag;
    int32_t *string_list_data;
    char *block;
    uint32_t length;
    const uint16_t *text;

    unknown_00712f07 = flag;
    if (saved_player_profile_slots_handle != -1) {
        string_list_tag = tag_lookup(0x75737472, (char *)"ui\\shell\\strings\\temp_strings"); // 'ustr'
        // s2 part 2 review: the broadcast text (EDX -> ESI of 0x495f50) is string 1 of the list,
        // L"<missing string>" (0x00671fac) when the list is too short or the string empty, and
        // L"" (0x00660c34) when the tag is missing (objdump 0x495fd9..0x49601a).
        text = empty_string;
        if (string_list_tag != (datum_index)0xffffffff) {
            string_list_data = (int32_t *)tag_instances[(uint16_t)string_list_tag].data;
            text = missing_string_text;
            if (string_list_data[0] > 1) {
                block = (char *)string_list_data[1];
                length = *(uint32_t *)(block + 0x14);
                if ((int32_t)length > 0) {
                    text = *(const uint16_t **)(block + 0x20);
                    *(int16_t *)(*(int32_t *)(block + 0x20) - 2 + (length & 0xfffffffe)) = 0;
                }
            }
        }
        hud_message_broadcast_to_local_players(text);
        if (saved_player_profile_slots_handle == -1) {
            console_out_printf(0, "profile not saved since it was a default profile");
            player_profile_refresh_settings_cache(0);
            return;
        }
        player_profile_write_data(saved_player_profile_slots_handle, profile_globals_block);
    }
    player_profile_refresh_settings_cache(0);
}

#if 0
Original Ghidra decompilation (0x495fb0):

void player_profile_save_495fb0(void)

{
  int *piVar1;
  int iVar2;
  undefined1 in_AL;
  uint uVar3;

  DAT_00712f07 = in_AL;
  if (DAT_00714dd4 != -1) {
    uVar3 = tag_lookup("ui\\shell\\strings\\temp_strings");
    if ((uVar3 != 0xffffffff) &&
       (piVar1 = *(int **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14), 1 < *piVar1)) {
      iVar2 = piVar1[1];
      uVar3 = *(uint *)(iVar2 + 0x14);
      if (0 < (int)uVar3) {
        *(undefined2 *)(*(int *)(iVar2 + 0x20) + -2 + (uVar3 & 0xfffffffe)) = 0;
      }
    }
    FUN_00495f50();
    if (DAT_00714dd4 == -1) {
      console_out_printf('\0',"profile not saved since it was a default profile");
      FUN_00496060();
      return;
    }
    player_profile_write_data(DAT_00714dd4,&DAT_00712dd8);
  }
  FUN_00496060();
  return;
}
#endif
