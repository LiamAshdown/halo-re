// input_device_get_pov_count  (Ghidra: FUN_00491650)
// address 0x491650, size 30 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: out/phase4/input_types_notes.md: "input_device_get_axis/button/pov_count
// 0x491610/0x491630/0x491650". 0x006b1aa4 is input_devices base 0x006b1868 + 0x23c, which is
// input_device::pov_count.
// register convention: joystick slot index in CX (in_CX)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern int32_t joystick_slot_devices[4]; // 0x006b2ce8
extern input_device input_devices[8];    // 0x006b1868

// blam-cc: slot in ECX
// Returns the POV-hat count of the input device mapped to joystick slot slot_index, or 0 if
// the slot has no device mapped.
int32_t input_device_get_pov_count(int16_t slot_index)
{
    int32_t device_index;
    int32_t result;

    result = 0;
    device_index = joystick_slot_devices[slot_index];
    if (device_index != -1) {
        result = input_devices[device_index].pov_count;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x491650):

undefined4 FUN_00491650(void)

{
  undefined4 uVar1;
  short in_CX;

  uVar1 = 0;
  if ((&DAT_006b2ce8)[in_CX] != -1) {
    uVar1 = *(undefined4 *)(&DAT_006b1aa4 + (&DAT_006b2ce8)[in_CX] * 0x240);
  }
  return uVar1;
}
#endif
