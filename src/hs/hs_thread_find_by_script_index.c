// hs_thread_find_by_script_index  (Ghidra: FUN_0048a960)
// address 0x48a960, size 138 bytes
// name confidence: 0.45 (out/phase4/hs_functions.md: "Finds the currently active or dormant HS
//   thread associated with a given script index")
// rewrite confidence: 0.7
// evidence: types/hs.h hs_thread (script_index 0x04); src/memory/data_iterator_next.c's sibling
//   datum_next.
// register convention: none (void); script index is the recognized stack parameter (param_1).

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern datum_index datum_next(int16_t after_index, data_array *array);
    // blam-cc: DX -> after_index, EDI -> array; memory module, 0x4d0630

extern data_array *hs_thread_data; // 0x0087a470

// Finds the thread whose script_index equals `script_index`, or k_datum_index_none if none.
datum_index hs_thread_find_by_script_index(int16_t script_index)
{
    datum_index thread_handle;
    hs_thread *thread;

    thread_handle = datum_next(-1, hs_thread_data);
    while (thread_handle != k_datum_index_none) {
        thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_handle & 0xffff) * 0x218);
        if (thread->script_index == script_index) {
            return thread_handle;
        }
        thread_handle = datum_next((int16_t)thread_handle, hs_thread_data);
    }
    return k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x48a960):

uint FUN_0048a960(short param_1)

{
  int iVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;

  iVar1 = DAT_0087a470;
  uVar2 = FUN_004d0630();
  if (uVar2 != 0xffffffff) {
    do {
      if (*(int *)((uVar2 & 0xffff) * 0x218 + 4 + *(int *)(iVar1 + 0x34)) == (int)param_1) {
        return uVar2;
      }
      iVar5 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar5;
      if ((-1 < sVar4) && (sVar4 < *(short *)(iVar1 + 0x2e))) {
        psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
        do {
          if (*psVar3 != 0) {
            uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar5;
            break;
          }
          iVar5 = iVar5 + 1;
          psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar1 + 0x22));
        } while ((short)iVar5 < *(short *)(iVar1 + 0x2e));
      }
    } while (uVar2 != 0xffffffff);
  }
  return 0xffffffff;
}
#endif
