// hs_global_get_name  (Ghidra: hs_global_get_address; renamed per out/phase4/hs_types_notes.md
// -- it returns the global's name, not its address: "for a builtin it returns
// *(char **)definition (field +0x00, the name) and for a scenario global the base of the
// ScenarioGlobal, whose first field is the TagString name. hs_parse_variable passes the result
// straight into a %s.")
// address 0x483450, size 42 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: see above; same builtin/scenario branch shape as hs_global_get_type (0x483420).
// register convention: the packed hs_global_reference argument is unrecognized by Ghidra
// (in_EAX); by the blam-cc convention this is the first register slot, EAX.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: packed global reference in EAX
// Returns the name of the global variable named by `global` (a builtin definition's name
// string, or a scenario global's TagString name).
char *hs_global_get_name(hs_global_reference global)
{
    hs_global_definition *definition;
    ScenarioGlobal *scenario_global;

    if ((global & k_hs_global_builtin_bit) != 0) {
        definition = hs_global_definitions[global & k_hs_global_index_mask];
        return definition->name;
    }
    scenario_global = (ScenarioGlobal *)global_scenario->globals.pointer + (global & k_hs_global_index_mask);
    return scenario_global->name.string;
}

#if 0
Original Ghidra decompilation (0x483450):

int hs_global_get_address(void)

{
  uint in_EAX;

  if ((char)(in_EAX >> 8) < '\0') {
    return *(int *)(&PTR_PTR_0068b398)[in_EAX & 0x7fff];
  }
  return (in_EAX & 0x7fff) * 0x5c + *(int *)(DAT_00746f8c + 0x4ac);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
