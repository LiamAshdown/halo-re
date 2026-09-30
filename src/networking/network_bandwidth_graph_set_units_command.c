// network_bandwidth_graph_set_units_command  (Ghidra: FUN_004d7d90; named per
// out/phase2/results/networking_01.json / symbols/review_queue.txt)
// address 0x4d7d90, size 73 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary ("Parses a units/direction
// command-line pair (e.g. 'bytes sent') and, if valid, (re)configures the network bandwidth
// debug graph to display them"); out/phase2/results/networking_01.json evidence ("only
// proceeds if the overlay is enabled (DAT_00710305), resolves a units token via
// network_bandwidth_unit_name_to_index (0x4d8a20, matches 'bytes'/'packets') and a direction
// token via network_bandwidth_direction_name_to_index (0x4d8a50), and if both parse, calls
// network_bandwidth_graph_instance_init (0x4d7de0) with them"); types/networking.h
// network_bandwidth_graph_globals.
// register convention: disassembly (`mov edi,ecx` before the first call, no incoming push of
// ecx) shows the units-name string arrives in ECX; the direction-name string is the sole
// Ghidra-recognized stack parameter (`mov edi,[esp+0x10]`, read after two pushes). Both
// 0x4d8a20 and 0x4d8a50 are outside this batch (addresses above 0x4d8620); their own
// disassembly (`push edi` immediately before their shared string-compare callee, with no
// earlier setup of edi in either function) shows they read their one argument from EDI, so
// each of this function's two calls first moves its string pointer into EDI.
// // blam-cc: ECX -> units_name, stack -> direction_name

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern network_bandwidth_graph network_bandwidth_graph_globals; // 0x00719ce0
extern uint8_t network_bandwidth_overlay_enabled; // 0x00710305

extern int32_t network_bandwidth_unit_name_to_index(const char *name); // 0x4d8a20, blam-cc: EDI -> name, outside this batch
extern int32_t network_bandwidth_direction_name_to_index(const char *name); // 0x4d8a50, blam-cc: EDI -> name, outside this
    // batch; evidence names this network_bandwidth_direction_name_to_index


// blam-cc: ECX -> units_name, stack -> direction_name
uint32_t network_bandwidth_graph_set_units_command(const char *units_name, const char *direction_name)
{
    int32_t units_index;
    int32_t direction_index;

    if (!network_bandwidth_overlay_enabled) {
        return 0;
    }

    units_index = network_bandwidth_unit_name_to_index(units_name);
    direction_index = network_bandwidth_direction_name_to_index(direction_name);
    if (units_index != -1 && direction_index != -1) {
        network_bandwidth_graph_instance_init(&network_bandwidth_graph_globals, units_index, direction_index);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4d7d90):

undefined4 FUN_004d7d90(void)

{
  int iVar1;
  int iVar2;

  if (DAT_00710305 == '\0') {
    return 0;
  }
  iVar1 = network_bandwidth_unit_name_to_index();
  iVar2 = FUN_004d8a50();
  if ((iVar1 != -1) && (iVar2 != -1)) {
    network_bandwidth_graph_instance_init(iVar1,iVar2);
    return 1;
  }
  return 0;
}
#endif
