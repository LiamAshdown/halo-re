// update_server_queue_get_history_entry  (Ghidra: FUN_00472ea0; renamed, no established name)
// address 0x472ea0, size 145 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Looks up and copies a previously pushed server-queue
// history entry for a requested tick index"); types/game.h update_server_queues (0x006f1d90,
// stride 0x64), update_server_history (0x006f1d94, 32 x update_record).
// register convention: a queue handle in ECX (Ghidra's `in_ECX`), a second output pointer in EAX
// (Ghidra's `in_EAX`, a `uint *` distinct from the stack parameter `param_1`).
//   // blam-cc: EAX -> out_tick, ECX -> queue_handle, stack -> out_record
// UNSURE: this is a low-confidence, mostly-literal transcription; the per-queue-entry field at
// +4 (compared against update_server_tick) and the history record layout beyond its own
// 0x308-byte stride are not attested in any header this module owns. Ghidra types EAX as a
// second `uint *` distinct from the stack buffer, so both are modeled as separate out-parameters
// here rather than folded into one.

// reconciled: copies history bytes 4..0x307 (0xc1 dwords) like the original; the draft copied all 0x308 bytes
//   from offset 0, shifting every field and writing 4 bytes past the caller's stack buffer (a jump to 0 later).
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *update_server_queues;          // 0x006f1d90
extern int32_t update_server_tick;                 // 0x006f1d8c
extern update_record update_server_history[32];     // 0x006f1d94

// blam-cc: EAX -> out_tick, ECX -> queue_handle, stack -> out_record
void update_server_queue_get_history_entry(int32_t *out_record, int32_t *out_tick, datum_index queue_handle)
{
    uint8_t counter_scratch[8]; // QueryPerformanceCounter's LARGE_INTEGER out-param, discarded
    uint8_t *entry = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)counter_scratch);

    if (queue_handle != k_datum_index_none) {
        entry = (uint8_t *)update_server_queues->data + (uint32_t)(uint16_t)queue_handle * update_server_queues->size;
        if (update_server_tick <= *(int32_t *)(entry + 4)) {
            *out_tick = -1;
            return;
        }
        *out_tick = *(int32_t *)(entry + 4);
    }

    {
        int32_t value = *out_tick;

        if (value != -1) {
            if (value < update_server_tick && update_server_tick - 0x20 <= value &&
                ((uint32_t)value & 0x1f) * sizeof(update_record) != (uint32_t)(-0x6f1d94)) {
                update_record *record = &update_server_history[value & 0x1f];
                int32_t *src = (int32_t *)record + 1;   // 0x472f0e: lea esi,[eax+0x4] -- the record's first dword is skipped
                int32_t i;

                for (i = 0; i < 0xc1; i++) {             // 0x472f11: mov ecx,0xc1 -- 0x304 bytes, the caller's buffer size
                    out_record[i] = src[i];
                }
            }
            if (entry != 0) {
                *(int32_t *)(entry + 4) = *(int32_t *)(entry + 4) + 1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x472ea0), from tools/pack.py 0x472ea0:

void FUN_00472ea0(undefined4 *param_1)

{
  uint uVar1;
  uint *in_EAX;
  uint in_ECX;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  iVar3 = 0;
  if (in_ECX != 0xffffffff) {
    iVar3 = (in_ECX & 0xffff) * 100 + *(int *)(DAT_006f1d90 + 0x34);
    if (DAT_006f1d8c <= (int)*(uint *)(iVar3 + 4)) {
      *in_EAX = 0xffffffff;
      return;
    }
    *in_EAX = *(uint *)(iVar3 + 4);
  }
  uVar1 = *in_EAX;
  if (uVar1 != 0xffffffff) {
    if ((((int)uVar1 < DAT_006f1d8c) && (DAT_006f1d8c + -0x20 <= (int)uVar1)) &&
       ((uVar1 & 0x1f) * 0x308 != -0x6f1d94)) {
      puVar4 = &DAT_006f1d98 + (uVar1 & 0x1f) * 0xc2;
      for (iVar2 = 0xc1; iVar2 != 0; iVar2 = iVar2 + -1) {
        *param_1 = *puVar4;
        puVar4 = puVar4 + 1;
        param_1 = param_1 + 1;
      }
    }
    if (iVar3 != 0) {
      *(int *)(iVar3 + 4) = *(int *)(iVar3 + 4) + 1;
    }
  }
  return;
}
#endif
