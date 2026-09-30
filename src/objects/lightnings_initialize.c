// lightnings_initialize  (not a Ghidra function; widget type callback)
// address 0x4fee80, size 31 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'elec'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4fee80..0x4fee9e: lightning_instances = game_state_new("lightnings", 0x100, 8).
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_saved_games.h"

extern data_array *lightning_instances; // 0x006b8d74


void lightnings_initialize(void)
{
    lightning_instances = game_state_new("lightnings", 0x100, 8);
}
