// unit_build_seat_occupant_zone_list  (Ghidra: FUN_0056bbd0)
// address 0x56bbd0, size 170 bytes, name confidence 0.35, rewrite confidence 0.4
// functions.md: "Allocates a new zone/list datum and populates it with references to every
// seated occupant of the unit."
// evidence: types/objects.h object.first_child_object (0x118), .next_object (0x114), .type
//   (0xb4); types/units.h unit_data.vehicle_seat_index (0x2f0); types/memory.h data_array
//   (datum_new already established in src/memory/datum_new.c).
// blam-cc: in_ECX -> unit_index.
// UNSURE: datum_new's original decompilation returns an 8-byte {index, array_pointer} pair here,
// but the already-established datum_new (0x4d0480) rewrite returns only the index; this rewrite
// uses object_list_header_data->data directly for the array pointer half instead, which is the
// same value the original's high dword would hold for this specific call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "fn_hs.h"
#include "fn_memory.h"

extern data_array *object_data;             // 0x008603b0
extern data_array *object_list_header_data; // 0x0087a464


datum_index unit_build_seat_occupant_zone_list(uint32_t unit_index) // blam-cc: in_ECX
{
    datum_index result = k_datum_index_none;
    if (unit_index == 0xffffffff) {
        return result;
    }

    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    result = datum_new(object_list_header_data);
    if (result != k_datum_index_none) {
        uint8_t *node = (uint8_t *)object_list_header_data->data + (result & 0xffff) * 0xc;
        *(int16_t *)(node + 6) = 0;
        *(uint32_t *)(node + 8) = 0xffffffff;

        datum_index child = unit_obj->first_child_object;
        while (child != k_datum_index_none) {
            object *child_obj = ((object_header *)object_data->data)[child & 0xffff].data;
            if (((_object_mask_unit & (1 << (child_obj->type & 0x1f))) != 0) &&
                (((unit_data *)((uint8_t *)child_obj + k_unit_data_offset))->vehicle_seat_index != -1)) {
                object_list_reference_add(result, child); // FIXED: EAX = the new list (EDI, 0x56bc5f)
            }
            child = child_obj->next_object;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x56bbd0):

uint FUN_0056bbd0(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint in_ECX;
  undefined8 uVar6;

  iVar4 = DAT_008603b0;
  uVar5 = 0xffffffff;
  if (in_ECX != 0xffffffff) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    uVar6 = datum_new();
    uVar5 = (uint)uVar6;
    if (uVar5 != 0xffffffff) {
      iVar1 = *(int *)((int)((ulonglong)uVar6 >> 0x20) + 0x34) + (uVar5 & 0xffff) * 0xc;
      *(undefined2 *)(iVar1 + 6) = 0;
      *(undefined4 *)(iVar1 + 8) = 0xffffffff;
      if (uVar5 != 0xffffffff) {
        uVar3 = *(uint *)(iVar2 + 0x118);
        while (uVar3 != 0xffffffff) {
          iVar2 = *(int *)(*(int *)(iVar4 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
          if (((1 << (*(byte *)(iVar2 + 0xb4) & 0x1f) & 3U) != 0) &&
             (*(short *)(iVar2 + 0x2f0) != -1)) {
            object_list_reference_add(uVar3);
          }
          uVar3 = *(uint *)(iVar2 + 0x114);
        }
      }
    }
  }
  return uVar5;
}
#endif
