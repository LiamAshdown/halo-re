// unit_calculate_luminosity  (Ghidra: already named unit_calculate_luminosity)
// address 0x56ec60, size 157 bytes
// name confidence: 0.6 (functions.md summary matches the code exactly)
// rewrite confidence: 0.45
// evidence: types/units.h unit_data.illumination (0x2e0), .attached_light_luminosity (0x2e4);
//   types/objects.h object.position (0x05c), object.location_leaf_index (0x098); standard
//   NTSC luma weights (0.299/0.587/0.114) confirm the RGB sample is being reduced to a
//   single brightness value.
// register convention: unit object index in EDI (unaff_EDI).
//   // blam-cc: EDI -> object_index
// UNSURE: object_try_and_get(3) is called with only its stack argument (the type mask, 3 =
//   biped|vehicle) visible; the object handle it queries (in ECX) is read here as
//   obj->parent_object, matching the "inherit from a parent/attached object" summary.
// UNSURE: object_sum_attached_light_luminance() has no visible argument at this call site;
//   read as taking the same object_index, which is the only value in scope.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_sample_total_lighting_at_point(real_point3d *point, int32_t *leaf_hint,
                                                   real_vector3d *out_rgb); // 0x4f1c20, UNSURE signature
extern real object_sum_attached_light_luminance(uint32_t object_index); // 0x4f1b30, UNSURE signature

// Computes and caches the unit's current light/luminosity value from its RGB color state, or
// inherits both cached values from a parent/attached object when one is present.
void unit_calculate_luminosity(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    object *parent = object_try_and_get(obj->parent_object, 3);

    if (parent == 0) {
        real_vector3d rgb;
        object_sample_total_lighting_at_point(&obj->position, &obj->location_leaf_index, &rgb);
        unit->illumination = rgb.i * 0.299f + rgb.j * 0.587f + rgb.k * 0.114f;
        unit->attached_light_luminosity = object_sum_attached_light_luminance(object_index);
        return;
    }

    unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
    unit->illumination = parent_unit->illumination;
    unit->attached_light_luminosity = parent_unit->attached_light_luminosity;
}

#if 0
Original Ghidra decompilation (0x56ec60):

void unit_calculate_luminosity(void)

{
  int iVar1;
  int iVar2;
  uint unaff_EDI;
  float10 fVar3;
  float local_c;
  float local_8;
  float local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  iVar2 = object_try_and_get(3);
  if (iVar2 == 0) {
    object_sample_total_lighting_at_point(iVar1 + 0x5c,iVar1 + 0x98,&local_c);
    *(float *)(iVar1 + 0x2e0) = local_c * 0.299 + local_8 * 0.587 + local_4 * 0.114;
    fVar3 = (float10)object_sum_attached_light_luminance();
    *(float *)(iVar1 + 0x2e4) = (float)fVar3;
    return;
  }
  *(undefined4 *)(iVar1 + 0x2e0) = *(undefined4 *)(iVar2 + 0x2e0);
  *(undefined4 *)(iVar1 + 0x2e4) = *(undefined4 *)(iVar2 + 0x2e4);
  return;
}
#endif
