// ui_new_profile_name_entry_commit  (Ghidra: FUN_004a19a0, renamed)
// renamed from FUN_004a19a0 in the naming pass
// address 0x4a19a0, size 342 bytes, callers=0 in this build
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: functions.md: "Finishes new-profile creation after name entry: verifies the chosen
// name is available, creates and loads the resulting profile, or shows an error and cancels entry
// if creation fails." Reuses the quit_confirm_error_* globals (established across five other files
// this session) and player_profile_load's established signature.
// register convention: none (void).
// UNSURE: saved_game_create_default_profile/player_profile_get_or_cached_default/saved_game_create_default_profile's second call and saved_game_allocate_new_slot's
// output-buffer argument are all foreign (profile module) with no established precedent found.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t new_profile_name_entry_player_00692b00; // 0x00692b00
extern virtual_keyboard_globals virtual_keyboard;       // 0x007193a8 (committed at 0x007193be)
extern uint16_t new_profile_name_buffer_006b37f4[0xb];   // 0x006b37f4
extern int16_t profile_slot_id[];                        // 0x00714dde
extern uint8_t new_profile_name_terminator_006b380a;      // 0x006b380a
extern uint8_t new_profile_name_flag_0071916e;             // 0x0071916e
extern uint8_t network_wait_flag_00719739;                 // 0x00719739
extern int16_t quit_confirm_error_string_index;             // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae;               // 0x00718fae
extern uint8_t quit_confirm_error_modal;                    // 0x00718fb0
extern uint8_t quit_confirm_error_is_error;                  // 0x00718fb1
extern uint16_t split_screen_quit_prompt_string;         // 0x00719754, TYPES-GAP (packed record)
extern uint8_t split_screen_quit_prompt_armed;    // 0x00719757
extern uint8_t network_join_error_reason; // 0x0071973c, TYPES-GAP

extern int32_t saved_game_create_default_profile(int16_t player_index); // 0x539ab0, foreign (profile module), UNSURE
extern uint8_t player_profile_get_or_cached_default(void); // 0x539bc0, foreign (profile module), UNSURE
extern void saved_game_allocate_new_slot(uint16_t *out_default_name); // 0x53ca80
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970
extern void saved_item_select(int32_t selection_id); // 0x495be0, UNSURE signature
extern void main_queue_map_change(void); // 0x4c8740
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90

uint32_t ui_new_profile_name_entry_commit(void)
{
    int32_t profile_id;

    if (new_profile_name_entry_player_00692b00 == -1) {
        return 0;
    }
    if (virtual_keyboard.committed == 0) {
        new_profile_name_entry_player_00692b00 = -1;
        return 0;
    }

    if (new_profile_name_buffer_006b37f4[0] != 0) {
        uint16_t default_name[4220];

        profile_slot_id[0] = new_profile_name_entry_player_00692b00;
        profile_id = saved_game_create_default_profile(new_profile_name_entry_player_00692b00);
        if (profile_id == -1) {
            saved_game_allocate_new_slot(default_name);
            wcsncpy((wchar_t *)new_profile_name_buffer_006b37f4, (const wchar_t *)default_name, 0xb);
            new_profile_name_terminator_006b380a = 0;
            profile_id = saved_game_create_default_profile(new_profile_name_entry_player_00692b00);
            if (profile_id == -1) {
                goto fail;
            }
        }
        if (player_profile_get_or_cached_default() != 0) {
            player_profile_load((int16_t)profile_id, (void *)0, profile_id);
            if (new_profile_name_flag_0071916e != 0) {
                saved_item_select(-1);
            }
            main_queue_map_change();
            new_profile_name_entry_player_00692b00 = -1;
            network_wait_flag_00719739 = 0;
            return 1;
        }
    }

fail:
    split_screen_quit_prompt_string = 0xffff;
    network_join_error_reason = 0;
    split_screen_quit_prompt_armed = 1;
    if (quit_confirm_error_string_index == -1) {
        quit_confirm_error_string_index = 0x25;
        quit_confirm_error_unknown_ae = -1;
        quit_confirm_error_modal = 1;
        quit_confirm_error_is_error = 0;
    }
    widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
    new_profile_name_entry_player_00692b00 = -1;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4a19a0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004a19a0(void)

{
  char cVar1;
  int iVar2;
  wchar_t local_20fc [4220];
  undefined4 uStack_4;

  uStack_4 = 0x4a19aa;
  if (DAT_00692b00 == -1) {
    return 0;
  }
  if (DAT_007193bc._2_1_ == '\0') {
    DAT_00692b00 = 0xffff;
    return 0;
  }
  if (DAT_006b37f4 == 0) goto LAB_004a1ace;
  _DAT_00714dde = DAT_00692b00;
  iVar2 = FUN_00539ab0(DAT_00692b00);
  if (iVar2 == -1) {
    saved_game_allocate_new_slot();
    _wcsncpy(&DAT_006b37f4,local_20fc,0xb);
    _DAT_006b380a = 0;
    iVar2 = FUN_00539ab0(DAT_00692b00);
    if (iVar2 != -1) goto LAB_004a1a38;
  }
  else {
LAB_004a1a38:
    cVar1 = FUN_00539bc0();
    if (cVar1 != '\0') {
      player_profile_load(iVar2);
      if (DAT_0071916e != '\0') {
        FUN_00495be0();
      }
      main_queue_map_change();
      DAT_00692b00 = 0xffff;
      DAT_00719739 = 0;
      return 1;
    }
  }
  DAT_00719754._0_2_ = 0xffff;
  DAT_0071973c = 0;
  DAT_00719754._3_1_ = 1;
  if (DAT_00718fac == -1) {
    DAT_00718fac = 0x25;
    DAT_00718fae = 0xffff;
    DAT_00718fb0 = 1;
    DAT_00718fb1 = 0;
  }
LAB_004a1ace:
  widget_play_sound_effect();
  DAT_00692b00 = 0xffff;
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
