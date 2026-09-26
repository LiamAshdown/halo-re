// biped_placement_offset_centered_pill  (not a Ghidra function: Ghidra folded it into
//   biped_ground_adjust_apply_node_rotations, the thunk at 0x558d40 ends in a jmp right before it)
// address 0x558d50, size 102 bytes
// name confidence: 0.5  rewrite confidence: 0.9
// evidence: the biped object_type_definition (0x0069b4f0) notify_created slot 0x0069b514 (+0x24) holds 0x558d50;
//   object_type_definitions_notify_0x24 calls it as (object_index, argument). Only reachable through that slot.
//   First-boot track: placing a campaign level's bipeds.
// objdump 0x558d50..0x558db5: the object's Biped tag (object_data element +0x8 -> definition datum -> tag data);
//   when its biped flags (+0x2f4) have bit 3 (physics pill centered at origin) and not bit 2 (flying), the
//   placement data's position (+0x18) moves along its up vector (+0x40) by the tag's collision radius (+0x42c).
// blam-cc: stack -> object_index, placement (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

void biped_placement_offset_centered_pill(datum_index object_index, object_placement_data *placement)
{
    uint8_t *object = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    uint8_t *biped_tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
    uint32_t flags = *(uint32_t *)(biped_tag + 0x2f4);
    float radius;

    if ((flags & 8) == 0 || (flags & 4) != 0) {
        return;
    }
    radius = *(float *)(biped_tag + 0x42c);
    placement->position.x = radius * placement->up.i + placement->position.x;
    placement->position.y = radius * placement->up.j + placement->position.y;
    placement->position.z = radius * placement->up.k + placement->position.z;
}
