// light_not_marked_this_frame  (not a Ghidra function; a light callback for the visible-object collection)
// address 0x4f3620, size 35 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: object_lights_update_all 0x4f0bd0 hands 0x4f3620 to structure_bsp_collect_visible_objects 0x554420 as
//   its predicate callback (0x4f0e3c..0x4f0e50 push the five). Only reachable as that pointer; first-boot track:
//   the first rendered frame needs it.
// objdump 0x4f3620..0x4f3642: true while the light's frame stamp (+0xc) differs from light_frame_counter.
// blam-cc: stack -> handle (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

extern data_array *light_data; // 0x00860b14
static uint8_t *light_get(datum_index handle)
{
    return (uint8_t *)light_data->data + (handle & 0xffff) * 0x7c;
}
extern int32_t light_frame_counter; // 0x008607c4

uint8_t light_not_marked_this_frame(datum_index handle)
{
    return *(int32_t *)(light_get(handle) + 0xc) != light_frame_counter;
}
