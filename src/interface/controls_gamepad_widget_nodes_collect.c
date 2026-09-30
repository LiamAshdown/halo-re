// controls_gamepad_widget_nodes_collect  (Ghidra: FUN_004b5560, named in phase 4)
// address 0x4b5560, size 107 bytes
// name confidence: 0.45   rewrite confidence: 0.9
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: checked against objdump 0x4b5560..0x4b55ca in the phase-4 review. Renamed from
// controls_server_list_widget_nodes_collect: every caller (controls_gamepad_lists_load @0x4b58d0,
// controls_gamepad_toggle_assignment @0x4b5b20 and controls_gamepad_lists_refresh @0x4b55d0) is on the
// gamepad assignment screen of the controls setup. Fills 17 widget pointers
// of the screen passed in ECX:
//   [0]      the assigned list (first child)       [1..4]   its four rows
//   [5]      the available list (next sibling)    [6..13]  its eight rows
//   [14..16] the three buttons after the available list
// register convention: EAX out array, ECX screen widget; no stack arguments.
//   // blam-cc: out -> EAX, screen -> ECX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

// blam-cc: out -> EAX, screen -> ECX
void controls_gamepad_widget_nodes_collect(widget_instance **out, widget_instance *screen)
{
    widget_instance *w;

    w = screen->first_child;
    out[0] = w;
    w = w->first_child;
    out[1] = w;
    w = w->next_sibling;
    out[2] = w;
    w = w->next_sibling;
    out[3] = w;
    out[4] = w->next_sibling;

    w = out[0]->next_sibling;
    out[5] = w;
    w = w->first_child;
    out[6] = w;
    w = w->next_sibling;
    out[7] = w;
    w = w->next_sibling;
    out[8] = w;
    w = w->next_sibling;
    out[9] = w;
    w = w->next_sibling;
    out[10] = w;
    w = w->next_sibling;
    out[11] = w;
    w = w->next_sibling;
    out[12] = w;
    out[13] = w->next_sibling;

    w = out[5]->next_sibling;
    out[14] = w;
    w = w->next_sibling;
    out[15] = w;
    out[16] = w->next_sibling;
}

#if 0
Original Ghidra decompilation (0x4b5560):

void FUN_004b5560(void)

{
  int iVar1;
  int *in_EAX;
  int in_ECX;

  iVar1 = *(int *)(in_ECX + 0x34);
  *in_EAX = iVar1;
  iVar1 = *(int *)(iVar1 + 0x34);
  in_EAX[1] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[2] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[3] = iVar1;
  in_EAX[4] = *(int *)(iVar1 + 0x2c);
  iVar1 = *(int *)(*in_EAX + 0x2c);
  in_EAX[5] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x34);
  in_EAX[6] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[7] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[8] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[9] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[10] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[0xb] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[0xc] = iVar1;
  in_EAX[0xd] = *(int *)(iVar1 + 0x2c);
  iVar1 = *(int *)(in_EAX[5] + 0x2c);
  in_EAX[0xe] = iVar1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  in_EAX[0xf] = iVar1;
  in_EAX[0x10] = *(int *)(iVar1 + 0x2c);
  return;
}
#endif
