// vehicle_calculate_mounted_controls_dispatch  (Ghidra: FUN_00573ee0; renamed from the phase2
//   proposal)
// address 0x573ee0, size 119 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary; dispatched from
//   vehicle_update's case 5)
// rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: the physics.tag_id-at-0x8c double-tag_instances-lookup idiom (Vehicle tag ->
//   Physics tag) matches the sibling dispatch functions in this batch; callees
//   vehicle_calculate_ground_contact_lean (0x573f60), vehicle_calculate_ground_contact_lean_alt
//   (0x574460), vehicle_create_hover_thruster_midpoint_effects (0x574bc0), all this batch.
// register convention: unit object index in ESI (unaff_ESI); out_record in EDX and out_transform
//   in ECX are also genuine live-ins, pushed as this function's own stack arguments 2 and 3 to
//   both vehicle_calculate_ground_contact_lean(_alt) call sites (objdump 0x573f21/0x573f28,
//   `push ecx` / `push edx`, both read with no local setup).
//   // blam-cc: ESI -> unit_index, ECX -> out_transform, EDX -> out_record
// UNSURE: the exact meaning of "the first float of the Physics tag's data" (its sign selects
//   between the two ground-contact lean variants) is not named in any header.
// FIXED (register inputs, objdump): ECX and EDX are genuine live-ins the notes did not map;
// added as `out_transform`/`out_record` and forwarded instead of the hardcoded 0/0. Also: the
// callees' own files document them as EAX/ECX/EDX register-passed, but vehicle_calculate_ground_
// contact_lean_alt's own prologue at 0x574469 (`mov edx,[ebp+0x8]`) reads its first parameter
// from the stack, matching this call site's 3 stack pushes -- both are actually cdecl stack
// arguments in this parameter order, at least for this caller.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void vehicle_calculate_ground_contact_lean(uint32_t unit_index, void *out_record,
                                                   void *out_transform); // 0x573f60, this batch
extern void vehicle_calculate_ground_contact_lean_alt(uint32_t unit_index, void *out_record,
                                                       void *out_transform); // 0x574460, this batch
extern void vehicle_create_hover_thruster_midpoint_effects(uint32_t unit_index); // 0x574bc0, this batch

// Selects between two ground-contact lean calculations for a mounted/turret-style vehicle unit
// based on the sign of its Physics tag's first field, and always triggers the hover-thruster
// midpoint effect afterward.
void vehicle_calculate_mounted_controls_dispatch(uint32_t unit_index, void *out_transform, void *out_record)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t *physics_tag = (uint8_t *)tag_instances[*(uint32_t *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;

    if (*(float *)physics_tag > 0.0f) {
        vehicle_calculate_ground_contact_lean_alt(unit_index, out_record, out_transform);
    } else {
        vehicle_calculate_ground_contact_lean(unit_index, out_record, out_transform);
    }
    vehicle_create_hover_thruster_midpoint_effects(unit_index);
}

#if 0
Original Ghidra decompilation (0x573ee0):

void FUN_00573ee0(void)

{
  uint unaff_ESI;

  if (0.0 < **(float **)
              ((*(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (unaff_ESI & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                  DAT_0087bc14) + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)) {
    FUN_00574460();
    vehicle_create_hover_thruster_midpoint_effects();
    return;
  }
  FUN_00573f60();
  vehicle_create_hover_thruster_midpoint_effects();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
