// player_profile_details_widget_refresh  (Ghidra: player_profile_details_widget_refresh, already
// named)
// address 0x4a6100, size 434 bytes
// name confidence: 0.5   rewrite confidence: 0.15
// evidence: matches the given name; functions.md: "Refreshes the player-profile detail widgets
// (name, sensitivity, controller type, invert flag) for a selected profile, or blanks them if
// none is selected."
// register convention: widget in EAX (in_EAX), profile record pointer as the one recognized stack
// parameter (NULL blanks the widgets). // blam-cc: EAX -> widget, stack -> profile_record
// UNSURE (significant): `local_4 = (short)iVar3` truncates what was, a few lines earlier, used as
// a widget pointer (row 3) down to 16 bits and uses THAT as a sensitivity-like value -- almost
// certainly a Ghidra register-reuse artifact (iVar3 is reassigned to something else by this point
// in the real code) rather than real behaviour, but preserved exactly as decompiled per this
// session's "no invented behaviour" rule rather than guessed at.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern uint16_t default_player_profile_name_0066a750[]; // 0x0066a750

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void player_profile_scan_campaign_progress(void); // 0x539e00, foreign (profile module), UNSURE
extern uint16_t *text_string_list_get_string(void); // 0x5578c0, UNSURE args
extern void _wcsncpy(uint16_t *dest, const uint16_t *src, uint32_t count);

// blam-cc: EAX -> widget, stack -> profile_record
void player_profile_details_widget_refresh(widget_instance *widget, const uint8_t *profile_record)
{
    widget_instance *row1 = widget->first_child;
    widget_instance *row2 = row1->next_sibling;
    widget_instance *row3 = row2->next_sibling->first_child;
    widget_instance *row4 = row3->next_sibling;
    widget_instance *row5 = row4->next_sibling;
    widget_instance *row6 = row5->next_sibling;
    widget_instance *row7 = row6->next_sibling;
    widget_instance *row8 = row7->next_sibling;

    if (profile_record == (const uint8_t *)0) {
        row1->state = 0;
        row2->background_bitmap_frame = 0x12;
        row3->state = 1;
        row4->state = 0;
        row5->state = 0;
        row6->state = 0;
        row7->state = 0;
        row8->state = 0;
        return;
    }

    row1->state = 1;
    row3->state = 0;
    row4->state = 1;
    row5->state = 1;
    row6->state = 1;
    row7->state = 1;
    row8->state = 1;

    {
        uint16_t *buffer = (uint16_t *)heap_reallocate(row1->text, 0x18, widget_memory_pool);

        row1->text = buffer;
        if (buffer != (uint16_t *)0) {
            if ((*(const uint16_t *)(profile_record + 0x11c) & 1) == 0) {
                _wcsncpy(buffer, (const uint16_t *)(profile_record + 2), 0xb);
            } else {
                datum_index names_tag =
                    tag_lookup(0x75737472 /* 'ustr' */, "ui\\shell\\strings\\default_player_profile_names");
                const uint16_t *source = (names_tag == (datum_index)-1) ? default_player_profile_name_0066a750
                                                                          : text_string_list_get_string();

                _wcsncpy((uint16_t *)row1->text, source, 0xb);
            }
            ((uint16_t *)row1->text)[0xb] = 0;
        }
    }

    {
        int16_t sensitivity = *(const int16_t *)(profile_record + 0x11a);

        if (sensitivity < 0) {
            sensitivity = 0;
        } else if (sensitivity > 0x11) {
            sensitivity = 0x11;
        }
        row2->background_bitmap_frame = sensitivity;
    }

    if ((profile_record[0x11c] & 1) == 0) {
        int32_t clamped;

        player_profile_scan_campaign_progress();
        // UNSURE: see file header -- `row3` reused as a raw 16-bit value here, not a widget.
        clamped = (int16_t)(int32_t)row3 + 1;
        if (clamped > 9) {
            clamped = 9;
        }
        row3->selection_index = (int16_t)clamped;
        row5->selection_index = (int16_t)(int32_t)profile_record;
        row7->selection_index = (profile_record[0x12f] == 1);
        return;
    }
    row3->state = 0;
    row5->state = 0;
}

#if 0
Original Ghidra decompilation (0x4a6100):

void player_profile_details_widget_refresh(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  short sVar8;
  int in_EAX;
  wchar_t *_Dest;
  int iVar9;
  undefined **_Source;
  int iVar10;
  short local_4;

  iVar10 = *(int *)(in_EAX + 0x34);
  iVar1 = *(int *)(iVar10 + 0x2c);
  iVar9 = *(int *)(*(int *)(iVar1 + 0x2c) + 0x34);
  iVar2 = *(int *)(iVar9 + 0x2c);
  iVar3 = *(int *)(iVar2 + 0x2c);
  iVar4 = *(int *)(iVar3 + 0x2c);
  iVar5 = *(int *)(iVar4 + 0x2c);
  iVar6 = *(int *)(iVar5 + 0x2c);
  iVar7 = *(int *)(iVar6 + 0x2c);
  if (param_1 == 0) {
    *(undefined1 *)(iVar10 + 0x10) = 0;
    *(undefined2 *)(iVar1 + 0x58) = 0x12;
    *(undefined1 *)(iVar9 + 0x10) = 1;
    *(undefined1 *)(iVar2 + 0x10) = 0;
    *(undefined1 *)(iVar3 + 0x10) = 0;
    *(undefined1 *)(iVar4 + 0x10) = 0;
    *(undefined1 *)(iVar5 + 0x10) = 0;
    *(undefined1 *)(iVar6 + 0x10) = 0;
    *(undefined1 *)(iVar7 + 0x10) = 0;
    return;
  }
  *(undefined1 *)(iVar10 + 0x10) = 1;
  *(undefined1 *)(iVar9 + 0x10) = 0;
  *(undefined1 *)(iVar2 + 0x10) = 1;
  *(undefined1 *)(iVar3 + 0x10) = 1;
  *(undefined1 *)(iVar4 + 0x10) = 1;
  *(undefined1 *)(iVar5 + 0x10) = 1;
  *(undefined1 *)(iVar6 + 0x10) = 1;
  *(undefined1 *)(iVar7 + 0x10) = 1;
  _Dest = (wchar_t *)heap_reallocate(0x18);
  *(wchar_t **)(iVar10 + 0x3c) = _Dest;
  if (_Dest != (wchar_t *)0x0) {
    if ((*(ushort *)(param_1 + 0x11c) & 1) == 0) {
      _wcsncpy(_Dest,(wchar_t *)(param_1 + 2),0xb);
    }
    else {
      iVar9 = tag_lookup("ui\\shell\\strings\\default_player_profile_names");
      if (iVar9 == -1) {
        _Source = &PTR_DAT_0066a750;
      }
      else {
        _Source = (undefined **)text_string_list_get_string();
      }
      _wcsncpy(*(wchar_t **)(iVar10 + 0x3c),(wchar_t *)_Source,0xb);
    }
    *(undefined2 *)(*(int *)(iVar10 + 0x3c) + 0x16) = 0;
  }
  sVar8 = *(short *)(param_1 + 0x11a);
  if (sVar8 < 0) {
    sVar8 = 0;
  }
  else if (0x11 < sVar8) {
    sVar8 = 0x11;
  }
  *(short *)(iVar1 + 0x58) = sVar8;
  if ((*(byte *)(param_1 + 0x11c) & 1) == 0) {
    FUN_00539e00();
    local_4 = (short)iVar3;
    iVar10 = local_4 + 1;
    if (9 < iVar10) {
      iVar10 = 9;
    }
    *(short *)(iVar3 + 0x40) = (short)iVar10;
    *(undefined2 *)(iVar5 + 0x40) = (undefined2)param_1;
    *(ushort *)(iVar7 + 0x40) = (ushort)(*(char *)(param_1 + 0x12f) == '\x01');
    return;
  }
  *(undefined1 *)(iVar3 + 0x10) = 0;
  *(undefined1 *)(iVar5 + 0x10) = 0;
  return;
}
#endif
