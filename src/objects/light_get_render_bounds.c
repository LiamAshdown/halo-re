// light_get_render_bounds  (not a Ghidra function; a light callback for the visible-object collection)
// address 0x4f3530, size 229 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: object_lights_update_all 0x4f0bd0 hands 0x4f3530 to structure_bsp_collect_visible_objects 0x554420 as
//   its bounds callback (0x4f0e3c..0x4f0e50 push the five). Only reachable as that pointer; first-boot track:
//   the first rendered frame needs it.
// objdump 0x4f3530..0x4f3614: reach = tag +0x0c * tag +0x04, times tag +0x24 unless tag flags bit 1 is set. When
//   reach < tag +0x18 the sphere is the light position (+0x30) with radius tag +0x18. Otherwise, by the cone angle
//   (tag +0x14): from pi/2 up (or NaN) the position with radius reach; from pi/4 up, radius reach * tag +0x28 and
//   the centre reach * tag +0x20 along the light's direction (+0x3c); below pi/4, radius and offset both
//   reach / tag +0x20.
// blam-cc: stack -> handle, center_out, radius_out (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

extern data_array *light_data; // 0x00860b14
static uint8_t *light_get(datum_index handle)
{
    return (uint8_t *)light_data->data + (handle & 0xffff) * 0x7c;
}
extern tag_instance *tag_instances; // 0x0087bc14

static void offset_center(real_point3d *out, uint8_t *light, float distance)
{
    out->x = distance * *(float *)(light + 0x3c) + *(float *)(light + 0x30);
    out->y = distance * *(float *)(light + 0x40) + *(float *)(light + 0x34);
    out->z = distance * *(float *)(light + 0x44) + *(float *)(light + 0x38);
}

void light_get_render_bounds(datum_index handle, real_point3d *center_out, float *radius_out)
{
    uint8_t *light = light_get(handle);
    uint8_t *definition = (uint8_t *)tag_instances[*(datum_index *)(light + 4) & 0xffff].data;
    float reach = *(float *)(definition + 0xc) * *(float *)(definition + 4);
    float angle = *(float *)(definition + 0x14);

    if ((definition[0] & 2) == 0) {
        reach = reach * *(float *)(definition + 0x24);
    }
    if (reach < *(float *)(definition + 0x18)) {
        *center_out = *(real_point3d *)(light + 0x30);
        *radius_out = *(float *)(definition + 0x18);
    } else if (!(angle < 1.5707964f)) {
        *center_out = *(real_point3d *)(light + 0x30);
        *radius_out = reach;
    } else if (!(angle < 0.78539819f)) {
        *radius_out = reach * *(float *)(definition + 0x28);
        offset_center(center_out, light, reach * *(float *)(definition + 0x20));
    } else {
        float distance = reach / *(float *)(definition + 0x20);
        *radius_out = distance;
        offset_center(center_out, light, distance);
    }
}
