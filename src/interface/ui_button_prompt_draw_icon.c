// ui_button_prompt_draw_icon  (Ghidra: FUN_0049ac80, unnamed; earlier src name
// ui_button_prompt_queue_icon_sound)
// address 0x49ac80, size 163 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: sole caller is ui_widget_draw_formatted_prompt_string @0x49ade0, which passes
// &button_icons[token] in ESI inside its icon branch. Rewritten from objdump 0x49ac80..0x49ad22
// in the phase-4 review: nothing here touches sound. It picks a bitmap tag out of the globals
// interface_bitmaps block (+0xec of the first GlobalsInterfaceBitmaps element, the tag_id of the
// dependency at +0xe0), computes an animation frame from a QueryPerformanceCounter millisecond
// clock when the icon has a frame_rate ((ms * 30 / 1000) / frame_rate, 32-bit unsigned), else
// frame 0, and hands bitmap, the icon sequence_index and the frame (EAX) to 0x4ab8d0. The first
// rewrite read the tag id as a pointer, lost the frame computation and the EAX argument, and
// named the routine after a sound queue.
// register convention: HUDGlobalsButtonIcon * in ESI.
// blam-cc: ESI -> icon
// UNSURE: 0x4ab8d0 (review queue name hud_meter_get_bitmap_frame) is taken to draw the icon
// frame at the text cursor; its two pointer arguments are this function's own locals (a zeroed
// dword and the 8 byte counter buffer). The name follows from the caller context.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern Globals *global_globals;       // 0x00746fa0
extern int64_t performance_frequency; // 0x006ac8f8 (LowPart) .. 0x006ac8fc (HighPart), LARGE_INTEGER

extern void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                           void **out_data, int32_t *out_offset); // 0x4ab8d0, blam-cc: EAX frame_index

// blam-cc: ESI -> icon
// Draws one HUD button icon (bitmap sequence icon->sequence_index), animated at
// icon->frame_rate frames per 30 ticks of a millisecond clock when that is nonzero.
void ui_button_prompt_draw_icon(HUDGlobalsButtonIcon *icon)
{
    uint8_t *bitmaps = (global_globals->interface_bitmaps.count != 0)
                           ? (uint8_t *)global_globals->interface_bitmaps.pointer
                           : (uint8_t *)0;
    datum_index bitmap_tag = *(datum_index *)(bitmaps + 0xec);
    int32_t zero = 0;
    int64_t counter;
    int32_t frame;

    if (icon->frame_rate == 0) {
        frame = 0;
    } else {
        uint32_t milliseconds;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        milliseconds = (uint32_t)((counter * 1000) / performance_frequency); // __allmul, __alldiv
        frame = (int32_t)((milliseconds * 30u / 1000u) / (uint32_t)(int32_t)icon->frame_rate);
    }
    // The outputs land in two locals (the zeroed dword and the low half of the counter) and
    // are never read: the resolved bitmap is discarded and nothing is drawn.
    hud_meter_resolve_bitmap_frame(bitmap_tag, (int16_t)icon->sequence_index, (uint16_t)frame, (void **)&zero,
                                   (int32_t *)&counter);
}

#if 0
Original Ghidra decompilation (0x49ac80):

void FUN_0049ac80(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined2 *unaff_ESI;
  undefined8 uVar3;
  undefined4 local_c;
  LARGE_INTEGER local_8;

  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar2 = 0;
  }
  else {
    iVar2 = *(int *)(DAT_00746fa0 + 0x144);
  }
  uVar1 = *(undefined4 *)(iVar2 + 0xec);
  local_c = 0;
  if (*(char *)(unaff_ESI + 6) != '\0') {
    QueryPerformanceCounter(&local_8);
    uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  }
  FUN_004ab8d0(uVar1,*unaff_ESI,&local_c,&local_8);
  return;
}
#endif
