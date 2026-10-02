// effect_marker_node_table_resolver  (not a Ghidra function: LAB_00451850, the marker resolver effect_new_with_color
//   and effect_new_on_object_with_node_table hand to effect_rebuild_markers)
// address 0x451850, size 211 bytes
// name confidence: 0.5  rewrite confidence: 0.9
// objdump 0x451850..0x451922: resolves an effect location name against the caller's node table
//   (effect_marker_callback_context 0x6b0adc: +0x00 node index, +0x04 node matrix or 0, +0x08 count, +0x0c names,
//   +0x10 points, +0x14 normals). Every name equal to the location (strcmp) becomes a marker, up to max_count;
//   with no names, an empty location or no match, entry 0 is used once. Returns the number of markers.
// blam-cc: stack -> object_index (unused), location, out, max_count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t *effect_marker_callback_context; // 0x006b0adc

extern void effect_marker_from_node_table(int16_t entry_index, uint8_t *context, object_marker *out); // 0x451930, AX, EBX, stack

int32_t effect_marker_node_table_resolver(uint32_t object_index, const char *location, object_marker *out,
    uint32_t max_count)
{
    uint8_t *context = effect_marker_callback_context;
    int16_t count = 0;

    (void)object_index;
    if (*(char ***)(context + 0xc) != 0 && strlen(location) != 0 && (int16_t)max_count > 0) {
        int16_t i;

        for (i = 0; count < (int16_t)max_count; i++) {
            if (i >= *(int16_t *)(context + 8)) {
                break;
            }
            if (strcmp(location, (*(char ***)(context + 0xc))[i]) == 0) {
                effect_marker_from_node_table(i, context, &out[count]);
                count++;
            }
        }
        if (count != 0) {
            return count;
        }
    }
    effect_marker_from_node_table(0, context, out);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
