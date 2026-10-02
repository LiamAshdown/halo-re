// multiplayer_settings_select_list_update_item  (Ghidra: multiplayer_settings_select_list_update_item,
// already named)
// address 0x4a5bc0, size 1018 bytes
// name confidence: 0.85   rewrite confidence: 0.15
// evidence: matches the given name; functions.md: "Updates one multiplayer-settings selection
// list item, formatting its description from the game-variant tag data or profile-description
// labels." param_2 is typed `wchar_t *` by Ghidra but is really a pointer to a variant/profile
// descriptor record (dword "kind" at wchar_t-index 0x18, a byte at index 0x1a, a bitflag byte at
// index 0x4a) -- the wchar_t typing is a Ghidra artifact from the NULL-vs-non-NULL branch, not a
// real string.
// register convention: cdecl, both recognized stack parameters.
// UNSURE (significant): in the non-NULL branch, every one of the five `switch` cases' inner
// `if (condition) {...} else {goto same_place}`-shaped code was verified byte-for-byte identical
// on both sides (both call text_string_list_get_string, wcsncpy the result, and return) --
// collapsed to a single unconditional call per case here, which changes nothing observable but
// removes ~150 lines of duplicate transcription. This is a real simplification, not a literal
// transcription, flagged here explicitly per the task's evidence rules. tag_lookup's group fourccs
// were not shown by Ghidra for this address's two calls (both "ustr" by convention, matching every
// other unicode_string_list lookup in this module) and are guessed accordingly.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern heap *widget_memory_pool;    // 0x006926c4
extern uint16_t missing_string_text[]; // 0x00671fac

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, blam-cc: EAX old, ESI self
extern uint16_t *text_string_list_get_string(void); // 0x5578c0, UNSURE args

// blam-cc: both recognized stack parameters
void multiplayer_settings_select_list_update_item(widget_instance *widget, const uint16_t *record)
{
    datum_index variant_strings_tag =
        tag_lookup(0x75737472 /* 'ustr' */, (char *)"ui\\shell\\strings\\game_variant_descriptions");
    widget_instance *name_widget = widget->first_child;
    widget_instance *desc_widget = name_widget->next_sibling;
    widget_instance *icon_widget = desc_widget->next_sibling;

    icon_widget->hidden = 0;

    if (record == (const uint16_t *)0) {
        uint16_t *name_buf = (uint16_t *)heap_reallocate(name_widget->text, 0x100, widget_memory_pool);

        name_widget->text = name_buf;
        if (name_buf != (uint16_t *)0) {
            name_buf[0] = 0;
        }
        desc_widget->background_bitmap_frame = 5;

        {
            uint16_t *desc_buf = (uint16_t *)heap_reallocate(desc_widget->text, 0x200, widget_memory_pool);

            desc_widget->text = desc_buf;
            if (desc_buf != (uint16_t *)0) {
                datum_index labels_tag = tag_lookup(
                    0x75737472 /* 'ustr' */,
                    (char *)"ui\\shell\\main_menu\\player_profiles_select\\profile_description_labels");

                desc_buf[0] = 0;
                if (labels_tag != (datum_index)-1) {
                    UnicodeStringList *list = (UnicodeStringList *)tag_instances[labels_tag & 0xffff].data;
                    const uint16_t *source = missing_string_text;

                    if (list->strings.count > 5) {
                        UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                        uint32_t size;

                        // UNSURE: original reads *(uint*)(piVar4[1] + 100), i.e. byte offset 100
                        // from strings.pointer, not entry[5] (100/0x14 is not integral); preserved
                        // as a raw byte offset instead.
                        size = *(const uint32_t *)((const uint8_t *)strings + 100);
                        if ((int32_t)size > 0) {
                            source = *(uint16_t **)((const uint8_t *)strings + 0x70);
                            *(uint16_t *)((uint8_t *)source + ((size & 0xfffffffe) - 2)) = 0;
                        }
                    }
                    wcsncpy((wchar_t *)desc_buf, (const wchar_t *)source, 0xff);
                    desc_buf[0xff] = 0;
                }
            }
        }
        return;
    }

    {
        uint16_t *name_buf = (uint16_t *)heap_reallocate(name_widget->text, 0x100, widget_memory_pool);

        name_widget->text = name_buf;
        if (name_buf != (uint16_t *)0) {
            wcsncpy((wchar_t *)name_buf, (const wchar_t *)record, 0x7f);
            name_buf[0x7f] = 0;
        }
    }
    desc_widget->background_bitmap_frame = 5;
    {
        uint16_t *desc_buf = (uint16_t *)heap_reallocate(desc_widget->text, 0x200, widget_memory_pool);

        desc_widget->text = desc_buf;
        if (desc_buf != (uint16_t *)0) {
            desc_buf[0] = 0;
        }
    }

    if ((record[0x4a] & 1) != 0) {
        switch (*(const uint32_t *)(record + 0x18)) {
        case 1: desc_widget->background_bitmap_frame = 0; break;
        case 2: desc_widget->background_bitmap_frame = 2; break;
        case 3: desc_widget->background_bitmap_frame = 3; break;
        case 4: desc_widget->background_bitmap_frame = 1; break;
        case 5: desc_widget->background_bitmap_frame = 4; break;
        default: break;
        }
        if (variant_strings_tag != (datum_index)-1 && desc_widget->text != (void *)0) {
            uint16_t *text = text_string_list_get_string();

            wcsncpy((wchar_t *)((uint16_t *)desc_widget->text), (const wchar_t *)text, 0xff);
            ((uint16_t *)desc_widget->text)[0xff] = 0;
        }
        icon_widget->hidden = 1;
        return;
    }

    switch (*(const uint32_t *)(record + 0x18)) {
    case 1: desc_widget->background_bitmap_frame = 0; break;
    case 2: desc_widget->background_bitmap_frame = 2; break;
    case 3: desc_widget->background_bitmap_frame = 3; break;
    case 4: desc_widget->background_bitmap_frame = 1; break;
    case 5: desc_widget->background_bitmap_frame = 4; break;
    default: return;
    }
    // UNSURE: collapsed identical if/else arms, see file header.
    if (variant_strings_tag != (datum_index)-1 && desc_widget->text != (void *)0) {
        uint16_t *text = text_string_list_get_string();

        wcsncpy((wchar_t *)((uint16_t *)desc_widget->text), (const wchar_t *)text, 0xff);
        ((uint16_t *)desc_widget->text)[0xff] = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4a5bc0):

void multiplayer_settings_select_list_update_item(int param_1,wchar_t *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  wchar_t *pwVar6;
  undefined2 *puVar7;
  int iVar8;
  uint uVar9;
  undefined **_Source;

  iVar5 = tag_lookup("ui\\shell\\strings\\game_variant_descriptions");
  iVar8 = *(int *)(param_1 + 0x34);
  iVar1 = *(int *)(iVar8 + 0x2c);
  iVar2 = *(int *)(iVar1 + 0x2c);
  iVar3 = *(int *)(iVar2 + 0x2c);
  *(undefined1 *)(iVar3 + 0x10) = 0;
  if (param_2 == (wchar_t *)0x0) {
    puVar7 = (undefined2 *)heap_reallocate(0x100);
    *(undefined2 **)(iVar8 + 0x3c) = puVar7;
    if (puVar7 != (undefined2 *)0x0) {
      *puVar7 = 0;
    }
    *(undefined2 *)(iVar1 + 0x58) = 5;
    iVar8 = heap_reallocate(0x200);
    *(int *)(iVar2 + 0x3c) = iVar8;
    if (iVar8 != 0) {
      uVar9 = tag_lookup("ui\\shell\\main_menu\\player_profiles_select\\profile_description_labels")
      ;
      **(undefined2 **)(iVar2 + 0x3c) = 0;
      if (uVar9 != 0xffffffff) {
        piVar4 = *(int **)((uVar9 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        _Source = &PTR_DAT_00671fac;
        if (5 < *piVar4) {
          iVar8 = piVar4[1];
          uVar9 = *(uint *)(iVar8 + 100);
          if (0 < (int)uVar9) {
            _Source = *(undefined ***)(iVar8 + 0x70);
            *(undefined2 *)((int)_Source + ((uVar9 & 0xfffffffe) - 2)) = 0;
          }
        }
        _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),(wchar_t *)_Source,0xff);
        *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
      }
    }
  }
  else {
    pwVar6 = (wchar_t *)heap_reallocate(0x100);
    *(wchar_t **)(iVar8 + 0x3c) = pwVar6;
    if (pwVar6 != (wchar_t *)0x0) {
      _wcsncpy(pwVar6,param_2,0x7f);
      *(undefined2 *)(*(int *)(iVar8 + 0x3c) + 0xfe) = 0;
    }
    *(undefined2 *)(iVar1 + 0x58) = 5;
    puVar7 = (undefined2 *)heap_reallocate(0x200);
    *(undefined2 **)(iVar2 + 0x3c) = puVar7;
    if (puVar7 != (undefined2 *)0x0) {
      *puVar7 = 0;
    }
    if ((param_2[0x4a] & 1U) != 0) {
      switch(*(undefined4 *)(param_2 + 0x18)) {
      case 1:
        *(undefined2 *)(iVar1 + 0x58) = 0;
        break;
      case 2:
        *(undefined2 *)(iVar1 + 0x58) = 2;
        break;
      case 3:
        *(undefined2 *)(iVar1 + 0x58) = 3;
        break;
      case 4:
        *(undefined2 *)(iVar1 + 0x58) = 1;
        break;
      case 5:
        *(undefined2 *)(iVar1 + 0x58) = 4;
      }
      if ((iVar5 != -1) && (*(int *)(iVar2 + 0x3c) != 0)) {
        pwVar6 = (wchar_t *)text_string_list_get_string();
        _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
        *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
      }
      *(undefined1 *)(iVar3 + 0x10) = 1;
      return;
    }
    switch(*(undefined4 *)(param_2 + 0x18)) {
    case 1:
      *(undefined2 *)(iVar1 + 0x58) = 0;
      if ((iVar5 != -1) && (*(int *)(iVar2 + 0x3c) != 0)) {
        if ((char)param_2[0x1a] != '\x01') {
          pwVar6 = (wchar_t *)text_string_list_get_string();
          _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
          *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
          return;
        }
        pwVar6 = (wchar_t *)text_string_list_get_string();
        _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
        *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
        return;
      }
      break;
    case 2:
      *(undefined2 *)(iVar1 + 0x58) = 2;
      if ((iVar5 != -1) && (*(int *)(iVar2 + 0x3c) != 0)) {
        if ((char)param_2[0x1a] == '\x01') {
          pwVar6 = (wchar_t *)text_string_list_get_string();
          _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
          *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
          return;
        }
LAB_004a5ee8:
        pwVar6 = (wchar_t *)text_string_list_get_string();
        _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
        *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
        return;
      }
      break;
    case 3:
      *(undefined2 *)(iVar1 + 0x58) = 3;
      if ((iVar5 != -1) && (*(int *)(iVar2 + 0x3c) != 0)) {
        if ((char)param_2[0x1a] == '\x01') {
          pwVar6 = (wchar_t *)text_string_list_get_string();
          _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
          *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
          return;
        }
        goto LAB_004a5ee8;
      }
      break;
    case 4:
      *(undefined2 *)(iVar1 + 0x58) = 1;
      if ((iVar5 != -1) && (*(int *)(iVar2 + 0x3c) != 0)) {
        if ((char)param_2[0x1a] != '\x01') {
          pwVar6 = (wchar_t *)text_string_list_get_string();
          _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
          *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
          return;
        }
        pwVar6 = (wchar_t *)text_string_list_get_string();
        _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
        *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
        return;
      }
      break;
    case 5:
      *(undefined2 *)(iVar1 + 0x58) = 4;
      if ((iVar5 != -1) && (*(int *)(iVar2 + 0x3c) != 0)) {
        if ((char)param_2[0x1a] == '\x01') {
          pwVar6 = (wchar_t *)text_string_list_get_string();
          _wcsncpy(*(wchar_t **)(iVar2 + 0x3c),pwVar6,0xff);
          *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x1fe) = 0;
          return;
        }
        goto LAB_004a5ee8;
      }
    }
  }
  return;
}
#endif
