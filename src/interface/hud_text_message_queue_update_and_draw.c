// hud_text_message_queue_update_and_draw  (Ghidra: hud_text_message_queue_update_and_draw, already named)
// address 0x4a3e30, size 716 bytes, callers=0 in this build (a widget procedure: the scrolling text list)
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-27 (static loop) from objdump 0x4a3e30..0x4a4103. The queue entries scroll upwards: the
// elapsed wall time since the last update (QueryPerformanceCounter in ms, as unsigned) times 0.08 px/ms is taken
// off every entry's top (+0x0c) and bottom (+0x10); entries whose bottom passes 0x32 are removed. While the last
// bottom is at most 0x1ae, the widget's next string is appended below it (hud_text_message_queue_add: EAX text,
// EBX the running bottom, stack the string index, or L"<missing string>"; returns the entry height). The draft passed the string index as
// the position, so every line was stacked at y = index. At the end of the list the widget either wraps or (cycle
// state set) closes once the queue has drained. Each entry is drawn clipped to {0x32, 0, 0x1ae, 0x280}.
// blam-cc: stack -> widget

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
extern int32_t hud_text_message_cycle_state_00719230; // 0x00719230, compared as a dword
extern ColorARGB *hud_text_message_hold_color;   // 0x006851f4, a POINTER (0x4a4056 loads then derefs it)
extern ColorARGB *hud_text_message_normal_color; // 0x00685200, a POINTER (0x4a404a)
extern int32_t hud_text_draw_font_006e472c;         // 0x006e472c, TYPES-GAP
extern ColorARGB hud_text_draw_color_006e4738;       // 0x006e4738, TYPES-GAP
extern uint32_t hud_text_draw_flags_006e4734;        // 0x006e4734, TYPES-GAP
extern int32_t hud_text_draw_unknown_006e4730;        // 0x006e4730, TYPES-GAP
extern uint16_t missing_string_text[];               // 0x00671fac, L"<missing string>"

extern int QueryPerformanceCounter(large_integer *counter); // 0x0063a0ac import thunk
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0, EAX
extern int32_t hud_text_message_queue_add(uint16_t *text, int32_t start_time, int32_t tag);
    // 0x4a3d90, EAX text, EBX top (the running bottom), stack string index; returns the entry height
extern void growable_array_remove_element(growable_array *array, uint32_t index); // 0x4cf890, ESI, EDI
extern void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
    uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text); // 0x514ab0, EAX clip, ECX dest

uint32_t hud_text_message_queue_update_and_draw(widget_instance *widget)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;
    UnicodeStringList *strings =
        (UnicodeStringList *)tag_instances[tag->text_label_unicode_strings_list.tag_id.index].data;
    int32_t string_count = strings->strings.count;  // esp+0x20
    int32_t bottom = 0x1ae;                         // esp+0x14
    int32_t message_index = -1;                     // ebp
    large_integer counter;
    int32_t now_ms;
    int32_t elapsed;

    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    // 0x4a3eb5..0x4a3ed1: fild of the difference, +2^32 when negative (unsigned), * 0.08 (0x673038), __ftol
    elapsed = (int32_t)(long long)((double)(uint32_t)(now_ms - hud_text_message_time_base) * (double)0.08f);

    if (elapsed != 0) {
        hud_text_message_time_base = now_ms;

        if (hud_text_message_queue.count > 0) {
            int32_t i;

            for (i = 0; i < hud_text_message_queue.count; i++) {
                hud_text_message *entry = &((hud_text_message *)hud_text_message_queue.data)[i];

                message_index = entry->unknown_04;
                entry->start_time = entry->start_time - elapsed; // top
                entry->end_time = entry->end_time - elapsed;     // bottom
                bottom = entry->end_time;
                if (bottom < 0x32) {
                    growable_array_remove_element(&hud_text_message_queue, (uint32_t)i);
                    i--;
                }
            }
            if (bottom > 0x1ae) {
                goto draw;
            }
        }

        do {
            uint16_t *text = missing_string_text;

            message_index++;
            if (message_index >= string_count) {
                if (hud_text_message_cycle_state_00719230 != 0) {
                    if (hud_text_message_queue.count > 0) {
                        goto draw;
                    }
                    hud_text_message_cycle_state_00719230 = 2;
                    widget_instance_close_and_restore_previous(widget);
                    return 1;
                }
                message_index = 0;
            }

            if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id != 0xffffffff) {
                UnicodeStringList *list =
                    (UnicodeStringList *)tag_instances[tag->text_label_unicode_strings_list.tag_id.index].data;

                if ((int16_t)message_index >= 0 && (int16_t)message_index < (int32_t)list->strings.count) {
                    UnicodeStringListString *string =
                        &((UnicodeStringListString *)list->strings.pointer)[(int16_t)message_index];
                    int32_t size = (int32_t)string->string.size;

                    if (size > 0) {
                        text = (uint16_t *)string->string.pointer;
                        text[((uint32_t)size >> 1) - 1] = 0;
                    }
                }
            }
            bottom += hud_text_message_queue_add(text, bottom, message_index);
        } while (bottom <= 0x1ae);
    }

draw:
    if (hud_text_message_queue.count > 0) {
        Rectangle2D clip;
        Rectangle2D dest;
        int32_t i;

        clip.top = 0x32;
        clip.left = 0;
        clip.bottom = 0x1ae;
        clip.right = 0x280;
        dest.left = 0;
        dest.right = 0x280;
        for (i = 0; i < hud_text_message_queue.count; i++) {
            hud_text_message *entry = &((hud_text_message *)hud_text_message_queue.data)[i];
            ColorARGB *color = (entry->hold == 1) ? hud_text_message_normal_color : hud_text_message_hold_color;

            dest.bottom = (int16_t)entry->end_time;
            dest.top = (int16_t)entry->start_time;
            hud_text_draw_font_006e472c = *(int32_t *)((uint8_t *)tag + 0x108);
            hud_text_draw_color_006e4738 = *color;
            hud_text_draw_flags_006e4734 = 0x0002ffff; // WORD 0x6e4734 = -1, WORD 0x6e4736 = 2
            hud_text_draw_unknown_006e4730 = 0;
            chimera__draw_16_bit_text(&clip, (int32_t *)&dest, 0, 0, (const int16_t *)entry->text);
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
