// hs_allocate_script_node_table  (Ghidra: hs_allocate_script_node_table, already named)
// address 0x483100, size 139 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: allocates a data_new("script node", 0x4a39) table (the "script node" string
// literal and the 0x4a39 count match k_hs_syntax_node_maximum_count in types/hs.h) and, when a
// scenario is loaded whose script_syntax_data tag block is not already exactly
// k_hs_syntax_node_table_size, frees the scenario's old block and hot-swaps the new table into
// Scenario::script_syntax_data (offsets 0x474 size / 0x480 pointer, confirmed in
// out/phase4/hs_types_notes.md). Matches hs_scripts_compile_and_link's call site, which expects
// this to (re)populate hs_syntax_data.
// register convention: __cdecl, no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_memory.h"


extern Scenario *global_scenario;              // 0x00746f8c
extern datum_index global_scenario_index;      // 0x0069e8d4
extern data_array *hs_syntax_data;             // 0x0087a474
extern uint8_t hs_syntax_data_is_local;        // 0x007102fc

// Allocates a fresh "script node" data_array for the compiler/runtime. If a scenario is loaded
// and its script_syntax_data tag block does not already carry exactly this table's byte size,
// the scenario's old (GlobalAlloc'd) block is freed and the new table is hot-swapped into
// Scenario::script_syntax_data; otherwise the allocation is left standalone and
// hs_syntax_data_is_local is set so hs_scripts_free knows to GlobalFree it itself.
void hs_allocate_script_node_table(void)
{
    Scenario *scenario;

    scenario = (global_scenario_index != k_datum_index_none) ? global_scenario : 0;
    if ((scenario == 0) || (scenario->script_syntax_data.size != k_hs_syntax_node_table_size)) {
        hs_syntax_data = data_new(sizeof(hs_syntax_node), "script node", k_hs_syntax_node_maximum_count);
        if (hs_syntax_data != 0) {
            hs_syntax_data->valid = 1;
            data_delete_all(hs_syntax_data);
            if (scenario != 0) {
                GlobalFree((void *)scenario->script_syntax_data.pointer);
                scenario->script_syntax_data.pointer = (uint32_t)hs_syntax_data;
                scenario->script_syntax_data.size = k_hs_syntax_node_table_size;
                return;
            }
            hs_syntax_data_is_local = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x483100):

void __cdecl hs_allocate_script_node_table(void)

{
  uint uVar1;

  uVar1 = (DAT_0069e8d4 == -1) - 1 & DAT_00746f8c;
  if (((uVar1 == 0) || (*(int *)(uVar1 + 0x474) != 0x5ccac)) &&
     (DAT_0087a474 = data_new("script node",0x4a39), DAT_0087a474 != 0)) {
    *(undefined1 *)(DAT_0087a474 + 0x24) = 1;
    data_delete_all();
    if (uVar1 != 0) {
      GlobalFree(*(HGLOBAL *)(uVar1 + 0x480));
      *(int *)(uVar1 + 0x480) = DAT_0087a474;
      *(undefined4 *)(uVar1 + 0x474) = 0x5ccac;
      return;
    }
    DAT_007102fc = 1;
  }
  return;
}
#endif
