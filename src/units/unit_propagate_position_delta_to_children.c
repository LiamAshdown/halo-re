// unit_propagate_position_delta_to_children  (Ghidra: FUN_00570cb0; renamed from the phase2
//   proposal)
// address 0x570cb0, size 184 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary)
// rewrite confidence: 0.95
// evidence: types/objects.h object.position (0x05c), .type (0x0b4), .next_object (0x114),
//   .first_child_object (0x118); types/units.h unit_data.unknown_34c (0x34c, "a cached point
//   and its per-frame delta"); callee object_set_position_and_recalculate (0x4f52c0).
// register convention: a new-position pointer in EAX (in_EAX); the unit object index in ECX
//   (in_ECX).
//   // blam-cc: EAX -> new_position, ECX -> unit_index
// UNSURE: object_set_position_and_recalculate's register argument is not visible at this call
//   site; guessed as (unit_index, new_position) matching its own established signature file's
//   note that its arguments are otherwise unconfirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern void object_set_position_and_recalculate(real_point3d *position, uint32_t object_index); // 0x4f52c0, ESI, EDI

// Propagates the unit's positional movement delta (new_position - its current position) to any
// attached child bipeds/vehicles, updating their cached relative offsets (unknown_34c), then
// recalculates the unit's own position/cluster.
void unit_propagate_position_delta_to_children(real_point3d *new_position, uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    real_vector3d delta;
    datum_index child;

    delta.i = new_position->x - obj->position.x;
    delta.j = new_position->y - obj->position.y;
    delta.k = new_position->z - obj->position.z;

    child = obj->first_child_object;
    while (child != k_datum_index_none) {
        object *child_obj = ((object_header *)object_data->data)[child & 0xffff].data;
        if ((1 << (child_obj->type & 0x1f) & 3) != 0) {
            unit_data *child_unit = (unit_data *)((uint8_t *)child_obj + k_unit_data_offset);
            child_unit->seat_acceleration_last_position.x += delta.i;
            child_unit->seat_acceleration_last_position.y += delta.j;
            child_unit->seat_acceleration_last_position.z += delta.k;
        }
        child = child_obj->next_object;
    }

    // 0x570d5c: ESI = new_position (kept from EAX), EDI = the unit (kept from ECX); the draft passed only the unit
    object_set_position_and_recalculate(new_position, unit_index);
}

#if 0
Original Ghidra decompilation (0x570cb0):

void FUN_00570cb0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  int iVar7;
  uint uVar8;
  int iVar9;
  float *in_EAX;
  uint in_ECX;

  iVar9 = DAT_008603b0;
  fVar1 = *in_EAX;
  iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  fVar2 = *(float *)(iVar7 + 0x5c);
  fVar3 = in_EAX[1];
  fVar4 = *(float *)(iVar7 + 0x60);
  fVar5 = in_EAX[2];
  fVar6 = *(float *)(iVar7 + 100);
  uVar8 = *(uint *)(iVar7 + 0x118);
  while (uVar8 != 0xffffffff) {
    iVar7 = *(int *)(*(int *)(iVar9 + 0x34) + 8 + (uVar8 & 0xffff) * 0xc);
    if ((1 << (*(byte *)(iVar7 + 0xb4) & 0x1f) & 3U) != 0) {
      *(float *)(iVar7 + 0x34c) = (fVar1 - fVar2) + *(float *)(iVar7 + 0x34c);
      *(float *)(iVar7 + 0x350) = (fVar3 - fVar4) + *(float *)(iVar7 + 0x350);
      *(float *)(iVar7 + 0x354) = (fVar5 - fVar6) + *(float *)(iVar7 + 0x354);
    }
    uVar8 = *(uint *)(iVar7 + 0x114);
  }
  object_set_position_and_recalculate();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
