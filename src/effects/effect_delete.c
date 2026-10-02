// effect_delete  (Ghidra: particle_system_delete_450be0; RENAMED per
// out/phase4/effects_types_notes.md's misattribution table: "0x450be0 particle_system_delete_450be0
// -> effect_delete")
// address 0x450be0, size 208 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/effects.h effect.location_markers[32] (0x5c) and effect_location_marker
// (next_marker 0x04); types/tags.h Effect.locations (count 0x28).
// register convention: __cdecl, effect handle on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *effect_data;          // 0x0087abdc
extern data_array *effect_location_data; // 0x0087abe0
extern tag_instance *tag_instances;      // 0x0087bc14

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680,
    // blam-cc: EDX -> handle, ESI -> array
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510,
    // blam-cc: EAX -> array, EDX -> handle

// Frees every effect_location_marker owned by an effect (across all of its Effect tag's
// locations) and then deletes the effect itself.
void effect_delete(datum_index effect_index)
{
    effect *self = (effect *)datum_get(effect_index, effect_data);

    if (self != 0) {
        Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
        int32_t location_index;

        for (location_index = 0; location_index < (int32_t)tag->locations.count; location_index++) {
            datum_index marker_index = self->location_markers[location_index];

            while (marker_index != k_datum_index_none) {
                datum_index next =
                    ((effect_location_marker *)effect_location_data->data)[(uint16_t)marker_index].next_marker;

                datum_delete(effect_location_data, marker_index);
                marker_index = next;
            }
        }

        datum_delete(effect_data, effect_index);
    }
}

#if 0
Original Ghidra decompilation (0x450be0):

void __cdecl particle_system_delete_450be0(int particle_system_index)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  short sVar4;
  int iVar5;
  short sVar6;
  int iVar7;

  if (((particle_system_index != -1) && (sVar6 = (short)particle_system_index, -1 < sVar6)) &&
     (sVar6 < *(short *)(DAT_0087abdc + 0x20))) {
    iVar7 = (int)*(short *)(DAT_0087abdc + 0x22) * (int)sVar6;
    sVar6 = *(short *)(iVar7 + *(int *)(DAT_0087abdc + 0x34));
    iVar7 = iVar7 + *(int *)(DAT_0087abdc + 0x34);
    if ((sVar6 != 0) &&
       ((sVar4 = (short)((uint)particle_system_index >> 0x10), sVar4 == 0 || (sVar6 == sVar4)))) {
      iVar1 = *(int *)((*(uint *)(iVar7 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      sVar6 = 0;
      if (0 < *(int *)(iVar1 + 0x28)) {
        iVar5 = 0;
        iVar3 = DAT_0087abe0;
        do {
          for (uVar2 = *(uint *)(iVar7 + 0x5c + iVar5 * 4); uVar2 != 0xffffffff;
              uVar2 = *(uint *)((uVar2 & 0xffff) * 0x3c + iVar5 + 4)) {
            iVar5 = *(int *)(iVar3 + 0x34);
            iVar3 = datum_delete();
          }
          sVar6 = sVar6 + 1;
          iVar5 = (int)sVar6;
        } while (iVar5 < *(int *)(iVar1 + 0x28));
      }
      datum_delete();
      return;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
