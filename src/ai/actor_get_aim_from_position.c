// actor_get_aim_from_position  (Ghidra: actor_get_aim_from_position, renamed)
// address 0x40f9b0, size 177 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against 0x40f9b0 (vehicle-forward path under +0x161 and tag flag 0x100, aim +0x23c fallback on the active unit, clamp EDI/stack))
// evidence: phase-4 summary "returns the 3D position to aim/look from for the actor (its
// eye offset if flagged, otherwise its object's base position)".
// register convention: actor_index in EAX, output real_point3d pointer in ECX.
// blam-cc: EAX -> actor_index, ECX -> out_position
// UNSURE: when actor.unknown_161 is set and the Actor tag data's dword at +0x2f0 has bit
// 0x100 set, the output is the unit object's `forward` vector (object+0x74), not an offset
// -- the phase-4 summary's "eye offset" framing does not match what the bytes do here.
// UNSURE: the fallback read at object-pointer+0x23c/+0x240/+0x244 is beyond the 0x1f4-byte
// base `object` struct in types/objects.h and does not land on a named unit_data field
// either; kept as a raw offset. FUN_005697a0 is called with no visible arguments afterward
// and its return value (if any) is discarded here, matching the decompiled void return.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction,
                                                          uint8_t use_aiming_bounds); // 0x5697a0; EDI unit, stack (direction, flag)

// blam-cc: EAX -> actor_index, ECX -> out_position
void actor_get_aim_from_position(datum_index actor_index, uint32_t out_position[3])
{
    actor *self;
    datum_index unit_index;
    object_header *hdr;
    object *unit_obj;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    unit_index = self->unit_index;

    if (self->vehicle_gunner != 0) {
        unit_index = self->active_unit_index;
        hdr = (object_header *)object_data->data + (unit_index & 0xffff);
        unit_obj = hdr->data;
        if ((*(uint32_t *)((uint8_t *)tag_instances[unit_obj->definition_tag & 0xffff].data + 0x2f0) & 0x100) != 0) {
            out_position[0] = *(uint32_t *)&unit_obj->forward.i;
            out_position[1] = *(uint32_t *)&unit_obj->forward.j;
            out_position[2] = *(uint32_t *)&unit_obj->forward.k;
            return;
        }
    }

    hdr = (object_header *)object_data->data + (unit_index & 0xffff);
    unit_obj = hdr->data;
    out_position[0] = *(uint32_t *)((uint8_t *)unit_obj + 0x23c);
    out_position[1] = *(uint32_t *)((uint8_t *)unit_obj + 0x240);
    out_position[2] = *(uint32_t *)((uint8_t *)unit_obj + 0x244);
    // 0x40fa4c: EDI = the unit, push 1, push the output (clamped in place to the aiming bounds)
    unit_clamp_direction_to_aim_or_look_bounds(unit_index, (real_vector3d *)out_position, 1);
}

#if 0
Original Ghidra decompilation (0x40f9b0):

void FUN_0040f9b0(void)

{
  uint *puVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  uint *in_ECX;
  uint uVar4;

  iVar2 = (in_EAX & 0xffff) * 0x724;
  iVar3 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  uVar4 = *(uint *)(iVar3 + 0x18);
  if (*(char *)(iVar2 + 0x161 + *(int *)(DAT_00880360 + 0x34)) != '\0') {
    uVar4 = *(uint *)(iVar3 + 0x158);
    puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
    if ((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2f0) & 0x100) != 0)
    {
      *in_ECX = puVar1[0x1d];
      in_ECX[1] = puVar1[0x1e];
      in_ECX[2] = puVar1[0x1f];
      return;
    }
  }
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
  *in_ECX = *(uint *)(iVar2 + 0x23c);
  in_ECX[1] = *(uint *)(iVar2 + 0x240);
  in_ECX[2] = *(uint *)(iVar2 + 0x244);
  FUN_005697a0();
  return;
}
#endif
