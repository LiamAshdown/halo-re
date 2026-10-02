// effect_release_first_person_markers  (Ghidra: FUN_00450d50; named per
// out/phase4/effects_types_notes.md, which refers to this address directly: "effect_marker_next
// 0x453180 ... and effect_release_first_person_markers 0x450d50 drops the first person entries")
// address 0x450d50, size 304 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/effects.h effect_marker_constants (k_effect_marker_first_person_bit 0x8000)
// and effect_location_marker (marker_index 0x02, next_marker 0x04); types/tags.h
// Effect.locations (count 0x28).
// register convention: __cdecl, the first-person weapon index is Ghidra's own recognised stack
// parameter.

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

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630,
    // memory module; blam-cc: DX -> after_index, EDI -> array
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510,
    // blam-cc: EAX -> array, EDX -> handle

// Invalidates and removes every first-person marker reference (a marker_index with the
// k_effect_marker_first_person_bit set) from every live effect using `first_person_weapon_index`,
// and clears that effect's first_person_weapon_index -- used when a first-person weapon's marker
// table is being freed.
void effect_release_first_person_markers(int16_t first_person_weapon_index)
{
    datum_index effect_index = datum_next(-1, effect_data);

    while (effect_index != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)effect_index];
        Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;

        if (self->first_person_weapon_index == first_person_weapon_index) {
            int32_t location_index;

            for (location_index = 0; location_index < (int32_t)tag->locations.count; location_index++) {
                datum_index *link = &self->location_markers[location_index];

                while (*link != k_datum_index_none) {
                    effect_location_marker *marker =
                        &((effect_location_marker *)effect_location_data->data)[(uint16_t)*link];

                    if (marker->marker_index == 0xffff || (int16_t)marker->marker_index >= 0) {
                        link = &marker->next_marker;
                    } else {
                        datum_index next = marker->next_marker;
                        datum_delete(effect_location_data, *link);
                        *link = next;
                    }
                }
            }

            self->first_person_weapon_index = -1;
        }

        effect_index = datum_next((int16_t)effect_index, effect_data);
    }
}

#if 0
Original Ghidra decompilation (0x450d50):

void FUN_00450d50(short param_1)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  short *psVar5;
  short sVar6;
  uint *puVar7;
  int iVar8;
  int iVar9;
  uint uVar10;

  iVar1 = DAT_0087abdc;
  uVar2 = datum_next();
  iVar3 = DAT_0087abe0;
  do {
    do {
      iVar8 = iVar1;
      if (uVar2 == 0xffffffff) {
        return;
      }
      iVar9 = (uVar2 & 0xffff) * 0xfc + *(int *)(iVar8 + 0x34);
      iVar1 = *(int *)((*(uint *)(iVar9 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(short *)(iVar9 + 0x4c) == param_1) {
        iVar4 = 0;
        sVar6 = 0;
        if (0 < *(int *)(iVar1 + 0x28)) {
          do {
            puVar7 = (uint *)(iVar9 + 0x5c + iVar4 * 4);
            uVar10 = *puVar7;
            iVar8 = DAT_0087abdc;
            while (DAT_0087abdc = iVar8, uVar10 != 0xffffffff) {
              iVar8 = (*puVar7 & 0xffff) * 0x3c + *(int *)(iVar3 + 0x34);
              if ((*(short *)(iVar8 + 2) == -1) || (-1 < *(short *)(iVar8 + 2))) {
                puVar7 = (uint *)(iVar8 + 4);
              }
              else {
                iVar3 = datum_delete();
                *puVar7 = *(uint *)(iVar8 + 4);
              }
              iVar8 = DAT_0087abdc;
              uVar10 = *puVar7;
            }
            sVar6 = sVar6 + 1;
            iVar4 = (int)sVar6;
          } while (iVar4 < *(int *)(iVar1 + 0x28));
        }
        *(undefined2 *)(iVar9 + 0x4c) = 0xffff;
      }
      uVar10 = 0xffffffff;
      iVar9 = uVar2 + 1;
      sVar6 = (short)iVar9;
      uVar2 = uVar10;
      iVar1 = DAT_0087abdc;
    } while ((sVar6 < 0) || (*(short *)(iVar8 + 0x2e) <= sVar6));
    psVar5 = (short *)((int)sVar6 * (int)*(short *)(iVar8 + 0x22) + *(int *)(DAT_0087abdc + 0x34));
    do {
      if (*psVar5 != 0) {
        uVar2 = (int)*psVar5 << 0x10 | (int)(short)iVar9;
        break;
      }
      iVar9 = iVar9 + 1;
      psVar5 = (short *)((int)psVar5 + (int)*(short *)(iVar8 + 0x22));
    } while ((short)iVar9 < *(short *)(iVar8 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
