// server_browser_list_row_populate  (Ghidra: FUN_004b67e0, still unnamed -> renamed)
// address 0x4b67e0, size 478 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase4/networking_functions.md summary ("populates one server/player list UI
// row's text and checkbox fields (name, player ratio, ping) from the supplied values"); uses
// the same widget-tree shape (label_text at +0x3c, highlight_flag at +0x58) as
// join_game_server_browser_tick.c and server_browser_open.c, declared here with its own local
// `network_ui_widget` node declared in types/networking.h.
// register convention: row widget in EAX (in_EAX), two boolean flags in CL and DL (in_CL,
// in_DL); param_1 and param_3 are real but unused stack parameters (kept for signature
// fidelity), matching network_channel_attempt_connect's unused_param_1 precedent.
// FIXED in the review pass: string_convert_ascii_to_unicode's three register arguments are invisible in Ghidra's
// decompile. The disassembly at 0x4b6829 and 0x4b68bf is
//   mov ebx,[esp+0x14] / mov edi,0x800 / mov eax,0x6b5e90 / call 0x557990
// (and the same with [esp+0x1c]), so it widens an ASCII string into the shared 0x800-byte
// buffer at 0x006b5e90 and returns it. With the four saved registers, [esp+0x14] and
// [esp+0x1c] are this function's param_1 and param_3, which an earlier draft called
// unused_param1/unused_param3 -- they are the server name and the gametype name.
// UNSURE: the fixed-size heap_reallocate buffers (0x80, 0x40, 0x40, 0x40, 0x10 bytes) and the
// column order they populate (two checkboxes, a "flags" label, name, another checkbox, a
// second "flags" label, a player-ratio label, a ping label) are inferred purely from traversal
// order; no individual column is independently confirmed beyond what the summary states
// (name, ratio, ping).
// UNSURE: PTR_s_parameter_handles_0063fff0_0x35_006607a0 (the ping format string) and
// empty_string (passed to FUN_00625b7a on the two "leave blank" paths) are declared only by
// address.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

extern wchar_t empty_string[]; // see UNSURE
extern const wchar_t PTR_s_parameter_handles_0063fff0_0x35_006607a0[]; // ping format string, see UNSURE

extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, memory module
extern heap widget_memory_pool; // 0x006926c4
// blam-cc: EAX -> dest, EDI -> dest capacity in BYTES, EBX -> ASCII source.
// Widens an ASCII string into dest and returns dest, or NULL when it does not fit.
extern wchar_t *string_convert_ascii_to_unicode(wchar_t *dest, int32_t dest_bytes, const char *source); // 0x557990
extern wchar_t string_widen_scratch[0x400]; // 0x006b5e90, the 0x800-byte shared target
extern void string_format_wide_va_bounded(uint32_t count, wchar_t *dest, const wchar_t *format, ...); // 0x557910, blam-cc: EDX count;
    // the bound (0x1f at the 0x4b84e0 call sites) rides in EDX and is not modeled here // foreign

// blam-cc: row widget in EAX (in_EAX), flag1 in CL (in_CL), flag2 in DL (in_DL)
void server_browser_list_row_populate(network_ui_widget *row, uint8_t flag1, uint8_t flag2,
                                        const char *server_name, wchar_t *map_name,
                                        const char *gametype_name,
                                        uint8_t flag3, int32_t count_a, int32_t count_b, int32_t ping)
{
    network_ui_widget *w1;
    network_ui_widget *w2;
    wchar_t *text;

    w1 = row->first_child;
    w1->visible = flag1 != 0;
    w1->highlight_flag = 1;
    w1 = w1->next_sibling;
    w2 = w1->next_sibling;
    w1->highlight_flag = 1;
    w1->visible = flag2 != 0;
    text = (wchar_t *)heap_reallocate(0, 0x80, &widget_memory_pool);
    w2->label_text = text;
    if (text != 0) {
        wchar_t *source = string_convert_ascii_to_unicode(string_widen_scratch, 0x800, server_name);
        wcsncpy(w2->label_text, source, 0x3f);
        *(uint16_t *)((uint8_t *)w2->label_text + 0x7e) = 0;
    }
    w1 = w2->next_sibling;
    text = (wchar_t *)heap_reallocate(0, 0x40, &widget_memory_pool);
    w1->label_text = text;
    if (text != 0) {
        wcsncpy(text, map_name, 0x1f);
        *(uint16_t *)((uint8_t *)w1->label_text + 0x3e) = 0;
    }
    w1 = w1->next_sibling;
    w2 = w1->next_sibling;
    w1->highlight_flag = 1;
    w1->visible = flag3 != 0;
    text = (wchar_t *)heap_reallocate(0, 0x40, &widget_memory_pool);
    w2->label_text = text;
    if (text != 0) {
        wchar_t *source = string_convert_ascii_to_unicode(string_widen_scratch, 0x800, gametype_name);
        wcsncpy(w2->label_text, source, 0x1f);
        *(uint16_t *)((uint8_t *)w2->label_text + 0x3e) = 0;
    }
    w1 = w2->next_sibling;
    text = (wchar_t *)heap_reallocate(0, 0x40, &widget_memory_pool);
    w1->label_text = text;
    if (text != 0) {
        if (count_a == -1 || count_b == -1) {
            wcscpy(text, L""); // FIXED 2026-09-28: the wcslen (0x625b7a) before it is the inlined copy's unused length
        } else {
            string_format_wide_va_bounded(0x1f, text, L"%d / %d", count_a, count_b); // FIXED: EDX = 0x1f at 0x4b6925
            *(uint16_t *)((uint8_t *)w1->label_text + 0x3e) = 0;
        }
    }
    w1 = w1->next_sibling;
    text = (wchar_t *)heap_reallocate(0, 0x10, &widget_memory_pool);
    w1->label_text = text;
    if (text != 0) {
        if (0 < ping && ping < 9999) {
            string_format_wide_va_bounded(7, text, PTR_s_parameter_handles_0063fff0_0x35_006607a0, ping); // FIXED: EDX = 7 at 0x4b6988
            *(uint16_t *)((uint8_t *)w1->label_text + 0xe) = 0;
            return;
        }
        wcscpy(text, L"");
    }
}

#if 0
Original Ghidra decompilation (0x4b67e0):

void FUN_004b67e0(undefined4 param_1,wchar_t *param_2,undefined4 param_3,char param_4,int param_5,
                 int param_6,int param_7)

{
  int iVar1;
  int in_EAX;
  int iVar2;
  wchar_t *pwVar3;
  char in_CL;
  char in_DL;

  iVar2 = *(int *)(in_EAX + 0x34);
  *(bool *)(iVar2 + 0x10) = in_CL != '\0';
  *(undefined2 *)(iVar2 + 0x58) = 1;
  iVar2 = *(int *)(iVar2 + 0x2c);
  iVar1 = *(int *)(iVar2 + 0x2c);
  *(undefined2 *)(iVar2 + 0x58) = 1;
  *(bool *)(iVar2 + 0x10) = in_DL != '\0';
  iVar2 = heap_reallocate(0x80);
  *(int *)(iVar1 + 0x3c) = iVar2;
  if (iVar2 != 0) {
    pwVar3 = (wchar_t *)FUN_00557990();
    _wcsncpy(*(wchar_t **)(iVar1 + 0x3c),pwVar3,0x3f);
    *(undefined2 *)(*(int *)(iVar1 + 0x3c) + 0x7e) = 0;
  }
  iVar2 = *(int *)(iVar1 + 0x2c);
  pwVar3 = (wchar_t *)heap_reallocate(0x40);
  *(wchar_t **)(iVar2 + 0x3c) = pwVar3;
  if (pwVar3 != (wchar_t *)0x0) {
    _wcsncpy(pwVar3,param_2,0x1f);
    *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x3e) = 0;
  }
  iVar2 = *(int *)(iVar2 + 0x2c);
  iVar1 = *(int *)(iVar2 + 0x2c);
  *(undefined2 *)(iVar2 + 0x58) = 1;
  *(bool *)(iVar2 + 0x10) = param_4 != '\0';
  iVar2 = heap_reallocate(0x40);
  *(int *)(iVar1 + 0x3c) = iVar2;
  if (iVar2 != 0) {
    pwVar3 = (wchar_t *)FUN_00557990();
    _wcsncpy(*(wchar_t **)(iVar1 + 0x3c),pwVar3,0x1f);
    *(undefined2 *)(*(int *)(iVar1 + 0x3c) + 0x3e) = 0;
  }
  iVar2 = *(int *)(iVar1 + 0x2c);
  pwVar3 = (wchar_t *)heap_reallocate(0x40);
  *(wchar_t **)(iVar2 + 0x3c) = pwVar3;
  if (pwVar3 != (wchar_t *)0x0) {
    if ((param_5 == -1) || (param_6 == -1)) {
      FUN_00625b7a(&DAT_00660c34);
      _wcscpy(pwVar3,L"");
    }
    else {
      string_format_wide_va_bounded(pwVar3,L"%d / %d",param_5,param_6);
      *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x3e) = 0;
    }
  }
  iVar2 = *(int *)(iVar2 + 0x2c);
  pwVar3 = (wchar_t *)heap_reallocate(0x10);
  *(wchar_t **)(iVar2 + 0x3c) = pwVar3;
  if (pwVar3 != (wchar_t *)0x0) {
    if ((0 < param_7) && (param_7 < 9999)) {
      string_format_wide_va_bounded(pwVar3,&PTR_s_parameter_handles_0063fff0_0x35_006607a0,param_7);
      *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0xe) = 0;
      return;
    }
    FUN_00625b7a(&DAT_00660c34);
    _wcscpy(pwVar3,L"");
  }
  return;
}
#endif
