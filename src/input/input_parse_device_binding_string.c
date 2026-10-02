// input_parse_device_binding_string  (Ghidra: already named)
// address 0x48fea0, size 431 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/input_functions.md summary "Parses a device-class name string
// (keyboard/mouse/mouseaxis/joystick/joystickaxis/joystickpov) and its input name into a
// structured binding descriptor."; objdump of 0x48fea0..0x49004e resolves every register: name
// is the sole stack parameter, forwarded into EAX right before the three calls
// (input_joystick_button/axis/pov_name_to_index) that expect it there, device_class_name is in
// EDI, and out_binding in ESI. The literal device-class strings (0x48fea0..0x48ffe5 in .rdata)
// resolve "keyboard" and its "key" alias, "mouse", "mouseaxis", "joystick", "joystickaxis" and
// "joystickpov" in that order.
// register convention: device_class_name in EDI (unaff_EDI); name as the one stack parameter
// (param_1), forwarded to EAX for the three callees that expect it there; out_binding in ESI
// (unaff_ESI)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t _stricmp(const char *a, const char *b); // 0x628d8b, libc

extern uint32_t input_keyboard_key_name_to_index(char *name);                     // this module, 0x490ea0
extern uint32_t input_mouse_button_name_to_index(char *name);                     // this module, 0x490f90
extern uint32_t input_mouse_axis_name_to_index(char *name, uint8_t *out_direction); // this module, 0x4910c0
extern int16_t input_joystick_button_name_to_index(char *name);                   // this module, 0x4912e0, blam-cc: EAX
extern int16_t input_joystick_axis_name_to_index(char *name, uint8_t *out_direction); // this module, 0x4913e0, blam-cc: EAX
extern int16_t input_joystick_pov_name_to_index(char *name, int16_t *out_direction);  // this module, 0x491590, blam-cc: EAX

// blam-cc: device_class_name in EDI, name on the stack, out_binding in ESI
// Parses device_class_name (keyboard/key, mouse, mouseaxis, joystick, joystickaxis, joystickpov)
// and name into *out_binding. Returns 1 on success, 0 if device_class_name is unrecognized or
// name doesn't resolve within that class.
uint8_t input_parse_device_binding_string(char *device_class_name, char *name,
                                           control_binding_descriptor *out_binding)
{
    uint32_t index;
    int16_t joystick_index;
    uint8_t byte_direction;
    int16_t pov_direction;

    if (_stricmp(device_class_name, "keyboard") == 0 || _stricmp(device_class_name, "key") == 0) {
        index = input_keyboard_key_name_to_index(name);
        if (index == 0xffff) {
            return 0;
        }
        out_binding->device_type = _control_device_keyboard;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_button;
        out_binding->input_index = (int16_t)index;
        out_binding->direction = 0;
        return 1;
    }

    if (_stricmp(device_class_name, "mouse") == 0) {
        index = input_mouse_button_name_to_index(name);
        if (index == 0xffff) {
            return 0;
        }
        out_binding->device_type = _control_device_mouse;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_button;
        out_binding->input_index = (int16_t)index;
        out_binding->direction = 0;
        return 1;
    }

    if (_stricmp(device_class_name, "mouseaxis") == 0) {
        index = input_mouse_axis_name_to_index(name, &byte_direction);
        if (index == 0xffff) {
            return 0;
        }
        out_binding->device_type = _control_device_mouse;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_axis;
        out_binding->input_index = (int16_t)index;
        out_binding->direction = (byte_direction == 0) ? 2 : 1;
        return 1;
    }

    if (_stricmp(device_class_name, "joystick") == 0) {
        joystick_index = input_joystick_button_name_to_index(name);
        if (joystick_index == -1) {
            return 0;
        }
        out_binding->device_type = _control_device_gamepad;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_button;
        out_binding->input_index = joystick_index;
        out_binding->direction = 0;
        return 1;
    }

    if (_stricmp(device_class_name, "joystickaxis") == 0) {
        joystick_index = input_joystick_axis_name_to_index(name, &byte_direction);
        if (joystick_index == -1) {
            return 0;
        }
        out_binding->device_type = _control_device_gamepad;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_axis;
        out_binding->input_index = joystick_index;
        out_binding->direction = (byte_direction == 0) ? 2 : 1;
        return 1;
    }

    if (_stricmp(device_class_name, "joystickpov") == 0) {
        joystick_index = input_joystick_pov_name_to_index(name, &pov_direction);
        if (joystick_index == -1) {
            return 0;
        }
        out_binding->device_type = _control_device_gamepad;
        out_binding->device_index = 0;
        out_binding->input_kind = _control_input_pov;
        out_binding->input_index = joystick_index;
        out_binding->direction = pov_direction;
        return 1;
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x48fea0):

undefined4 input_parse_device_binding_string(undefined4 param_1)

{
  short sVar1;
  int iVar2;
  undefined2 *unaff_ESI;
  char *unaff_EDI;
  undefined4 local_4;

  iVar2 = __stricmp(unaff_EDI,"keyboard");
  if ((iVar2 == 0) || (iVar2 = __stricmp(unaff_EDI,(char *)&PTR_DAT_00669328), iVar2 == 0)) {
    sVar1 = input_keyboard_key_name_to_index(param_1);
    if (sVar1 == -1) {
      return 0;
    }
    *unaff_ESI = 1;
  }
  else {
    iVar2 = __stricmp(unaff_EDI,"mouse");
    if (iVar2 == 0) {
      sVar1 = input_mouse_button_name_to_index(param_1);
      if (sVar1 == -1) {
        return 0;
      }
      *unaff_ESI = 2;
    }
    else {
      iVar2 = __stricmp(unaff_EDI,"mouseaxis");
      if (iVar2 == 0) {
        sVar1 = input_mouse_axis_name_to_index(param_1,&local_4);
        if (sVar1 == -1) {
          return 0;
        }
        unaff_ESI[1] = 0;
        unaff_ESI[3] = sVar1;
        *unaff_ESI = 2;
        unaff_ESI[2] = 1;
        *(uint *)(unaff_ESI + 4) = ((char)local_4 == '\0') + 1;
        return 1;
      }
      iVar2 = __stricmp(unaff_EDI,"joystick");
      if (iVar2 != 0) {
        iVar2 = __stricmp(unaff_EDI,"joystickaxis");
        if (iVar2 == 0) {
          sVar1 = FUN_004913e0(&local_4);
          if (sVar1 == -1) {
            return 0;
          }
          unaff_ESI[3] = sVar1;
          unaff_ESI[1] = 0;
          *unaff_ESI = 3;
          unaff_ESI[2] = 1;
          *(uint *)(unaff_ESI + 4) = ((char)local_4 == '\0') + 1;
          return 1;
        }
        iVar2 = __stricmp(unaff_EDI,"joystickpov");
        if (iVar2 != 0) {
          return 0;
        }
        sVar1 = FUN_00491590(&local_4);
        if (sVar1 == -1) {
          return 0;
        }
        unaff_ESI[1] = 0;
        unaff_ESI[3] = sVar1;
        *unaff_ESI = 3;
        unaff_ESI[2] = 2;
        *(int *)(unaff_ESI + 4) = (int)(short)local_4;
        return 1;
      }
      sVar1 = FUN_004912e0();
      if (sVar1 == -1) {
        return 0;
      }
      *unaff_ESI = 3;
    }
  }
  *(undefined4 *)(unaff_ESI + 4) = 0;
  unaff_ESI[2] = 0;
  unaff_ESI[1] = 0;
  unaff_ESI[3] = sVar1;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
