// ai_conversation_new  (Ghidra: squad_new_instance, renamed per ai_types_notes.md)
// address 0x431590, size 233 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/ai_types_notes.md's misattribution table names this address
// ai_conversation_new. Ghidra's own decompile removed the eviction call as an "unreachable
// block", losing the actual eviction (ai_conversation_stop on the lowest-priority, oldest
// instance) and the retry (datum_new_at_index_with_salt at that same slot) entirely;
// recovered from the real disassembly (objdump -d -M intel --start-address=0x431590
// --stop-address=0x431679 bin/halo.exe).
// register convention: plain __cdecl, matching Ghidra's own recognized signature.
// blam-cc: stack -> conversation_definition_index, allow_eviction
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"
#include <stdint.h>

extern data_array *ai_conversation_data; // 0x008802d4
extern game_time_globals *game_time;     // 0x006f1d6c

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array); // 0x4d03d0
extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b); // 0x430ea0, this batch

// blam-cc: stack -> conversation_definition_index, allow_eviction
// Allocates a new ai_conversation instance. If the pool is full and allow_eviction is set,
// picks the instance with the lowest priority (ties broken by the oldest start_tick),
// stops it, and re-allocates at that same slot. On success, initializes the new instance's
// definition index, resets its line cursor to -1, records allow_eviction as its own
// priority, and stamps its start_tick to the current tick.
datum_index ai_conversation_new(int16_t conversation_definition_index, uint8_t allow_eviction)
{
    datum_index handle;
    data_iterator iterator;
    ai_conversation *candidate;
    uint8_t best_priority;
    int32_t best_tick;
    datum_index best_handle;
    ai_conversation *instance;

    handle = datum_new(ai_conversation_data);
    if (handle == (datum_index)k_datum_index_none) {
        if (allow_eviction != 0) {
            best_priority = 1;
            best_tick = 0x7fffffff;
            best_handle = (datum_index)k_datum_index_none;

            iterator.data = ai_conversation_data;
            iterator.next_index = 0;
            iterator.index = (datum_index)k_datum_index_none;
            iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
            candidate = data_iterator_next(&iterator);
            while (candidate != 0) {
                if (candidate->priority < best_priority || candidate->start_tick < best_tick) {
                    best_tick = candidate->start_tick;
                    best_priority = candidate->priority;
                    best_handle = iterator.index;
                }
                candidate = data_iterator_next(&iterator);
            }

            if (best_handle != (datum_index)k_datum_index_none) {
                ai_conversation_stop(best_handle, 0, 0);
                handle = datum_new_at_index_with_salt(best_handle, ai_conversation_data);
            }
        }
    }

    if (handle != (datum_index)k_datum_index_none) {
        instance = &((ai_conversation *)ai_conversation_data->data)[handle & 0xffff];
        instance->definition_index = conversation_definition_index;
        instance->current_line_index = -1;
        instance->priority = allow_eviction;
        instance->start_tick = game_time->game_time;
    }
    return handle;
}

#if 0
Original Ghidra decompilation (0x431590):

/* WARNING: Removing unreachable block (ram,0x0043161f) */

uint __cdecl squad_new_instance(ushort squad_definition_index,char allow_eviction)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  byte bVar4;
  undefined8 uVar5;
  int local_14;

  uVar5 = datum_new();
  uVar1 = (uint)uVar5;
  if (uVar1 == 0xffffffff) {
    if (allow_eviction != '\0') {
      bVar4 = 1;
      local_14 = 0x7fffffff;
      iVar2 = data_iterator_next();
      while (iVar2 != 0) {
        if ((*(byte *)(iVar2 + 4) < bVar4) || (*(int *)(iVar2 + 0xc) < local_14)) {
          local_14 = *(int *)(iVar2 + 0xc);
          bVar4 = *(byte *)(iVar2 + 4);
        }
        iVar2 = data_iterator_next();
      }
    }
  }
  else {
    iVar3 = (uVar1 & 0xffff) * 100 + *(int *)((int)((ulonglong)uVar5 >> 0x20) + 0x34);
    *(ushort *)(iVar3 + 2) = squad_definition_index;
    iVar2 = DAT_006f1d6c;
    *(undefined2 *)(iVar3 + 0x48) = 0xffff;
    *(char *)(iVar3 + 4) = allow_eviction;
    *(undefined4 *)(iVar3 + 0xc) = *(undefined4 *)(iVar2 + 0xc);
  }
  return uVar1;
}

Real disassembly (0x431590-0x431678), used to recover the eviction path Ghidra discarded:

00431590: mov    edx,ds:0x8802d4
00431596: sub    esp,0x14
...
0043159d: call   0x4d0480             ; datum_new(ai_conversation_data)
004315a2: mov    ebp,eax
004315a4: or     esi,0xffffffff
004315a7: cmp    ebp,esi
004315a9: jne    0x43163e             ; got a handle, skip eviction entirely
004315af: mov    al,[esp+0x2c]        ; allow_eviction
004315b5: je     0x43166f             ; not allowed, return none
...                                   ; builds a data_iterator over ai_conversation_data
004315f0: mov    cl,[eax+0x4]         ; candidate.priority
004315fb: cmp    [eax+0xc],edx        ; candidate.start_tick vs best_tick
0043161f: push   eax
00431620: push   eax
00431621: push   esi                  ; best_handle
00431622: call   0x430ea0             ; ai_conversation_stop(best_handle, 0, 0)
00431630: mov    eax,esi
00431632: call   0x4d03d0             ; datum_new_at_index_with_salt(best_handle, ai_conversation_data)
#endif
