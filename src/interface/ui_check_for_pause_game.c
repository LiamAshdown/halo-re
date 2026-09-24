// ui_check_for_pause_game  (Ghidra: ui_check_for_pause_game, already named)
// address 0x49c1a0, size 489 bytes
// name confidence: 0.8   rewrite confidence: 0.35
// evidence: matches the given name; functions.md: "Determines which pause-menu widget (1p/2p/4p
// multiplayer or solo/split-screen) to load based on the current player count and session state,
// loads it, and decrements the pending pause-request counter." types/networking.h names
// 0x0071c2d4/0x0071c2d8 as network_server/network_client, types/game.h names 0x0087a478 as
// local_player_globals, 0x0087aa10 as game_engine_state_value and 0x00719720 as network_game_mode.
// This function's compiler-duplicated tail fragment display_scenario_help_fail (0x49c369,
// callers=0) is skipped per out/phase4/interface_types_notes.md.
// register convention: none (void); returns a packed {handled_byte, pending_count[3]} dword in
// EAX, of which only the low byte is ever consulted by any caller found in this session.
// UNSURE: DAT_006b3858 and DAT_007127d1 are not documented by any header read this session
// (TYPES-GAP); named generically and gated exactly as Ghidra shows. console_state_006f1d6c is
// read here as *(byte*), [1] and [2] -- i.e. game_time_globals::unknown_00/active/paused per
// types/game.h -- even though that header calls unknown_00 "never read"; this function is new
// evidence against that claim, left as a note rather than an edit to game.h. The bit tested on
// player_control_globals::action_flags_latched (bit 3) has no established name.
// TYPES-GAP: ui_pause_pending_count_00718fa0 (0x00718fa0) is a per-something pending-pause-request
// counter distinct from types/interface.h's ui_pause_depth (0x00718fa6); not documented there.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern network_client_globals *network_client; // 0x0071c2d8
extern network_server_globals *network_server; // 0x0071c2d4
extern game_time_globals *game_time; // 0x006f1d6c (types/game.h)
extern uint8_t *cinematic_globals; // 0x006f187c, byte +9 tested elsewhere
extern int16_t network_game_mode;                    // 0x00719720
extern uint8_t ui_split_screen;                       // 0x00718fc9
extern int32_t ui_pause_pending_count_00718fa0;       // 0x00718fa0, TYPES-GAP
extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern uint8_t unknown_006b3858;                      // 0x006b3858, TYPES-GAP
extern uint8_t unknown_007127d1;                      // 0x007127d1, TYPES-GAP
extern player_globals *local_player_globals;          // 0x0087a478
extern game_engine_state game_engine_state_value;     // 0x0087aa10
extern widget_instance *ui_root_widget[1];            // 0x00718f94
extern uint8_t widget_memory_pool_valid;              // 0x00718fc2
extern widget_history_node *ui_widget_history[3];     // 0x00718f98

extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args

// Chooses and loads the appropriately-sized pause-menu widget (or, off-line, the solo/split-screen
// one) for the current session, gated on a long list of "not a safe time to pause" checks, and
// always decrements the pending pause-request counter by one (floored at zero). Returns 1 (in the
// low byte) if a widget was loaded, 0 otherwise.
uint32_t ui_check_for_pause_game(void)
{
    uint8_t handled = 0;
    uint8_t networked = (network_client != (network_client_globals *)0) ||
                         (network_server != (network_server_globals *)0);
    int16_t active_player;
    int16_t player_count = 0;
    uint8_t single_player_at_start = 1;
    int16_t co_op_flag = -1;
    char *tag_path;

    if (game_time->initialized == 0 ||
        (game_time->active == 0 && game_time->paused == 0) ||
        cinematic_globals[9] != 0 ||
        network_game_mode == 3 || ui_split_screen != 0 || ui_pause_pending_count_00718fa0 != 0 ||
        (player_control_globals_ptr->action_flags_latched >> 3 & 1) != 0 || unknown_006b3858 != 0 ||
        unknown_007127d1 != 1) {
        goto decrement_and_return;
    }

    active_player = (local_player_globals->local_players[0] != (datum_index)-1) ? 0 : -1;
    while (active_player != -1) {
        if (active_player == 0 && player_count > 0) {
            single_player_at_start = 0;
        }
        co_op_flag = 0;
        player_count = player_count + 1;
        active_player = (local_player_globals->local_players[0] != (datum_index)-1 && active_player < 0)
                             ? 0 : -1;
    }

    if (networked) {
        if (game_engine_state_value != 0 || co_op_flag != 0 || ui_root_widget[0] != (widget_instance *)0) {
            goto decrement_and_return;
        }
        switch (player_count) {
        case 1:
            tag_path = "ui\\shell\\multiplayer_game\\pause_game\\1p_pause_game";
            break;
        case 2:
            tag_path = "ui\\shell\\multiplayer_game\\pause_game\\2p_pause_game";
            break;
        case 3:
            if (!single_player_at_start) {
                tag_path = "ui\\shell\\multiplayer_game\\pause_game\\4p_pause_game";
            } else {
                tag_path = "ui\\shell\\multiplayer_game\\pause_game\\2p_pause_game";
            }
            break;
        case 4:
            tag_path = "ui\\shell\\multiplayer_game\\pause_game\\4p_pause_game";
            break;
        default:
            goto decrement_and_return;
        }
    } else {
        if (player_count < 0) {
            // Ghidra's loop here walks &ui_root_widget[0] in 4-byte steps while the raw pointer
            // stays below 0x00718f98 (ui_widget_history's address) -- with ui_root_widget[1]
            // that is exactly one iteration, i.e. this is just "is slot 0's root widget set".
            if (widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0) {
                goto decrement_and_return;
            }
            tag_path = "ui\\shell\\solo_game\\pause_game\\pause_game";
        } else if (player_count > 1) {
            if (player_count == 2) {
                if (ui_root_widget[0] != (widget_instance *)0 ||
                    game_time->paused != 0) {
                    goto decrement_and_return;
                }
                tag_path = "ui\\shell\\solo_game\\pause_game\\pause_game_split_screen";
            } else {
                if (widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0) {
                    goto decrement_and_return;
                }
                tag_path = "ui\\shell\\solo_game\\pause_game\\pause_game";
            }
        } else {
            if (ui_root_widget[0] != (widget_instance *)0) {
                goto decrement_and_return;
            }
            tag_path = "ui\\shell\\solo_game\\pause_game\\pause_game";
        }
    }

    chimera__load_ui_widget(tag_path, (datum_index)-1, (widget_instance *)0, 0,
                             (datum_index)-1, (datum_index)-1, -1);
    handled = 1;

decrement_and_return:
    ui_pause_pending_count_00718fa0 =
        (ui_pause_pending_count_00718fa0 - 1 < 0) ? 0 : ui_pause_pending_count_00718fa0 - 1;
    return handled;
}

#if 0
Original Ghidra decompilation (0x49c1a0):

undefined4 ui_check_for_pause_game(void)

{
  bool bVar1;
  short sVar2;
  undefined1 uVar3;
  bool bVar4;
  short sVar5;
  int *piVar6;
  short sVar7;
  short sVar8;
  char *pcVar9;
  undefined1 local_2;

  if ((DAT_0071c2d8 != 0) || (bVar4 = false, DAT_0071c2d4 != 0)) {
    bVar4 = true;
  }
  uVar3 = 0;
  local_2 = 0;
  if ((((*DAT_006f1d6c == '\0') ||
       (((DAT_006f1d6c[1] == '\0' && (DAT_006f1d6c[2] == '\0')) ||
        (*(char *)(DAT_006f187c + 9) != '\0')))) ||
      (((DAT_00719720 == 3 || (DAT_00718fc9 != '\0')) || (DAT_00718fa0 != 0)))) ||
     ((((*(uint *)(DAT_006b145c + 4) >> 3 & 1) != 0 || (DAT_006b3858 != '\0')) ||
      (DAT_007127d1 != '\x01')))) goto switchD_0049c2dc_default;
  sVar7 = 0;
  bVar1 = true;
  sVar8 = -1;
  sVar5 = -1;
  if (*(int *)(DAT_0087a478 + 4) != -1) {
    sVar5 = 0;
  }
  while (sVar2 = sVar5, sVar2 != -1) {
    if ((sVar2 == 0) && (sVar8 = 0, 0 < sVar7)) {
      bVar1 = false;
    }
    sVar7 = sVar7 + 1;
    sVar5 = -1;
    if ((*(int *)(DAT_0087a478 + 4) != -1) && (sVar2 < 0)) {
      sVar5 = 0;
    }
  }
  local_2 = uVar3;
  if (bVar4) {
    if (((DAT_0087aa10 != 0) || (sVar8 != 0)) || (DAT_00718f94 != 0)) goto switchD_0049c2dc_default;
    switch(sVar7) {
    case 1:
      pcVar9 = "ui\\shell\\multiplayer_game\\pause_game\\1p_pause_game";
      break;
    case 2:
      pcVar9 = "ui\\shell\\multiplayer_game\\pause_game\\2p_pause_game";
      break;
    case 3:
      pcVar9 = "ui\\shell\\multiplayer_game\\pause_game\\2p_pause_game";
      if (!bVar1) goto switchD_0049c2dc_caseD_4;
      break;
    case 4:
switchD_0049c2dc_caseD_4:
      pcVar9 = "ui\\shell\\multiplayer_game\\pause_game\\4p_pause_game";
      break;
    default:
      goto switchD_0049c2dc_default;
    }
  }
  else {
    if (sVar7 < 0) {
LAB_0049c36d:
      if (DAT_00718fc2 != '\0') {
        piVar6 = &DAT_00718f94;
        do {
          if (*piVar6 != 0) goto switchD_0049c2dc_default;
          piVar6 = piVar6 + 1;
        } while ((int)piVar6 < 0x718f98);
      }
    }
    else {
      if (1 < sVar7) {
        if (sVar7 == 2) {
          if ((DAT_00718f94 != 0) || (DAT_006f1d6c[2] != '\0')) goto switchD_0049c2dc_default;
          pcVar9 = "ui\\shell\\solo_game\\pause_game\\pause_game_split_screen";
          goto LAB_0049c3a1;
        }
        goto LAB_0049c36d;
      }
      if (DAT_00718f94 != 0) goto switchD_0049c2dc_default;
    }
    pcVar9 = "ui\\shell\\solo_game\\pause_game\\pause_game";
  }
LAB_0049c3a1:
  chimera__load_ui_widget(pcVar9,0xffffffff,0,0,0xffffffff,0xffffffff,0xffffffff);
  local_2 = 1;
switchD_0049c2dc_default:
  DAT_00718fa0 = ((int)(DAT_00718fa0 - 1) < 0) - 1 & DAT_00718fa0 - 1;
  return CONCAT31((int3)(DAT_00718fa0 >> 8),local_2);
}
#endif
