// device_blend_animations  (not a Ghidra function; the device object_type_definition's +0x48 entry)
// address 0x44bc20, size 334 bytes
// name confidence: 0.5  rewrite confidence: 0.85
// evidence: the device object_type_definition (0x0069bbf8) +0x48 slot 0x0069bc40 holds 0x44bc20; object node
//   orientation building calls it through object_type_definitions_notify_two_args_0x48 with the object and the
//   orientation buffer. Only reachable through that slot. First-boot track: a campaign level's machines/controls.
// objdump 0x44bc20..0x44bd6d: with a device block on the object's animation graph (+0x30 count, +0x34 first
//   entry) the entry's animation list (+0x54 count, +0x58 int16 indices) drives:
//   [0] position: the device position (+0x208), or 1 - it when device flag bit 0 (+0x1f4) is set, times the frame
//       count (Device tag flags +0x17c bit 0) or frame count - 1. Tag flag bit 1 plays the whole frame
//       ftol'd through animation_overlay_frame_orientations (ESI = animation), otherwise the interpolated
//       animation_overlay_interpolated_frame_orientations (EDI = animation);
//   [1] power: the frame count times the device power (+0x1fc), interpolated.
// blam-cc: stack -> object_index, orientations (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "models.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void animation_overlay_interpolated_frame_orientations(ModelAnimationsAnimation *animation, float frame,
    real_orientation *out_orientations); // 0x4d53f0, blam-cc: EDI -> animation, stack -> (frame, out)
extern void animation_overlay_frame_orientations(ModelAnimationsAnimation *animation, int16_t frame,
    real_orientation *out_orientations); // 0x4d4f90, blam-cc: ESI -> animation, stack -> (frame, out)

void device_blend_animations(datum_index object_index, real_orientation *orientations)
{
    uint8_t *obj = *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
    uint8_t *device_tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *graph = (uint8_t *)tag_instances[*(datum_index *)(device_tag + 0x44) & 0xffff].data;
    uint8_t *entry;
    uint8_t *animations;
    int32_t count;
    int16_t *indices;

    if (*(int32_t *)(graph + 0x30) == 0) {
        return;
    }
    entry = *(uint8_t **)(graph + 0x34);
    if (entry == 0) {
        return;
    }
    animations = *(uint8_t **)(graph + 0x78);
    count = *(int32_t *)(entry + 0x54);
    indices = *(int16_t **)(entry + 0x58);

    if (count > 0 && indices[0] != -1) {
        ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(animations + indices[0] * 0xb4);
        double position = (*(uint32_t *)(obj + 0x1f4) & 1) ? 1.0 - *(float *)(obj + 0x208) : *(float *)(obj + 0x208);
        uint32_t tag_flags = *(uint32_t *)(device_tag + 0x17c);
        int32_t frames = *(int16_t *)((uint8_t *)animation + 0x22);
        float frame;

        if ((tag_flags & 1) == 0) {
            frames = frames - 1;
        }
        frame = (float)((double)frames * position);
        if (tag_flags & 2) {
            animation_overlay_frame_orientations(animation, (int16_t)(int32_t)frame, orientations); // _ftol
        } else {
            animation_overlay_interpolated_frame_orientations(animation, frame, orientations);
        }
    }
    if (count > 1 && indices[1] != -1) {
        ModelAnimationsAnimation *animation = (ModelAnimationsAnimation *)(animations + indices[1] * 0xb4);
        int32_t frames = *(int16_t *)((uint8_t *)animation + 0x22);

        animation_overlay_interpolated_frame_orientations(animation,
            (float)((double)frames * *(float *)(obj + 0x1fc)), orientations);
    }
}
