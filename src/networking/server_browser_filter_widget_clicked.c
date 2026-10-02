// server_browser_filter_widget_clicked  (Ghidra: server_browser_filter_widget_clicked, already
// named)
// address 0x4b7d80, size 385 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md's assigned name; the toggled globals
// (server_browser_allow_password 0x006953f9, server_browser_filter_dedicated_only 0x0071948b,
// server_browser_filter_classic_only 0x0071948c, server_browser_sort_ascending 0x006953f8) all
// match this module's other server_browser_* files; DAT_00719489 (server_browser_sort_column)
// is set to exactly the values (0,1,2,3,4) server_browser_sort_comparator_select.c's switch
// already keys on.
// register convention: clicked widget as a real parameter (param_1).
// UNSURE: the eight sibling widgets this function walks (password, dedicated, five sort
// columns and classic, in an order that does not match server_browser_sort_comparator_select's
// own column numbering 1:1) are identified only by click-time identity comparison, not by any
// independently confirmed column label; kept as plain local variables rather than named fields.
// The two `goto`s that jump into the middle of another case's logic (LAB_004b7e04,
// DAT_007196c8's shared reset tail) are preserved exactly via C `goto`.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t server_browser_allow_password;         // 0x006953f9
extern uint8_t server_browser_query_pending;             // 0x0071948a
extern uint8_t server_browser_filter_dedicated_only;  // 0x0071948b
extern uint8_t server_browser_sort_column;            // 0x00719489
extern uint8_t server_browser_sort_ascending;         // 0x006953f8
extern uint8_t server_browser_filter_classic_only;    // 0x0071948c
extern int32_t server_browser_query_elapsed_ms;       // 0x007196c8

extern void widget_play_sound_effect(void); // 0x498e90, outside this session's range

// blam-cc: clicked widget as param_1
int32_t server_browser_filter_widget_clicked(network_ui_widget *clicked)
{
    network_ui_widget *w1;
    network_ui_widget *w2;
    network_ui_widget *w3;
    network_ui_widget *w4;
    network_ui_widget *w5;
    network_ui_widget *w6;
    network_ui_widget *w7;
    uint8_t new_sort_column;

    w1 = clicked->parent->first_child;
    w2 = w1->next_sibling;
    w3 = w2->next_sibling;
    w4 = w3->next_sibling;
    w5 = w4->next_sibling;
    w6 = w5->next_sibling;
    w7 = w6->next_sibling;

    if (clicked == w1) {
        server_browser_allow_password = (server_browser_allow_password == 0);
        goto play_and_set_query_mode;
    }
    if (clicked == w2) {
        server_browser_filter_dedicated_only = (server_browser_filter_dedicated_only == 0);
        goto play_and_set_query_mode;
    }
    if (clicked == w3) {
        if (server_browser_sort_column == 0) {
        toggle_direction:
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
            goto play_and_reset_query_timer;
        }
        server_browser_sort_column = 0;
        new_sort_column = server_browser_sort_column;
    } else if (clicked == w4) {
        if (server_browser_sort_column == 1) {
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
            goto play_and_reset_query_timer;
        }
        server_browser_sort_column = 1;
        new_sort_column = server_browser_sort_column;
    } else if (clicked == w5) {
        server_browser_filter_classic_only = (server_browser_filter_classic_only == 0);
        goto play_and_set_query_mode;
    } else if (clicked == w6) {
        if (server_browser_sort_column == 2) {
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
        } else {
            server_browser_sort_column = 2;
            server_browser_sort_ascending = 1;
        }
        goto play_and_reset_query_timer;
    } else if (clicked == w7) {
        new_sort_column = 4;
        if (server_browser_sort_column == 4) {
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
            goto play_and_reset_query_timer;
        }
    } else if (clicked == w7->next_sibling) {
        new_sort_column = 3;
        if (server_browser_sort_column == 3) {
            goto toggle_direction;
        }
    } else {
        return 1;
    }
    server_browser_sort_column = new_sort_column;
    server_browser_sort_ascending = 1;
play_and_reset_query_timer:
    widget_play_sound_effect();
    server_browser_query_elapsed_ms = 9999;
    return 1;
play_and_set_query_mode:
    widget_play_sound_effect();
    server_browser_query_pending = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b7d80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 server_browser_filter_widget_clicked(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  char cVar8;

  iVar1 = *(int *)(*(int *)(param_1 + 0x30) + 0x34);
  iVar2 = *(int *)(iVar1 + 0x2c);
  iVar3 = *(int *)(iVar2 + 0x2c);
  iVar4 = *(int *)(iVar3 + 0x2c);
  iVar5 = *(int *)(iVar4 + 0x2c);
  iVar6 = *(int *)(iVar5 + 0x2c);
  iVar7 = *(int *)(iVar6 + 0x2c);
  if (param_1 == iVar1) {
    DAT_006953f9 = DAT_006953f9 == '\0';
LAB_004b7dc8:
    widget_play_sound_effect();
    DAT_0071948a = 1;
    return 1;
  }
  if (param_1 == iVar2) {
    DAT_0071948b = DAT_0071948b == '\0';
    goto LAB_004b7dc8;
  }
  if (param_1 == iVar3) {
    if (DAT_00719489 == '\0') {
LAB_004b7e04:
      DAT_006953f8 = DAT_006953f8 == '\0';
      goto LAB_004b7eea;
    }
    DAT_00719489 = '\0';
    cVar8 = DAT_00719489;
  }
  else if (param_1 == iVar4) {
    if (DAT_00719489 == '\x01') {
      DAT_006953f8 = DAT_006953f8 == '\0';
      goto LAB_004b7eea;
    }
    DAT_00719489 = '\x01';
    cVar8 = DAT_00719489;
  }
  else {
    if (param_1 == iVar5) {
      DAT_0071948c = DAT_0071948c == '\0';
      goto LAB_004b7dc8;
    }
    if (param_1 == iVar6) {
      if (DAT_00719489 == '\x02') {
        DAT_006953f8 = DAT_006953f8 == '\0';
      }
      else {
        DAT_00719489 = '\x02';
        DAT_006953f8 = true;
      }
      goto LAB_004b7eea;
    }
    if (param_1 == iVar7) {
      cVar8 = '\x04';
      if (DAT_00719489 == '\x04') {
        DAT_006953f8 = DAT_006953f8 == '\0';
        goto LAB_004b7eea;
      }
    }
    else {
      if (param_1 != *(int *)(iVar7 + 0x2c)) {
        return 1;
      }
      cVar8 = '\x03';
      if (DAT_00719489 == '\x03') goto LAB_004b7e04;
    }
  }
  DAT_00719489 = cVar8;
  DAT_006953f8 = true;
LAB_004b7eea:
  widget_play_sound_effect();
  _DAT_007196c8 = 9999;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
