// update_run_catchup_ticks  (Ghidra: FUN_00473310; renamed, no established name)
// address 0x473310, size 116 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Runs the server update-queue push/read cycle for a
// given number of catch-up ticks"); update_client_stage_entry.c (this batch, staged 8-dword
// record); update_server_queue_push_history.c and update_server_dispose.c /
// update_client_distribute_staged_entry.c (this batch), both called here.
// register convention: a tick count in BX (Ghidra's `unaff_BX`).
//   // blam-cc: BX -> tick_count
// FIXED (verified against 0x473310..0x473383): update_server_queue_push_history gets AX = 0 (xor eax,eax),
//   EDX = the tick count, the staged copy and the counter's value before the bump. The loop calls
//   update_server_push_player_tick_history (no arguments) and update_server_queue_get_history_entry with
//   ECX = 0 (queue 0, not none), EAX = a 4-byte local for the tick and the 0x304-byte record buffer on the
//   stack; the draft passed only the buffer, so the tick was written through a null pointer.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t update_client_unknown_102d4; // 0x007102d4, UNSURE raw counter
extern uint32_t update_client_staged[8];     // 0x006f7ea4

extern void update_server_queue_push_history(int16_t machine_index, int32_t tick_count, uint32_t *source,
    uint32_t extra); // 0x473390, blam-cc: EAX -> machine_index, EDX -> tick_count, stack -> source, extra
extern void update_server_push_player_tick_history(void); // 0x472cc0
extern void update_server_queue_get_history_entry(int32_t *out_record, int32_t *out_tick, datum_index queue_handle);
    // 0x472ea0, blam-cc: EAX -> out_tick, ECX -> queue_handle, stack -> out_record

// blam-cc: BX -> tick_count
void update_run_catchup_ticks(int16_t tick_count)
{
    int32_t tick;
    uint32_t staged_copy[8];
    int32_t record[0xc1];
    int32_t previous;
    int32_t next;
    uint32_t remaining;
    int32_t i;

    if (tick_count <= 0) {
        return;
    }
    previous = update_client_unknown_102d4;
    next = (previous + 1) & 0x8000003f;
    if (next < 0) {
        next = ((next - 1) | 0xffffffc0) + 1;
    }
    update_client_unknown_102d4 = next;
    for (i = 0; i < 8; i++) {
        staged_copy[i] = update_client_staged[i];
    }
    update_server_queue_push_history(0, tick_count, staged_copy, (uint32_t)previous);

    remaining = (uint16_t)tick_count;
    do {
        update_server_push_player_tick_history();
        update_server_queue_get_history_entry(record, &tick, 0);
        remaining = remaining - 1;
    } while (remaining != 0);
}

#if 0
Original Ghidra decompilation (0x473310), from tools/pack.py 0x473310:

void FUN_00473310(void)

{
  int iVar1;
  ushort unaff_BX;
  undefined4 *puVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 local_324 [8];
  undefined1 local_304 [772];

  uVar3 = DAT_007102d4;
  if (0 < (short)unaff_BX) {
    DAT_007102d4 = DAT_007102d4 + 1 & 0x8000003f;
    if ((int)DAT_007102d4 < 0) {
      DAT_007102d4 = (DAT_007102d4 - 1 | 0xffffffc0) + 1;
    }
    puVar2 = &DAT_006f7ea4;
    puVar4 = local_324;
    for (iVar1 = 8; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar4 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar4 = puVar4 + 1;
    }
    FUN_00473390(local_324,uVar3);
    uVar3 = (uint)unaff_BX;
    do {
      FUN_00472cc0();
      FUN_00472ea0(local_304);
      uVar3 = uVar3 - 1;
    } while (uVar3 != 0);
  }
  return;
}
#endif
