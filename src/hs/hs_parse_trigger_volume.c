// hs_parse_trigger_volume  (Ghidra: hs_compile_and_evaluate -- misattributed; renamed per
// out/phase4/hs_types_notes.md: this is slot 0x0b of hs_parse_primitive_procedures and forwards
// to hs_parse_scenario_datum with stride 0x60 == sizeof(ScenarioTriggerVolume). The real
// hs_compile_and_evaluate is 0x484400, today named chimera__execute_script, outside this batch.)
// address 0x487030, size 36 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: out/phase4/hs_types_notes.md misattribution note; types/tags.h ScenarioTriggerVolume
//   (size 0x60).
// register convention: node index in EAX, name-field offset in EBX, array descriptor in ESI, all
//   passed straight through unchanged to hs_parse_scenario_datum; no stack arguments of its own.
//   // blam-cc: EAX -> node_index, EBX -> name_offset, ESI -> array (pass-through)

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern char hs_parse_scenario_datum(datum_index node_index, int16_t name_offset,
    TagReflexive *array, int32_t stride); // this module, 0x486f90

// Parses a trigger_volume-typed token: resolves the quoted name against Scenario::trigger_volumes.
char hs_parse_trigger_volume(datum_index node_index, int16_t name_offset, TagReflexive *array)
{
    return hs_parse_scenario_datum(node_index, name_offset, array, sizeof(ScenarioTriggerVolume));
}

#if 0
Original Ghidra decompilation (0x487030):

void hs_compile_and_evaluate(void)

{
  hs_parse_scenario_datum(0x60);
  return;
}
#endif
