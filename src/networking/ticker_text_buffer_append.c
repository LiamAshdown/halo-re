// ticker_text_buffer_append  (Ghidra: ticker_text_buffer_append, already named)
// address 0x4b8a60, size 213 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Appends a wide string to a growable
// text-line buffer object (the 'this' object is passed in EDI)... used to build the
// server-browser game-rules description ticker text"); symbols/functions.txt resolves the
// 0x625b7a callee to wcslen; types/memory.h heap layout for the heap_reallocate call.
// register convention: text-to-append (or NULL) in EAX (Ghidra-recognized param_1), an extra
// int in ECX (Ghidra-recognized param_2, only meaningful on the NULL/reset path), this-pointer
// (ticker_text_buffer *) in EDI (unaff_EDI).
// ticker_text_buffer lives in types/networking.h (shared layout with ticker_text_buffer_reset.c
// and ticker_text_buffer_advance.c, folded into the header during the review pass).
// UNSURE: `param_2` (here `reset_column`) is only ever observed called as 0, 1 or 2 from a
// server-browser description generator not in this batch's address range; its exact meaning
// (a tab-stop / column index synced into an external widget by the scroll-advance function)
// is not independently confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

extern heap *widget_memory_pool; //  0x006926c4, "widget_memory_pool" (built by 0x4979b0) -- the global holds a POINTER to the heap (mov esi,[0x6926c4] at every call site)

extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80


// blam-cc: EAX -> text (NULL resets/re-seeds the buffer instead of appending), ECX -> reset_column
// (only used on the NULL path), EDI -> self
// With text == NULL, (re)allocates the buffer to hold at least 0x20 wide characters, empties it
// and stores reset_column into start_column. Otherwise grows the buffer (doubling capacity, or
// seeding it at 0x20 characters) until it can hold the existing text plus the new text plus a
// NUL, then appends the new text. Either way the result is always left NUL-terminated and
// scroll_cursor is reset to the beginning.
void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self)
{
    if (text == 0) {
        if (self->capacity == 0) {
            self->capacity = 0x20;
        }
        self->text = (uint16_t *)heap_reallocate(0, (uint16_t)((int16_t)self->capacity) << 1,
            widget_memory_pool);
        self->length = 0;
        self->start_column = reset_column;
    } else {
        int32_t text_length = (int32_t)wcslen(text);

        if (self->capacity <= self->length + 1 + text_length) {
            do {
                if (self->capacity == 0) {
                    self->capacity = 0x20;
                } else {
                    self->capacity = self->capacity * 2;
                }
                self->text = (uint16_t *)heap_reallocate(self->text,
                    (uint16_t)((int16_t)self->capacity) << 1, widget_memory_pool);
            } while (self->capacity <= self->length + 1 + text_length);
        }
        if (self->text != 0) {
            wcscpy((wchar_t *)self->text + self->length, (const wchar_t *)text);
            self->length = self->length + text_length;
        }
    }
    self->text[self->length] = 0;
    self->scroll_cursor = 0;
}

#if 0
Original Ghidra decompilation (0x4b8a60):

void ticker_text_buffer_append(wchar_t *param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int *unaff_EDI;

  if (param_1 == (wchar_t *)0x0) {
    if (unaff_EDI[4] == 0) {
      unaff_EDI[4] = 0x20;
    }
    iVar1 = heap_reallocate((short)unaff_EDI[4] << 1);
    *unaff_EDI = iVar1;
    unaff_EDI[3] = 0;
    unaff_EDI[1] = param_2;
  }
  else {
    iVar1 = FUN_00625b7a(param_1);
    if (unaff_EDI[4] <= unaff_EDI[3] + 1 + iVar1) {
      do {
        if (unaff_EDI[4] == 0) {
          unaff_EDI[4] = 0x20;
        }
        else {
          unaff_EDI[4] = unaff_EDI[4] * 2;
        }
        iVar2 = heap_reallocate((short)unaff_EDI[4] << 1);
        *unaff_EDI = iVar2;
      } while (unaff_EDI[4] <= unaff_EDI[3] + 1 + iVar1);
    }
    if (*unaff_EDI != 0) {
      _wcscpy((wchar_t *)(*unaff_EDI + unaff_EDI[3] * 2),param_1);
      unaff_EDI[3] = unaff_EDI[3] + iVar1;
    }
  }
  *(undefined2 *)(*unaff_EDI + unaff_EDI[3] * 2) = 0;
  unaff_EDI[2] = 0;
  return;
}
#endif
