// hs_thread_find_by_script_name  (Ghidra: hs_thread_find_by_script_name, already named)
// address 0x48a9f0, size 181 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: types/hs.h hs_thread (script_index 0x04) and types/tags.h ScenarioScript (name is
//   its first field, a TagString).
// register convention: none (void); name is an ordinary recognized parameter (param_1).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index datum_next(int16_t after_index, data_array *array);
    // blam-cc: DX -> after_index, EDI -> array; memory module, 0x4d0630

extern data_array *hs_thread_data; // 0x0087a470
extern Scenario *global_scenario;  // 0x00746f8c

// Finds the (active or dormant) thread whose associated script's name matches `name`
// case-insensitively, or k_datum_index_none if none does.
datum_index hs_thread_find_by_script_name(char *name)
{
    datum_index thread_handle;
    hs_thread *thread;
    ScenarioScript *scripts;

    scripts = (ScenarioScript *)global_scenario->scripts.pointer;
    thread_handle = datum_next(-1, hs_thread_data);
    while (thread_handle != k_datum_index_none) {
        thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_handle & 0xffff) * 0x218);
        if (thread->script_index != -1 &&
            _stricmp(scripts[thread->script_index].name.string, name) == 0) {
            return thread_handle;
        }
        thread_handle = datum_next((int16_t)thread_handle, hs_thread_data);
    }
    return k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x48a9f0):

uint hs_thread_find_by_script_name(char *param_1)

{
  uint uVar1;
  int iVar2;
  short *psVar3;
  short sVar4;
  int iVar5;

  iVar5 = DAT_0087a470;
  uVar1 = FUN_004d0630();
  do {
    do {
      if (uVar1 == 0xffffffff) {
        return 0xffffffff;
      }
      iVar2 = *(int *)((uVar1 & 0xffff) * 0x218 + *(int *)(iVar5 + 0x34) + 4);
      if ((iVar2 != -1) &&
         (iVar2 = __stricmp((char *)(iVar2 * 0x5c + *(int *)(DAT_00746f8c + 0x4a0)),param_1),
         iVar5 = DAT_0087a470, iVar2 == 0)) {
        return uVar1;
      }
      iVar2 = uVar1 + 1;
      uVar1 = 0xffffffff;
      sVar4 = (short)iVar2;
    } while ((sVar4 < 0) || (*(short *)(iVar5 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar5 + 0x22) + *(int *)(iVar5 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar1 = (int)*psVar3 << 0x10 | (int)(short)iVar2;
        break;
      }
      iVar2 = iVar2 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar5 + 0x22));
    } while ((short)iVar2 < *(short *)(iVar5 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
