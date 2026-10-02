// actor_compute_accuracy_scale  (Ghidra: actor_compute_accuracy_scale, renamed)
// address 0x429620, size 156 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x429620..0x4296bb (0.5 default, the unit tag's +0x384, mode 11 override, mode 9 -> 0.7, floor 0.2).)
// evidence: types/ai.h actor.active_unit_index(0x158)/mode(0x6c)/mode_data; types/objects.h
//   object.definition_tag (0x000). Phase-4 summary: "Computes the actor's effective accuracy
//   scale for the current tick, based on difficulty settings and special-cased combat modes
//   9 and 11."
//   UNSURE: the tag-data float at +0x384 (read off whatever tag the active unit's own object
//   uses) has no established name; mode_data[0x54]/[0x58] are conversation-mode-specific
//   fields with no individual names either.
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> actor_index
float actor_compute_accuracy_scale(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    float scale = 0.5f;

    if (self->active_unit_index != (datum_index)k_datum_index_none) {
        object *unit_object = ((object_header *)object_data->data)[self->active_unit_index & 0xffff].data;
        void *tag_data = tag_instances[unit_object->definition_tag & 0xffff].data;
        scale = *(float *)((uint8_t *)tag_data + 0x384); // UNSURE offset
    }

    if (self->mode == 11 && self->mode_data.raw[0x54] != 0) { // UNSURE offset (mode_data union)
        scale = *(float *)&self->mode_data.raw[0x58]; // UNSURE offset
    }
    if (self->mode == 9) {
        return 0.7f;
    }
    if (!(scale > 0.2f)) { // 0x4296a6: test ah,0x41 / je keeps only a strictly larger value
        scale = 0.2f;
    }
    return scale;
}

#if 0
Original Ghidra decompilation (0x429620):

float10 FUN_00429620(void)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;
  float10 fVar3;

  fVar3 = (float10)0.5;
  iVar2 = (in_EAX & 0xffff) * 0x724;
  uVar1 = *(uint *)(iVar2 + 0x158 + *(int *)(DAT_00880360 + 0x34));
  iVar2 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  if (uVar1 != 0xffffffff) {
    fVar3 = (float10)*(float *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                     (uVar1 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14
                                        + DAT_0087bc14) + 900);
  }
  if ((*(short *)(iVar2 + 0x6c) == 0xb) && (*(char *)(iVar2 + 0xf0) != '\0')) {
    fVar3 = (float10)*(float *)(iVar2 + 0xf4);
  }
  if (*(short *)(iVar2 + 0x6c) == 9) {
    return (float10)0.7;
  }
  if (fVar3 <= (float10)0.2) {
    fVar3 = (float10)0.2;
  }
  return fVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
