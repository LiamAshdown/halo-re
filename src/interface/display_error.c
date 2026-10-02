// display_error  (Ghidra: display_error, already named)
// address 0x498f20, size 559 bytes
// name confidence: 0.9   rewrite confidence: 0.45
// evidence: matches the given name; signature fully resolved by Ghidra itself (cc=__cdecl).
// Queues into types/interface.h's ui_pending_errors[4] when a "not ready" flag is set, otherwise
// picks one of the six ui\shell\error\* tags by local-player count and modal-ness, reopens the
// main menu first if returning from a network wait that just ended, opens the dialog via
// chimera__load_ui_widget (all 7 arguments: no parent, the old root as history definition, -1
// for the history list and selection, objdump 0x4990a2), writes the clamped error string index into the
// dialog's grandchild list widget's selection_index, and updates the pause-depth bookkeeping
// this module's other functions (widget_close, interface_tick) already touch.
// register convention: cdecl, all four parameters recognized by Ghidra.
// UNSURE: console_state_006f1d6c (0x006f1d6c) is used here, consistently with interface_tick.c,
// as a POINTER whose value is itself dereferenced and indexed; kept as `uint8_t *`.
// TYPES-GAP: DAT_006f187c (a "not ready to show dialogs yet" flag block, byte at +9 tested here)
// and DAT_00719739 are not documented anywhere in types/interface.h.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "units.h"
#include "cutscene.h"

extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern ui_pending_error ui_pending_errors[4]; // 0x00718fb6
extern player_globals *local_player_globals;  // 0x0087a478 (types/game.h)
extern uint8_t ui_split_screen;               // 0x00718fc9
extern float ui_unknown_718fa8;               // 0x00718fa8, reset to -1.0
extern uint8_t network_wait_flag_00719739;    // 0x00719739, TYPES-GAP, UNSURE name
extern widget_instance *ui_root_widget[1];    // 0x00718f94
extern int16_t network_game_mode; // 0x00719720, types/game.h: 0 local, 1 client, 2 host (word access)
extern int16_t ui_pause_depth;                // 0x00718fa6
extern game_time_globals *game_time; // 0x006f1d6c (types/game.h)

extern void chimera__load_main_menu(void); // 0x4989f0
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args (objdump)

// Queues or immediately opens the appropriately-sized modal/non-modal error dialog widget for the
// given error message id, tag'd by how many local players are active and whether it should block
// input.
void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error)
{
    int16_t slot = (int16_t)player_index;
    int16_t active_player;  // Ghidra's sVar5
    int16_t player_count = 0; // Ghidra's sVar7
    uint8_t half_screen = 1;
    char *tag_path;
    widget_instance *root;
    datum_index history_source;
    widget_instance *dialog;

    if (cinematic_globals_ptr->in_progress != 0) {
        int32_t index = (slot == -1) ? 0 : slot;

        if (ui_pending_errors[index].error_string_index != -1) {
            return;
        }
        ui_pending_errors[index].error_string_index = error_string_index;
        ui_pending_errors[index].modal = modal;
        ui_pending_errors[index].is_error = is_error;
        return;
    }

    active_player = -1;
    if (slot == -1) {
        if (ui_split_screen == 0) {
            player_index = -1;
        }
    } else {
        int32_t matched_index = -1;

        if (local_player_globals->local_players[0] != (datum_index)-1) {
            active_player = 0;
        }
        if (active_player != -1) {
            do {
                if (active_player == slot && player_count > 0) {
                    matched_index = player_index;
                    half_screen = 0;
                }
                player_count = player_count + 1;
                active_player = (local_player_globals->local_players[0] != (datum_index)-1 && active_player < 0)
                                    ? 0
                                    : -1;
            } while (active_player != -1);
            if ((int16_t)matched_index == -1) {
                if (ui_split_screen == 0) {
                    player_index = -1;
                }
            }
        } else {
            if (ui_split_screen == 0) {
                player_index = -1;
            }
        }
    }

    switch (player_count) {
    case 0:
    case 1:
        tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_fullscreen"
                                 : (char *)"ui\\shell\\error\\error_modal_fullscreen";
        break;
    case 2:
        tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_halfscreen"
                                 : (char *)"ui\\shell\\error\\error_modal_halfscreen";
        break;
    case 3:
        if (!half_screen) {
            tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_qtrscreen"
                                     : (char *)"ui\\shell\\error\\error_modal_qtrscreen";
        } else if (modal == 0) {
            tag_path = (char *)"ui\\shell\\error\\error_nonmodal_halfscreen";
        } else {
            tag_path = (char *)"ui\\shell\\error\\error_modal_halfscreen";
        }
        break;
    case 4:
        tag_path = (modal == 0) ? (char *)"ui\\shell\\error\\error_nonmodal_qtrscreen"
                                 : (char *)"ui\\shell\\error\\error_modal_qtrscreen";
        break;
    default:
        return;
    }

    if (ui_split_screen != 0 && (ui_unknown_718fa8 < 1.0f) != (ui_unknown_718fa8 == 1.0f) &&
        0.0f <= ui_unknown_718fa8) {
        chimera__load_main_menu();
        network_wait_flag_00719739 = 0;
        ui_unknown_718fa8 = -1.0f;
    }

    slot = (int16_t)(((uint16_t)player_index == 0xffff) ? 0 : (uint16_t)player_index);
    root = ui_root_widget[slot];
    if (root == (widget_instance *)0) {
        history_source = (datum_index)-1;
    } else {
        history_source = root->definition;
        if (root->is_error_dialog == 1) {
            return;
        }
    }

    dialog = chimera__load_ui_widget(tag_path, (datum_index)-1, (widget_instance *)0,
                                     (uint16_t)player_index, history_source, (datum_index)-1, -1);
    if (dialog != (widget_instance *)0) {
        int16_t clamped;

        if (error_string_index < 0) {
            clamped = 0;
        } else {
            clamped = 0x3b;
            if (error_string_index < 0x3c) {
                clamped = error_string_index;
            }
        }
        dialog->first_child->first_child->selection_index = clamped;
        dialog->is_error_dialog = 1;
        if (dialog->pauses_game_time == 0) {
            dialog->pauses_game_time = is_error;
            if (is_error == 1 && network_game_mode != 2) {
                ui_pause_depth = ui_pause_depth + 1;
                if (game_time->paused == 0) {
                    if (game_time->initialized != 0) {
                        game_time->active = 0;
                    }
                    game_time->paused = 1;
                }
            }
        }
        if (error_string_index != 0xc) {
            if (error_string_index != 0xd) {
                dialog->close_on_controller_connected[0] = 0;
                return;
            }
            dialog->close_on_controller_connected[0] = 1;
        }
        dialog->milliseconds_to_auto_close = 0;
        dialog->milliseconds_auto_close_fade = 0;
    }
}

#if 0
Original Ghidra decompilation (0x498f20):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl display_error(short error_string_index,int player_index,char modal,char is_error)

{
  undefined4 *puVar1;
  bool bVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  int iVar6;
  short sVar7;
  undefined4 uVar8;
  char *pcVar9;

  sVar4 = (short)player_index;
  if (*(char *)(DAT_006f187c + 9) != '\0') {
    if (sVar4 == -1) {
      sVar4 = 0;
    }
    iVar6 = (int)sVar4;
    if ((&DAT_00718fb6)[iVar6 * 2] != -1) {
      return;
    }
    (&DAT_00718fb6)[iVar6 * 2] = error_string_index;
    (&DAT_00718fb8)[iVar6 * 4] = modal;
    (&DAT_00718fb9)[iVar6 * 4] = is_error;
    return;
  }
  sVar5 = -1;
  sVar7 = 0;
  bVar2 = true;
  if (sVar4 == -1) {
LAB_00498fc4:
    if (DAT_00718fc9 == '\0') {
      player_index = -1;
    }
  }
  else {
    iVar6 = -1;
    if (*(int *)(DAT_0087a478 + 4) != -1) {
      sVar5 = 0;
    }
    if (sVar5 == -1) goto LAB_00498fc4;
    do {
      if ((sVar5 == sVar4) && (iVar6 = player_index, 0 < sVar7)) {
        bVar2 = false;
      }
      sVar7 = sVar7 + 1;
      sVar3 = -1;
      if ((*(int *)(DAT_0087a478 + 4) != -1) && (sVar5 < 0)) {
        sVar3 = 0;
      }
      sVar5 = sVar3;
    } while (sVar5 != -1);
    if ((short)iVar6 == -1) goto LAB_00498fc4;
  }
  switch(sVar7) {
  case 0:
  case 1:
    if (modal == '\0') {
      pcVar9 = "ui\\shell\\error\\error_nonmodal_fullscreen";
    }
    else {
      pcVar9 = "ui\\shell\\error\\error_modal_fullscreen";
    }
    break;
  case 2:
    if (modal == '\0') {
LAB_0049901c:
      pcVar9 = "ui\\shell\\error\\error_nonmodal_halfscreen";
    }
    else {
      pcVar9 = "ui\\shell\\error\\error_modal_halfscreen";
    }
    break;
  case 3:
    if (!bVar2) goto LAB_00499027;
    if (modal == '\0') goto LAB_0049901c;
    pcVar9 = "ui\\shell\\error\\error_modal_halfscreen";
    break;
  case 4:
LAB_00499027:
    pcVar9 = "ui\\shell\\error\\error_modal_qtrscreen";
    if (modal == '\0') {
      pcVar9 = "ui\\shell\\error\\error_nonmodal_qtrscreen";
    }
    break;
  default:
    goto switchD_00498fdc_default;
  }
  if (((DAT_00718fc9 != '\0') && (_DAT_00718fa8 < 1.0 != (_DAT_00718fa8 == 1.0))) &&
     (0.0 <= _DAT_00718fa8)) {
    chimera__load_main_menu();
    DAT_00719739 = 0;
    _DAT_00718fa8 = -1.0;
  }
  puVar1 = (undefined4 *)
           (&DAT_00718f94)[(short)(((ushort)player_index == 0xffff) - 1 & (ushort)player_index)];
  if (puVar1 == (undefined4 *)0x0) {
    uVar8 = 0xffffffff;
  }
  else {
    uVar8 = *puVar1;
    if (*(char *)((int)puVar1 + 0x15) == '\x01') {
      return;
    }
  }
  iVar6 = chimera__load_ui_widget(pcVar9,0xffffffff,0,player_index,uVar8,0xffffffff,0xffffffff);
  if (iVar6 != 0) {
    if (error_string_index < 0) {
      sVar4 = 0;
    }
    else {
      sVar4 = 0x3b;
      if (error_string_index < 0x3c) {
        sVar4 = error_string_index;
      }
    }
    *(short *)(*(int *)(*(int *)(iVar6 + 0x34) + 0x34) + 0x40) = sVar4;
    *(undefined1 *)(iVar6 + 0x15) = 1;
    if (((*(char *)(iVar6 + 0x13) == '\0') &&
        (*(char *)(iVar6 + 0x13) = is_error, pcVar9 = DAT_006f1d6c, is_error == '\x01')) &&
       ((DAT_00719720 != 2 && (_DAT_00718fa6 = _DAT_00718fa6 + 1, DAT_006f1d6c[2] == '\0')))) {
      if (*DAT_006f1d6c != '\0') {
        DAT_006f1d6c[1] = '\0';
      }
      pcVar9[2] = '\x01';
    }
    if (error_string_index != 0xc) {
      if (error_string_index != 0xd) {
        *(undefined1 *)(iVar6 + 0x16) = 0;
        return;
      }
      *(undefined1 *)(iVar6 + 0x16) = 1;
    }
    *(undefined4 *)(iVar6 + 0x1c) = 0;
    *(undefined4 *)(iVar6 + 0x20) = 0;
  }
switchD_00498fdc_default:
  return;
}
#endif
