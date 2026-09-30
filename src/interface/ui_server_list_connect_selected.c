// ui_server_list_connect_selected  (Ghidra: FUN_0049d2e0, renamed)
// renamed from FUN_0049d2e0 in the naming pass
// address 0x49d2e0, size 351 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: functions.md: "Initiates a network connection to the server currently selected in the
// server-list widget and, once connected, transitions the UI to the connected pre-game lobby
// screen." Shape matches types/interface.h's ui_event_function typedef
// (widget, event, out_handled) -> uint8_t, though the event parameter is never read here.
// register convention: cdecl, all three parameters recognized by Ghidra.
// UNSURE / TYPES-GAP: `list_items` here is indexed as a plain pointer array (4-byte stride), not
// types/interface.h's ui_list_item (0x10-byte stride) -- consistent with this being a
// server-browser list rather than a generic UI list, but that per-entry record's own layout
// (offsets 0x00, 0x12, 0x4b*4, 0x12a tested here) was not resolved against
// a server browser entry and is left as raw byte offsets. widget_get_sibling_
// index and widget_instance_find_root are both called with no visible arguments in Ghidra; modeled on the
// widget parameter (the natural single value in scope) per this module's established pattern for
// such calls.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_networking.h"

extern network_client_globals *network_client; // 0x0071c2d8
extern int16_t network_game_mode;               // 0x00719720
extern uint8_t network_host_handoff_requested;                 // 0x0071c2de

extern void *widget_instance_find_root(widget_instance *widget); // 0x498e10, UNSURE args
extern int32_t widget_get_sibling_index(widget_instance *widget); // 0x498e30
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90
extern void chat_close(void); // 0x4aa900

extern void network_debug_fill_canary_buffer(void); // 0x4e0790, UNSURE argument
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args

// blam-cc: matches ui_event_function (widget, event, out_handled)
// If a connectable server entry is selected, initiates a connection to it and, once connected,
// opens the connected pre-game lobby widget.
uint8_t ui_server_list_connect_selected(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    (void)event;

    if (widget->focused_child != (widget_instance *)0 && widget->selection_index >= 0 &&
        widget->selection_index < (int16_t)widget->item_count && widget->list_items != (void *)0 &&
        widget->item_count != 0) {
        uint8_t **entries = (uint8_t **)widget->list_items;
        uint8_t *entry = entries[widget->selection_index];

        if (entry[0x12c] == 1) { // UNSURE: raw offset, see header
            if (*(int16_t *)(entry + 0x12a) == 1 && *(uint32_t *)entry != 0 &&
                *(int16_t *)(entry + 0x12) != 0) {
                uint32_t session_info[1] = {0}; // only the high 16 bits are cleared in Ghidra
                int32_t connected;

                network_debug_fill_canary_buffer();
                connected = network_connection_initiate(network_client, (const uint32_t *)entry,
                                                         session_info);
                if ((uint8_t)connected == 0) {
                    network_host_handoff_requested = 1;
                    chat_close();
                    return 0;
                }

                {
                    void *page = widget_instance_find_root(widget);
                    datum_index parent_definition = (widget->parent != (widget_instance *)0)
                                                         ? widget->parent->definition
                                                         : (datum_index)-1;
                    int32_t sibling = widget_get_sibling_index(widget);
                    widget_instance *opened;

                    opened = chimera__load_ui_widget(
                        "ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen",
                        (datum_index)-1, (widget_instance *)0, (uint16_t)-1,
                        *(datum_index *)page, parent_definition, (int16_t)sibling);
                    if (opened != (widget_instance *)0) {
                        network_game_mode = 1;
                    }
                    *out_handled = 1;
                    return opened != (widget_instance *)0;
                }
            }
        } else {
            widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x49d2e0):

bool FUN_0049d2e0(int param_1,undefined4 param_2,undefined1 *param_3)

{
  short sVar1;
  uint *puVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 local_24;

  if ((*(int *)(param_1 + 0x38) != 0) && (sVar1 = *(short *)(param_1 + 0x40), -1 < sVar1)) {
    if (((int)sVar1 < (int)(uint)*(ushort *)(param_1 + 0x48)) &&
       ((*(int *)(param_1 + 0x44) != 0 && (*(ushort *)(param_1 + 0x48) != 0)))) {
      puVar2 = *(uint **)(*(int *)(param_1 + 0x44) + sVar1 * 4);
      if ((char)puVar2[0x4b] == '\x01') {
        if (((*(short *)((int)puVar2 + 0x12a) == 1) && (*puVar2 != 0)) &&
           (*(short *)((int)puVar2 + 0x12) != 0)) {
          local_24._2_2_ = 0;
          FUN_004e0790();
          iVar3 = network_connection_initiate(DAT_0071c2d8,puVar2,&local_24);
          if ((char)iVar3 == '\0') {
            DAT_0071c2de = 1;
            chat_close();
            return false;
          }
          puVar4 = (undefined4 *)FUN_00498e10();
          if (*(undefined4 **)(param_1 + 0x30) == (undefined4 *)0x0) {
            uVar6 = 0xffffffff;
          }
          else {
            uVar6 = **(undefined4 **)(param_1 + 0x30);
          }
          uVar5 = widget_get_sibling_index();
          iVar3 = chimera__load_ui_widget
                            ("ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen"
                             ,0xffffffff,0,0xffffffff,*puVar4,uVar6,uVar5);
          if (iVar3 != 0) {
            DAT_00719720 = 1;
          }
          *param_3 = 1;
          return iVar3 != 0;
        }
      }
      else {
        widget_play_sound_effect();
      }
    }
  }
  return false;
}
#endif
