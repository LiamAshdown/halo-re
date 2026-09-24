// hs_scripts_reload  (Ghidra: hs_scripts_reload, already named)
// address 0x483250, size 91 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: allocates a fresh syntax-node table, relinks the scenario's already-compiled
// scripts if it has a non-empty script_syntax_data block, resets both object-list data_arrays,
// then re-runs hs_scenario_scripts_initialize -- the standard "scripts changed under us" reset
// path documented in out/phase4/hs_functions.md.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void hs_allocate_script_node_table(void); // 0x00483100, this batch
extern char hs_scripts_compile_and_link(char restore_previous); // 0x00483190, this batch
extern void data_delete_all(data_array *array); // 0x004d0580
extern void hs_scenario_scripts_initialize(void); // 0x00489ef0, outside this batch's assigned range

extern Scenario *global_scenario;                   // 0x00746f8c
extern datum_index global_scenario_index;           // 0x0069e8d4
extern data_array *object_list_header_data;         // 0x0087a464
extern data_array *object_list_reference_data;      // 0x0087a468

// Rebuilds the syntax-node table, relinks the scenario's scripts if any are already compiled,
// resets both object-list containers, and re-initializes the HS runtime for the scenario.
void hs_scripts_reload(void)
{
    Scenario *scenario;

    scenario = (global_scenario_index != k_datum_index_none) ? global_scenario : 0;
    hs_allocate_script_node_table();
    if ((scenario != 0) && (scenario->script_syntax_data.size != 0)) {
        hs_scripts_compile_and_link(0);
    }
    object_list_header_data->valid = 1;
    data_delete_all(object_list_header_data);
    object_list_reference_data->valid = 1;
    data_delete_all(object_list_reference_data);
    hs_scenario_scripts_initialize();
}

#if 0
Original Ghidra decompilation (0x483250):

void __cdecl hs_scripts_reload(void)

{
  uint uVar1;

  uVar1 = (DAT_0069e8d4 == -1) - 1 & DAT_00746f8c;
  hs_allocate_script_node_table();
  if ((uVar1 != 0) && (*(int *)(uVar1 + 0x474) != 0)) {
    hs_scripts_compile_and_link('\0');
  }
  *(undefined1 *)(DAT_0087a464 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0087a468 + 0x24) = 1;
  data_delete_all();
  hs_scenario_scripts_initialize();
  return;
}
#endif
