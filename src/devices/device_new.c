// device_new  (Ghidra: device_new, already named; functions.md: "Initializes a newly created
// device object's power and position group links (allocating new groups if unassigned) and
// copies the initial group values and flags onto the object")
// address 0x44bf90, size 251 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: types/devices.h device_placement_data (power_group/position_group/flags),
// device_data (flags/power_group/power/position_group/position), device_group (flags/value);
// types/tags.h ScenarioDeviceFlags (initially_open bit 0, initially_off bit 1,
// can_change_only_once bit 2).
// register convention: object index in EAX (in_EAX), placement pointer in EDI (unaff_EDI).
//   // blam-cc: EAX -> object_index, EDI -> placement
// Resolved against disassembly (objdump -d -M intel bin/halo.exe, 0x44bf90-0x44c08a), because
// Ghidra's decompile completely drops two conditional branches, folding each into an
// unexplained `extraout_ST0`/`extraout_ST0_00` float that "arrives" at the store with no
// visible source:
//   - power_group's initial value (only written when a fresh group is allocated, i.e. the
//     placement did not already name one) is 0.0 if placement->flags has initially_off (bit 1,
//     0x02) set, else 1.0 (0x44bfb6-0x44bfc4).
//   - position_group's initial value, on the same fresh-allocation path, is 1.0 if
//     placement->flags has initially_open (bit 0, 0x01) set, else 0.0 (0x44bfff-0x44c00e).
// Also confirmed by that same disassembly: datum_new here returns only its usual 32-bit
// datum_index in EAX; the "iVar6 = (int)((ulonglong)uVar7 >> 0x20)" Ghidra shows is not a real
// 64-bit return, it is Ghidra misreading the device_groups pointer sitting untouched in EDX
// across the call (datum_new does not clobber it).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern data_array *device_groups; // 0x0087abf0

extern datum_index datum_new(data_array *array); // 0x4d0480, established

void device_new(uint32_t object_index, device_placement_data *placement)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_data *dev = (device_data *)((uint8_t *)obj + sizeof(object));
    int16_t power_group = placement->power_group;
    uint16_t position_group;

    if (power_group == -1) {
        datum_index new_group = datum_new(device_groups);
        power_group = (int16_t)new_group;
        if (power_group != -1) {
            device_group *group = &((device_group *)device_groups->data)[(uint16_t)new_group];
            group->flags = (1u << _device_group_object_created_bit);
            group->value = (placement->flags & 0x02) != 0 ? 0.0f : 1.0f; // ScenarioDeviceFlags.initially_off
        }
    }
    dev->power_group = power_group;

    position_group = placement->position_group;
    if (position_group == 0xffff) {
        datum_index new_group = datum_new(device_groups);
        position_group = (uint16_t)new_group;
        if (position_group != 0xffff) {
            device_group *group = &((device_group *)device_groups->data)[position_group];
            group->flags = (uint16_t)(((placement->flags & 0x04) | 0x10) >> 2); // ScenarioDeviceFlags
                // .can_change_only_once (bit 2) -> _device_group_can_change_only_once_bit (bit 0),
                // plus _device_group_object_created_bit (bit 2) always set
            group->value = (placement->flags & 0x01) != 0 ? 1.0f : 0.0f; // ScenarioDeviceFlags.initially_open
        }
    }
    dev->position_group = (int16_t)position_group;

    dev->power = ((device_group *)device_groups->data)[(uint16_t)dev->power_group].value;
    dev->position = ((device_group *)device_groups->data)[position_group].value;

    if ((placement->flags & 0x08) != 0) { // ScenarioDeviceFlags.position_reversed
        dev->flags |= (1u << _device_position_reversed_bit);
    }
    if ((placement->flags & 0x10) != 0) { // ScenarioDeviceFlags.not_usable_from_any_side
        dev->flags |= (1u << _device_not_usable_from_any_side_bit);
    }
}

#if 0
Original Ghidra decompilation (0x44bf90), from tools/pack.py 0x44bf90:

void device_new(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  short sVar4;
  ushort uVar5;
  uint in_EAX;
  int iVar6;
  short *unaff_EDI;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  undefined8 uVar7;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar4 = *unaff_EDI;
  iVar6 = DAT_0087abf0;
  if (sVar4 == -1) {
    uVar7 = datum_new();
    iVar6 = (int)((ulonglong)uVar7 >> 0x20);
    sVar4 = (short)uVar7;
    if (sVar4 != -1) {
      iVar1 = *(int *)(iVar6 + 0x34) + ((uint)uVar7 & 0xffff) * 8;
      *(undefined2 *)(iVar1 + 2) = 4;
      *(float *)(iVar1 + 4) = (float)extraout_ST0;
    }
  }
  *(short *)(iVar2 + 0x1f8) = sVar4;
  uVar5 = unaff_EDI[1];
  if (uVar5 == 0xffff) {
    uVar3 = *(uint *)(unaff_EDI + 2);
    uVar7 = datum_new();
    iVar6 = (int)((ulonglong)uVar7 >> 0x20);
    uVar5 = (ushort)uVar7;
    if (uVar5 != 0xffff) {
      iVar1 = *(int *)(iVar6 + 0x34) + ((uint)uVar7 & 0xffff) * 8;
      *(short *)(iVar1 + 2) = (short)((uVar3 & 4 | 0x10) >> 2);
      *(float *)(iVar1 + 4) = (float)extraout_ST0_00;
    }
  }
  *(ushort *)(iVar2 + 0x204) = uVar5;
  *(undefined4 *)(iVar2 + 0x1fc) =
       *(undefined4 *)(*(int *)(iVar6 + 0x34) + 4 + (uint)*(ushort *)(iVar2 + 0x1f8) * 8);
  *(undefined4 *)(iVar2 + 0x208) = *(undefined4 *)(*(int *)(iVar6 + 0x34) + 4 + (uint)uVar5 * 8);
  if ((*(byte *)(unaff_EDI + 2) & 8) != 0) {
    *(uint *)(iVar2 + 500) = *(uint *)(iVar2 + 500) | 1;
  }
  if ((*(byte *)(unaff_EDI + 2) & 0x10) != 0) {
    *(uint *)(iVar2 + 500) = *(uint *)(iVar2 + 500) | 2;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
