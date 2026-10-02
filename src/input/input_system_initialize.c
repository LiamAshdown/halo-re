// input_system_initialize  (Ghidra: already named)
// address 0x491a80, size 297 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "One-time initialization of the input
// subsystem: clears all action-binding tables and per-device state, and registers for
// DirectInput device-change notifications."; types/input.h documents every field this builds:
// the 0x50-entry joystick_objects DIOBJECTDATAFORMAT table (axes offsets 0..0x7c type
// 0x80ffff03, POVs 0x80..0xbc type 0x80ffff10, buttons 0xc0..0xdf type 0x80ffff0c), the
// input_devices[8] reset (zeroed, slot seeded -1), the EnumDevices registration (callback
// 0x491d70, "not a Ghidra function" per out/phase4/input_types_notes.md), joystick_neutral_state
// (zeroed, povs seeded -1) and joystick_slot_devices[4] (seeded -1).
// UNSURE: returns the literal 0xffffff01; only the low byte (1) looks meaningful.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern di_object_data_format joystick_objects[k_input_joystick_object_count]; // 0x00879f60
extern input_device input_devices[8];       // 0x006b1868
extern int32_t nojoystick;                  // 0x00712c2c, shell module
extern void *direct_input;                  // 0x006b15fc, IDirectInput8A*
extern joystick_state joystick_neutral_state; // 0x006b2cf8
extern int32_t joystick_slot_devices[4];    // 0x006b2ce8


// Builds the joystick DIOBJECTDATAFORMAT table, clears every input_device slot (seeding
// slot = -1), registers the (non-Ghidra) EnumDevices callback at 0x491d70 for attached game
// controllers unless -nojoystick was passed, and resets the neutral joystick state and the
// slot-to-device map.
// FIXED 2026-09-28 (retail-independence loop): the EnumDevices callback is the C input_enumerate_gamepad_callback,
// not the literal retail address 0x491d70 (original code the standalone cannot run).
extern int32_t __stdcall input_enumerate_gamepad_callback(const di_device_instance *instance, void *reference); // 0x491d70

uint32_t input_system_initialize(void)
{
    int32_t i;

    for (i = 0; i < 0x20; i++) {
        joystick_objects[i].guid = 0;
        joystick_objects[i].offset = i * 4;
        joystick_objects[i].type = 0x80ffff03; // axis
        joystick_objects[i].flags = 0;
    }
    for (i = 0; i < 0x10; i++) {
        joystick_objects[0x20 + i].guid = 0;
        joystick_objects[0x20 + i].offset = 0x80 + i * 4;
        joystick_objects[0x20 + i].type = 0x80ffff10; // pov
        joystick_objects[0x20 + i].flags = 0;
    }
    for (i = 0; i < 0x20; i++) {
        joystick_objects[0x30 + i].guid = 0;
        joystick_objects[0x30 + i].offset = 0xc0 + i;
        joystick_objects[0x30 + i].type = 0x80ffff0c; // button
        joystick_objects[0x30 + i].flags = 0;
    }

    for (i = 0; i < 8; i++) {
        memset(&input_devices[i], 0, sizeof(input_device));
        input_devices[i].slot = -1;
    }

    if (nojoystick == 0) {
        ((idirectinput8_enumdevices_proc)(*(void ***)direct_input)[4])(direct_input, 4,
            (void *)input_enumerate_gamepad_callback, (void *)0, 1); // EnumDevices, DI8DEVCLASS_GAMECTRL, DIEDFL_ATTACHEDONLY
    }

    memset(&joystick_neutral_state, 0, sizeof(joystick_neutral_state));
    for (i = 0; i < 0x10; i++) {
        joystick_neutral_state.povs[i] = -1;
    }

    for (i = 0; i < 4; i++) {
        joystick_slot_devices[i] = -1;
    }

    return 0xffffff01;
}

#if 0
Original Ghidra decompilation (0x491a80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __cdecl input_system_initialize(void)

{
  int *piVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;

  iVar2 = 0;
  piVar1 = &DAT_00879f64;
  do {
    piVar1[-1] = 0;
    *piVar1 = -1;
    piVar1[1] = -0x7f0000fd;
    piVar1[2] = 0;
    *piVar1 = iVar2;
    piVar1 = piVar1 + 4;
    iVar2 = iVar2 + 4;
  } while ((int)piVar1 < 0x87a164);
  piVar1 = &DAT_0087a164;
  iVar2 = 0x80;
  do {
    piVar1[-1] = 0;
    *piVar1 = -1;
    piVar1[1] = -0x7f0000f0;
    piVar1[2] = 0;
    *piVar1 = iVar2;
    iVar2 = iVar2 + 4;
    piVar1 = piVar1 + 4;
  } while (iVar2 < 0xc0);
  iVar2 = 0;
  piVar1 = &DAT_0087a264;
  do {
    piVar1[-1] = 0;
    *piVar1 = -1;
    piVar1[1] = -0x7f0000f4;
    piVar1[2] = 0;
    *piVar1 = iVar2 + 0xc0;
    piVar1 = piVar1 + 4;
    iVar2 = iVar2 + 1;
  } while ((int)piVar1 < 0x87a464);
  puVar3 = &DAT_006b1a98;
  do {
    puVar4 = puVar3 + -0x8c;
    for (iVar2 = 0x90; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 0x90;
  } while ((int)puVar3 < 0x6b2c98);
  if (DAT_00712c2c == 0) {
    (**(code **)(*DAT_006b15fc + 0x10))(DAT_006b15fc,4,&LAB_00491d70,0,1);
  }
  puVar3 = &DAT_006b2cf8;
  for (iVar2 = 0x28; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar3 = &DAT_006b2d58;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0xffffffff;
    puVar3 = puVar3 + 1;
  }
  DAT_006b2ce8 = 0xffffffff;
  _DAT_006b2cec = 0xffffffff;
  _DAT_006b2cf0 = 0xffffffff;
  _DAT_006b2cf4 = 0xffffffff;
  return 0xffffff01;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
