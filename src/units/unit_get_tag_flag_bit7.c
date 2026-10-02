// unit_get_tag_flag_bit7  (Ghidra: FUN_00571c70; renamed from the phase2 proposal)
// address 0x571c70, size 55 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.95 (VERIFIED against objdump (jump tables decoded))
// evidence: types/tags.h VehicleFlags bit 7 = causes_collision_damage; this function's only
//   caller (unit_melee_attack_scan.c) reaches it after confirming the target is a vehicle, so
//   tag+0x2f0 (the byte immediately after Unit's own 0x2f0 bytes) is read as Vehicle.vehicle_flags
//   rather than the Biped-side field at the same offset.
// register convention: unit object index in EAX (in_EAX).
//   // blam-cc: EAX -> unit_index
// UNSURE: for a non-vehicle unit this would instead read Biped.moving_turning_speed's low byte,
//   so this function is only meaningful when the caller already knows the object is a vehicle.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// Returns bit 7 of the tag word immediately following Unit (Vehicle.vehicle_flags's
// causes_collision_damage bit, for a vehicle).
uint32_t unit_get_tag_flag_bit7(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[obj->definition_tag & 0xffff].data;
    return (*(uint32_t *)(tag + 0x2f0) >> 7) & 1;
}

#if 0
Original Ghidra decompilation (0x571c70):

uint FUN_00571c70(void)

{
  uint in_EAX;

  return *(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc
                                        ) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f0) >> 7 & 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
