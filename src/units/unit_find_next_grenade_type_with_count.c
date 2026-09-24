// unit_find_next_grenade_type_with_count  (Ghidra: FUN_005699a0)
// address 0x5699a0, size 117 bytes, name confidence 0.4, rewrite confidence 0.5
// evidence: types/units.h unit_data.grenade_counts[2] (0x31e), k_maximum_grenade_types (2,
//   "0x5699a0 wraps the 0x31e counts at index 1"). functions.md's summary ("seat/marker usage-
//   count table") does not match the actual field this function walks; this rewrite follows the
//   stronger evidence in units_types_notes.md instead.
// blam-cc: in_EAX -> unit_index, in_ECX -> start_index, param_1 (stack) -> direction.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

int32_t unit_find_next_grenade_type_with_count(uint32_t unit_index, int32_t start_index, int16_t direction) // blam-cc: in_EAX, in_ECX, stack
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

    int32_t fallback = -1;
    int32_t original = start_index;
    if ((int16_t)start_index == -1) {
        start_index = 0;
        original = start_index;
    }

    for (;;) {
        int16_t index = (int16_t)start_index;
        if (0 < unit->grenade_counts[index]) {
            if (index != (int16_t)original) {
                return start_index;
            }
            fallback = start_index;
            if (direction == 0) {
                return start_index;
            }
        }
        if (direction < 0) {
            start_index = (index == 0) ? 1 : (int32_t)index - 1;
        } else if (index == 1) {
            start_index = 0;
        } else {
            start_index = (int32_t)index + 1;
        }
        if ((int16_t)start_index == (int16_t)original) {
            return fallback;
        }
    }
}

#if 0
Original Ghidra decompilation (0x5699a0):

int FUN_005699a0(short param_1)

{
  uint in_EAX;
  int iVar1;
  short sVar2;
  int in_ECX;
  int iVar3;
  int iVar4;

  iVar1 = -1;
  iVar4 = in_ECX;
  if ((short)in_ECX == -1) {
    in_ECX = 0;
    iVar4 = in_ECX;
  }
  do {
    sVar2 = (short)in_ECX;
    iVar3 = (int)sVar2;
    if ('\0' < *(char *)(iVar3 + 0x31e +
                        *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc))) {
      if (sVar2 != (short)iVar4) {
        return in_ECX;
      }
      iVar1 = in_ECX;
      if (param_1 == 0) {
        return in_ECX;
      }
    }
    if (param_1 < 0) {
      if (sVar2 == 0) {
        in_ECX = 1;
      }
      else {
        in_ECX = iVar3 + -1;
      }
    }
    else if (sVar2 == 1) {
      in_ECX = 0;
    }
    else {
      in_ECX = iVar3 + 1;
    }
    if ((short)in_ECX == (short)iVar4) {
      return iVar1;
    }
  } while( true );
}
#endif
