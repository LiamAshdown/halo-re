// input_get_binding_display_name  (Ghidra: FUN_0048c7f0; renamed per its behavior)
// address 0x48c7f0, size 174 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: objdump 0x48c7f0..0x48c89d fully traced. Dispatches on binding->device_type /
//   input_kind to the matching already-rewritten per-device name formatter, all of which this
//   function reaches with the exact register/stack layout each one documents: EAX/EBX for the
//   keyboard and mouse-button single-index formatters, three plain stack args for the axis/pov
//   formatters (chimera__axis_text, chimera__pov_text), and axis_index+direction on the stack
//   plus ESI for input_get_mouse_axis_name.
// register convention: EAX -> binding, ECX -> out_text.
//   // blam-cc: EAX -> binding, ECX -> out_text

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

extern void input_get_keyboard_key_name(int16_t key_index, uint16_t *out_name);            // this module, 0x490e30
extern void input_get_mouse_button_name(int16_t button_index, uint16_t *out_name);         // this module, 0x490f20
extern void input_get_mouse_axis_name(int16_t axis_index, uint8_t direction, uint16_t *out_name); // this module, 0x491010
extern void chimera__button_text(int16_t button_index, uint16_t *out_text);                 // this module, 0x491270
extern void chimera__axis_text(int16_t axis_index, uint8_t direction, uint16_t *out_text);   // this module, 0x491340
extern void chimera__pov_text(int16_t pov_index, int16_t direction_index, uint16_t *out_text); // this module, 0x4914c0

// Given a device-input descriptor, dispatches to the correct per-device name/text formatting
// routine (keyboard key, mouse button/axis, or joystick button/axis/pov), writing the result
// into out_text. Does nothing for a device_type/input_kind combination that is not recognized
// (keyboard with input_kind != button, or a device_type outside 1..3).
void input_get_binding_display_name(control_binding_descriptor *binding, uint16_t *out_text)
{
    switch (binding->device_type) {
    case _control_device_keyboard:
        if (binding->input_kind == _control_input_button) {
            input_get_keyboard_key_name(binding->input_index, out_text);
        }
        break;

    case _control_device_mouse:
        if (binding->input_kind == _control_input_button) {
            input_get_mouse_button_name(binding->input_index, out_text);
        } else if (binding->input_kind == _control_input_axis) {
            input_get_mouse_axis_name(binding->input_index, binding->direction == 1, out_text);
        }
        break;

    case _control_device_gamepad:
        switch (binding->input_kind) {
        case _control_input_button:
            chimera__button_text(binding->input_index, out_text);
            break;
        case _control_input_axis:
            chimera__axis_text(binding->input_index, binding->direction == 1, out_text);
            break;
        case _control_input_pov:
            chimera__pov_text(binding->input_index, (int16_t)binding->direction, out_text);
            break;
        }
        break;

    default:
        break;
    }
}

#if 0
Original Ghidra decompilation (0x48c7f0), from tools/pack.py 0x48c7f0:

void FUN_0048c7f0(void)

{
  short sVar1;
  short *in_EAX;

  sVar1 = *in_EAX;
  if (sVar1 == 1) {
    if (in_EAX[2] == 0) {
      input_get_keyboard_key_name();
      return;
    }
  }
  else if (sVar1 == 2) {
    if (in_EAX[2] == 0) {
      input_get_mouse_button_name();
      return;
    }
    if (in_EAX[2] == 1) {
      input_get_mouse_axis_name(in_EAX[3],*(int *)(in_EAX + 4) == 1);
      return;
    }
  }
  else if (sVar1 == 3) {
    sVar1 = in_EAX[2];
    if (sVar1 == 0) {
      chimera__button_text((int)in_EAX[3]);
      return;
    }
    if (sVar1 == 1) {
      chimera__axis_text(in_EAX[3],*(int *)(in_EAX + 4) == 1);
      return;
    }
    if (sVar1 == 2) {
      chimera__pov_text(in_EAX[3],in_EAX[4]);
    }
  }
  return;
}

objdump 0x48c7f0..0x48c89d (Intel syntax):

0048c7f0: push   ebx
0048c7f1: mov    ebx,ecx                        ; ebx = out_text
0048c7f3: movsx  ecx,WORD PTR [eax]              ; device_type
0048c7f6: dec    ecx
0048c7f7: je     0x48c88c                        ; == 1 (keyboard)
0048c7fd: dec    ecx
0048c7fe: push   esi
0048c7ff: je     0x48c858                        ; == 2 (mouse)
0048c801: dec    ecx
0048c802: jne    0x48c82a                        ; != 3 -> return
0048c804: movsx  ecx,WORD PTR [eax+0x4]          ; input_kind (gamepad)
0048c808: sub    ecx,0x0
0048c80b: je     0x48c848                        ; button
0048c80d: dec    ecx
0048c80e: je     0x48c82d                        ; axis
0048c810: dec    ecx
0048c811: jne    0x48c82a                        ; != pov -> return
0048c813: xor    ecx,ecx
0048c815: mov    cx,WORD PTR [eax+0x8]           ; direction (octant)
0048c819: xor    edx,edx
0048c81b: mov    dx,WORD PTR [eax+0x6]           ; input_index (pov)
0048c81f: push   ebx
0048c820: push   ecx
0048c821: push   edx
0048c822: call   0x4914c0                        ; chimera__pov_text(pov_index, octant, out_text)
0048c827: add    esp,0xc
0048c82a: pop    esi
0048c82b: pop    ebx
0048c82c: ret
0048c82d: cmp    DWORD PTR [eax+0x8],0x1
0048c831: sete   cl                              ; cl = (direction == 1)
0048c834: xor    edx,edx
0048c836: mov    dx,WORD PTR [eax+0x6]           ; input_index (axis)
0048c83a: push   ebx
0048c83b: push   ecx
0048c83c: push   edx
0048c83d: call   0x491340                        ; chimera__axis_text(axis_index, direction, out_text)
0048c842: add    esp,0xc
0048c845: pop    esi
0048c846: pop    ebx
0048c847: ret
0048c848: movsx  eax,WORD PTR [eax+0x6]          ; input_index (button)
0048c84c: push   eax
0048c84d: call   0x491270                        ; chimera__button_text(button_index, ebx=out_text)
0048c852: add    esp,0x4
0048c855: pop    esi
0048c856: pop    ebx
0048c857: ret
0048c858: movsx  ecx,WORD PTR [eax+0x4]          ; input_kind (mouse)
0048c85c: sub    ecx,0x0
0048c85f: je     0x48c880                        ; button
0048c861: dec    ecx
0048c862: jne    0x48c82a                        ; != axis -> return
0048c864: cmp    DWORD PTR [eax+0x8],0x1
0048c868: sete   cl                              ; cl = (direction == 1)
0048c86b: xor    edx,edx
0048c86d: mov    dx,WORD PTR [eax+0x6]           ; input_index (axis)
0048c871: mov    esi,ebx                          ; esi = out_text
0048c873: push   ecx
0048c874: push   edx
0048c875: call   0x491010                        ; input_get_mouse_axis_name(axis_index, direction, esi=out_text)
0048c87a: add    esp,0x8
0048c87d: pop    esi
0048c87e: pop    ebx
0048c87f: ret
0048c880: mov    ax,WORD PTR [eax+0x6]           ; input_index (button)
0048c884: call   0x490f20                        ; input_get_mouse_button_name(ax=button_index, ebx=out_text)
0048c889: pop    esi
0048c88a: pop    ebx
0048c88b: ret
0048c88c: cmp    WORD PTR [eax+0x4],0x0          ; input_kind == button ?
0048c891: jne    0x48c82b
0048c893: mov    ax,WORD PTR [eax+0x6]           ; input_index (key)
0048c897: call   0x490e30                        ; input_get_keyboard_key_name(ax=key_index, ebx=out_text)
0048c89c: pop    ebx
0048c89d: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
