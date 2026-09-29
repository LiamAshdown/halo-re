// hs_scripts_free  (Ghidra: hs_scripts_free, already named)
// address 0x4832b0, size 87 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: garbage collects then tears down hs_syntax_data (GlobalFree only when
// hs_syntax_data_is_local is set, i.e. it is not the scenario's own tag block), calls the
// unrecovered 0x48a130 (documented in out/phase4/hs_types_notes.md as
// "hs_dispose_dynamic_globals ... deletes everything from 0x1eb upward"), and marks both
// object-list data_arrays invalid.
// register convention: __cdecl, no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include <string.h>


extern data_array *hs_syntax_data;                  // 0x0087a474
extern uint8_t hs_syntax_data_is_local;             // 0x007102fc
extern data_array *object_list_header_data;         // 0x0087a464
extern data_array *object_list_reference_data;      // 0x0087a468

// Tears down the active HS syntax-node table (freeing it only if hs owns the allocation rather
// than the scenario tag) and marks both object-list containers unused.
void hs_scripts_free(void)
{
    data_array *nodes;

    nodes = hs_syntax_data;
    if (hs_syntax_data != 0) {
        hs_syntax_node_garbage_collect();
        if (hs_syntax_data_is_local != 0) {
            nodes->valid = 0;
            memset(nodes, 0, sizeof(*nodes)); // the original zeroes all 0x38 bytes word by word
            GlobalFree(nodes);
            hs_syntax_data_is_local = 0;
        }
        hs_syntax_data = 0;
    }
    hs_dispose_dynamic_globals();
    object_list_header_data->valid = 0;
    object_list_reference_data->valid = 0;
}

#if 0
Original Ghidra decompilation (0x4832b0):

void __cdecl hs_scripts_free(void)

{
  undefined4 *hMem;
  int iVar1;
  undefined4 *puVar2;

  hMem = DAT_0087a474;
  if (DAT_0087a474 != (undefined4 *)0x0) {
    hs_syntax_node_garbage_collect();
    if (DAT_007102fc != '\0') {
      *(undefined1 *)(hMem + 9) = 0;
      puVar2 = hMem;
      for (iVar1 = 0xe; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar2 = 0;
        puVar2 = puVar2 + 1;
      }
      GlobalFree(hMem);
      DAT_007102fc = '\0';
    }
    DAT_0087a474 = (undefined4 *)0x0;
  }
  FUN_0048a130();
  iVar1 = DAT_0087a468;
  *(undefined1 *)(DAT_0087a464 + 0x24) = 0;
  *(undefined1 *)(iVar1 + 0x24) = 0;
  return;
}
#endif
