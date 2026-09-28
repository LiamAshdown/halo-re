// player_profile_details_widget_refresh  (Ghidra: player_profile_details_widget_refresh, already
// named)
// address 0x4a6100, size 434 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: matches the given name; functions.md: "Refreshes the player-profile detail widgets
// (name, sensitivity, controller type, invert flag) for a selected profile, or blanks them if
// none is selected."
// REWRITTEN 2026-09-27 (static loop) from objdump 0x4a6100..0x4a62b1. Widgets: A = widget->first_child,
//   B = A->next, C = B->next, D = C->first_child, then E..J = D's next-sibling chain (the draft missed J, the
//   ninth). With a profile: A, E..J shown and D hidden; A's text = the profile name (0xb chars, from the
//   default-name string list when +0x11c bit 0 marks a default profile, index = +0x11c >> 8); B's bitmap frame =
//   look sensitivity (+0x11a clamped 0..17). A default profile hides F and H; otherwise
//   player_profile_scan_campaign_progress(ECX = &type (the argument slot is reused), EDX = profile, ESI = &level)
//   and F.selection = min(level + 1, 9), H.selection = type, J.selection = (+0x12f == 1). The draft passed no
//   arguments to the scan and fed a widget pointer through as the level. Without a profile: A, E..J hidden,
//   D shown, B's frame 0x12.
// blam-cc: EAX -> widget, stack -> profile_record

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern uint16_t hud_text_unknown[]; // 0x0066a750

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern heap *widget_memory_pool; // 0x006926c4
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern void player_profile_scan_campaign_progress(int16_t *out_type, void *profile,
    int16_t *out_level); // 0x539e00, ECX out_type, EDX profile, ESI out_level
extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index); // 0x5578c0, ECX list, EDX index

// blam-cc: EAX -> widget, stack -> profile_record
void player_profile_details_widget_refresh(widget_instance *widget, const uint8_t *profile_record)
{
    widget_instance *a = widget->first_child;
    widget_instance *b = a->next_sibling;
    widget_instance *d = b->next_sibling->first_child;
    widget_instance *e = d->next_sibling;
    widget_instance *f = e->next_sibling;
    widget_instance *g = f->next_sibling;
    widget_instance *h = g->next_sibling;
    widget_instance *i = h->next_sibling;
    widget_instance *j = i->next_sibling;
    int16_t sensitivity;

    if (profile_record == (const uint8_t *)0) {
        a->state = 0;
        b->background_bitmap_frame = 0x12;
        d->state = 1;
        e->state = 0;
        f->state = 0;
        g->state = 0;
        h->state = 0;
        i->state = 0;
        j->state = 0;
        return;
    }

    a->state = 1;
    d->state = 0;
    e->state = 1;
    f->state = 1;
    g->state = 1;
    h->state = 1;
    i->state = 1;
    j->state = 1;

    a->text = heap_reallocate(a->text, 0x18, widget_memory_pool);
    if (a->text != 0) {
        uint16_t flags = *(const uint16_t *)(profile_record + 0x11c);

        if ((flags & 1) != 0) {
            datum_index names_tag =
                tag_lookup(0x75737472 /* 'ustr' */, "ui\\shell\\strings\\default_player_profile_names");
            const uint16_t *source = names_tag != (datum_index)-1
                ? text_string_list_get_string(names_tag, (int16_t)(flags >> 8))
                : hud_text_unknown;

            wcsncpy((uint16_t *)a->text, source, 0xb);
        } else {
            wcsncpy((uint16_t *)a->text, (const uint16_t *)(profile_record + 2), 0xb);
        }
        ((uint16_t *)a->text)[0xb] = 0;
    }

    sensitivity = *(const int16_t *)(profile_record + 0x11a);
    if (sensitivity < 0) {
        sensitivity = 0;
    } else if (sensitivity > 0x11) {
        sensitivity = 0x11;
    }
    b->background_bitmap_frame = sensitivity;

    if ((profile_record[0x11c] & 1) != 0) {
        f->state = 0;
        h->state = 0;
        return;
    }

    {
        int16_t type = 0;
        int16_t level = 0;
        int32_t next_level;

        player_profile_scan_campaign_progress(&type, (void *)profile_record, &level);
        next_level = level + 1;
        if (next_level > 9) {
            next_level = 9;
        }
        f->selection_index = (int16_t)next_level;
        h->selection_index = type;
        j->selection_index = profile_record[0x12f] == 1;
    }
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
