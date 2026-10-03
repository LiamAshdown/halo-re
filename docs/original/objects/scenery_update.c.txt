// scenery_update  (not a Ghidra function; the scenery type's +0x34 (update) callback)
// address 0x4fa870, size 86 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition scenery (0x0069ba68) field +0x34 (update); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x4fa870..0x4fa8c5: an animating scenery object (+0x1f4 bit 0) advances its animation state (+0xd0,
//   graph +0xcc, no sound, random stream 1); on the last frame of a non-looping animation the frame index (+0xd2)
//   steps back one so it holds there. The result is always 1.
// blam-cc: stack -> object_index (cdecl); returns AL

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

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}


extern int32_t animation_state_advance(uint32_t animation_graph_tag_index, void *state, int32_t *sound_tag_id,
    int32_t random_stream); // 0x4d48d0, blam-cc: EAX -> graph, ESI -> state, EBX -> sound_tag_id, stack -> stream

uint8_t scenery_update(datum_index object_index)
{
    uint8_t *object = object_get(object_index);

    if ((object[0x1f4] & 1) != 0 &&
        animation_state_advance(*(uint32_t *)(object + 0xcc), object + 0xd0, 0, 1) == 2) { // last frame
        *(int16_t *)(object + 0xd2) -= 1;
    }
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
