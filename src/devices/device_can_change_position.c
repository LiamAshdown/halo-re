// device_can_change_position  (Ghidra: FUN_0044c0c0; kept per out/phase4/devices_types_notes.md,
// which also corrects phase 2's evidence text: the function REFUSES a change when both device
// group flag bits are set, it does not require them both set to allow one)
// address 0x44c0c0, size 102 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: types/devices.h device_group (flags at +0x02), device_group_flags
// (_device_group_can_change_only_once_bit, _device_group_changed_bit), device_data
// (flags/power_group/position_group), device_flags (_device_not_usable_from_any_side_bit).
// register convention: object index in EAX (in_EAX).
//   // blam-cc: EAX -> object_index
// Determines whether a device is allowed to change its position, requiring its position group
// to be assigned, not locked (can_change_only_once and already changed), not marked
// not_usable_from_any_side, and fully powered (power group value == 1.0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"

extern data_array *object_data; // 0x008603b0
extern data_array *device_groups; // 0x0087abf0

int device_can_change_position(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    int can_change = 0;

    if (dev->position_group != -1) {
        uint16_t flags = ((device_group *)device_groups->data)[(uint16_t)dev->position_group].flags;
        can_change = 1;

        if ((flags & (1u << _device_group_can_change_only_once_bit)) != 0 &&
            (flags & (1u << _device_group_changed_bit)) != 0) {
            can_change = 0;
        }
        if ((dev->flags & (1u << _device_not_usable_from_any_side_bit)) != 0) {
            can_change = 0;
        }
        if (((device_group *)device_groups->data)[(uint16_t)dev->power_group].value != 1.0f) {
            can_change = 0;
        }
    }

    return can_change;
}

#if 0
Original Ghidra decompilation (0x44c0c0), from tools/pack.py 0x44c0c0:

uint FUN_0044c0c0(void)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  uint uVar4;
  uint3 uVar5;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar4 = (in_EAX & 0xffff) * 3 & 0xffffff00;
  if (*(ushort *)(iVar2 + 0x204) != 0xffff) {
    iVar3 = *(int *)(DAT_0087abf0 + 0x34);
    uVar1 = *(ushort *)(iVar3 + (uint)*(ushort *)(iVar2 + 0x204) * 8 + 2);
    uVar5 = (uint3)((uint)iVar3 >> 8);
    uVar4 = CONCAT31(uVar5,1);
    if (((uVar1 & 1) != 0) && ((uVar1 & 2) != 0)) {
      uVar4 = (uint)uVar5 << 8;
    }
    if ((*(byte *)(iVar2 + 500) & 2) != 0) {
      uVar4 = uVar4 & 0xffffff00;
    }
    if (*(int *)(iVar3 + (uint)*(ushort *)(iVar2 + 0x1f8) * 8 + 4) != 0x3f800000) {
      uVar4 = uVar4 & 0xffffff00;
    }
  }
  return uVar4;
}
#endif
