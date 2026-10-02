// device_group_set_value_immediate  (Ghidra: device_group_set_value_immediate, already named;
// functions.md: "Hard-sets a device group's value and immediately resyncs every object
// referencing that group as either its power or position group, bypassing the normal
// interpolated transition")
// address 0x44bea0, size 227 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/devices.h device_group (value at +0x04), device_data (flags,
// power/power_change, position/position_change), device_flags (_device_position_changed_bit);
// types/objects.h object_iterator, _object_mask_device.
// register convention: group index in ESI (unaff_SI), value as the sole recognized stack
// parameter (Ghidra's own `device_group_set_value_immediate(float param_1)`).
//   // blam-cc: ESI -> group_index, stack -> value
// Unlike device_group_set_value (0x44bd70, this batch), this function never calls
// device_play_state_change_effect -- confirmed both by its own callee list (only
// object_iterator_next) and by disassembly; it resyncs every matching object silently.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *device_groups; // 0x0087abf0

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, objects module

void device_group_set_value_immediate(uint16_t group_index, float value)
{
    device_group *group;
    object_iterator iterator;
    object *obj;

    if (value < 0.0f) {
        value = 0.0f;
    } else if (1.0f < value) {
        value = 1.0f;
    }

    group = &((device_group *)device_groups->data)[group_index];
    group->value = value;

    iterator.type_mask = _object_mask_device;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));

        if (dev->power_group == (int16_t)group_index) {
            dev->flags |= (1u << _device_position_changed_bit);
            dev->power = value;
            dev->power_change = 0.0f;
        }
        if (dev->position_group == (int16_t)group_index) {
            dev->flags |= (1u << _device_position_changed_bit);
            dev->position = value;
            dev->position_change = 0.0f;
        }
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x44bea0), from tools/pack.py 0x44bea0:

void device_group_set_value_immediate(float param_1)

{
  int iVar1;
  ushort unaff_SI;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  if (0.0 <= param_1) {
    if (1.0 < param_1) {
      param_1 = 1.0;
    }
  }
  else {
    param_1 = 0.0;
  }
  *(float *)(*(int *)(DAT_0087abf0 + 0x34) + 4 + (uint)unaff_SI * 8) = param_1;
  local_4 = 0x86868686;
  local_10 = 0x380;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar1 = object_iterator_next(&local_10);
  while (iVar1 != 0) {
    if (*(ushort *)(iVar1 + 0x1f8) == unaff_SI) {
      *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) | 4;
      *(float *)(iVar1 + 0x1fc) = param_1;
      *(undefined4 *)(iVar1 + 0x200) = 0;
    }
    if (*(ushort *)(iVar1 + 0x204) == unaff_SI) {
      *(uint *)(iVar1 + 500) = *(uint *)(iVar1 + 500) | 4;
      *(float *)(iVar1 + 0x208) = param_1;
      *(undefined4 *)(iVar1 + 0x20c) = 0;
    }
    iVar1 = object_iterator_next(&local_10);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
