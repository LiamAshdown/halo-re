// ui_network_game_options_populate  (Ghidra: FUN_004a3960, renamed)
// renamed from FUN_004a3960 in the naming pass
// address 0x4a3960, size 89 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: functions.md: "Populates a network game-options list-widget value and two related
// global settings from a session/options structure."
// register convention: widget in ECX (in_ECX), options record in ESI (unaff_ESI), both
// unresolved register reads. // blam-cc: ECX -> widget, ESI -> options_record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint32_t network_game_option_a_00719210; // 0x00719210, TYPES-GAP
extern uint32_t network_game_option_b_00719214; // 0x00719214, TYPES-GAP

// blam-cc: ECX -> widget, ESI -> options_record
uint8_t ui_network_game_options_populate(widget_instance *widget, const uint8_t *options_record)
{
    widget_instance *control;
    uint8_t value;

    if (options_record == (const uint8_t *)0) {
        return 0;
    }

    for (control = widget->first_child->first_child;
         control != (widget_instance *)0 && control->widget_type != 2;
         control = control->next_sibling) {
    }
    value = options_record[0xfc0];
    control->selection_index = (value < 5) ? value : 4;
    network_game_option_a_00719210 = *(const uint16_t *)(options_record + 0x1002);
    network_game_option_b_00719214 = *(const uint16_t *)(options_record + 0x1004);
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a3960):

undefined1 FUN_004a3960(void)

{
  int iVar1;
  int in_ECX;
  ushort uVar2;
  int unaff_ESI;

  if (unaff_ESI != 0) {
    for (iVar1 = *(int *)(*(int *)(in_ECX + 0x34) + 0x34);
        (iVar1 != 0 && (*(short *)(iVar1 + 0xe) != 2)); iVar1 = *(int *)(iVar1 + 0x2c)) {
    }
    if (*(byte *)(unaff_ESI + 0xfc0) < 5) {
      uVar2 = (ushort)*(byte *)(unaff_ESI + 0xfc0);
    }
    else {
      uVar2 = 4;
    }
    *(ushort *)(iVar1 + 0x40) = uVar2;
    DAT_00719210 = (uint)*(ushort *)(unaff_ESI + 0x1002);
    DAT_00719214 = (uint)*(ushort *)(&DAT_00001004 + unaff_ESI);
    return 1;
  }
  return 0;
}
#endif
