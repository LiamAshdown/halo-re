// effect_new  (Ghidra: particle_system_new; RENAMED per out/phase4/effects_types_notes.md's
// misattribution table: "0x451500 particle_system_new -> effect_new -- and its two arguments are
// reversed: argument 1 is the effe tag index, not an object index")
// address 0x451500, size 246 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump 0x451500..0x4515f5 (event 0 is xor edi,edi))
// evidence: types/effects.h effect (definition_index 0x04, creator_object_index 0x40,
// first_person_weapon_index 0x4c, flags); types/tags.h Effect (flags EffectFlags bit 2
// must_be_deterministic_pc, events TagReflexive at 0x34).
// register convention: __cdecl -- Ghidra recovered the full stack signature, but with the first
// two arguments' roles swapped from what their Ghidra names (object_index, definition_id)
// suggest; see the rename note above.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *effect_data;     // 0x0087abdc
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index datum_new(data_array *array); // 0x4d0480, memory module; blam-cc: array in EDX
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510,
    // blam-cc: EAX -> array, EDX -> handle
extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630,
    // memory module; blam-cc: DX -> after_index, EDI -> array
extern void effect_start_event(datum_index effect_handle, int16_t event_index); // 0x451660,
    // this module; blam-cc: EAX -> effect_handle, EDI -> event_index

// Allocates a new effect for `definition_index`, refusing effects flagged
// must_be_deterministic_pc unless `force_create` is set or, once the pool is exhausted,
// evicting the first non-deterministic effect found (via a full-table scan) to make room.
// Requires the tag to have at least one event.
datum_index effect_new(datum_index definition_index, datum_index creator_object_index,
    uint8_t force_create)
{
    datum_index handle = k_datum_index_none;

    if (definition_index != k_datum_index_none) {
        Effect *tag = (Effect *)tag_instances[(uint16_t)definition_index].data;

        if ((force_create != 0 || (tag->flags & 4) == 0) && tag->events.count > 0) {
            handle = datum_new(effect_data);

            if (handle == k_datum_index_none) {
                if ((tag->flags & 4) != 0) {
                    handle = datum_next(-1, effect_data);
                    if (handle == k_datum_index_none) {
                        return k_datum_index_none;
                    }
                    for (;;) {
                        effect *candidate = &((effect *)effect_data->data)[(uint16_t)handle];
                        Effect *candidate_tag =
                            (Effect *)tag_instances[(uint16_t)candidate->definition_index].data;

                        if ((candidate_tag->flags & 4) == 0) {
                            break;
                        }
                        handle = datum_next((int16_t)handle, effect_data);
                        if (handle == k_datum_index_none) {
                            return k_datum_index_none;
                        }
                    }
                    datum_delete(effect_data, handle);
                    handle = datum_new(effect_data);
                }
                if (handle == k_datum_index_none) {
                    return k_datum_index_none;
                }
            }

            {
                effect *self = &((effect *)effect_data->data)[(uint16_t)handle];

                self->definition_index = definition_index;
                self->creator_object_index = creator_object_index;
                self->first_person_weapon_index = -1;
                self->flags = 0;
                effect_start_event(handle, 0); // UNSURE: event_index elided (unaff_DI in the
                    // original); 0 (the first event) is a guess
            }
        }
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x451500):

uint __cdecl particle_system_new(uint object_index,uint definition_id,char force_create)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  int iVar5;

  iVar3 = DAT_0087bc14;
  iVar5 = DAT_0087abdc;
  uVar4 = 0xffffffff;
  if ((object_index != 0xffffffff) &&
     (((pbVar1 = *(byte **)((object_index & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
       force_create != '\0' || ((*pbVar1 & 4) == 0)) && (0 < *(int *)(pbVar1 + 0x34))))) {
    uVar4 = datum_new();
    if (uVar4 == 0xffffffff) {
      if ((*pbVar1 & 4) != 0) {
        uVar4 = datum_next();
        if (uVar4 == 0xffffffff) {
          return 0xffffffff;
        }
        iVar2 = *(int *)(iVar5 + 0x34);
        while ((**(byte **)((*(uint *)((uVar4 & 0xffff) * 0xfc + 4 + iVar2) & 0xffff) * 0x20 + 0x14
                           + iVar3) & 4) != 0) {
          uVar4 = datum_next();
          if (uVar4 == 0xffffffff) {
            return 0xffffffff;
          }
        }
        datum_delete();
        uVar4 = datum_new();
      }
      if (uVar4 == 0xffffffff) {
        return 0xffffffff;
      }
    }
    iVar5 = (uVar4 & 0xffff) * 0xfc + *(int *)(iVar5 + 0x34);
    *(uint *)(iVar5 + 4) = object_index;
    *(uint *)(iVar5 + 0x40) = definition_id;
    *(undefined2 *)(iVar5 + 0x4c) = 0xffff;
    *(undefined2 *)(iVar5 + 2) = 0;
    FUN_00451660();
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
