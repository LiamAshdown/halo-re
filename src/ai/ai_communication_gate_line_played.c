// ai_communication_gate_line_played  (Ghidra: ai_communication_gate_line_played; named for this rewrite)
// address 0x42e970, size 50 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: phase-4 summary ("gates whether a communication-played timestamp should be
// recorded for a given event id, skipping a handful of event types and silenced records").
// register convention: CX -> event_id, EDX -> record (both unresolved registers in Ghidra's
// own decompile).
// blam-cc: CX -> event_id, EDX -> record
//
// UNSURE, substantially: ai_communication_record_line_played (0x42f9e0, this batch, already
// rewritten) actually takes four arguments (object_index, tier, communication_line_id,
// conversation_line_id); this call site shows none of them, only the record pointer and
// event_id this function itself receives. Passed record's assumed object_index field and
// event_id through as a best guess for two of the four; the other two are left as "none".
// Needs a disassembly pass to recover the real argument mapping.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_communication_record_line_played(datum_index object_index, int16_t tier,
                                                 int16_t communication_line_id,
                                                 int16_t conversation_line_id); // 0x42f9e0

// blam-cc: CX -> event_id, EDX -> record, stack -> object_index
// FIXED from objdump 0x42e970..0x42e9a1: the speaker is the first STACK argument ([esp+4], its only caller
// unit_update_animation_timers pushes the unit), and the line recorded is the record's +0x06 word.
// Skips event ids 0, 1, 2, 7 and 10 (table 0x42e9ac) and silenced records (+0x0a); otherwise
// ai_communication_record_line_played(speaker, event, record +0x06, -1).
void ai_communication_gate_line_played(int16_t event_id, ai_communication_record *record, datum_index object_index)
{
    switch (event_id) {
        case 0: case 1: case 2: case 7: case 10:
            break;
        default:
            if (record->silenced == 0) {
                ai_communication_record_line_played(object_index, event_id,
                    *(int16_t *)((uint8_t *)record + 0x6), -1);
            }
            break;
    }
}

#if 0
Original Ghidra decompilation (0x42e970):

void FUN_0042e970(void)

{
  undefined2 in_CX;
  int in_EDX;

  switch(in_CX) {
  case 0:
  case 1:
  case 2:
  case 7:
  case 10:
    break;
  default:
    if (*(char *)(in_EDX + 10) == '\0') {
      ai_communication_record_line_played();
    }
  }
  return;
}
#endif
