// scenario_structure_bsp_activate_callbacks  (Ghidra: FUN_0053e680, still unnamed there; name
// from out/phase2/results/scenario_00.json, matching the sibling table's name above it)
// address 0x53e680, size 27 bytes
// name confidence: 0.45   rewrite confidence: 0.9
// evidence: calls the 13 consecutive function pointers of structure_bsp_activate_procedures
// (&PTR_..._0069e8dc in Ghidra's naming) with no arguments and discards any result; its only
// caller is scenario_structure_bsp_switch (0x53eeb0, out of this batch's range), which calls it
// only after a new bsp index has successfully loaded -- a post-switch/activate fan-out, mirroring
// scenario_structure_bsp_deactivate_callbacks (0x53e660) which that same caller runs beforehand.
// register convention: __cdecl, no parameters, no return value.

#include "tags.h"
#include "scenario.h"

extern structure_bsp_procedure structure_bsp_activate_procedures[k_structure_bsp_activate_procedure_count]; // 0x0069e8dc

// Runs every registered structure-bsp activate procedure in table order, ignoring their
// results. Called by scenario_structure_bsp_switch after the new structure bsp is current.
void scenario_structure_bsp_activate_callbacks(void)
{
    int32_t i;

    for (i = 0; i < k_structure_bsp_activate_procedure_count; i++) {
        structure_bsp_activate_procedures[i]();
    }
}

#if 0
Original Ghidra decompilation (0x53e680):

void FUN_0053e680(void)

{
  undefined **ppuVar1;
  int iVar2;

  ppuVar1 = &PTR_FUN_0069e8dc;
  iVar2 = 0xd;
  do {
    (*(code *)*ppuVar1)();
    ppuVar1 = ppuVar1 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
