// unit_set_custom_animation_frame  (Ghidra: already named unit_set_custom_animation_frame)
// address 0x570220, size 124 bytes
// name confidence: 0.5 (functions.md summary matches the code)
// rewrite confidence: 0.35
// evidence: types/objects.h object.animation_graph (0x0cc), object.animation_index (0x0d0),
//   object.animation_frame (0x0d2); callee unit_start_user_animation (0x5702a0, this batch).
// register convention: unit object index in ECX (in_ECX); the two stack parameters are
//   forwarded to unit_start_user_animation as its graph-tag-id and object-index arguments,
//   which is the best available reading given both functions' register-only inputs.
//   // blam-cc: ECX -> unit_index, stack -> (graph_tag_id, frame)
// UNSURE: unit_start_user_animation's real third argument (warn_if_missing) is not visible at
//   this call site; passed as 0 (false).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t unit_start_user_animation(uint32_t object_index, datum_index graph_tag_id,
                                          uint8_t warn_if_missing); // 0x5702a0

// Sets the current playback frame of the unit's active custom animation, if valid (i.e. if
// unit_start_user_animation reports the animation is already active/continuing, and the frame
// falls within the graph's animation length).
uint8_t unit_set_custom_animation_frame(uint32_t unit_index, datum_index graph_tag_id, int16_t frame)
{
    object *obj;
    uint8_t *graph_tag;

    if (unit_start_user_animation(unit_index, graph_tag_id, 0) == 0) {
        return 0;
    }

    obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    if (frame < 0) {
        return 0;
    }

    graph_tag = tag_instances[obj->animation_graph & 0xffff].data;
    {
        uint8_t *anim_block = *(uint8_t **)(graph_tag + 0x78);
        int16_t frame_count = *(int16_t *)(anim_block + obj->animation_index * 0xb4 + 0x22);
        if (frame >= frame_count) {
            return 0;
        }
    }

    obj->animation_frame = frame;
    return 1;
}

#if 0
Original Ghidra decompilation (0x570220):

uint unit_set_custom_animation_frame(undefined4 param_1,short param_2)

{
  uint uVar1;
  uint in_ECX;

  uVar1 = unit_start_user_animation();
  if ((((char)uVar1 != '\0') &&
      (uVar1 = *(uint *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc), -1 < param_2)
      ) && (param_2 < *(short *)(*(int *)(*(int *)((*(uint *)(uVar1 + 0xcc) & 0xffff) * 0x20 + 0x14
                                                  + DAT_0087bc14) + 0x78) + 0x22 +
                                *(short *)(uVar1 + 0xd0) * 0xb4))) {
    *(short *)(uVar1 + 0xd2) = param_2;
    return CONCAT31((int3)(uVar1 >> 8),1);
  }
  return uVar1 & 0xffffff00;
}
#endif
