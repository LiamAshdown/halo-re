// unit_trigger_material_hit_effect  (Ghidra: FUN_0056f210; renamed from the phase2 proposal)
// address 0x56f210, size 183 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary)
// rewrite confidence: 0.4
// evidence: types/units.h globals note "+0x194/0x198 the material table (stride 0x374)";
//   callee sound_start_at_object_marker established elsewhere in this module as an effect-trigger helper.
// register convention: material index in AX (in_AX), a tag id in ECX (in_ECX) -- no stack
//   arguments at all.
//   // blam-cc: AX -> material_index, ECX -> unit_tag_id
// UNSURE: DAT_006e3208 is a fallback material-effect record this function initializes once
//   (via the DAT_00721e4c latch and DAT_006e3578) when the index is out of range; only its
//   +0x370 field (the effect tag) is touched here, so it is declared as an opaque byte array.
// UNSURE: tag+0x120 (relative to whatever tag unit_tag_id names) lands in objects.h's
//   undocumented _pad_110 padding; kept as a raw offset.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern uint8_t *globals_tag_data;   // 0x00746fa0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t DAT_00721e4c;        // UNSURE: one-time-init latch for the fallback record
extern int32_t DAT_006e3578;        // UNSURE: a field of the fallback record, zeroed on init
extern uint8_t DAT_006e3208[0x374]; // UNSURE: fallback material-effect record, this module only
                                     //   reads +0x370 of it

extern datum_index sound_start_at_object_marker(datum_index effect_index, void *position, float intensity,
                                 uint32_t flag); // 0x543ce0, UNSURE signature

// Triggers a material/impact visual effect associated with a given material index (looked up in
// the global material-effects table, or a per-material fallback record when the index is out of
// range), and, if a tag id is also given, a second effect from that tag's own +0x120 field --
// used after melee hits.
void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id)
{
    uint8_t *material_record;

    if (material_index < 0 || material_index >= *(int32_t *)(globals_tag_data + 0x194)) {
        if (DAT_00721e4c == 0) {
            DAT_006e3578 = -1;
            DAT_00721e4c = 1;
        }
        material_record = DAT_006e3208;
    } else {
        material_record = *(uint8_t **)(globals_tag_data + 0x198) + material_index * 0x374;
    }

    if (*(datum_index *)(material_record + 0x370) != k_datum_index_none) {
        sound_start_at_object_marker(*(datum_index *)(material_record + 0x370), (void *)0xffffffff, 1.0f, 0);
    }

    if (unit_tag_id != k_datum_index_none) {
        uint8_t *tag_data = tag_instances[unit_tag_id & 0xffff].data;
        datum_index effect = *(datum_index *)(tag_data + 0x120);
        if (effect != k_datum_index_none) {
            sound_start_at_object_marker(effect, (void *)0xffffffff, 1.0f, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x56f210):

void FUN_0056f210(void)

{
  int iVar1;
  short in_AX;
  undefined *puVar2;
  uint in_ECX;

  if ((in_AX < 0) || (*(int *)(DAT_00746fa0 + 0x194) <= (int)in_AX)) {
    if (DAT_00721e4c == '\0') {
      DAT_006e3578 = 0xffffffff;
      DAT_00721e4c = '\x01';
    }
    puVar2 = &DAT_006e3208;
  }
  else {
    puVar2 = (undefined *)(in_AX * 0x374 + *(int *)(DAT_00746fa0 + 0x198));
  }
  if (*(int *)(puVar2 + 0x370) != -1) {
    FUN_00543ce0(*(int *)(puVar2 + 0x370),0xffffffff,0x3f800000,0);
  }
  if ((in_ECX != 0xffffffff) &&
     (iVar1 = *(int *)(*(int *)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x120),
     iVar1 != -1)) {
    FUN_00543ce0(iVar1,0xffffffff,0x3f800000,0);
  }
  return;
}
#endif
