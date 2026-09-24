// ai_communication_target_result_reset  (Ghidra: ai_communication_target_result_reset; named for this rewrite)
// address 0x42d310, size 45 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: phase-4 summary ("resets a small ai-communication target/result record to its
// empty (no target) state").
// register convention: EAX -> record (in_EAX, the only register Ghidra's decompile shows).
// blam-cc: EAX -> record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// blam-cc: EAX -> record
// Clears an ai-communication target/result record: zeroes the whole 0x20-byte record, then
// stamps its target handle and two leading fields to "none" (-1).
void ai_communication_target_result_reset(ai_communication_target_result *record)
{
    uint8_t *bytes = (uint8_t *)record;
    int32_t i;
    for (i = 0; i < 0x20; i++) {
        bytes[i] = 0;
    }
    record->target = (datum_index)k_datum_index_none;
    record->unknown_04 = -1;
    record->unknown_06 = -1;
    record->unknown_08 = -1;
}

#if 0
Original Ghidra decompilation (0x42d310):

void FUN_0042d310(void)

{
  undefined4 *in_EAX;

  *in_EAX = 0;
  in_EAX[1] = 0;
  in_EAX[2] = 0;
  in_EAX[3] = 0;
  in_EAX[4] = 0;
  in_EAX[5] = 0;
  in_EAX[6] = 0;
  in_EAX[7] = 0;
  *in_EAX = 0xffffffff;
  *(undefined2 *)(in_EAX + 1) = 0xffff;
  *(undefined2 *)((int)in_EAX + 6) = 0xffff;
  *(undefined2 *)(in_EAX + 2) = 0xffff;
  return;
}
#endif
