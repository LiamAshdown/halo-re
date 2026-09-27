// ui_build_level_select_list  (Ghidra: ui_build_level_select_list, already named)
// address 0x49c8f0, size 892 bytes, callers=0 in this build (dead code, kept per task rules --
// not listed as misattributed by out/phase4/interface_types_notes.md)
// name confidence: 0.75   rewrite confidence: 0.6 (2026-09-27 static loop: the two register-argument calls
//   and the unlock test were fixed from objdump 0x49c93a..0x49ca69; the rest is not re-verified)
// evidence: matches the given name; functions.md: "Populates the level-selection widget's list
// with up to ten playable campaign levels (localized name, level index, current-selection flag)
// based on the player's unlock progress." ui_build_level_select_list_coop (0x49cc80, this session's range) is the
// near-duplicate co-op variant of the same body.
// register convention: cdecl, all three parameters recognized by Ghidra (widget, and two more
// whose use was not resolved in this pass -- forwarded unchanged to ui_build_level_select_list_coop).
// UNSURE / TYPES-GAP: this function's stack layout could not be recovered from the batch pack
// alone (no objdump was read for it, given callers=0 and the size of the remaining range). Ghidra
// declares a 284-byte `local_2008` array immediately followed by a 7902-byte `acStack_1eea` with
// no writes to the latter ever shown -- the only sensible reading is that both are one contiguous
// on-stack copy of the profile record at profile_globals_block (0x00712dd8), and `acStack_1eea`
// is simply the part of that single memcpy Ghidra failed to fold into the declared array size.
// Modeled here as one `uint8_t profile_copy[0x2000]` buffer with `acStack_1eea[i]` recovered as
// `profile_copy[0x11e + i]` (0x11c == sizeof the originally-declared local_2008). The per-level
// path table at 0x00692acc, the per-level path/flag record arrays at 0x00719018/0x0071901c, and
// the packed byte trio at 0x0071916a are all new to this module (not in types/interface.h) and
// are declared as local TYPES-GAP records rather than added to the header, since this function is
// unreachable in retail and the layout evidence here is weak (single call site, never taken).
// game_state_read_checkpoint_summary and player_profile_scan_campaign_progress are foreign (profile/saved-games module) with unresolved
// signatures; modeled as a bool-returning getter and a void refresh call respectively, matching
// how their results are (or are not) used here.
// reconciled: R55 per-level progress byte is profile +0x11e, not +0x11c (+0x11c is the flags word): 0x49c8f0 copies the profile to esp+0x20 and reads [esp+edi+0x13e] (0x49ca5e)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include <string.h>

// TYPES-GAP: one entry of the campaign level path table at 0x00692acc (stride 8, second dword
// always read as part of the (&table)[i*2] indexing but never itself examined here).
// TYPES-GAP: one entry of the per-level flag table at 0x0071901c (stride 8).
extern int16_t local_player_count;                        // 0x006894b8, TYPES-GAP
extern char level_select_current_path_00719068[0x106];            // 0x00719068, TYPES-GAP
extern level_select_entry level_select_entries[10];  // 0x00719018
extern int32_t current_profile_index;                              // 0x00714dd4
extern int32_t level_select_cached_profile_index_00692af8;         // 0x00692af8, TYPES-GAP
extern uint8_t level_select_flags_0071916a;                        // 0x0071916a, TYPES-GAP
extern uint8_t level_select_flags_0071916b;                        // 0x0071916b, TYPES-GAP
extern uint8_t level_select_flags_0071916c;                        // 0x0071916c, TYPES-GAP
extern uint8_t profile_globals_block[0x60a4];                       // 0x00712dd8
extern int16_t known_solo_level_index_00712f00;                     // 0x00712f00, TYPES-GAP
extern growable_array ui_lists[3];                                  // 0x006b3830
extern int32_t ui_list_current;                                     // 0x00692c04
extern uint8_t ui_list_has_default;                                 // 0x007192f8
extern campaign_level_entry known_campaign_levels_00692acc[10];  // 0x00692acc, TYPES-GAP
extern uint16_t default_profile_name_suffix_00671fac[];             // 0x00671fac
extern tag_instance *tag_instances;                                 // 0x0087bc14
extern int16_t level_select_frame_00719168;                         // 0x00719168, TYPES-GAP
extern int32_t last_level_widget_selection_00692afc;                 // 0x00692afc, TYPES-GAP
extern int16_t quit_confirm_error_string_index;                          // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae;                        // 0x00718fae
extern uint8_t quit_confirm_error_modal;                          // 0x00718fb0
extern uint8_t quit_confirm_error_is_error;                       // 0x00718fb1

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern void ui_build_level_select_list_coop(widget_instance *widget, void *param_2, void *param_3); // 0x49cc80
extern int32_t growable_array_add_element(growable_array *array); // 0x4cf810
extern uint8_t game_state_read_checkpoint_summary(uint8_t *corrupt_flag, int16_t *out_difficulty,
    char *out_scenario_name); // 0x538320, EAX corrupt_flag, ESI out_difficulty, EDI out_scenario_name
extern void player_profile_scan_campaign_progress(int16_t *out_type, void *profile,
    int16_t *out_level); // 0x539e00, ECX out_type, EDX profile, ESI out_level
extern uint32_t wcslen(const uint16_t *s); // 0x625b7a, wide strlen
extern void _wcscpy(uint16_t *dest, const uint16_t *src);
extern int32_t __stricmp(const char *a, const char *b);
extern void *GlobalAlloc(uint32_t flags, uint32_t size);

// Builds the campaign level-selection list. If more than one known level exists, delegates
// entirely to the co-op variant ui_build_level_select_list_coop. Otherwise rebuilds the 10-slot known-level table
// from the current profile's per-level flags and adds each as a growable-array list entry (name
// looked up from the map_list_oneline string list, falling back to a default), marking whichever
// slot matches the profile's currently-selected level.
uint32_t ui_build_level_select_list(widget_instance *widget, void *param_2, void *param_3)
{
    datum_index string_list_tag;
    uint8_t profile_copy[0x2000]; // see file header: covers Ghidra's local_2008 + acStack_1eea
    int16_t scan_type = 0;        // [esp+0x10]
    int16_t scan_level = 0;       // [esp+0x14]
    int32_t i;

    if (local_player_count > 1) {
        memset(level_select_current_path_00719068, 0, sizeof(level_select_current_path_00719068));
        ui_build_level_select_list_coop(widget, param_2, param_3);
        return 1;
    }

    string_list_tag = tag_lookup(0x75737472 /* 'ustr' */, "ui\\shell\\main_menu\\map_list_oneline");
    memset(level_select_entries, 0, sizeof(level_select_entries));

    if (current_profile_index != level_select_cached_profile_index_00692af8) {
        memset(level_select_current_path_00719068, 0, sizeof(level_select_current_path_00719068));
        // 0x49c97d..0x49c991: EAX = &0x0071916c, ESI = &0x00719168, EDI = the path buffer 0x00719068.
        level_select_flags_0071916b = game_state_read_checkpoint_summary(&level_select_flags_0071916c,
            &level_select_frame_00719168, level_select_current_path_00719068);
        level_select_cached_profile_index_00692af8 = current_profile_index;
    }

    memcpy(profile_copy, profile_globals_block, sizeof(profile_copy) < sizeof(profile_globals_block)
                                                     ? sizeof(profile_copy)
                                                     : sizeof(profile_globals_block));
    // 0x49c9ac..0x49c9b8: ECX = &out_type, EDX = the profile copy, ESI = &out_level (the loop's "last level + 1"
    // unlock test at 0x49ca69 reads out_level, not 0x00712f00).
    player_profile_scan_campaign_progress(&scan_type, profile_copy, &scan_level);

    ui_lists[0].element_size = 0x10;
    ui_lists[1].element_size = 0x10;
    ui_lists[2].element_size = 0x10;
    ui_lists[0].count = 0;
    ui_lists[1].count = 0;
    ui_lists[2].count = 0;
    ui_lists[0].data = (void *)0;
    ui_lists[1].data = (void *)0;
    ui_lists[2].data = (void *)0;
    ui_list_current = -1;
    ui_list_has_default = 0;

    if (known_solo_level_index_00712f00 < 0) {
        widget->selection_index = 0;
        i = 0;
    } else if (known_solo_level_index_00712f00 < 10) {
        widget->selection_index = (int16_t)known_solo_level_index_00712f00;
        i = 0;
    } else {
        widget->selection_index = 9;
        i = 0;
    }

    do {
        uint16_t *entry_name;
        uint8_t is_selected;
        int32_t element_index;

        level_select_entries[i].path = known_campaign_levels_00692acc[i].path;

        if (profile_copy[0x11e + i] != 0 || i == scan_level + 1 || i == 0) {
            uint32_t flags = (uint32_t)(uint8_t)profile_copy[0x11e + i];

            level_select_entries[i].flag_bit1 = (uint8_t)((flags >> 1) & 1);
            level_select_entries[i].valid = 1;
            level_select_entries[i].flag_bit2 = (uint8_t)((flags >> 2) & 1);
            level_select_entries[i].flag_bit3 = (uint8_t)((flags >> 3) & 1);
        }

        entry_name = default_profile_name_suffix_00671fac;
        if (string_list_tag != (datum_index)-1) {
            UnicodeStringList *list = (UnicodeStringList *)tag_instances[string_list_tag & 0xffff].data;

            if (i >= 0 && i < (int32_t)list->strings.count) {
                UnicodeStringListString *strings = (UnicodeStringListString *)list->strings.pointer;
                uint32_t size = strings[i].string.size;

                if ((int32_t)size > 0) {
                    entry_name = (uint16_t *)strings[i].string.pointer;
                    *(uint16_t *)((uint8_t *)entry_name + ((size & 0xfffffffe) - 2)) = 0;
                }
            }
        }

        is_selected = (i == widget->selection_index);
        element_index = growable_array_add_element(&ui_lists[1]); // DAT_006b3838-backed array
        if (element_index != -1) {
            ui_list_item *item = (ui_list_item *)ui_lists[1].data + element_index;
            uint32_t name_length = wcslen(entry_name);

            item->data = (void *)0;
            item->name = (uint16_t *)GlobalAlloc(0, name_length * 2 + 2);
            item->id = i;
            item->is_default = is_selected;
            if (is_selected) {
                ui_list_has_default = 1;
            }
            _wcscpy(item->name, entry_name);
        }
        i = i + 1;
    } while (i < 10);

    // UNSURE: offsets 0x3c/0x3e fall inside widget_instance::text for this widget type; preserved
    // as raw byte offsets rather than asserting a (likely wrong) field name here.
    *(int16_t *)((uint8_t *)widget + 0x3c) = widget->selection_index;
    widget->list_items = level_select_entries;
    widget->item_count = 10;
    *(int16_t *)((uint8_t *)widget + 0x3e) = -1;

    if (level_select_flags_0071916b == 1) {
        level_select_frame_00719168 = 0; // UNSURE: shares storage with DAT_00719167 per Ghidra
        for (i = 0; i < 10; i++) {
            int32_t cmp = __stricmp(level_select_current_path_00719068, known_campaign_levels_00692acc[i].path);
            int16_t saved_frame = level_select_frame_00719168;

            if (cmp == 0) {
                level_select_flags_0071916a = (uint8_t)i;
                if (level_select_frame_00719168 < 0) {
                    level_select_frame_00719168 = 0;
                } else {
                    level_select_frame_00719168 = 3;
                    if (saved_frame < 4) {
                        level_select_frame_00719168 = saved_frame;
                    }
                }
                break;
            }
            level_select_frame_00719168 = saved_frame;
        }
        if (i == 10) {
            level_select_flags_0071916b = 0;
            return 1;
        }
    } else if (level_select_flags_0071916c == 1 && current_profile_index != -1) {
        if (last_level_widget_selection_00692afc == -1) {
            if (quit_confirm_error_string_index == -1) {
                quit_confirm_error_string_index = 0x27;
                quit_confirm_error_unknown_ae = 0xffff;
                quit_confirm_error_modal = 1;
                quit_confirm_error_is_error = 0;
            }
            last_level_widget_selection_00692afc = current_profile_index;
            return 1;
        }
        last_level_widget_selection_00692afc = -1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x49c8f0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 ui_build_level_select_list(int param_1,undefined4 param_2,undefined4 param_3)

{
  uint *puVar1;
  int *piVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  wchar_t *_Dest;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  short sVar9;
  undefined4 *puVar10;
  bool bVar11;
  undefined **local_2018;
  short local_2014;
  undefined4 local_2008 [71];
  char acStack_1eea [7902];
  undefined4 uStack_c;

  uStack_c = 0x49c900;
  if (1 < DAT_006894b8) {
    puVar8 = &DAT_00719068;
    for (iVar6 = 0x41; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
    *(undefined2 *)puVar8 = 0;
    FUN_0049cc80(param_1,param_2,param_3);
    return 1;
  }
  uVar3 = tag_lookup("ui\\shell\\main_menu\\map_list_oneline");
  iVar6 = DAT_00714dd4;
  puVar8 = &DAT_00719018;
  for (iVar7 = 0x14; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  if (iVar6 != DAT_00692af8) {
    puVar8 = &DAT_00719068;
    for (iVar7 = 0x41; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar8 = 0;
      puVar8 = puVar8 + 1;
    }
    *(undefined2 *)puVar8 = 0;
    DAT_0071916a._1_1_ = FUN_00538320();
    DAT_00692af8 = iVar6;
  }
  puVar8 = &DAT_00712dd8;
  puVar10 = local_2008;
  for (iVar6 = 0x7ff; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar10 = *puVar8;
    puVar8 = puVar8 + 1;
    puVar10 = puVar10 + 1;
  }
  FUN_00539e00();
  DAT_006b3830 = 0x10;
  DAT_006b383c = 0x10;
  _DAT_006b3848 = 0x10;
  DAT_006b3834 = 0;
  DAT_006b3838 = 0;
  DAT_006b3840 = 0;
  DAT_006b3844 = 0;
  _DAT_006b384c = 0;
  _DAT_006b3850 = 0;
  DAT_00692c04 = 0xffffffff;
  DAT_007192f8 = 0;
  if (DAT_00712f00 < 0) {
    *(undefined2 *)(param_1 + 0x40) = 0;
    iVar6 = 0;
  }
  else if (DAT_00712f00 < 10) {
    *(short *)(param_1 + 0x40) = DAT_00712f00;
    iVar6 = 0;
  }
  else {
    *(undefined2 *)(param_1 + 0x40) = 9;
    iVar6 = 0;
  }
  do {
    (&DAT_00719018)[iVar6 * 2] = (&PTR_s_levels_a10_a10_00692acc)[iVar6];
    if (((acStack_1eea[iVar6] != '\0') || (iVar6 == local_2014 + 1)) || (iVar6 == 0)) {
      uVar4 = (uint)acStack_1eea[iVar6];
      *(byte *)((int)&DAT_0071901c + iVar6 * 8 + 1) = (byte)(uVar4 >> 1) & 1;
      *(undefined1 *)(&DAT_0071901c + iVar6 * 2) = 1;
      *(byte *)((int)&DAT_0071901c + iVar6 * 8 + 2) = (byte)(uVar4 >> 2) & 1;
      *(byte *)((int)&DAT_0071901c + iVar6 * 8 + 3) = (byte)(uVar4 >> 3) & 1;
    }
    local_2018 = &PTR_DAT_00671fac;
    if (uVar3 != 0xffffffff) {
      sVar9 = (short)iVar6;
      piVar2 = *(int **)((uVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if ((-1 < sVar9) && ((int)sVar9 < *piVar2)) {
        puVar1 = (uint *)(piVar2[1] + sVar9 * 0x14);
        uVar4 = *puVar1;
        if (0 < (int)uVar4) {
          local_2018 = (undefined **)puVar1[3];
          *(undefined2 *)((int)local_2018 + ((uVar4 & 0xfffffffe) - 2)) = 0;
        }
      }
    }
    bVar11 = iVar6 == *(short *)(param_1 + 0x40);
    iVar7 = growable_array_add_element();
    if (iVar7 != -1) {
      iVar5 = FUN_00625b7a(local_2018);
      puVar8 = (undefined4 *)(iVar7 * 0x10 + DAT_006b3838);
      puVar8[1] = 0;
      _Dest = GlobalAlloc(0,iVar5 * 2 + 2);
      *puVar8 = _Dest;
      puVar8[2] = iVar6;
      *(bool *)(puVar8 + 3) = bVar11;
      if (bVar11) {
        DAT_007192f8 = 1;
      }
      FUN_00625b7a(local_2018);
      _wcscpy(_Dest,(wchar_t *)local_2018);
    }
    iVar6 = iVar6 + 1;
  } while (iVar6 < 10);
  *(undefined2 *)(param_1 + 0x3c) = *(undefined2 *)(param_1 + 0x40);
  *(undefined4 **)(param_1 + 0x44) = &DAT_00719018;
  *(undefined2 *)(param_1 + 0x48) = 10;
  *(undefined2 *)(param_1 + 0x3e) = 0xffff;
  if (DAT_0071916a._1_1_ == '\x01') {
    DAT_00719167 = 0;
    iVar6 = 0;
    do {
      iVar7 = __stricmp((char *)&DAT_00719068,(&PTR_s_levels_a10_a10_00692acc)[iVar6]);
      sVar9 = DAT_00719168;
      if (iVar7 == 0) {
        DAT_0071916a._0_1_ = (undefined1)iVar6;
        if (DAT_00719168 < 0) {
          DAT_00719168 = 0;
        }
        else {
          DAT_00719168 = 3;
          if (sVar9 < 4) {
            DAT_00719168 = sVar9;
          }
        }
        break;
      }
      iVar6 = iVar6 + 1;
      DAT_00719168 = sVar9;
    } while (iVar6 < 10);
    if (iVar6 == 10) {
      DAT_0071916a._1_1_ = 0;
      return 1;
    }
  }
  else if ((DAT_0071916a._2_1_ == '\x01') && (DAT_00714dd4 != -1)) {
    if (DAT_00692afc == -1) {
      if (DAT_00718fac == -1) {
        DAT_00718fac = 0x27;
        DAT_00718fae = 0xffff;
        DAT_00718fb0 = 1;
        DAT_00718fb1 = 0;
      }
      DAT_00692afc = DAT_00714dd4;
      return 1;
    }
    DAT_00692afc = -1;
  }
  return 1;
}
#endif
