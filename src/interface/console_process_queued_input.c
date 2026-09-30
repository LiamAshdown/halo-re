// console_process_queued_input  (Ghidra: console_process_queued_input, already named)
// address 0x4965e0, size 248 bytes
// name confidence: 0.7   rewrite confidence: 0.45
// evidence: out/phase4/interface_functions.md "Per-frame update of the developer console that
// feeds a queued/scripted stream of characters into the console input line one at a time,
// debounced by elapsed time."; out/phase4/interface_types_notes.md's ui_key_event provenance
// note ("pulls one 4-byte record out of the key ring at 0x006b16fe, cursor 0x006b16fa, limit
// 0x006b16fc"); widget_close_all.c's precedent name for 0x00712542.
// register convention: none used (see UNSURE below).
// UNSURE: Ghidra models an incoming/outgoing `in_EAX` that only ever contributes its high 24
// bits to the return value and is otherwise unused; this is the classic decompiler artifact for
// a BOOL function whose caller only reads AL, not a real parameter, so it is dropped rather than
// added as a phantom C parameter. The function's actual effect (queue draining, caret blink) is
// unaffected either way.
// UNSURE: the queued-key ring at 0x006b16fa/fc/fe is not documented by any module's types
// header; owned by an unidentified input/scripting subsystem, named locally.
// UNSURE: FUN_0044c290 (widget_text_edit_process_key) needs a `text_edit_state*` in EAX that
// this call site never materializes explicitly; &console->edit is the only state object this
// function otherwise touches, so it is supplied here as the inferred argument.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern terminal_console *console_active; // 0x006b2f0c
extern uint8_t console_caret_visible;    // 0x006b2f10
extern int32_t console_caret_blink_time; // 0x006b2f14
extern int64_t performance_frequency;    // 0x006ac8f8/0x006ac8fc

extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE (per src/interface/widget_close_all.c)
extern int16_t key_event_read_index;  // 0x006b16fa, int16 (word compares at 0x4a8c0d)
extern int16_t key_event_count; // 0x006b16fc, int16
extern ui_key_event key_events[];     // 0x006b16fe, UNSURE: ring array, capacity unknown


// Per-frame developer-console update: while a scripted/queued key stream is active (state byte
// 0x00712542 is not exactly 1, has bit 0x08 clear and bit 0x04 set) and the ring still has
// buffered events, drains one event per call into console_active's own key_events log (capped
// at 0x20) and feeds it to widget_text_edit_process_key against the console's edit state,
// refreshing the caret-blink timer each time. With nothing left to drain, toggles the caret's
// visibility once 500ms have passed since the last change.
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t console_process_queued_input(void)
{
    large_integer counter;
    int32_t now_ms;
    ui_key_event event;

    if (console_active == (terminal_console *)0) {
        return 0;
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    console_active->key_event_count = 0;

    while (controls_input_capture_flags != 1 && (controls_input_capture_flags & 8) == 0 &&
           (controls_input_capture_flags & 4) != 0 &&
           key_event_read_index < key_event_count) {
        event = key_events[key_event_read_index];
        key_event_read_index = key_event_read_index + 1;
        if (console_active->key_event_count < 0x20) {
            console_active->key_events[console_active->key_event_count] = event;
            console_active->key_event_count = console_active->key_event_count + 1;
        }
        widget_text_edit_process_key(&console_active->edit, &event);
        console_caret_visible = 1;
        console_caret_blink_time = now_ms;
    }

    if (console_caret_blink_time + 500 < now_ms) {
        console_caret_visible = (console_caret_visible == 0);
        console_caret_blink_time = now_ms;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4965e0):

uint console_process_queued_input(void)

{
  uint in_EAX;
  uint uVar1;
  int iVar2;
  short *psVar3;
  int iVar4;
  undefined8 uVar5;
  LARGE_INTEGER local_8;

  uVar1 = in_EAX & 0xffffff00;
  if (DAT_006b2f0c != (short *)0x0) {
    QueryPerformanceCounter(&local_8);
    uVar5 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar2 = __alldiv(uVar5,DAT_006ac8f8,DAT_006ac8fc);
    psVar3 = DAT_006b2f0c;
    *DAT_006b2f0c = 0;
    while ((((DAT_00712542 != 1 && ((DAT_00712542 & 8) == 0)) && ((DAT_00712542 & 4) != 0)) &&
           (DAT_006b16fa < DAT_006b16fc))) {
      local_8.s.LowPart = (&DAT_006b16fe)[DAT_006b16fa];
      DAT_006b16fa = DAT_006b16fa + 1;
      if (*psVar3 < 0x20) {
        *(undefined4 *)(psVar3 + *psVar3 * 2 + 1) = local_8.s.LowPart;
        *psVar3 = *psVar3 + 1;
      }
      FUN_0044c290(&local_8);
      DAT_006b2f10 = '\x01';
      psVar3 = DAT_006b2f0c;
      DAT_006b2f14 = iVar2;
    }
    iVar4 = DAT_006b2f14 + 500;
    if (iVar4 < iVar2) {
      iVar4 = CONCAT31((int3)((uint)iVar4 >> 8),DAT_006b2f10);
      DAT_006b2f10 = DAT_006b2f10 == '\0';
      DAT_006b2f14 = iVar2;
    }
    uVar1 = CONCAT31((int3)((uint)iVar4 >> 8),1);
  }
  return uVar1;
}
#endif
