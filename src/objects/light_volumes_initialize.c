// light_volumes_initialize  (not a Ghidra function; widget type callback)
// address 0x4fe680, size 31 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'mgs2'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4fe680..0x4fe69e: light_volume_instances = game_state_new("light volumes", 0x100, 8).
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *light_volume_instances; // 0x006b8d70
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count

void light_volumes_initialize(void)
{
    light_volume_instances = game_state_new("light volumes", 0x100, 8);
}
