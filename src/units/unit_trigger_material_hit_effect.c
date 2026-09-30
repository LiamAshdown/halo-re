// unit_trigger_material_hit_effect  (Ghidra: FUN_0056f210; renamed from the phase2 proposal)
// address 0x56f210, size 183 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary)
// rewrite confidence: 0.4
// evidence: types/units.h globals note "+0x194/0x198 the material table (stride 0x374)";
//   callee sound_start_at_object_marker established elsewhere in this module as an effect-trigger helper.
// register convention: material index in AX (in_AX), a tag id in ECX (in_ECX) -- no stack
//   arguments at all.
//   // blam-cc: AX -> material_index, ECX -> unit_tag_id, EDX -> object_index
// UNSURE: material_table_fallback is a fallback material-effect record this function initializes once
//   (via the material_table_warning_issued latch and material_table_bad_index) when the index is out of range; only its
//   +0x370 field (the effect tag) is touched here, so it is declared as an opaque byte array.
// UNSURE: tag+0x120 (relative to whatever tag unit_tag_id names) lands in objects.h's
//   undocumented _pad_110 padding; kept as a raw offset.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "fn_units.h"

extern Globals *global_globals;
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t material_table_warning_issued;        // UNSURE: one-time-init latch for the fallback record
extern int32_t material_table_bad_index;        // UNSURE: a field of the fallback record, zeroed on init
extern uint8_t material_table_fallback[0x374]; // UNSURE: fallback material-effect record, this module only
                                     //   reads +0x370 of it

extern const real_point3d *global_zero_vector3d_pointer; // 0x006966f8
extern const real_vector3d *global_forward3d_pointer;   // 0x00696718
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint); // 0x543ce0, ESI, ECX, EAX, stack

// Triggers a material/impact visual effect associated with a given material index (looked up in
// the global material-effects table, or a per-material fallback record when the index is out of
// range), and, if a tag id is also given, a second effect from that tag's own +0x120 field --
// used after melee hits.
// FIXED (objdump 0x56f210..0x56f2cc): EDX carries the object the sounds play on (0x56f21d mov esi,edx); both
//   sounds are sound_start_at_object_marker(ESI object, ECX *0x006966f8, EAX *0x00696718, tag, -1, 1.0, 0).
void unit_trigger_material_hit_effect(int16_t material_index, datum_index unit_tag_id, datum_index object_index)
{
    uint8_t *material_record;

    if (material_index < 0 || material_index >= *(int32_t *)&global_globals->materials.count) {
        if (material_table_warning_issued == 0) {
            material_table_bad_index = -1;
            material_table_warning_issued = 1;
        }
        material_record = material_table_fallback;
    } else {
        material_record = (uint8_t *)global_globals->materials.pointer + material_index * 0x374;
    }

    if (*(datum_index *)(material_record + 0x370) != k_datum_index_none) {
        sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
            (Vector3D *)global_forward3d_pointer, *(datum_index *)(material_record + 0x370), -1, 1.0f, 0);
    }

    if (unit_tag_id != k_datum_index_none) {
        uint8_t *tag_data = tag_instances[unit_tag_id & 0xffff].data;
        datum_index effect = *(datum_index *)(tag_data + 0x120);
        if (effect != k_datum_index_none) {
            sound_start_at_object_marker(object_index, (Point3D *)global_zero_vector3d_pointer,
                (Vector3D *)global_forward3d_pointer, effect, -1, 1.0f, 0);
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
