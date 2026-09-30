// antenna_render_callback  (not a Ghidra function; an object widget type callback)
// address 0x4fac90, size 129 bytes
// name confidence: 0.6  rewrite confidence: 0.85
// evidence: object widget type table (records of 0x28 from 0x0069c010: fourcc, flag, initialize, dispose,
//   clear_disposing_flag, reset, new, delete, update, render) slot 0x69c05c = render entry of 'ant!'. Only reachable
//   through that table. First-boot track: placing a campaign level's objects (weapons carry widgets).
// objdump 0x4fac90..0x4fad10: skipped for a degenerate antenna (+0x05). Otherwise it records the object (+0x0c);
//   when the update counter (+0x06) is above 5 antenna_update_physics(antenna, tag, 0.05) runs three times; the
//   counter is reset and antenna_render_geometry(EDI = tag, stack antenna) draws it.
// blam-cc: stack -> object_index, antenna_index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "fn_objects.h"

extern data_array *antenna_data; // 0x008603ac
extern tag_instance *tag_instances; // 0x0087bc14
extern void antenna_update_physics(antenna *ant, Antenna *antenna_tag, float dt); // 0x4fae10


void antenna_render_callback(datum_index object_index, datum_index antenna_index)
{
    antenna *self = (antenna *)((uint8_t *)antenna_data->data + (antenna_index & 0xffff) * 0x2bc);
    Antenna *tag = (Antenna *)tag_instances[self->definition_tag & 0xffff].data;

    if (self->degenerate) {
        return;
    }
    self->object_index = object_index;
    if (self->frames_since_rendered > 5) {
        antenna_update_physics(self, tag, 0.05f);
        antenna_update_physics(self, tag, 0.05f);
        antenna_update_physics(self, tag, 0.05f);
    }
    self->frames_since_rendered = 0;
    antenna_render_geometry(tag, self);
}
