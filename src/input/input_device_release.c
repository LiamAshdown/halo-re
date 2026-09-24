// input_device_release  (Ghidra: already named)
// address 0x491f80, size 68 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Unacquires and releases the DirectInput
// device object at the given slot index and clears its cached device-info struct."; 0x006b1848
// is joystick_devices[8] (input.h globals list), 0x006b1868 input_devices[8] (0x240 bytes,
// stride confirmed by every other table walk in this module). The IUnknown/IDirectInputDevice8
// vtable slots used are +0x20 (Unacquire) and +8 (Release).
// register convention: joystick slot index in SI (unaff_ESI, low 16 bits)
// COM method typedefs: types/input.h (the DirectInput 8 method block).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern void *joystick_devices[8];      // 0x006b1848, IDirectInputDevice8A*
extern input_device input_devices[8];  // 0x006b1868


// blam-cc: slot index in ESI
// Unacquires and releases the DirectInput device object mapped to joystick slot slot_index (if
// any), clears the device pointer, and zeroes its cached input_devices entry.
void input_device_release(int16_t slot_index)
{
    void *device;
    void **vtable;
    uint32_t *cursor;
    int32_t count;

    device = joystick_devices[slot_index];
    if (device != 0) {
        vtable = *(void ***)device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(device);
        ((idirectinputdevice8_release_proc)vtable[2])(device);
        joystick_devices[slot_index] = 0;

        cursor = (uint32_t *)&input_devices[slot_index];
        for (count = 0x90; count != 0; count--) {
            *cursor = 0;
            cursor = cursor + 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x491f80):

void input_device_release(void)

{
  int *piVar1;
  int iVar2;
  short unaff_SI;
  int iVar3;
  undefined4 *puVar4;

  iVar3 = (int)unaff_SI;
  piVar1 = (int *)(&DAT_006b1848)[iVar3];
  if (piVar1 != (int *)0x0) {
    (**(code **)(*piVar1 + 0x20))(piVar1);
    (**(code **)(*(int *)(&DAT_006b1848)[iVar3] + 8))((int *)(&DAT_006b1848)[iVar3]);
    (&DAT_006b1848)[iVar3] = 0;
    puVar4 = &DAT_006b1868 + iVar3 * 0x90;
    for (iVar2 = 0x90; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
  }
  return;
}
#endif
