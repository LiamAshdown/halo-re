// server_browser_filter_headers_refresh  (Ghidra: FUN_004b7f70, still unnamed -> renamed)
// address 0x4b7f70, size 378 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md summary ("refreshes the sort-column header
// widgets of the join-game server browser list, applying sort-arrow icons and highlight alpha
// for whichever column is the active sort key"); the three explicitly-computed widgets
// (`in_EAX`'s first_child, its next sibling, and that sibling's third next sibling) resolve to
// exactly the password/dedicated/classic checkboxes server_browser_filter_widget_clicked.c
// identifies at the same three sibling positions in the same 8-wide row, which is what anchors
// this function's own widget-tree position.
// register convention: filter-row widget in EAX (in_EAX).
// UNSURE: the five bare `server_browser_column_header_update()` (server_browser_column_header_update) calls show no
// visible arguments at all. They are reconstructed here as the same five "sort column" siblings
// server_browser_filter_widget_clicked.c identifies in this row (values 0, 1, 2, 4, 3), each
// passed its own current sort direction (+1/-1 if it is the active sort_column, matching
// server_browser_sort_ascending, else 0) -- this mapping is plausible given the shared row
// layout but is not independently confirmed by this function's own decompile.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t server_browser_sort_column;            // 0x00719489
extern uint8_t server_browser_sort_ascending;          // 0x006953f8
extern uint8_t server_browser_allow_password;          // 0x006953f9
extern uint8_t server_browser_filter_dedicated_only;   // 0x0071948b
extern uint8_t server_browser_filter_classic_only;     // 0x0071948c

extern void server_browser_column_header_update(network_ui_widget *header, int32_t sort_direction); // 0x4b7f10, this module

// blam-cc: filter-row widget in EAX (in_EAX)
void server_browser_filter_headers_refresh(network_ui_widget *row)
{
    network_ui_widget *password_check;
    network_ui_widget *dedicated_check;
    network_ui_widget *classic_check;
    network_ui_widget *sort_col0;
    network_ui_widget *sort_col1;
    network_ui_widget *sort_col2;
    network_ui_widget *sort_col4;
    network_ui_widget *sort_col3;
    network_ui_widget *mark;
    int32_t direction;

    password_check = row->first_child;
    dedicated_check = password_check->next_sibling;
    sort_col0 = dedicated_check->next_sibling;
    sort_col1 = sort_col0->next_sibling;
    classic_check = sort_col1->next_sibling;
    sort_col2 = classic_check->next_sibling;
    sort_col4 = sort_col2->next_sibling;
    sort_col3 = sort_col4->next_sibling;

    direction = (server_browser_sort_column == 0) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    server_browser_column_header_update(sort_col0, direction);
    direction = (server_browser_sort_column == 1) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    server_browser_column_header_update(sort_col1, direction);
    direction = (server_browser_sort_column == 2) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    server_browser_column_header_update(sort_col2, direction);
    direction = (server_browser_sort_column == 4) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    server_browser_column_header_update(sort_col4, direction);
    direction = (server_browser_sort_column == 3) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    server_browser_column_header_update(sort_col3, direction);

    mark = password_check->first_child;
    mark->highlight_flag = (server_browser_allow_password == 0) + 1;
    if (password_check->parent->selected_child == password_check) {
        password_check->highlight_flag = 1;
        *(uint32_t *)&mark->alpha = 0x3f800000;
    } else {
        password_check->highlight_flag = 0;
        *(uint32_t *)&mark->alpha = 0x3f400000;
    }

    mark = dedicated_check->first_child;
    mark->highlight_flag = (server_browser_filter_dedicated_only != 0) + 1;
    if (dedicated_check->parent->selected_child == dedicated_check) {
        dedicated_check->highlight_flag = 1;
        *(uint32_t *)&mark->alpha = 0x3f800000;
    } else {
        dedicated_check->highlight_flag = 0;
        *(uint32_t *)&mark->alpha = 0x3f400000;
    }

    mark = classic_check->first_child;
    mark->highlight_flag = (server_browser_filter_classic_only != 0) + 1;
    if (classic_check->parent->selected_child == classic_check) {
        classic_check->highlight_flag = 1;
        *(uint32_t *)&mark->alpha = 0x3f800000;
        return;
    }
    classic_check->highlight_flag = 0;
    *(uint32_t *)&mark->alpha = 0x3f400000;
}

#if 0
Original Ghidra decompilation (0x4b7f70):

void FUN_004b7f70(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int in_EAX;

  iVar1 = *(int *)(in_EAX + 0x34);
  iVar2 = *(int *)(iVar1 + 0x2c);
  iVar3 = *(int *)(*(int *)(*(int *)(iVar2 + 0x2c) + 0x2c) + 0x2c);
  FUN_004b7f10();
  FUN_004b7f10();
  FUN_004b7f10();
  FUN_004b7f10();
  FUN_004b7f10();
  iVar4 = *(int *)(iVar1 + 0x34);
  *(ushort *)(iVar4 + 0x58) = (DAT_006953f9 == '\0') + 1;
  if (*(int *)(*(int *)(iVar1 + 0x30) + 0x38) == iVar1) {
    *(undefined2 *)(iVar1 + 0x58) = 1;
    *(undefined4 *)(iVar4 + 0x24) = 0x3f800000;
  }
  else {
    *(undefined2 *)(iVar1 + 0x58) = 0;
    *(undefined4 *)(iVar4 + 0x24) = 0x3f400000;
  }
  iVar1 = *(int *)(iVar2 + 0x34);
  *(ushort *)(iVar1 + 0x58) = (DAT_0071948b != '\0') + 1;
  if (*(int *)(*(int *)(iVar2 + 0x30) + 0x38) == iVar2) {
    *(undefined2 *)(iVar2 + 0x58) = 1;
    *(undefined4 *)(iVar1 + 0x24) = 0x3f800000;
  }
  else {
    *(undefined2 *)(iVar2 + 0x58) = 0;
    *(undefined4 *)(iVar1 + 0x24) = 0x3f400000;
  }
  iVar1 = *(int *)(iVar3 + 0x34);
  *(ushort *)(iVar1 + 0x58) = (DAT_0071948c != '\0') + 1;
  if (*(int *)(*(int *)(iVar3 + 0x30) + 0x38) == iVar3) {
    *(undefined2 *)(iVar3 + 0x58) = 1;
    *(undefined4 *)(iVar1 + 0x24) = 0x3f800000;
    return;
  }
  *(undefined2 *)(iVar3 + 0x58) = 0;
  *(undefined4 *)(iVar1 + 0x24) = 0x3f400000;
  return;
}
#endif
