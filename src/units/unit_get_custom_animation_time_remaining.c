// unit_get_custom_animation_time_remaining  (Ghidra: already named
//   unit_get_custom_animation_time_remaining)
// address 0x5701b0, size 106 bytes
// name confidence: 0.5 (functions.md summary matches the code)
// rewrite confidence: 0.9 (VERIFIED against objdump 0x5701b0..0x570219)
// evidence: types/objects.h object.animation_graph (0x0cc), object.animation_index (0x0d0),
//   object.animation_frame (0x0d2); types/units.h unit_data.animation_state (0x2a3,
//   _unit_animation_state_custom_animation = 0x1c); types/cache.h tag_instance.
// register convention: unit object index in EAX (in_EAX). Returns 0 (with the low 16 bits of
//   the input preserved in the high word, per Ghidra's `in_EAX & 0xffff0000`) when there is no
//   active custom animation.
//   // blam-cc: EAX -> object_index
// UNSURE: tag_data+0x78 (the animation graph's "animation block" pointer) and the +0x22 frame
//   count within its 0xb4-byte stride records are not named in any header; this is a different
//   block of the graph tag than the "unit" block types/units.h documents for
//   animation_definition_index (stride 0x64 there, 0xb4 here).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Returns the number of frames remaining in the unit's currently playing custom animation, or 0
// if none is active.
int32_t unit_get_custom_animation_time_remaining(uint32_t object_index)
{
    object *obj;
    int32_t frames_remaining;

    if (object_index == 0xffffffff) {
        return 0;
    }

    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (((unit_data *)((uint8_t *)obj + k_unit_data_offset))->animation_state != _unit_animation_state_custom_animation) {
        return 0;
    }

    {
        uint8_t *graph_tag = tag_instances[obj->animation_graph & 0xffff].data;
        uint8_t *anim_block = *(uint8_t **)(graph_tag + 0x78);
        int16_t frame_count = *(int16_t *)(anim_block + obj->animation_index * 0xb4 + 0x22);

        frames_remaining = (int32_t)frame_count - (int32_t)obj->animation_frame - 2;
    }

    return (frames_remaining < 1) ? 0 : frames_remaining;
}

#if 0
Original Ghidra decompilation (0x5701b0):

uint unit_get_custom_animation_time_remaining(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;

  if (in_EAX != 0xffffffff) {
    uVar2 = in_EAX & 0xffff;
    in_EAX = uVar2 * 3;
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + uVar2 * 0xc);
    if (*(char *)(iVar1 + 0x2a3) == '\x1c') {
      uVar2 = ((int)*(short *)(*(short *)(iVar1 + 0xd0) * 0xb4 +
                               *(int *)(*(int *)((*(uint *)(iVar1 + 0xcc) & 0xffff) * 0x20 + 0x14 +
                                                DAT_0087bc14) + 0x78) + 0x22) -
              (int)*(short *)(iVar1 + 0xd2)) - 2;
      return uVar2 & ((int)uVar2 < 1) - 1;
    }
  }
  return in_EAX & 0xffff0000;
}
#endif
