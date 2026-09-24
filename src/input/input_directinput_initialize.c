// input_directinput_initialize  (Ghidra: already named)
// address 0x490520, size 90 bytes
// name confidence: 0.85   rewrite confidence: 0.75
// evidence: out/phase4/input_functions.md summary "Creates the DirectInput object and the
// keyboard, mouse, and joystick device objects, then acquires them; logs and tears down on
// failure."; types/input.h documents 0x0064e2ac as IID_IDirectInput8A and 0x00746268 /
// 0x007461c0 as the shell's cached DirectInput8Create FARPROC / hInstance (this module reads,
// does not own, both -- see src/shell/engine_initialize_subsystems.c and
// src/shell/shell_display_fatal_error_dialog.c for their established names/types).
// directinput8create_proc: types/input.h (the DirectInput 8 method block).
// register convention: no parameters; returns AL (success/failure).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern void *shell_instance;         // 0x007461c0, HINSTANCE
extern void *direct_input8_create;   // 0x00746268, FARPROC (shell module)
extern input_guid iid_directinput8a; // 0x0064e2ac, IID_IDirectInput8A
extern void *direct_input;           // 0x006b15fc, IDirectInput8A*

extern void input_directinput_release_devices(void); // this module, 0x490580
extern uint8_t input_keyboard_device_create(void);    // this module, 0x4918a0 (result ignored here)
extern uint8_t input_mouse_device_create(void);       // this module, 0x4919c0 (result ignored here)
extern uint32_t input_system_initialize(void);        // this module, 0x491a80 (result ignored here)
extern void input_directinput_acquire_devices(void);  // this module, 0x490620
extern void input_error_log_once(int32_t error_code, char *description, ...); // this module, 0x492150


// Creates the shared IDirectInput8A object and, on success, the keyboard, mouse, and joystick
// device objects, then acquires everything; logs and releases whatever was created on failure.
// Returns nonzero on success.
uint8_t input_directinput_initialize(void)
{
    int32_t hr;

    hr = ((directinput8create_proc)direct_input8_create)(shell_instance, 0x800,
        &iid_directinput8a, &direct_input, (void *)0);
    if (hr < 0) {
        input_error_log_once(hr, "DirectInputCreate");
        input_directinput_release_devices();
    } else {
        input_keyboard_device_create();
        input_mouse_device_create();
        input_system_initialize();
        input_directinput_acquire_devices();
    }
    return hr >= 0;
}

#if 0
Original Ghidra decompilation (0x490520):

bool input_directinput_initialize(void)

{
  int iVar1;

  iVar1 = (*DAT_00746268)(DAT_007461c0,0x800,&DAT_0064e2ac,&DAT_006b15fc,0);
  if (iVar1 < 0) {
    input_error_log_once(iVar1,"DirectInputCreate");
    input_directinput_release_devices();
  }
  else {
    input_keyboard_device_create();
    input_mouse_device_create();
    input_system_initialize();
    input_directinput_acquire_devices();
  }
  return -1 < iVar1;
}
#endif
