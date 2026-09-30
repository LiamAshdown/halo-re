// unit_set_custom_animation  (Ghidra: already named unit_set_custom_animation)
// address 0x56ebd0, size 53 bytes
// name confidence: 0.55 (functions.md summary matches the code exactly)
// rewrite confidence: 0.75
// evidence: types/objects.h object.animation_graph (0x0cc), object.animation_index (0x0d0),
//   object.animation_frame (0x0d2) -- a plain object-level animation slot, not a unit-only
//   field despite the "custom animation" name; unit_get_custom_animation_time_remaining
//   (0x5701b0) and unit_start_user_animation (0x5702a0) read the same three fields back.
// register convention: unit object index in EAX (in_EAX); graph handle and animation index are
//   stack parameters (param_1, param_2).
//   // blam-cc: EAX -> object_index, stack -> (graph, animation_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_units.h"

extern data_array *object_data; // 0x008603b0

// Sets the unit's active custom animation (graph handle and animation index) and resets its
// playback frame to zero.
void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    obj->animation_graph = graph;
    obj->animation_index = animation_index;
    obj->animation_frame = 0;
}

#if 0
Original Ghidra decompilation (0x56ebd0):

void unit_set_custom_animation(undefined4 param_1,undefined2 param_2)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  *(undefined4 *)(iVar1 + 0xcc) = param_1;
  *(undefined2 *)(iVar1 + 0xd0) = param_2;
  *(undefined2 *)(iVar1 + 0xd2) = 0;
  return;
}
#endif
