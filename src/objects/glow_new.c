// glow_new  (not a Ghidra function; an object widget type callback)
// address 0x4fcc50, size 237 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c078 = new entry of 'glw!'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fcc50..0x4fcd3c: -1 for a -1 tag. datum_new(glow_data); on success, when the Glow tag's bitmap
//   (+0x150) is of type 3 (sprites), the glow records the tag (+0x224) and the tag's +0x20 word (+0x24c), and
//   +0x228 becomes ftol((sprite right - left) * bitmap width) for sprite 0 of sequence 0 (bitmap data from
//   bitmap_group_sequence_get_bitmap_data(EAX = bitmap tag, EDI = the sprite's bitmap index, stack 0)).
// blam-cc: stack -> glow_tag (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern data_array *glow_data; // 0x008603a0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index,
    int16_t sequence_index); // 0x43f290, blam-cc: EAX -> bitmap_tag_index, EDI -> frame_index, stack -> sequence_index
static void *datum_try_get(data_array *array, datum_index index)
{
    int16_t absolute = (int16_t)index;
    int16_t salt;

    if (absolute < 0 || absolute >= array->last_index) {
        return 0;
    }
    salt = *(int16_t *)((uint8_t *)array->data + absolute * array->size);
    if (salt == 0 || ((int16_t)(index >> 16) != 0 && (int16_t)(index >> 16) != salt)) {
        return 0;
    }
    return (uint8_t *)array->data + absolute * array->size;
}

datum_index glow_new(datum_index glow_tag)
{
    datum_index index;
    uint8_t *self;
    uint8_t *tag;
    datum_index bitmap_tag;
    uint8_t *bitmap;

    if (glow_tag == k_datum_index_none) {
        return k_datum_index_none;
    }
    index = datum_new(glow_data);
    if (index == k_datum_index_none) {
        return index;
    }
    self = (uint8_t *)datum_try_get(glow_data, index);
    tag = (uint8_t *)tag_instances[glow_tag & 0xffff].data;
    bitmap_tag = *(datum_index *)(tag + 0x150);
    bitmap = (uint8_t *)tag_instances[bitmap_tag & 0xffff].data;
    if (*(int16_t *)bitmap == 3) {
        uint8_t *sequence = *(uint8_t **)(bitmap + 0x58);
        uint8_t *sprite = *(uint8_t **)(sequence + 0x38);
        uint8_t *bitmap_data = bitmap_group_sequence_get_bitmap_data(bitmap_tag, *(int16_t *)sprite, 0);

        *(datum_index *)(self + 0x224) = glow_tag;
        *(int16_t *)(self + 0x24c) = *(int16_t *)(tag + 0x20);
        *(int16_t *)(self + 0x228) = (int16_t)(int32_t)(((double)*(float *)(sprite + 0xc) - *(float *)(sprite + 8)) *
            (double)*(int16_t *)(bitmap_data + 4)); // x87: fsub, fimul, _ftol
    }
    return index;
}
