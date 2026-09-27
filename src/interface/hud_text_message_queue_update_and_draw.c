// hud_text_message_queue_update_and_draw  (Ghidra: hud_text_message_queue_update_and_draw,
// already named)
// address 0x4a3e30, size 716 bytes, callers=0 in this build
// name confidence: 0.5   rewrite confidence: 0.15
// evidence: matches the given name; functions.md: "Updates message timers/removes expired
// entries, pulls in new messages, and draws the remaining HUD text message queue." Reuses
// hud_text_message_queue/hud_text_message_time_base (this session) and
// widget_instance_close_and_restore_previous (0x49c3e0, this session).
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE (significant): the elapsed-time value fed to __ftol (`iVar6`) is computed from a float
// expression Ghidra's decompile drops entirely (only the truncated integer result survives);
// modeled as "current time in ms minus hud_text_message_time_base", the only reading consistent
// with hud_text_message_time_base being overwritten with the new absolute time immediately after.
// The string-list lookup at tag+0xf8 is read as UIWidgetDefinition::text_label_unicode_strings_list
// per the struct's own field order; its double-dereference (`**(int**)...`) is preserved as
// "that tag's own strings.count". growable_array_remove_element's signature is a best guess (no
// established precedent found elsewhere in the module this session).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <wchar.h>
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern int32_t hud_text_message_time_base; // 0x0071922c
extern growable_array hud_text_message_queue; // 0x006b37e8
extern uint8_t hud_text_message_cycle_state_00719230; // 0x00719230, TYPES-GAP
extern ColorARGB *hud_text_message_hold_color;   // 0x006851f4, a POINTER (0x4a4056 loads then derefs it)
extern ColorARGB *hud_text_message_normal_color; // 0x00685200, a POINTER (0x4a404a)
extern int32_t hud_text_draw_font_006e472c;         // 0x006e472c, TYPES-GAP
extern ColorARGB hud_text_draw_color_006e4738;       // 0x006e4738, TYPES-GAP
extern uint32_t hud_text_draw_flags_006e4734;        // 0x006e4734, TYPES-GAP
extern int32_t hud_text_draw_unknown_006e4730;        // 0x006e4730, TYPES-GAP

extern int QueryPerformanceCounter(large_integer *counter); // 0x0063a0ac import thunk
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0
extern int32_t hud_text_message_queue_add(uint16_t *text, int32_t start_time, int32_t tag); // 0x4a3d90
extern void growable_array_remove_element(growable_array *array, int32_t index); // 0x4cf890, UNSURE signature
extern int32_t chimera__draw_16_bit_text(int32_t unknown_0, int32_t unknown_1, wchar_t *text); // 0x514ab0

// Ages every HUD text message, deletes any that have run past their duration, pulls in new
// messages from the widget's string-list tag until the per-update time budget (0x1af ms) is
// spent, then draws whatever remains.
uint32_t hud_text_message_queue_update_and_draw(widget_instance *widget)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    UnicodeStringList *strings =
        (UnicodeStringList *)tag_instances[tag->text_label_unicode_strings_list.tag_id.index].data;
    int32_t string_count = strings->strings.count;
    int32_t remaining = 0x1ae;
    int32_t message_index = -1;
    large_integer counter;
    int32_t now_ms;
    int32_t elapsed;

    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    elapsed = (int32_t)((float)now_ms - (float)hud_text_message_time_base); // see file header

    if (elapsed == 0) {
        goto after_expiry;
    }

    hud_text_message_time_base = now_ms;

    if (hud_text_message_queue.count > 0) {
        int32_t i = 0;
        int32_t byte_offset = 0;

        while (i < hud_text_message_queue.count) {
            hud_text_message *entry = (hud_text_message *)((uint8_t *)hud_text_message_queue.data + byte_offset);

            message_index = entry->unknown_04;
            remaining = entry->end_time - elapsed;
            entry->start_time = entry->start_time - elapsed;
            entry->end_time = remaining;
            if (remaining < 0x32) {
                growable_array_remove_element(&hud_text_message_queue, byte_offset / 0x14);
                i = i - 1;
                byte_offset = byte_offset - 0x14;
            }
            i = i + 1;
            byte_offset = byte_offset + 0x14;
        }
        if (remaining > 0x1ae) {
            goto after_expiry;
        }
    }

    for (;;) {
        message_index = message_index + 1;
        if (message_index >= string_count) {
            if (hud_text_message_cycle_state_00719230 != 0) {
                if (hud_text_message_queue.count < 1) {
                    hud_text_message_cycle_state_00719230 = 2;
                    widget_instance_close_and_restore_previous(widget);
                    return 1;
                }
                goto after_expiry;
            }
            message_index = 0;
        }

        if (tag->text_label_unicode_strings_list.tag_id.index != 0xffff) {
            if (message_index >= 0 && message_index < strings->strings.count) {
                UnicodeStringListString *entry = (UnicodeStringListString *)strings->strings.pointer + message_index;
                uint32_t size = entry->string.size;

                if ((int32_t)size > 0) {
                    *(uint16_t *)((uint8_t *)entry->string.pointer - 2 + (size & 0xfffffffe)) = 0;
                }
            }
        }
        {
            UnicodeStringListString *entry = (UnicodeStringListString *)strings->strings.pointer + message_index;
            int32_t duration = hud_text_message_queue_add((uint16_t *)entry->string.pointer, message_index, 0);

            remaining = remaining + duration;
        }
        if (remaining >= 0x1af) {
            break;
        }
    }

after_expiry:
    if (hud_text_message_queue.count > 0) {
        int32_t i;
        int32_t byte_offset = 0;

        for (i = 0; i < hud_text_message_queue.count; i++) {
            hud_text_message *entry = (hud_text_message *)((uint8_t *)hud_text_message_queue.data + byte_offset);
            ColorARGB *palette = hud_text_message_normal_color; // default: 0x00685200

            if (entry->hold != 1) {
                palette = hud_text_message_hold_color; // override: 0x006851f4
            }

            hud_text_draw_font_006e472c = *(int32_t *)((uint8_t *)tag + 0x108); // UNSURE: raw tag offset
            hud_text_draw_color_006e4738 = *palette;
            hud_text_draw_flags_006e4734 = 0x0002ffff; // low16 0xffff, high16 2, see file header
            hud_text_draw_unknown_006e4730 = 0;
            chimera__draw_16_bit_text(0, 0, (wchar_t *)entry->text);
            byte_offset = byte_offset + 0x14;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a3e30):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 hud_text_message_queue_update_and_draw(uint *param_1)

{
  uint *puVar1;
  int iVar2;
  int *piVar3;
  uint uVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  int iVar9;
  short sVar10;
  int iVar11;
  int iVar12;
  undefined8 uVar13;
  int local_20;
  int local_1c;
  LARGE_INTEGER local_8;

  iVar2 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar9 = **(int **)((*(uint *)(iVar2 + 0xf8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_1c = 0x1ae;
  iVar11 = -1;
  QueryPerformanceCounter(&local_8);
  uVar13 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar5 = __alldiv(uVar13,DAT_006ac8f8,DAT_006ac8fc);
  iVar6 = __ftol();
  if (iVar6 != 0) {
    iVar12 = 0;
    DAT_0071922c = uVar5;
    if (0 < DAT_006b37ec) {
      local_20 = 0;
      do {
        iVar11 = *(int *)(local_20 + 4 + DAT_006b37f0);
        iVar7 = local_20 + DAT_006b37f0;
        local_1c = *(int *)(iVar7 + 0x10) - iVar6;
        *(int *)(iVar7 + 0xc) = *(int *)(local_20 + 0xc + DAT_006b37f0) - iVar6;
        *(int *)(iVar7 + 0x10) = local_1c;
        if (local_1c < 0x32) {
          growable_array_remove_element();
          iVar12 = iVar12 + -1;
          local_20 = local_20 + -0x14;
        }
        iVar12 = iVar12 + 1;
        local_20 = local_20 + 0x14;
      } while (iVar12 < DAT_006b37ec);
      if (0x1ae < local_1c) goto LAB_004a3fe4;
    }
    do {
      iVar11 = iVar11 + 1;
      if (iVar9 <= iVar11) {
        if (DAT_00719230 != 0) {
          if (DAT_006b37ec < 1) {
            DAT_00719230 = 2;
            FUN_0049c3e0();
            return 1;
          }
          goto LAB_004a3ff2;
        }
        iVar11 = 0;
      }
      if (*(uint *)(iVar2 + 0xf8) != 0xffffffff) {
        sVar10 = (short)iVar11;
        piVar3 = *(int **)((*(uint *)(iVar2 + 0xf8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        if ((-1 < sVar10) && ((int)sVar10 < *piVar3)) {
          puVar1 = (uint *)(piVar3[1] + sVar10 * 0x14);
          uVar4 = *puVar1;
          if (0 < (int)uVar4) {
            *(undefined2 *)((puVar1[3] - 2) + (uVar4 & 0xfffffffe)) = 0;
          }
        }
      }
      iVar6 = hud_text_message_queue_add(iVar11);
      local_1c = local_1c + iVar6;
    } while (local_1c < 0x1af);
  }
LAB_004a3fe4:
  if (0 < DAT_006b37ec) {
LAB_004a3ff2:
    iVar11 = 0;
    iVar9 = 0;
    if (0 < DAT_006b37ec) {
      do {
        puVar8 = (undefined4 *)PTR_DAT_00685200;
        if (*(int *)(iVar11 + 8 + DAT_006b37f0) != 1) {
          puVar8 = (undefined4 *)PTR_DAT_006851f4;
        }
        DAT_006e472c = *(undefined4 *)(iVar2 + 0x108);
        DAT_006e4738 = *puVar8;
        DAT_006e473c = puVar8[1];
        DAT_006e4740 = puVar8[2];
        DAT_006e4744 = puVar8[3];
        DAT_006e4734._0_2_ = 0xffff;
        DAT_006e4734._2_2_ = 2;
        _DAT_006e4730 = 0;
        chimera__draw_16_bit_text(0,0,*(undefined4 *)(iVar11 + DAT_006b37f0));
        iVar9 = iVar9 + 1;
        iVar11 = iVar11 + 0x14;
      } while (iVar9 < DAT_006b37ec);
    }
  }
  return 1;
}
#endif
