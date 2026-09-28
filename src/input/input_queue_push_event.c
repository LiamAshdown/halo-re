// input_queue_push_event  (Ghidra: FUN_00492340)
// address 0x492340, size 135 bytes
// name confidence: 0.65   rewrite confidence: 0.6
// evidence: types/input.h names this "input_queue_push_event 0x492340" and documents it in
// full: "The push takes the queue index in AX and the record in EDI, writes the queue index
// into record +0x02, moves slots 1..7 down onto 0..6 (memmove 0x38 bytes) and stores the record
// in slot 0." The struct note also records the observed quirk: the push shifts 1..7 down onto
// 0..6 and then overwrites slot 0, so slot 7 is never replaced by a push while the pop takes the
// highest nonzero slot; that behaviour is kept as-is, unchanged.
// register convention: queue index in EAX (in_AX), record pointer in EDI (unaff_EDI); the
// controller_index field of *record is written as an output parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern input_event_queue event_queue;                    // 0x00712cc0
extern int64_t performance_frequency;                           // 0x006ac8f8/0x006ac8fc
extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);  // 0x0063a0ac IAT
extern void *memmove(void *dst, const void *src, uint32_t count); // 0x006236f0 _memmove

// blam-cc: queue index in EAX, record pointer in EDI
// Pushes record onto queue queue_index (0..3): stamps record->controller_index with the queue
// index, shifts the existing 8 slots down by one (dropping the oldest, slot 7), stores record
// into slot 0, and refreshes last_event_time when record->kind is nonzero. A no-op while
// push_disabled is set.
void input_queue_push_event(int16_t queue_index, ui_input_event *record)
{
    large_integer counter;
    uint32_t now;
    ui_input_event *slots;

    if (event_queue.push_disabled == 0) {
        QueryPerformanceCounter(&counter);
        now = (uint32_t)((counter.quad_part * 1000) / performance_frequency);

        record->controller_index = queue_index;
        slots = event_queue.events[queue_index];
        memmove(&slots[0], &slots[1], sizeof(ui_input_event) * 7);
        slots[0] = *record;
        if (record->kind != 0) {
            event_queue.last_event_time = now;
        }
    }
}

#if 0
Original Ghidra decompilation (0x492340):

void FUN_00492340(void)

{
  short in_AX;
  undefined4 uVar1;
  int iVar2;
  short *unaff_EDI;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  if (DAT_00712cc1 == '\0') {
    QueryPerformanceCounter(&local_8);
    uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    uVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
    iVar2 = (int)in_AX;
    unaff_EDI[1] = in_AX;
    _memmove(&DAT_00712ccc + iVar2 * 0x10,&DAT_00712cd4 + iVar2 * 0x40,0x38);
    (&DAT_00712ccc)[iVar2 * 0x10] = *(undefined4 *)unaff_EDI;
    (&DAT_00712cd0)[iVar2 * 0x10] = *(undefined4 *)(unaff_EDI + 2);
    if (*unaff_EDI != 0) {
      DAT_00712cc4 = uVar1;
    }
  }
  return;
}
#endif
