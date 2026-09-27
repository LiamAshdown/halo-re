// unit_seat_is_occupied_by_other  (Ghidra: unit_seat_is_occupied_by_other)
// address 0x566840, size 189 bytes
// name confidence: 0.4 (phase2 candidate)   rewrite confidence: 0.3
// evidence: types/objects.h object.type (0xb4), .first_child_object (0x118), .next_object
//   (0x114); types/units.h unit_data.vehicle_seat_index (0x2f0), .controlling_player (0x218).
// register convention: unit index (self) in EAX, seat index in DX, the vehicle/parent object
//   index in EDX, out-occupant pointer on the stack.
//   // blam-cc: in_EAX -> self_index, in_DX -> seat_index (param_1), in_EDX -> vehicle_index,
//   //   param_2 -> out_occupant_index
// UNSURE: the `uVar5 = uVar6` inside the comma expression on the teams_are_enemies branch is
//   reproduced literally -- on every iteration after the first match it makes the following
//   `uVar6 = uVar5` a no-op re-assignment to itself, which reads like a genuine (if odd) source
//   quirk rather than something to simplify away. teams_are_enemies is a leaf helper outside this
//   batch; its argument (if any) is not shown by Ghidra at this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern uint8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, CX, DX

uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index,
                                        uint32_t *out_occupant_index) // blam-cc: see file header
{
    object *self_obj = ((object_header *)object_data->data)[self_index & 0xffff].data;
    unit_data *self_unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);

    uint32_t found = (uint32_t)-1;
    uint8_t not_found = (self_index != vehicle_index);

    object *vehicle_obj = ((object_header *)object_data->data)[vehicle_index & 0xffff].data;
    datum_index child = vehicle_obj->first_child_object;

    while (child != (datum_index)-1) {
        object *child_obj = ((object_header *)object_data->data)[child & 0xffff].data;

        if (((1 << (child_obj->type & 0x1f)) & 3) != 0) {
            unit_data *child_unit = (unit_data *)((uint8_t *)child_obj + k_unit_data_offset);
            uint8_t match = child_unit->vehicle_seat_index == seat_index;
            uint32_t reassigned = child; // in case the teams_are_enemies branch below fires
            if (!match && self_unit->controlling_player != (datum_index)-1) {
                reassigned = found; // see file header UNSURE note
                match = teams_are_enemies(*(int16_t *)((uint8_t *)child_obj + 0xb8), *(int16_t *)((uint8_t *)self_obj + 0xb8)) != 0; // 0x5668ce
            }
            if (match) {
                not_found = 0;
                found = reassigned;
            }
        }
        child = child_obj->next_object;
    }

    if (out_occupant_index != 0) {
        *out_occupant_index = found;
    }
    return not_found;
}

#if 0
Original Ghidra decompilation (0x566840):

bool FUN_00566840(short param_1,uint *param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  char cVar4;
  uint in_EAX;
  uint in_EDX;
  uint uVar5;
  uint uVar6;
  bool bVar7;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar6 = 0xffffffff;
  bVar7 = in_EAX != in_EDX;
  uVar5 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc) + 0x118);
  if (uVar5 != 0xffffffff) {
    iVar2 = *(int *)(DAT_008603b0 + 0x34);
    do {
      iVar3 = *(int *)(iVar2 + 8 + (uVar5 & 0xffff) * 0xc);
      if (((1 << (*(byte *)(iVar3 + 0xb4) & 0x1f) & 3U) != 0) &&
         ((*(short *)(iVar3 + 0x2f0) == param_1 ||
          ((*(int *)(iVar1 + 0x218) != -1 && (cVar4 = FUN_0045bd50(), uVar5 = uVar6, cVar4 != '\0')))
          )))) {
        bVar7 = false;
        uVar6 = uVar5;
      }
      uVar5 = *(uint *)(iVar3 + 0x114);
    } while (uVar5 != 0xffffffff);
  }
  if (param_2 != (uint *)0x0) {
    *param_2 = uVar6;
  }
  return bVar7;
}
#endif
