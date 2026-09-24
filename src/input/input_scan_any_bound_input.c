// input_scan_any_bound_input  (Ghidra: already named)
// address 0x48f8c0, size 1043 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: out/phase4/input_functions.md summary "Waits for and detects the next raw input
// activation (any key, mouse button/axis, joystick button/axis/pov) for use by the
// control-rebinding capture UI."; types/input.h documents the exact result layout
// (control_binding_descriptor scan_result at input_globals.scan_result) and the axis-compare
// baseline (scan_baselines[4], ending exactly at scan_result, confirming the struct boundary),
// plus both scan thresholds (k_input_scan_mouse_threshold, k_input_scan_axis_threshold).
// UNSURE: when mouse_device is 0, the mouse-axis section dereferences a NULL pointer
// (`piVar4 = (int*)0` is never reassigned in that case, per the decompiled `&&` short-circuit),
// exactly as the binary does; not defended against here.
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern void *mouse_device;                    // 0x006b1804
extern uint8_t input_suppressed;              // 0x006b15f9
extern mouse_state live_mouse_state;          // 0x006b180c
extern mouse_state mouse_neutral_state;       // 0x006b1828
extern int32_t joystick_slot_devices[4];      // 0x006b2ce8
extern input_device input_devices[8];         // 0x006b1868
extern joystick_state joystick_states[4];     // 0x006b2a68
extern joystick_state joystick_neutral_state; // 0x006b2cf8
extern input_abstraction_globals input_globals; // 0x00710328

extern uint8_t input_get_key_state(int16_t key_index); // this module, 0x490b50

// Scans, in priority order, for the next "fresh" raw input activation: a mouse button just
// pressed (hold count exactly 1), a keyboard key just pressed, a joystick button currently held
// (any hold count), the mouse X/Y axes or wheel past k_input_scan_mouse_threshold, a joystick
// axis whose delta from its scan_baselines snapshot exceeds k_input_scan_axis_threshold, or a
// non-centered joystick POV. Writes the result into input_globals.scan_result (all zero if
// nothing is found).
void input_scan_any_bound_input(void)
{
    control_binding_descriptor *result;
    int32_t slot;
    int32_t i;
    joystick_state *source;
    mouse_state *mouse;
    int32_t delta;

    result = &input_globals.scan_result;

    for (i = 0; i < k_input_mouse_button_count; i++) {
        if (mouse_device != 0 && input_suppressed == 0 && live_mouse_state.button_frames[i] == 1) {
            result->device_type = _control_device_mouse;
            result->device_index = 0;
            result->input_kind = _control_input_button;
            result->input_index = (int16_t)i;
            result->direction = 0;
            return;
        }
    }

    for (i = 0; i < k_control_keyboard_key_count; i++) {
        if (input_get_key_state((int16_t)i) == 1) {
            result->device_type = _control_device_keyboard;
            result->device_index = 0;
            result->input_kind = _control_input_button;
            result->input_index = (int16_t)i;
            result->direction = 0;
            return;
        }
    }

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] != -1) {
            source = (input_suppressed == 0) ? &joystick_states[slot] : &joystick_neutral_state;
            for (i = 0; i < input_devices[joystick_slot_devices[slot]].button_count; i++) {
                if (source->button_frames[i] != 0) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_button;
                    result->input_index = (int16_t)i;
                    result->direction = 0;
                    return;
                }
            }
        }
    }

    // UNSURE: if mouse_device is 0, `mouse` is never assigned and the reads below are a NULL
    // dereference in the original binary; reproduced literally (see file header).
    mouse = (mouse_device == 0) ? (mouse_state *)0
            : (input_suppressed != 0) ? &mouse_neutral_state : &live_mouse_state;

    if (mouse->x > k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 0;
        result->direction = 1;
        return;
    }
    if (mouse->x < -k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 0;
        result->direction = 2;
        return;
    }

    if (mouse->y > k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 1;
        result->direction = 1;
        return;
    }

    if (mouse->y < -k_input_scan_mouse_threshold) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 1;
        result->direction = 2;
        return;
    }
    if (mouse->wheel > 0) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 2;
        result->direction = 1;
        return;
    }
    if (mouse->wheel < 0) {
        result->device_type = _control_device_mouse;
        result->device_index = 0;
        result->input_kind = _control_input_axis;
        result->input_index = 2;
        result->direction = 2;
        return;
    }

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] != -1) {
            source = (input_suppressed == 0) ? &joystick_states[slot] : &joystick_neutral_state;
            for (i = 0; i < input_devices[joystick_slot_devices[slot]].axis_count; i++) {
                delta = (int32_t)source->axes[i] - (int32_t)input_globals.scan_baselines[slot].axes[i];
                if (delta > k_input_scan_axis_threshold) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_axis;
                    result->input_index = (int16_t)i;
                    result->direction = 1;
                    return;
                }
                if (delta < -k_input_scan_axis_threshold) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_axis;
                    result->input_index = (int16_t)i;
                    result->direction = 2;
                    return;
                }
            }
        }
    }

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] != -1) {
            source = (input_suppressed == 0) ? &joystick_states[slot] : &joystick_neutral_state;
            for (i = 0; i < input_devices[joystick_slot_devices[slot]].pov_count; i++) {
                if (source->povs[i] != -1) {
                    result->device_type = _control_device_gamepad;
                    result->device_index = (int16_t)slot;
                    result->input_kind = _control_input_pov;
                    result->input_index = (int16_t)i;
                    result->direction = source->povs[i];
                    return;
                }
            }
        }
    }

    result->device_type = 0;
    result->device_index = 0;
    result->input_kind = 0;
    result->input_index = 0;
    result->direction = 0;
}

#if 0
Original Ghidra decompilation (0x48f8c0):

void input_scan_any_bound_input(void)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  int *piVar4;
  undefined4 *puVar5;
  int iVar6;
  short *psVar7;
  short sVar8;

  iVar2 = 0;
  do {
    if (((DAT_006b1804 != 0) && (DAT_006b15f9 == '\0')) && ((&DAT_006b1818)[(short)iVar2] == '\x01')
       ) {
      DAT_007127cc = 0;
      DAT_007127c4 = 2;
      DAT_007127c8 = iVar2 << 0x10;
      return;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 8);
  iVar2 = 0;
  do {
    cVar1 = FUN_00490b50();
    if (cVar1 == '\x01') {
      DAT_007127c8 = iVar2 << 0x10;
      DAT_007127cc = 0;
      DAT_007127c4 = 1;
      return;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x6d);
  iVar2 = 0;
  do {
    sVar8 = (short)iVar2;
    if ((&DAT_006b2ce8)[sVar8] != -1) {
      iVar6 = 0;
      if (0 < (short)*(undefined4 *)(&DAT_006b1aa0 + (&DAT_006b2ce8)[sVar8] * 0x240)) {
        do {
          if (DAT_006b15f9 == '\0') {
            puVar3 = &DAT_006b2a68 + sVar8 * 0x28;
            if (puVar3 != (undefined4 *)0x0) goto LAB_0048f9ab;
          }
          else {
            puVar3 = &DAT_006b2cf8;
LAB_0048f9ab:
            if (*(char *)((int)puVar3 + (int)(short)iVar6) != '\0') {
              DAT_007127c4 = CONCAT22(sVar8,3);
              DAT_007127c8 = iVar6 << 0x10;
              DAT_007127cc = 0;
              return;
            }
          }
          iVar6 = iVar6 + 1;
        } while (iVar6 < (short)*(undefined4 *)(&DAT_006b1aa0 + (&DAT_006b2ce8)[sVar8] * 0x240));
      }
    }
    iVar2 = iVar2 + 1;
    if (3 < iVar2) {
      piVar4 = (int *)0x0;
      if ((DAT_006b1804 != 0) && (piVar4 = &DAT_006b1828, DAT_006b15f9 == '\0')) {
        piVar4 = &DAT_006b180c;
      }
      if (0x46 < *piVar4) {
        DAT_007127c8 = 1;
        DAT_007127cc = 1;
        DAT_007127c4 = 2;
        return;
      }
      if (*piVar4 < -0x46) {
        DAT_007127c8 = 1;
        DAT_007127c4 = 2;
        DAT_007127cc = 2;
        return;
      }
      if (piVar4[1] < 0x47) {
        if (piVar4[1] < -0x46) {
          DAT_007127c8 = 0x10001;
          DAT_007127c4 = 2;
          DAT_007127cc = 2;
          return;
        }
        if (0 < piVar4[2]) {
          DAT_007127cc = 1;
          DAT_007127c4 = 2;
          DAT_007127c8 = 0x20001;
          return;
        }
        if (piVar4[2] < 0) {
          DAT_007127c4 = 2;
          DAT_007127c8 = 0x20001;
          DAT_007127cc = 2;
          return;
        }
        sVar8 = 0;
        puVar3 = &DAT_00712544;
        do {
          if ((&DAT_006b2ce8)[sVar8] != -1) {
            if (DAT_006b15f9 == '\0') {
              puVar5 = &DAT_006b2a68 + sVar8 * 0x28;
            }
            else {
              puVar5 = &DAT_006b2cf8;
            }
            iVar2 = 0;
            if (0 < (short)*(undefined4 *)(&DAT_006b1a9c + (&DAT_006b2ce8)[sVar8] * 0x240)) {
              psVar7 = (short *)(puVar5 + 8);
              do {
                iVar6 = (int)*psVar7 - (int)*(short *)(((int)puVar3 - (int)puVar5) + (int)psVar7);
                if (0x4cc < iVar6) {
                  DAT_007127c4 = CONCAT22(sVar8,3);
                  DAT_007127c8 = CONCAT22((short)iVar2,1);
                  DAT_007127cc = 1;
                  return;
                }
                if (iVar6 < -0x4cc) {
                  DAT_007127c4 = CONCAT22(sVar8,3);
                  DAT_007127c8 = CONCAT22((short)iVar2,1);
                  DAT_007127cc = 2;
                  return;
                }
                iVar2 = iVar2 + 1;
                psVar7 = psVar7 + 1;
              } while (iVar2 < (short)*(undefined4 *)
                                       (&DAT_006b1a9c + (&DAT_006b2ce8)[sVar8] * 0x240));
            }
          }
          sVar8 = sVar8 + 1;
          puVar3 = puVar3 + 0x28;
          if (0x7127c3 < (int)puVar3) {
            iVar2 = 0;
            do {
              sVar8 = (short)iVar2;
              if ((&DAT_006b2ce8)[sVar8] != -1) {
                if (DAT_006b15f9 == '\0') {
                  puVar3 = &DAT_006b2a68 + sVar8 * 0x28;
                }
                else {
                  puVar3 = &DAT_006b2cf8;
                }
                iVar6 = 0;
                if (0 < (short)*(undefined4 *)(&DAT_006b1aa4 + (&DAT_006b2ce8)[sVar8] * 0x240)) {
                  piVar4 = puVar3 + 0x18;
                  do {
                    if (*piVar4 != -1) {
                      DAT_007127c4 = CONCAT22(sVar8,3);
                      DAT_007127c8 = CONCAT22((short)iVar6,2);
                      DAT_007127cc = puVar3[iVar6 + 0x18];
                      return;
                    }
                    iVar6 = iVar6 + 1;
                    piVar4 = piVar4 + 1;
                  } while (iVar6 < (short)*(undefined4 *)
                                           (&DAT_006b1aa4 + (&DAT_006b2ce8)[sVar8] * 0x240));
                }
              }
              iVar2 = iVar2 + 1;
            } while (iVar2 < 4);
            DAT_007127c4 = 0;
            DAT_007127c8 = 0;
            DAT_007127cc = 0;
            return;
          }
        } while( true );
      }
      DAT_007127c8 = 0x10001;
      DAT_007127cc = 1;
      DAT_007127c4 = 2;
      return;
    }
  } while( true );
}
#endif
