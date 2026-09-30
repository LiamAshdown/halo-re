// device_control_touched  (Ghidra: FUN_0044c090; renamed per out/phase4/devices_types_notes.md:
// "device_control_maybe_update" was phase 2's guess; this dispatches the power-change guard
// only for device_control objects)
// address 0x44c090, size 45 bytes
// VERIFIED against disassembly 0x44c090..0x44c0bd (2026-09-30). the original tests `type != 7 && type == 8`; the
//   single equality is equivalent; the tail `jmp 0x44adf0` passes EAX unchanged
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: types/objects.h object (type 0x0b4), _object_type_device_machine (7),
// _object_type_device_control (8); functions.md: "Dispatches to the device power-change guard
// only for device_control objects (object subtype 8), skipping device_machine objects
// (subtype 7)".
// register convention: object index in EAX (in_EAX).
//   // blam-cc: EAX -> object_index
// The original tests `type != 7 && type == 8`, which is redundant (type == 8 already implies
// type != 7); simplified to the single equality it is equivalent to for every int16_t value.
// Callee renamed to device_control_activate per src/devices/device_control_activate.c (0x44adf0,
// outside this batch's address range, written alongside this one).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern void device_control_activate(uint32_t object_id); // 0x44adf0, EAX = object_id; outside
    // this batch's address range

void device_control_touched(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->type == _object_type_device_control) {
        device_control_activate(object_index);
    }
}

#if 0
Original Ghidra decompilation (0x44c090), from tools/pack.py 0x44c090:

void FUN_0044c090(void)

{
  short sVar1;
  uint in_EAX;

  sVar1 = *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) + 0xb4);
  if ((sVar1 != 7) && (sVar1 == 8)) {
    FUN_0044adf0();
    return;
  }
  return;
}
#endif
