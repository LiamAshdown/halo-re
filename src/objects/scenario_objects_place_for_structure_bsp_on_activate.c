// scenario_objects_place_for_structure_bsp_on_activate  (not a Ghidra function; structure bsp activate slot 12)
// address 0x4f4860, size 28 bytes
// name confidence: 0.4  rewrite confidence: 0.95
// evidence: structure_bsp_activate_procedures[12] (0x0069e90c) holds 0x4f4860.
// objdump 0x4f4860..0x4f487b: unless both bytes +0x09 and +0x0b of the globals at 0x006f187c are set,
//   calls scenario_objects_place_for_structure_bsp 0x4f4880 (stack 1).
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"

extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c

extern void scenario_objects_place_for_structure_bsp(uint8_t place); // 0x4f4880, stack -> place

void scenario_objects_place_for_structure_bsp_on_activate(void)
{
    if (cinematic_globals_ptr->in_progress == 0 || cinematic_globals_ptr->suppress_bsp_object_creation == 0) {
        scenario_objects_place_for_structure_bsp(1);
    }
}
