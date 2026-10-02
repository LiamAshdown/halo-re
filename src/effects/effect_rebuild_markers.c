// effect_rebuild_markers  (Ghidra: FUN_00451710; named per out/phase4/effects_types_notes.md,
// which refers to this address directly: "effect_rebuild_markers 0x451710 drives that per
// location" and "the object_marker array effect_rebuild_markers 0x451710 reserves 0x6c0 bytes
// for, which is 0x10 * sizeof(object_marker)")
// address 0x451710, size 182 bytes
// name confidence: 0.45   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/effects.h effect.location_markers / effect_marker_new 0x4517d0; types/tags.h
// Effect.locations (count 0x28, pointer 0x2c, EffectLocation stride 0x20).
// register convention: effect* and the marker-resolving callback are Ghidra's own recognised
// stack parameters (param_1, param_2).
// UNSURE: the callback's per-call output slot inside the 16-entry object_marker buffer (which
// FUN_004517d0 then reads from) is never shown as an explicit address in the decompile -- Ghidra
// only shows the buffer's base address being passed to the resolver once per location, not
// per-marker. Indexing `markers[marker_slot]` here is the only sensible reading, but the exact
// register that carries it to effect_marker_new could not be confirmed.
// UNSURE: the resolver callback's signature (object_get_node_local_transform or
// first_person_weapon_get_marker_data) is assumed to be
// int16_t (*)(datum_index object_index, void *location, object_marker *out, int32_t max_count)
// by analogy with object_get_node_local_transform's own established 4-argument shape; the
// EffectLocation field passed as `location` is not otherwise typed here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index effect_marker_new(effect *self, int16_t location_index, object_marker *resolved_marker,
    uint8_t first_person); // 0x4517d0, this module
extern int32_t first_person_weapon_get_marker_data(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count); // outside this batch's range

typedef int32_t (*effect_marker_resolver)(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count);

// For each EffectLocation of the effect's tag, resolves up to 16 matching object markers through
// `resolve_marker` and creates an effect_location_marker for each one found.
void effect_rebuild_markers(effect *self, effect_marker_resolver resolve_marker)
{
    Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;
    uint8_t *locations = (uint8_t *)tag->locations.pointer;
    int16_t location_index;

    for (location_index = 0; (int32_t)location_index < (int32_t)tag->locations.count; location_index++) {
        object_marker markers[16];
        int16_t count = resolve_marker(self->object_index, (const char *)(locations + location_index * 0x20),
            markers, 0x10);
        int16_t i;
        // 0x45176e: cmp [resolver],0x492ad0 -- the ORIGINAL's address; original callers pass that, C callers the
        // rewrite's own function, so accept either
        uint8_t first_person = (uint8_t)((uint32_t)resolve_marker == 0x492ad0u ||
            resolve_marker == first_person_weapon_get_marker_data);

        for (i = 0; i < count; i++) {
            // 0x451799: the inner loop stops at the first marker datum_new cannot allocate
            if (effect_marker_new(self, location_index, &markers[i], first_person) == k_datum_index_none) {
                break;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x451710):

void FUN_00451710(int param_1,code *param_2)

{
  int iVar1;
  short sVar2;
  int iVar3;
  short sVar4;
  int local_6c8;
  undefined1 local_6c0 [1728];

  iVar1 = *(int *)((*(uint *)(param_1 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = 0;
  local_6c8 = 0;
  if (0 < *(int *)(iVar1 + 0x28)) {
    do {
      sVar2 = (*param_2)(*(undefined4 *)(param_1 + 0x3c),iVar3 * 0x20 + *(int *)(iVar1 + 0x2c),
                         local_6c0,0x10);
      sVar4 = 0;
      if (0 < sVar2) {
        do {
          iVar3 = FUN_004517d0(param_1,local_6c8,
                               CONCAT31((int3)((uint)iVar1 >> 8),
                                        param_2 == first_person_weapon_get_marker_data));
          if (iVar3 == -1) break;
          sVar4 = sVar4 + 1;
        } while (sVar4 < sVar2);
      }
      local_6c8 = local_6c8 + 1;
      iVar3 = (int)(short)local_6c8;
    } while (iVar3 < *(int *)(iVar1 + 0x28));
  }
  return;
}
#endif
