// light_volumes_dispose  (not a Ghidra function; widget type callback)
// address 0x4fe6a0, size 22 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'mgs2'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4fe6a0..0x4fe6b5: when the pool exists, its disposing byte (+0x24) is set and data_delete_all runs.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *light_volume_instances; // 0x006b8d70
extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array

void light_volumes_dispose(void)
{
    if (light_volume_instances != 0) {
        light_volume_instances->valid = 1;
        data_delete_all(light_volume_instances);
    }
}
