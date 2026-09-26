// light_volumes_clear_disposing_flag  (not a Ghidra function; widget type callback)
// address 0x4fe6c0, size 14 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'mgs2'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4fe6c0..0x4fe6cd: clears the pool's disposing byte (+0x24) when it exists.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *light_volume_instances; // 0x006b8d70

void light_volumes_clear_disposing_flag(void)
{
    if (light_volume_instances != 0) {
        light_volume_instances->valid = 0;
    }
}
