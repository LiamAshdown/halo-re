// ui_server_type_option_selected  (Ghidra: FUN_004a47c0, renamed)
// renamed from FUN_004a47c0 in the naming pass
// address 0x4a47c0, size 141 bytes; real extent 0x4a47c0..0x4a484c plus the jump table at 0x4a4850
// name confidence: 0.3   rewrite confidence: 0.7
// evidence: rewritten in the phase-4 review from objdump -d 0x4a47c0..0x4a4870. The widget
// position among its siblings (0 based) minus one indexes a six entry jump table; the targets
// only store 0 or 1 into five byte flags and return 1:
//   index 0 and 2: 0x00719234 = 0, 0x00719235 = 0, 0x00692b10 = 1, 0x006894a2 = 1, 0x006953f0 = 1
//   index 1:       0x006894a2 = 0, 0x006953f0 = 0, 0x00692b10 = 1
//   index 4:       0x00692b10 = 0, 0x00719234 = 0, 0x00719235 = 0, 0x006894a2 = 1, 0x006953f0 = 1
//   index 5:       0x00692b10 = 0, 0x006894a2 = 0, 0x006953f0 = 0
// Entry 3 of the table is NULL and the first sibling reads the dword before the table, so
// those two positions jump to garbage in the binary; they are left unhandled here. A widget
// that is not found among its siblings returns 0.
// UNSURE: the meaning of the five flags; 0x006953f0 is server_browser_require_valid_entry in
// src/networking, 0x00719234/0x00719235 are the autopatch_status_widget_update.c flags.
// register convention: cdecl, the one stack parameter (widget); returns a byte.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t autopatch_status_state_00719234;   // 0x00719234
extern uint8_t autopatch_status_active_00719235;  // 0x00719235
extern uint8_t ui_server_option_flag_00692b10;    // 0x00692b10
extern uint8_t network_capability_flag_006894a2;  // 0x006894a2
extern uint8_t server_browser_require_valid_entry; // 0x006953f0

uint8_t ui_server_type_option_selected(widget_instance *widget)
{
    widget_instance *sibling = widget->parent->first_child;
    int32_t index = 0;

    while (sibling != (widget_instance *)0 && sibling != widget) {
        sibling = sibling->next_sibling;
        index++;
    }
    if (sibling == (widget_instance *)0) {
        return 0;
    }

    switch (index - 1) {
    case 0:
    case 2:
        autopatch_status_state_00719234 = 0;
        autopatch_status_active_00719235 = 0;
        ui_server_option_flag_00692b10 = 1;
        network_capability_flag_006894a2 = 1;
        server_browser_require_valid_entry = 1;
        break;
    case 1:
        network_capability_flag_006894a2 = 0;
        server_browser_require_valid_entry = 0;
        ui_server_option_flag_00692b10 = 1;
        break;
    case 4:
        ui_server_option_flag_00692b10 = 0;
        autopatch_status_state_00719234 = 0;
        autopatch_status_active_00719235 = 0;
        network_capability_flag_006894a2 = 1;
        server_browser_require_valid_entry = 1;
        break;
    case 5:
        ui_server_option_flag_00692b10 = 0;
        network_capability_flag_006894a2 = 0;
        server_browser_require_valid_entry = 0;
        break;
    default: // -1 and 3 jump through garbage in the binary
        break;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a47c0):

void FUN_004a47c0(int param_1)

{
  int iVar1;
  int iVar2;

  iVar1 = *(int *)(*(int *)(param_1 + 0x30) + 0x34);
  iVar2 = 0;
  while( true ) {
    if (iVar1 == 0) {
      return;
    }
    if (iVar1 == param_1) break;
    iVar1 = *(int *)(iVar1 + 0x2c);
    iVar2 = iVar2 + 1;
  }
                    /* WARNING: Could not recover jumptable at 0x004a47e4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(&PTR_LAB_004a4850)[iVar2 + -1])();
  return;
}
#endif
