// player_profile_1wide_list_update  (Ghidra: player_profile_1wide_list_update, already named)
// address 0x4a6380, size 802 bytes
// name confidence: 0.85   rewrite confidence: 0.65
// evidence: matches the given name (cea-pdb hint via the "%s%hs%s" format string).
// Rewritten in the phase-4 review from objdump -d 0x4a6380..0x4a66a4. The list widget holds
// profile ids in list_items; the selected id is loaded into the profile carousel
// (ui_profile_carousel_slot_cache_populate @0x4a74b0 with a count of 1 and EBX pointing at the
// id) and looked up in the three profile_carousel_slot records at 0x00873d60. The profile name
// goes into a heap block kept at widget +0x50 (list_render_data of the list widget itself, not
// the name row text), the player color into the name row frame, and a
// "%s%hs%s" joystick / button description into the description row text.
// When the id is not in the carousel the id list is sorted (qsort with
// ui_carousel_slot_compare_valid_first @0x4a7630), cut at the first -1, the selection clamped
// into it, and the whole routine starts over (jmp 0x4a6390); the earlier rewrite dropped that
// loop, wrote the name into the wrong widget, lost the string list indices and read the slot
// table with a dword stride.
// UNSURE: the loop does not terminate when the selected id can never be loaded; kept.
// register convention: cdecl, the one stack parameter (widget).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_memory.h"
#include <wchar.h>

extern profile_carousel_slot profile_carousel_slots[3]; // 0x00873d60
extern char joystick_set_separator_0065f010[];           // 0x0065f010, CR LF (8-bit, for %hs)
extern uint16_t empty_string[];            // 0x00660c34
extern heap *widget_memory_pool;                         // 0x006926c4

extern void ui_profile_carousel_slot_cache_populate(int32_t count, const int32_t *candidate_ids); // 0x4a74b0, blam-cc: EBX candidate_ids
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group

extern uint16_t *text_string_list_get_string(datum_index tag, int16_t index); // 0x5578c0, blam-cc: ECX tag, DX index
extern wchar_t *string_format_wide_va_bounded(wchar_t *dest, const wchar_t *format, ...); // 0x557910, blam-cc: EDX max chars
extern int32_t ui_carousel_slot_compare_valid_first(const void *a, const void *b); // 0x4a7630

void player_profile_1wide_list_update(widget_instance *widget)
{
    for (;;) {
        widget_instance *name_row = widget->parent->first_child;
        widget_instance *description_row = name_row->next_sibling;
        int32_t *ids = (int32_t *)widget->list_items;
        int32_t profile_id = ids[widget->selection_index];

        ui_profile_carousel_slot_cache_populate(1, &profile_id);

        if (profile_id != -1) {
            int32_t slot;
            for (slot = 0; slot < 3 && profile_carousel_slots[slot].profile_id != profile_id; slot++) {
            }
            if (slot < 3) {
                const uint8_t *profile = profile_carousel_slots[slot].profile;
                uint16_t flags = *(const uint16_t *)(profile + 0x11c);
                int16_t color = *(const int16_t *)(profile + 0x11a);
                uint16_t *name = (uint16_t *)heap_reallocate(widget->list_render_data, 0x18, widget_memory_pool);

                widget->list_render_data = name;
                if (name == 0) {
                    return;
                }
                if (flags & 1) {
                    datum_index names = tag_lookup(0x75737472, "ui\\shell\\strings\\default_player_profile_names");
                    const uint16_t *source = empty_string;
                    if (names != (datum_index)-1) {
                        source = text_string_list_get_string(names, (int16_t)(flags >> 8));
                    }
                    wcsncpy(name, source, 0xb);
                } else {
                    wcsncpy(name, (const uint16_t *)(profile + 2), 0xb);
                }
                name[0xb] = 0;

                name_row->background_bitmap_frame = (color < 0) ? 0 : (color > 0x11) ? 0x11 : color;

                description_row->text = heap_reallocate(description_row->text, 0x200, widget_memory_pool);
                if (description_row->text == 0) {
                    return;
                }
                {
                    datum_index joysticks;
                    datum_index buttons;
                    if (*(const uint8_t *)(profile + 0x11c) & 1) {
                        joysticks = tag_lookup(0x75737472, "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_defaults_descriptions");
                        buttons = tag_lookup(0x75737472, "ui\\shell\\main_menu\\player_profiles_select\\button_set_long_descriptions");
                        if (joysticks == (datum_index)-1 || buttons == (datum_index)-1) {
                            ((uint16_t *)description_row->text)[0] = 0;
                            ((uint16_t *)description_row->text)[0xff] = 0;
                            return;
                        }
                    } else {
                        joysticks = tag_lookup(0x75737472, "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_short_descriptions");
                        buttons = tag_lookup(0x75737472, "ui\\shell\\main_menu\\player_profiles_select\\button_set_short_descriptions");
                        if (joysticks == (datum_index)-1 || buttons == (datum_index)-1) {
                            ((uint16_t *)description_row->text)[0xff] = 0;
                            return;
                        }
                    }
                    {
                        uint16_t *joystick_text = text_string_list_get_string(joysticks, *(const uint8_t *)(profile + 0x12d));
                        uint16_t *button_text = text_string_list_get_string(buttons, *(const uint8_t *)(profile + 0x12c));
                        string_format_wide_va_bounded((wchar_t *)description_row->text, L"%s%hs%s", // EDX 0xff
                                                      joystick_text, joystick_set_separator_0065f010, button_text);
                    }
                    ((uint16_t *)description_row->text)[0xff] = 0;
                }
                return;
            }
        }

        if (widget->item_count == 0) {
            uint16_t *text = (uint16_t *)heap_reallocate(widget->list_render_data, 4, widget_memory_pool);
            widget->list_render_data = text;
            if (text != 0) {
                text[0] = 0;
            }
            name_row->background_bitmap_frame = 0;
            text = (uint16_t *)heap_reallocate(description_row->text, 4, widget_memory_pool);
            description_row->text = text;
            if (text != 0) {
                text[0] = 0;
            }
            return;
        }

        {
            int32_t count = widget->item_count;
            int32_t valid = 0;
            qsort(ids, (uint32_t)count, 4, ui_carousel_slot_compare_valid_first);
            while (valid < count && ids[valid] != -1) {
                valid++;
            }
            widget->item_count = (uint16_t)valid;
            if (widget->selection_index < 0) {
                widget->selection_index = 0;
            } else if (widget->selection_index > (int32_t)(uint16_t)valid - 1) {
                widget->selection_index = (int16_t)((uint16_t)valid - 1);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a6380):

void player_profile_1wide_list_update(int param_1)

{
  int iVar1;
  void *_Base;
  short sVar2;
  int *piVar3;
  uint uVar4;
  int iVar5;
  wchar_t *pwVar6;
  int iVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined2 *puVar10;
  int iVar11;
  uint _NumOfElements;

  do {
    iVar5 = *(int *)(*(int *)(param_1 + 0x30) + 0x34);
    iVar1 = *(int *)(iVar5 + 0x2c);
    iVar7 = *(int *)(*(int *)(param_1 + 0x44) + *(short *)(param_1 + 0x40) * 4);
    FUN_004a74b0(1);
    if (iVar7 != -1) {
      iVar11 = 0;
      piVar3 = &DAT_00873d60;
      do {
        if (*piVar3 == iVar7) {
          iVar11 = iVar11 * 0x2000;
          if (iVar11 != -0x873d64) {
            pwVar6 = (wchar_t *)heap_reallocate(0x18);
            *(wchar_t **)(param_1 + 0x50) = pwVar6;
            if (pwVar6 == (wchar_t *)0x0) {
              return;
            }
            if ((*(ushort *)(iVar11 + 0x873e80) & 1) == 0) {
              _wcsncpy(pwVar6,(wchar_t *)((int)&DAT_00873d64 + iVar11 + 2),0xb);
              *(undefined2 *)(*(int *)(param_1 + 0x50) + 0x16) = 0;
            }
            else {
              iVar7 = tag_lookup("ui\\shell\\strings\\default_player_profile_names");
              pwVar6 = L"";
              if (iVar7 != -1) {
                pwVar6 = (wchar_t *)text_string_list_get_string();
              }
              _wcsncpy(*(wchar_t **)(param_1 + 0x50),pwVar6,0xb);
              *(undefined2 *)(*(int *)(param_1 + 0x50) + 0x16) = 0;
            }
            sVar2 = *(short *)(iVar11 + 0x873e7e);
            if (sVar2 < 0) {
              sVar2 = 0;
            }
            else if (0x11 < sVar2) {
              sVar2 = 0x11;
            }
            *(short *)(iVar5 + 0x58) = sVar2;
            iVar5 = heap_reallocate(0x200);
            *(int *)(iVar1 + 0x3c) = iVar5;
            if (iVar5 == 0) {
              return;
            }
            if ((*(byte *)(iVar11 + 0x873e80) & 1) == 0) {
              iVar5 = tag_lookup(
                                "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_short_descriptions"
                                );
              iVar7 = tag_lookup(
                                "ui\\shell\\main_menu\\player_profiles_select\\button_set_short_descriptions"
                                );
              if ((iVar5 != -1) && (iVar7 != -1)) {
                uVar8 = text_string_list_get_string();
                uVar9 = text_string_list_get_string();
                string_format_wide_va_bounded
                          (*(undefined4 *)(iVar1 + 0x3c),L"%s%hs%s",uVar8,&DAT_0065f010,uVar9);
              }
            }
            else {
              iVar5 = tag_lookup(
                                "ui\\shell\\main_menu\\player_profiles_select\\joystick_set_defaults_descriptions"
                                );
              iVar7 = tag_lookup(
                                "ui\\shell\\main_menu\\player_profiles_select\\button_set_long_descriptions"
                                );
              if ((iVar5 == -1) || (iVar7 == -1)) {
                **(undefined2 **)(iVar1 + 0x3c) = 0;
                *(undefined2 *)(*(int *)(iVar1 + 0x3c) + 0x1fe) = 0;
                return;
              }
              uVar8 = text_string_list_get_string();
              uVar9 = text_string_list_get_string();
              string_format_wide_va_bounded
                        (*(undefined4 *)(iVar1 + 0x3c),L"%s%hs%s",uVar8,&DAT_0065f010,uVar9);
              *(undefined2 *)(*(int *)(iVar1 + 0x3c) + 0x1fe) = 0;
            }
            *(undefined2 *)(*(int *)(iVar1 + 0x3c) + 0x1fe) = 0;
            return;
          }
          break;
        }
        piVar3 = piVar3 + 0x800;
        iVar11 = iVar11 + 1;
      } while ((int)piVar3 < 0x879d60);
    }
    if (*(ushort *)(param_1 + 0x48) == 0) {
      puVar10 = (undefined2 *)heap_reallocate(4);
      *(undefined2 **)(param_1 + 0x50) = puVar10;
      if (puVar10 != (undefined2 *)0x0) {
        *puVar10 = 0;
      }
      *(undefined2 *)(iVar5 + 0x58) = 0;
      puVar10 = (undefined2 *)heap_reallocate(4);
      *(undefined2 **)(iVar1 + 0x3c) = puVar10;
      if (puVar10 != (undefined2 *)0x0) {
        *puVar10 = 0;
      }
      return;
    }
    _Base = *(void **)(param_1 + 0x44);
    _NumOfElements = (uint)*(ushort *)(param_1 + 0x48);
    _qsort(_Base,_NumOfElements,4,FUN_004a7630);
    uVar4 = 0;
    if (_NumOfElements != 0) {
      do {
        if (*(int *)((int)_Base + uVar4 * 4) == -1) break;
        uVar4 = uVar4 + 1;
      } while ((int)uVar4 < (int)_NumOfElements);
    }
    sVar2 = *(short *)(param_1 + 0x40);
    *(short *)(param_1 + 0x48) = (short)uVar4;
    if (sVar2 < 0) {
      *(undefined2 *)(param_1 + 0x40) = 0;
    }
    else {
      iVar5 = (uVar4 & 0xffff) - 1;
      if (sVar2 <= iVar5) {
        iVar5 = (int)sVar2;
      }
      *(short *)(param_1 + 0x40) = (short)iVar5;
    }
  } while( true );
}
#endif
