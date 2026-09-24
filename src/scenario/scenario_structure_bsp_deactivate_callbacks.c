// scenario_structure_bsp_deactivate_callbacks  (Ghidra: FUN_0053e660, still unnamed there; name
// from out/phase2/results/scenario_00.json, matching the sibling table's name below)
// address 0x53e660, size 27 bytes
// name confidence: 0.45   rewrite confidence: 0.9
// evidence: calls the 10 consecutive function pointers of structure_bsp_deactivate_procedures
// (&PTR_..._0069e910 in Ghidra's naming) with no arguments and discards any result; its only
// caller is scenario_structure_bsp_switch (0x53eeb0, out of this batch's range), which calls it
// first, before the active bsp index changes -- a pre-switch/deactivate fan-out, mirroring
// scenario_structure_bsp_activate_callbacks (0x53e680) which that same caller runs afterward.
// register convention: __cdecl, no parameters, no return value.

#include "tags.h"
#include "scenario.h"

extern structure_bsp_procedure structure_bsp_deactivate_procedures[k_structure_bsp_deactivate_procedure_count]; // 0x0069e910

// Runs every registered structure-bsp deactivate procedure in table order, ignoring their
// results. Called by scenario_structure_bsp_switch before it unloads the current structure bsp.
void scenario_structure_bsp_deactivate_callbacks(void)
{
    int32_t i;

    for (i = 0; i < k_structure_bsp_deactivate_procedure_count; i++) {
        structure_bsp_deactivate_procedures[i]();
    }
}

#if 0
Original Ghidra decompilation (0x53e660):

void FUN_0053e660(void)

{
  undefined **ppuVar1;
  int iVar2;

  ppuVar1 = &PTR_objects_delete_unparented_of_type_mask_0069e910;
  iVar2 = 10;
  do {
    (*(code *)*ppuVar1)();
    ppuVar1 = ppuVar1 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
