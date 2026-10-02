// effect_reattach_markers_for_object  (Ghidra: FUN_00450cb0, still unnamed there; named from its
// own summary in out/phase4/effects_functions.md: "Re-binds every particle system attached to a
// given object to a new first-person-weapon marker index")
// address 0x450cb0, size 159 bytes
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: types/effects.h effect (object_index 0x3c, first_person_weapon_index 0x4c);
// src/memory/datum_next.c is byte-for-byte the tail scan this function inlines.
// register convention: __cdecl, both arguments are Ghidra's own recognised stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *effect_data; // 0x0087abdc

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630,
    // memory module; blam-cc: DX -> after_index, EDI -> array
extern void effect_rebuild_markers(effect *self,
    int32_t (*resolve_marker)(uint32_t, const char *, object_marker *, uint32_t)); // 0x451710, this module
extern int32_t first_person_weapon_get_marker_data(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count); // outside this batch's range

// For every live effect attached to `object_index`, updates its first-person weapon index and
// re-resolves its first-person markers.
void effect_reattach_markers_for_object(int16_t first_person_weapon_index, datum_index object_index)
{
    datum_index effect_index = datum_next(-1, effect_data);

    while (effect_index != k_datum_index_none) {
        effect *self = &((effect *)effect_data->data)[(uint16_t)effect_index];

        if (self->object_index == object_index) {
            self->first_person_weapon_index = first_person_weapon_index;
            effect_rebuild_markers(self, first_person_weapon_get_marker_data);
        }

        effect_index = datum_next((int16_t)effect_index, effect_data);
    }
}

#if 0
Original Ghidra decompilation (0x450cb0):

void FUN_00450cb0(undefined2 param_1,int param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  short *psVar4;
  short sVar5;
  int iVar6;

  iVar6 = DAT_0087abdc;
  uVar1 = datum_next();
  do {
    do {
      if (uVar1 == 0xffffffff) {
        return;
      }
      iVar2 = (uVar1 & 0xffff) * 0xfc;
      iVar3 = iVar2 + *(int *)(iVar6 + 0x34);
      if (*(int *)(iVar2 + 0x3c + *(int *)(iVar6 + 0x34)) == param_2) {
        *(undefined2 *)(iVar3 + 0x4c) = param_1;
        FUN_00451710(iVar3,first_person_weapon_get_marker_data);
        iVar6 = DAT_0087abdc;
      }
      iVar2 = uVar1 + 1;
      uVar1 = 0xffffffff;
      sVar5 = (short)iVar2;
    } while ((sVar5 < 0) || (*(short *)(iVar6 + 0x2e) <= sVar5));
    psVar4 = (short *)((int)sVar5 * (int)*(short *)(iVar6 + 0x22) + *(int *)(iVar6 + 0x34));
    do {
      if (*psVar4 != 0) {
        uVar1 = (int)*psVar4 << 0x10 | (int)(short)iVar2;
        break;
      }
      iVar2 = iVar2 + 1;
      psVar4 = (short *)((int)psVar4 + (int)*(short *)(iVar6 + 0x22));
    } while ((short)iVar2 < *(short *)(iVar6 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
