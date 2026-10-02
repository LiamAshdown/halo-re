// server_browser_filter_panel_set_mode  (Ghidra: FUN_004b61c0, still unnamed -> renamed)
// address 0x4b61c0, size 429 bytes
// name confidence: 0.35   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md summary ("configures the visibility and value
// fields of a chain of server-browser filter UI widgets, switching between two display modes
// (e.g. internet vs LAN)"); the five globals read while filling in widget default values
// (0x006953fa, 0x006953fb, 0x0071948d, 0x0071948e, 0x0071948f, 0x00719490) are exactly
// server_browser_allow_empty, server_browser_allow_full and four of the six
// server_browser_filters bytes (allow_unknown_map, gametype, teamplay, ping_limit_index) from
// types/networking.h's "server browser" section.
// register convention: root widget pointer in EAX (in_EAX); mode flag as an ordinary stack
// parameter (param_1: nonzero selects "internet", zero selects "LAN").
// UNSURE: this operates on a generic UI menu widget tree (parent/first_child/next_sibling at
// +0x30/+0x34/+0x2c, a type discriminant at +0xe, a numeric value/max pair at +0x40/+0x48),
// which is not a Blam networking type -- it belongs to the not-yet-rewritten interface module.
// The widget node is types/networking.h's network_ui_widget, which captures only the fields
// this module reads or writes; every other field is unlabeled padding. Field names beyond the
// ones the evidence above confirms (visible/hidden and the selected-child pointer at +0x38)
// are guesses.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t server_browser_allow_empty; // 0x006953fa
extern uint8_t server_browser_allow_full;  // 0x006953fb
extern uint8_t server_browser_filter_allow_unknown_map;               // 0x0071948d, server_browser_filters.allow_unknown_map
extern uint8_t server_browser_filter_gametype;               // 0x0071948e, server_browser_filters.gametype
extern uint8_t server_browser_filter_teamplay;               // 0x0071948f, server_browser_filters.teamplay
extern uint8_t server_browser_filter_ping_limit_index;               // 0x00719490, server_browser_filters.ping_limit_index
extern uint8_t server_browser_filter_panel_mode; // 0x007196b4

// Walks `container`'s children for the first one whose type is 2 (a control widget); returns
// NULL if the container has no such child.
static network_ui_widget *ui_widget_find_control(network_ui_widget *container)
{
    network_ui_widget *w;
    for (w = container->first_child; w != 0 && w->type != 2; w = w->next_sibling) {
    }
    return w;
}

// blam-cc: root widget pointer in EAX (in_EAX), mode flag as an ordinary stack parameter
void server_browser_filter_panel_set_mode(network_ui_widget *panel, uint8_t internet_mode)
{
    network_ui_widget *w;
    network_ui_widget *control;
    uint16_t clamped;

    w = panel->first_child;
    if (internet_mode == 0) {
        w->visible = 1;
        w->hidden = 0;
    } else {
        w->visible = 0;
        w->hidden = 1;
    }
    w = w->next_sibling;
    if (internet_mode == 0) {
        w->visible = 1;
        w->hidden = 0;
    } else {
        w->visible = 0;
        w->hidden = 1;
    }
    w = w->next_sibling;
    if (internet_mode == 0) {
        w->visible = 1;
        w->hidden = 0;
    } else {
        w->visible = 0;
        w->hidden = 1;
    }
    w = w->next_sibling;
    w->visible = 0;
    w->hidden = 1;
    w = w->next_sibling;
    if (internet_mode != 0) {
        w->visible = 1;
        w->hidden = 0;
        w->parent->selected_child = w;

        w = w->first_child->next_sibling->first_child;
        control = ui_widget_find_control(w);
        control->value = (int16_t)(server_browser_allow_empty != 0);

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        control->value = (int16_t)(server_browser_allow_full != 0);

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        clamped = control->max_value - 1;
        if ((uint16_t)server_browser_filter_ping_limit_index <= clamped) {
            clamped = server_browser_filter_ping_limit_index;
        }
        control->value = (int16_t)clamped;

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        clamped = control->max_value - 1;
        if ((uint16_t)server_browser_filter_gametype <= clamped) {
            clamped = server_browser_filter_gametype;
        }
        control->value = (int16_t)clamped;

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        clamped = control->max_value - 1;
        if ((uint16_t)server_browser_filter_teamplay <= clamped) {
            clamped = server_browser_filter_teamplay;
        }
        control->value = (int16_t)clamped;

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        control->value = (int16_t)(server_browser_filter_allow_unknown_map != 0);

        w->visible = 0;
        w->hidden = 1;
        server_browser_filter_panel_mode = internet_mode;
        return;
    }
    w->visible = 0;
    w->hidden = 1;
    w->parent->selected_child = w->parent->first_child->next_sibling;
    server_browser_filter_panel_mode = internet_mode;
}

#if 0
Original Ghidra decompilation (0x4b61c0):

void FUN_004b61c0(char param_1)

{
  int iVar1;
  int iVar2;
  int in_EAX;
  uint uVar3;

  iVar1 = *(int *)(in_EAX + 0x34);
  if (param_1 == '\0') {
    *(undefined1 *)(iVar1 + 0x10) = 1;
    *(undefined1 *)(iVar1 + 0x12) = 0;
  }
  else {
    *(undefined1 *)(iVar1 + 0x10) = 0;
    *(undefined1 *)(iVar1 + 0x12) = 1;
  }
  iVar1 = *(int *)(iVar1 + 0x2c);
  if (param_1 == '\0') {
    *(undefined1 *)(iVar1 + 0x10) = 1;
    *(undefined1 *)(iVar1 + 0x12) = 0;
  }
  else {
    *(undefined1 *)(iVar1 + 0x10) = 0;
    *(undefined1 *)(iVar1 + 0x12) = 1;
  }
  iVar1 = *(int *)(iVar1 + 0x2c);
  if (param_1 == '\0') {
    *(undefined1 *)(iVar1 + 0x10) = 1;
    *(undefined1 *)(iVar1 + 0x12) = 0;
  }
  else {
    *(undefined1 *)(iVar1 + 0x10) = 0;
    *(undefined1 *)(iVar1 + 0x12) = 1;
  }
  iVar1 = *(int *)(iVar1 + 0x2c);
  *(undefined1 *)(iVar1 + 0x10) = 0;
  *(undefined1 *)(iVar1 + 0x12) = 1;
  iVar1 = *(int *)(iVar1 + 0x2c);
  if (param_1 != '\0') {
    *(undefined1 *)(iVar1 + 0x10) = 1;
    *(undefined1 *)(iVar1 + 0x12) = 0;
    *(int *)(*(int *)(iVar1 + 0x30) + 0x38) = iVar1;
    iVar1 = *(int *)(*(int *)(*(int *)(iVar1 + 0x34) + 0x2c) + 0x34);
    for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
        iVar2 = *(int *)(iVar2 + 0x2c)) {
    }
    *(ushort *)(iVar2 + 0x40) = (ushort)(DAT_006953fa != '\0');
    iVar1 = *(int *)(iVar1 + 0x2c);
    for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
        iVar2 = *(int *)(iVar2 + 0x2c)) {
    }
    *(ushort *)(iVar2 + 0x40) = (ushort)(DAT_006953fb != '\0');
    iVar1 = *(int *)(iVar1 + 0x2c);
    for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
        iVar2 = *(int *)(iVar2 + 0x2c)) {
    }
    uVar3 = *(ushort *)(iVar2 + 0x48) - 1;
    if ((int)(uint)DAT_00719490 <= (int)uVar3) {
      uVar3 = (uint)DAT_00719490;
    }
    *(short *)(iVar2 + 0x40) = (short)uVar3;
    iVar1 = *(int *)(iVar1 + 0x2c);
    for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
        iVar2 = *(int *)(iVar2 + 0x2c)) {
    }
    uVar3 = *(ushort *)(iVar2 + 0x48) - 1;
    if ((int)(uint)DAT_0071948e <= (int)uVar3) {
      uVar3 = (uint)DAT_0071948e;
    }
    *(short *)(iVar2 + 0x40) = (short)uVar3;
    iVar1 = *(int *)(iVar1 + 0x2c);
    for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
        iVar2 = *(int *)(iVar2 + 0x2c)) {
    }
    uVar3 = *(ushort *)(iVar2 + 0x48) - 1;
    if ((int)(uint)DAT_0071948f <= (int)uVar3) {
      uVar3 = (uint)DAT_0071948f;
    }
    *(short *)(iVar2 + 0x40) = (short)uVar3;
    iVar1 = *(int *)(iVar1 + 0x2c);
    for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
        iVar2 = *(int *)(iVar2 + 0x2c)) {
    }
    *(ushort *)(iVar2 + 0x40) = (ushort)(DAT_0071948d != '\0');
    *(undefined1 *)(iVar1 + 0x10) = 0;
    *(undefined1 *)(iVar1 + 0x12) = 1;
    DAT_007196b4 = param_1;
    return;
  }
  *(undefined1 *)(iVar1 + 0x10) = 0;
  *(undefined1 *)(iVar1 + 0x12) = 1;
  *(undefined4 *)(*(int *)(iVar1 + 0x30) + 0x38) =
       *(undefined4 *)(*(int *)(*(int *)(*(int *)(iVar1 + 0x30) + 0x34) + 0x2c) + 0x2c);
  DAT_007196b4 = param_1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
