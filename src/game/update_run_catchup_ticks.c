// update_run_catchup_ticks  (Ghidra: FUN_00473310; renamed, no established name)
// address 0x473310, size 116 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Runs the server update-queue push/read cycle for a
// given number of catch-up ticks"); update_client_stage_entry.c (this batch, staged 8-dword
// record); update_server_queue_push_history.c and update_server_dispose.c /
// update_client_distribute_staged_entry.c (this batch), both called here.
// register convention: a tick count in BX (Ghidra's `unaff_BX`).
//   // blam-cc: BX -> tick_count
// UNSURE: the machine-index argument update_server_queue_push_history expects is not shown at
// this call site (Ghidra elides it entirely); modeled as 0 pending a disassembly pass. The
// DAT_007102d4 wraparound arithmetic (masked to a 64-wide signed range) is transcribed literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern int32_t update_client_unknown_102d4; // 0x007102d4, UNSURE raw counter
extern uint32_t update_client_staged[8];     // 0x006f7ea4

extern uint32_t update_server_queue_push_history(int32_t machine_index, uint32_t *source, uint32_t extra); // this batch, 0x473390
extern void update_server_push_player_tick_history(uint8_t out[0x160]); // this batch, 0x472cc0; UNSURE signature
extern void update_server_queue_get_history_entry(uint8_t out[772]); // this batch, 0x472ea0; UNSURE signature

// blam-cc: BX -> tick_count
void update_run_catchup_ticks(int16_t tick_count)
{
    uint32_t staged_copy[8];
    uint32_t extra = (uint32_t)update_client_unknown_102d4;
    int32_t i;

    if (tick_count <= 0) {
        return;
    }

    update_client_unknown_102d4 = (update_client_unknown_102d4 + 1) & 0x8000003f;
    if (update_client_unknown_102d4 < 0) {
        update_client_unknown_102d4 = (update_client_unknown_102d4 - 1 | 0xffffffc0) + 1;
    }

    for (i = 0; i < 8; i++) {
        staged_copy[i] = update_client_staged[i];
    }
    update_server_queue_push_history(0, staged_copy, extra); // UNSURE: machine_index

    {
        uint8_t scratch_a[0x160]; // Ghidra's local_324 is 8 dwords (0x20) but the *second*,
                                   // larger stack local (local_304, 772 bytes) is the one passed
                                   // below; local_324 itself is only used for the staged copy
                                   // above.
        uint8_t scratch_b[772];
        uint32_t remaining = (uint32_t)tick_count;

        (void)scratch_a;
        do {
            update_server_push_player_tick_history(scratch_b); // UNSURE: real argument shape not recovered
            update_server_queue_get_history_entry(scratch_b);
            remaining = remaining - 1;
        } while (remaining != 0);
    }
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
