// hs_global_get_type  (Ghidra: FUN_00483420; renamed per out/phase4/hs_types_notes.md, which
// identifies this as the type-returning mirror of hs_global_get_address/hs_global_get_name)
// address 0x483420, size 42 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: same builtin/scenario branch shape as hs_global_get_name (0x483450), reading
// offset 4 instead of offset 0 -- hs_global_definition::type and ScenarioGlobal::type are both
// int16_t at +4/+0x20 respectively (types/hs.h, types/tags.h).
// register convention: the packed hs_global_reference argument is unrecognized by Ghidra
// (in_EAX); by the blam-cc convention this is the first register slot, EAX.
// UNSURE: Ghidra infers a CONCAT22 32-bit return (high word = garbage upper bits of a pointer
// or index expression that real callers never consume); only the low 16 bits (the type) are
// meaningful, so this is rewritten as a plain hs_type_t return.

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: packed global reference in EAX
// Returns the declared value type of the global variable named by `global` (a builtin
// definition's type, or a scenario global's type).
hs_type_t hs_global_get_type(hs_global_reference global)
{
    hs_global_definition *definition;
    ScenarioGlobal *scenario_global;

    if ((global & k_hs_global_builtin_bit) != 0) {
        definition = hs_global_definitions[global & k_hs_global_index_mask];
        return definition->type;
    }
    scenario_global = (ScenarioGlobal *)global_scenario->globals.pointer + (global & k_hs_global_index_mask);
    return scenario_global->type;
}

#if 0
Original Ghidra decompilation (0x483420):

undefined4 FUN_00483420(void)

{
  uint in_EAX;
  int iVar1;

  if ((char)(in_EAX >> 8) < '\0') {
    return CONCAT22((short)((uint)(&PTR_PTR_0068b398)[in_EAX & 0x7fff] >> 0x10),
                    *(undefined2 *)((&PTR_PTR_0068b398)[in_EAX & 0x7fff] + 4));
  }
  iVar1 = (in_EAX & 0x7fff) * 0x5c;
  return CONCAT22((short)((uint)iVar1 >> 0x10),
                  *(undefined2 *)(iVar1 + 0x20 + *(int *)(DAT_00746f8c + 0x4ac)));
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
