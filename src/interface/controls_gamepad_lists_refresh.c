// controls_gamepad_lists_refresh  (Ghidra: FUN_004b55d0, named in phase 4)
// address 0x4b55d0, size 394 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// phase-4 review: this family was named as a server history / favorites list; every caller
// is on the controls setup gamepad screen (see types/interface.h controls_gamepad_record), so
// it was renamed; the old names are logged in symbols/agent_phase4_interface.txt.
// evidence: rewritten from objdump 0x4b55d0..0x4b5759 in the phase-4 review. Renamed from
// controls_server_list_widgets_refresh (it only touches the gamepad assignment screen). The first rewrite dropped the ECX screen widget (it passed the history array as
// the widget), the heap_reallocate register arguments and the list hidden flags, and read
// row i instead of row i + 1 of the node array.
//   The assigned list (node 0) is hidden when controls_assigned_gamepad_count is 0, the available list
// (node 5) when controls_available_gamepad_count is 0. Each assigned row (nodes 1..4) and available row
// (nodes 6..13) gets a 0x80 byte text block on its first child (heap_reallocate in
// widget_memory_pool) holding the first 0x3f characters of the entry (the entry starts with
// its wide device name), terminated at +0x7e; a filled row is shown at scale 1.0. An empty
// assigned row shows L"---" (0x0066aab4), an empty available row L"" (0x00660c34); both are
// hidden, scale 0.333 (0x3eaa7efa), background frame 0. A filled available row is also dimmed
// while the assigned list is full (4 entries), since it cannot be moved there.
// register convention: ECX screen widget (passed straight to 0x4b5560); no stack arguments.
//   // blam-cc: screen -> ECX

#include <wchar.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern heap *widget_memory_pool;                  // 0x006926c4
extern controls_gamepad_record controls_available_gamepads[8]; // 0x006b42d8
extern controls_gamepad_record controls_assigned_gamepads[4];   // 0x006b53d8
extern int32_t controls_assigned_gamepad_count;             // 0x00719448
extern int32_t controls_available_gamepad_count;           // 0x0071944c

extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void controls_gamepad_widget_nodes_collect(widget_instance **out, widget_instance *screen); // 0x4b5560, blam-cc: EAX out, ECX screen

static void controls_gamepad_row_set_disabled(widget_instance *row)
{
    row->hidden = 1;
    row->scale = 0.333f; // 0x3eaa7efa
    row->background_bitmap_frame = 0;
}

// blam-cc: screen -> ECX
void controls_gamepad_lists_refresh(widget_instance *screen)
{
    static const uint16_t dashes_text[4] = {'-', '-', '-', 0}; // 0x0066aab4
    static const uint16_t empty_text[1] = {0};                 // 0x00660c34
    widget_instance *nodes[17];
    int32_t i;

    controls_gamepad_widget_nodes_collect(nodes, screen);

    nodes[0]->hidden = controls_assigned_gamepad_count == 0;
    for (i = 0; i < 4; i++) {
        widget_instance *text = nodes[1 + i]->first_child;
        uint16_t *buffer = (uint16_t *)heap_reallocate(text->text, 0x80, widget_memory_pool);

        text->text = buffer;
        if (buffer == 0) {
            continue;
        }
        if (i < controls_assigned_gamepad_count) {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)&controls_assigned_gamepads[i], 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
            text->parent->hidden = 0;
            text->parent->scale = 1.0f;
        } else {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)dashes_text, 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
            controls_gamepad_row_set_disabled(text->parent);
        }
    }

    nodes[5]->hidden = controls_available_gamepad_count == 0;
    for (i = 0; i < 8; i++) {
        widget_instance *text = nodes[6 + i]->first_child;
        uint16_t *buffer = (uint16_t *)heap_reallocate(text->text, 0x80, widget_memory_pool);

        text->text = buffer;
        if (buffer == 0) {
            continue;
        }
        if (i < controls_available_gamepad_count) {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)&controls_available_gamepads[i], 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
            if (controls_assigned_gamepad_count != 4) {
                text->parent->hidden = 0;
                text->parent->scale = 1.0f;
                continue;
            }
        } else {
            wcsncpy((wchar_t *)buffer, (const wchar_t *)empty_text, 0x3f);
            ((uint16_t *)text->text)[0x3f] = 0;
        }
        controls_gamepad_row_set_disabled(text->parent);
    }
}

#if 0
Original Ghidra decompilation (0x4b55d0):

void FUN_004b55d0(void)

{
  wchar_t *pwVar1;
  wchar_t *pwVar2;
  int iVar3;
  int local_48;
  int local_44;
  int aiStack_40 [4];
  int local_30;
  int aiStack_2c [11];

  FUN_004b5560();
  *(bool *)(local_44 + 0x12) = DAT_00719448 == 0;
  local_48 = 0;
  pwVar2 = (wchar_t *)&DAT_006b53d8;
  do {
    iVar3 = *(int *)(aiStack_40[local_48] + 0x34);
    pwVar1 = (wchar_t *)heap_reallocate(0x80);
    *(wchar_t **)(iVar3 + 0x3c) = pwVar1;
    if (pwVar1 != (wchar_t *)0x0) {
      if (local_48 < DAT_00719448) {
        _wcsncpy(pwVar1,pwVar2,0x3f);
        *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x7e) = 0;
        iVar3 = *(int *)(iVar3 + 0x30);
        *(undefined1 *)(iVar3 + 0x12) = 0;
        *(undefined4 *)(iVar3 + 0x24) = 0x3f800000;
      }
      else {
        _wcsncpy(pwVar1,L"---",0x3f);
        *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x7e) = 0;
        iVar3 = *(int *)(iVar3 + 0x30);
        *(undefined1 *)(iVar3 + 0x12) = 1;
        *(undefined4 *)(iVar3 + 0x24) = 0x3eaa7efa;
        *(undefined2 *)(iVar3 + 0x58) = 0;
      }
    }
    local_48 = local_48 + 1;
    pwVar2 = pwVar2 + 0x110;
  } while ((int)pwVar2 < 0x6b5c58);
  *(bool *)(local_30 + 0x12) = DAT_0071944c == 0;
  local_48 = 0;
  pwVar2 = (wchar_t *)&DAT_006b42d8;
  do {
    iVar3 = *(int *)(aiStack_2c[local_48] + 0x34);
    pwVar1 = (wchar_t *)heap_reallocate(0x80);
    *(wchar_t **)(iVar3 + 0x3c) = pwVar1;
    if (pwVar1 != (wchar_t *)0x0) {
      if (local_48 < DAT_0071944c) {
        _wcsncpy(pwVar1,pwVar2,0x3f);
        *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x7e) = 0;
        iVar3 = *(int *)(iVar3 + 0x30);
        if (DAT_00719448 != 4) {
          *(undefined1 *)(iVar3 + 0x12) = 0;
          *(undefined4 *)(iVar3 + 0x24) = 0x3f800000;
          goto LAB_004b5737;
        }
      }
      else {
        _wcsncpy(pwVar1,L"",0x3f);
        *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x7e) = 0;
        iVar3 = *(int *)(iVar3 + 0x30);
      }
      *(undefined1 *)(iVar3 + 0x12) = 1;
      *(undefined4 *)(iVar3 + 0x24) = 0x3eaa7efa;
      *(undefined2 *)(iVar3 + 0x58) = 0;
    }
LAB_004b5737:
    local_48 = local_48 + 1;
    pwVar2 = pwVar2 + 0x110;
    if (0x6b53d7 < (int)pwVar2) {
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
