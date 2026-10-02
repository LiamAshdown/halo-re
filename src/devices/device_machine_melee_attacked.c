// device_machine_melee_attacked  (Ghidra: FUN_0044b5d0; renamed per
// out/phase4/devices_types_notes.md: "device_apply_initial_open_flag" was phase 2's guess, but
// the tested bit is opened_by_melee_attack and the single caller (this module's melee-response
// path) plus the immediate, non-interpolated open are exactly a melee response)
// address 0x44b5d0, size 68 bytes
// name confidence: 0.6 (per devices_types_notes.md's correction)   rewrite confidence: 0.65
// evidence: types/devices.h device_machine_flags (_device_machine_opened_by_melee_attack_bit,
//   bit 3 of type_flags at object+0x214), device_data (position_group at object+0x204).
// register convention: object index in ECX (in_ECX). Confirmed against disassembly (objdump
//   0x44b5d0-0x44b613): ESI is pushed and set to the position_group value read out of the
//   object before the call to device_group_set_value_immediate, matching that function's own
//   ESI -> group_index convention.
//   // blam-cc: ECX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "devices.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void device_group_set_value_immediate(uint16_t group_index, float value); // 0x44bea0
    // blam-cc: ESI -> group_index, stack -> value

void device_machine_melee_attacked(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    device_machine_data *dev = (device_machine_data *)((uint8_t *)obj + sizeof(object));

    if ((dev->device.type_flags & (1u << _device_machine_opened_by_melee_attack_bit)) != 0 &&
        object_index != 0xffffffff &&
        dev->device.position_group != -1) {
        device_group_set_value_immediate((uint16_t)dev->device.position_group, 1.0f);
    }
}

#if 0
Original Ghidra decompilation (0x44b5d0), from tools/pack.py 0x44b5d0:

void FUN_0044b5d0(void)

{
  int iVar1;
  uint in_ECX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if ((((*(byte *)(iVar1 + 0x214) & 8) != 0) && (in_ECX != 0xffffffff)) &&
     (*(short *)(iVar1 + 0x204) != -1)) {
    device_group_set_value_immediate(0x3f800000);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
