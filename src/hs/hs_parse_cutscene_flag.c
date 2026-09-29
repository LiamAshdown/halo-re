// hs_parse_cutscene_flag  (not a Ghidra function; an hs primitive parser)
// address 0x487060, size 36 bytes
// name confidence: 0.7  rewrite confidence: 0.90
// evidence: hs_parse_primitive_procedures 0x0065b668 (indexed by hs type) entry 12 (cutscene_flag) = 0x487060; hs_parse_primitive
//   0x486480 calls it with the node pushed (0x486522). Only reachable through that table. First-boot track:
//   hs_compile_postprocess re-parses every primitive when a scenario's scripts load.
// objdump 0x487060: hs_parse_scenario_datum(EAX = node, EBX = 4, ESI = &global_scenario->cutscene_flags (+0x4e4), stride 0x5c).
// blam-cc: stack -> node_index (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


    // 0x486f90, blam-cc: EAX -> node_index, EBX -> name_offset, ESI -> array, stack -> stride
extern Scenario *global_scenario; // 0x00746f8c

char hs_parse_cutscene_flag(datum_index node_index)
{
    return hs_parse_scenario_datum(node_index, 4, &global_scenario->cutscene_flags, 0x5c);
}
