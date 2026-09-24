// object_apply_network_placement  (Ghidra: FUN_004f8b70; renamed, Blam-style, not previously
// named)
// address 0x4f8b70, size 86 bytes
// name confidence: 0.3 (a thin, fully-forwarding wrapper around object_set_position_network;
//   functions.md: "Forwards to object_set_position_network; likely a differently-conditioned
//   call site for network position updates")
// rewrite confidence: 0.5
// evidence: types/objects.h object_placement_data.network_vectors (0x58); global 0x008603b0
//   object_data, global 0x0087bc14 tag_instances; callee object_set_position_network (0x4f8bd0,
//   this batch).
// register convention: object index in EAX, network_vectors pointer as the sole stack
//   parameter. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f8b82 and eax,0xffff at
//   entry with the stack read at [esp+0x24] coming later.
//   // blam-cc: EAX -> object_index, stack -> network_vectors
// UNSURE: the destination array at object+0x188 falls inside types/objects.h's documented
//   "untouched by this module" 0x188..0x1b7 range; this call site is new evidence that at
//   least the first 0x30 bytes of it (four vectors' worth of blend weights) are written here,
//   which the struct comment does not yet reflect (out of scope to amend from this batch).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_set_position_network(uint8_t *tag, real_vector3d *dest, int32_t start_index,
    real_vector3d *source, int32_t count); // 0x4f8bd0, this batch

void object_apply_network_placement(uint32_t object_index, real_vector3d *network_vectors) // blam-cc: EAX -> object_index, stack -> network_vectors
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[obj->definition_tag & 0xffff].data;

    object_set_position_network(tag, (real_vector3d *)((uint8_t *)obj + 0x188), 0, network_vectors, 4);
}

#if 0
Original Ghidra decompilation (0x4f8b70):

void FUN_004f8b70(void)

{
  object_set_position_network();
  return;
}
#endif
