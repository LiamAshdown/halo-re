// flag_render_callback  (not a Ghidra function; an object widget type callback)
// address 0x4fb980, size 127 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c034 = render entry of 'flag'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fb980..0x4fb9fe: the flag records the object (+0x08); when its update counter (+0x06) is above 5 or
//   it was never simulated (+0x03), flag_cloth_update(flag, tag, 5.0) runs and +0x03 is set; the counter is
//   reset; unless the flag is invalid (+0x02), flag_render(EAX = tag, stack: flag, arg3, arg4).
// UNSURE: flag_render.c's own signature is unverified (it had no known caller); arg3/arg4 are mapped onto its
//   second and fourth parameters by its own register notes (stack, EAX, stack).
// blam-cc: stack -> object_index, flag_index, arg3, arg4 (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *flag_data; // 0x008603a8
extern tag_instance *tag_instances; // 0x0087bc14
extern void flag_cloth_update(flag *entry, Flag *tag, float dt); // 0x4fbae0
extern void flag_render(uint32_t *entry, uint32_t *submission_block, Flag *tag, uint8_t *second_geometry); // 0x4fc350

void flag_render_callback(datum_index object_index, datum_index flag_index, uint32_t arg3, uint32_t arg4)
{
    uint8_t *self = (uint8_t *)flag_data->data + (flag_index & 0xffff) * 0x16bc;
    Flag *tag = (Flag *)tag_instances[*(datum_index *)(self + 0xc) & 0xffff].data;

    *(datum_index *)(self + 8) = object_index;
    if (*(int16_t *)(self + 6) > 5 || self[3] == 0) {
        flag_cloth_update((flag *)self, tag, 5.0f);
        self[3] = 1;
    }
    *(int16_t *)(self + 6) = 0;
    if (self[2] == 0) {
        flag_render((uint32_t *)self, (uint32_t *)arg3, tag, (uint8_t *)arg4);
    }
}
