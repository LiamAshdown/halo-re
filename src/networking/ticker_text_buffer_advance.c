// ticker_text_buffer_advance  (Ghidra: FUN_004b8b40; named per this rewrite)
// address 0x4b8b40, size 489 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Advances a scrolling ticker text
// buffer to its next visible window, extracting the substring to display and computing a
// per-character scroll delay (slower for double-byte glyphs)"); out/phase2/text/00.md for
// text_measure_string_fit_width's signature (int __cdecl(int *max_width_inout), returning the
// number of characters that fit and decrementing *max_width_inout by the width they used, or
// returning 0 and leaving *max_width_inout as the leftover width when the whole remaining
// string fits); types/memory.h heap layout for the heap_reallocate calls.
// register convention: an owning widget/list-row pointer in EAX (in_EAX, unresolved), this-
// pointer (ticker_text_buffer *) in EDI (unaff_EDI).
// ticker_text_buffer lives in types/networking.h; see ticker_text_buffer_reset.c.
// UNSURE: `widget` and everything read through it (row_object, text_row, font_record, and the
// text-measurement engine's shared scratch globals at 0x006e4730..0x006e4744) belong to the
// widget/text-rendering subsystem, which has no typed header in this repository (out/phase2/text
// is unreviewed past pass 1). Every offset below is transcribed as-is from the decompilation;
// none of it is Blam networking state, so nothing here is named beyond what the raw arithmetic
// already implies. Field-offset comments record only what Ghidra shows, not a confirmed meaning.
// UNSURE: every heap_reallocate() call site elides its EAX (old payload) argument the same way
// heap_reallocate.c's own header describes; reconstructed here as the text-row's previous
// display-buffer pointer (text_row[0xf]), matching realloc semantics, not independently confirmed.
// reconciled: R36 0x006e4738..0x006e4744 is ColorARGB text_color, alpha first: externs renamed r/g/b/a -> alpha/red/green/blue by address (same bytes)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern heap *widget_memory_pool; //  0x006926c4, "widget_memory_pool" (built by 0x4979b0) -- the global holds a POINTER to the heap (mov esi,[0x6926c4] at every call site)

extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80


// UNSURE: the text engine's shared "current string" scratch state; see file header.
extern void *hud_text_draw_font_tag_id;      // 0x006e472c
extern uint16_t hud_text_draw_color_or_flags;   // 0x006e4734 low word
extern uint16_t hud_text_draw_column;  // 0x006e4736 high word
extern float hud_text_draw_color_a;  // 0x006e4738
extern float hud_text_draw_color_r;  // 0x006e473c
extern float hud_text_draw_color_g;  // 0x006e4740
extern float hud_text_draw_color_b;  // 0x006e4744
extern int32_t hud_text_draw_unknown_4730;    // 0x006e4730

extern int32_t text_measure_string_fit_width(int32_t *max_width_inout); // 0x557530

// UNSURE: base of a 0x20-byte-stride font-record table, indexed by a 16-bit id read out of the
// widget's text-row object; see file header.
extern uint8_t *tag_instances; // 0x0087bc14

// blam-cc: EAX -> widget, EDI -> self
void ticker_text_buffer_advance(uint8_t *widget, ticker_text_buffer *self)
{
    uint8_t *row_object = *(uint8_t **)(widget + 0x34);     // UNSURE: widget internals
    uint32_t *text_row = *(uint32_t **)(row_object + 0x2c); // UNSURE: widget internals
    uint8_t *font_record;
    int32_t max_width[5];
    int32_t fit_count;
    wchar_t *display_text;

    *(int16_t *)(row_object + 0x40) = (int16_t)self->start_column;

    font_record = *(uint8_t **)(tag_instances + (*text_row & 0xffff) * 0x20 + 0x14);
    hud_text_draw_font_tag_id = *(void **)(font_record + 0x108);
    max_width[0] = (int32_t)*(int16_t *)(font_record + 0x2a) - (int32_t)*(int16_t *)(font_record + 0x26);
    max_width[1] = 0;
    hud_text_draw_color_a = 0.0f;
    max_width[2] = 0;
    max_width[3] = 0;
    max_width[4] = 0;
    hud_text_draw_color_r = 0.0f;
    hud_text_draw_color_g = 0.0f;
    hud_text_draw_color_b = 0.0f;
    hud_text_draw_color_or_flags = 0xffff;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;

    fit_count = text_measure_string_fit_width(max_width);
    if (fit_count == 0) {
        if (self->scroll_cursor == 0) {
            // The whole buffer fits inside the display width: a straight copy from the start.
            display_text = (wchar_t *)heap_reallocate((void *)(uintptr_t)text_row[0xf],
                (uint32_t)((uint16_t)((int16_t)(self->length + 1)) & 0x7fff) << 1,
                widget_memory_pool);
            text_row[0xf] = (uint32_t)(uintptr_t)display_text;
            if (display_text != 0) {
                wcsncpy(display_text, (const wchar_t *)self->text, self->length);
                display_text[self->length] = 0;
            }
            goto wrap_cursor;
        } else {
            int32_t tail_length = self->length - self->scroll_cursor;
            int32_t wrap_length;

            max_width[0] = text_measure_string_fit_width(max_width);
            display_text = (wchar_t *)heap_reallocate((void *)(uintptr_t)text_row[0xf],
                (uint32_t)(((tail_length + max_width[0]) * 2 + 2) & 0xffff), widget_memory_pool);
            text_row[0xf] = (uint32_t)(uintptr_t)display_text;
            if (display_text != 0) {
                wcsncpy(display_text, (const wchar_t *)self->text + self->scroll_cursor, tail_length);
                wrap_length = max_width[0];
                wcsncpy(display_text + tail_length, (const wchar_t *)self->text, wrap_length);
                fit_count = tail_length + wrap_length;
                display_text[fit_count] = 0;
                self->scroll_delay_ms = 100 +
                    (((*(uint16_t *)(self->text + self->scroll_cursor) & 0xff00) != 0) ? 0x52 : 0);
                self->scroll_cursor = self->scroll_cursor + 1;
                goto wrap_cursor;
            }
        }
    } else {
        display_text = (wchar_t *)heap_reallocate((void *)(uintptr_t)text_row[0xf],
            (uint32_t)((fit_count * 2 + 2) & 0xffff), widget_memory_pool);
        text_row[0xf] = (uint32_t)(uintptr_t)display_text;
        if (display_text != 0) {
            wcsncpy(display_text, (const wchar_t *)self->text + self->scroll_cursor, fit_count);
            display_text[fit_count] = 0;
        }
    }

    self->scroll_delay_ms = 100 +
        (((*(uint16_t *)(self->text + self->scroll_cursor) & 0xff00) != 0) ? 0x52 : 0);
    self->scroll_cursor = self->scroll_cursor + 1;

wrap_cursor:
    if (self->length <= self->scroll_cursor) {
        self->scroll_cursor = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4b8b40):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004b8b40(void)

{
  int iVar1;
  uint *puVar2;
  size_t sVar3;
  int in_EAX;
  size_t sVar4;
  wchar_t *pwVar5;
  int *unaff_EDI;
  size_t local_14 [5];

  iVar1 = *(int *)(in_EAX + 0x34);
  *(short *)(iVar1 + 0x40) = (short)unaff_EDI[1];
  puVar2 = *(uint **)(iVar1 + 0x2c);
  iVar1 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  DAT_006e472c = *(undefined4 *)(iVar1 + 0x108);
  local_14[0] = (int)*(short *)(iVar1 + 0x2a) - (int)*(short *)(iVar1 + 0x26);
  local_14[1] = 0;
  DAT_006e4738 = 0;
  local_14[2] = 0;
  local_14[3] = 0;
  local_14[4] = 0;
  DAT_006e473c = 0;
  DAT_006e4744 = 0;
  DAT_006e4740 = 0;
  DAT_006e4734._0_2_ = 0xffff;
  DAT_006e4734._2_2_ = 0;
  _DAT_006e4730 = 0;
  sVar4 = text_measure_string_fit_width((int *)local_14);
  if (sVar4 == 0) {
    if (unaff_EDI[2] == 0) {
      pwVar5 = (wchar_t *)heap_reallocate(((ushort)((short)unaff_EDI[3] + 1) & 0x7fff) << 1);
      puVar2[0xf] = (uint)pwVar5;
      if (pwVar5 != (wchar_t *)0x0) {
        _wcsncpy(pwVar5,(wchar_t *)*unaff_EDI,unaff_EDI[3]);
        *(undefined2 *)(puVar2[0xf] + unaff_EDI[3] * 2) = 0;
      }
      goto LAB_004b8d13;
    }
    sVar4 = unaff_EDI[3] - unaff_EDI[2];
    local_14[0] = text_measure_string_fit_width((int *)local_14);
    pwVar5 = (wchar_t *)heap_reallocate((local_14[0] + sVar4) * 2 + 2 & 0xffff);
    puVar2[0xf] = (uint)pwVar5;
    if (pwVar5 != (wchar_t *)0x0) {
      _wcsncpy(pwVar5,(wchar_t *)(*unaff_EDI + unaff_EDI[2] * 2),sVar4);
      sVar3 = local_14[0];
      _wcsncpy((wchar_t *)(puVar2[0xf] + sVar4 * 2),(wchar_t *)*unaff_EDI,local_14[0]);
      sVar4 = sVar4 + sVar3;
      goto LAB_004b8ce8;
    }
  }
  else {
    pwVar5 = (wchar_t *)heap_reallocate(sVar4 * 2 + 2 & 0xffff);
    puVar2[0xf] = (uint)pwVar5;
    if (pwVar5 != (wchar_t *)0x0) {
      _wcsncpy(pwVar5,(wchar_t *)(*unaff_EDI + unaff_EDI[2] * 2),sVar4);
LAB_004b8ce8:
      *(undefined2 *)(puVar2[0xf] + sVar4 * 2) = 0;
    }
  }
  unaff_EDI[5] = (-(uint)((*(ushort *)(*unaff_EDI + unaff_EDI[2] * 2) & 0xff00) != 0) & 0x52) + 100;
  unaff_EDI[2] = unaff_EDI[2] + 1;
LAB_004b8d13:
  if (unaff_EDI[3] <= unaff_EDI[2]) {
    unaff_EDI[2] = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
