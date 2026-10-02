// object_animation_get_frames_remaining  (Ghidra: object_animation_get_frames_remaining,
// already named)
// address 0x4fa9b0, size 101 bytes
// name confidence: 0.55 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Returns the number of frames left in the object's currently
//   playing animation, or 0 if none is active")
// rewrite confidence: 0.4 (zero recorded callers in this module)
// evidence: types/objects.h object (animation_graph 0x0cc, animation_index 0x0d0,
//   animation_frame 0x0d2); global 0x008603b0 object_data, 0x0087bc14 tag_instances.
// register convention: object index in EAX. Consistent with Ghidra's own "in_EAX" and no other
//   input.
//   // blam-cc: EAX -> object_index
// VERIFIED against disassembly 0x4fa9b0..0x4faa14 (2026-09-30): the flag byte at object+0x1f4 (bit 0) gates the branch; the
//   fallback return is ((object_index & 0xffff) * 3) & 0xffff0000 (xor ax,ax on idx*3), preserved literally.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

uint32_t object_animation_get_frames_remaining(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t *extended_flags = (uint8_t *)obj + 0x1f4; // flag byte at +0x1f4 (Ghidra +500), bit 0

    if ((*extended_flags & 1) != 0) {
        void *graph = tag_instances[obj->animation_graph & 0xffff].data;
        uint8_t *nodes = *(uint8_t **)((uint8_t *)graph + 0x78);
        int32_t frame_count = *(int16_t *)(nodes + obj->animation_index * 0xb4 + 0x22);
        int32_t remaining = (frame_count - obj->animation_frame) - 2;
        return (remaining < 1) ? 0 : (uint32_t)remaining;
    }

    return ((object_index & 0xffff) * 3) & 0xffff0000; // preserved literally; always 0, see file header
}

#if 0
Original Ghidra decompilation (0x4fa9b0):

uint object_animation_get_frames_remaining(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((*(byte *)(iVar1 + 500) & 1) != 0) {
    uVar2 = ((int)*(short *)(*(short *)(iVar1 + 0xd0) * 0xb4 +
                             *(int *)(*(int *)((*(uint *)(iVar1 + 0xcc) & 0xffff) * 0x20 + 0x14 +
                                              DAT_0087bc14) + 0x78) + 0x22) -
            (int)*(short *)(iVar1 + 0xd2)) - 2;
    return uVar2 & ((int)uVar2 < 1) - 1;
  }
  return (in_EAX & 0xffff) * 3 & 0xffff0000;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
