// hs_unbind_control  (Ghidra: hs_unbind_control, already named)
// address 0x48b8d0, size 209 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: objdump 0x48b8d0..0x48b9a0 fully traced. Device-type dispatch (1/2/3) and
//   input-kind dispatch (0/1/2) match control_device_type / control_input_kind exactly. The
//   mouse branch (device_type == 2) genuinely, unconditionally prints "unbound %s key" (the
//   same immediate string address 0x00669410 used by the keyboard branch) before also printing
//   its own message -- this is reproduced exactly, not corrected, per the no-invented-behaviour
//   rule; it is flagged UNSURE below since it reads like a shipped bug rather than intent.
// register convention: EDI -> device_class_name, EBX -> input_name (EDI is used directly by
//   input_parse_device_binding_string with no incoming move, and EBX is pushed once as that
//   call's stack argument -- cleaned up right after by `add esp,0x4` -- then reused unmodified
//   as the %s argument of every message below, so it is never reloaded from anywhere).
//   // blam-cc: EDI -> device_class_name, EBX -> input_name

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

extern uint8_t input_parse_device_binding_string(const char *device_class_name, const char *input_name,
                                                  control_binding_descriptor *out_binding);
    // 0x0048fea0, blam-cc: EDI -> device_class_name, stack -> input_name, ESI -> out_binding

    // 0x0048b9b0 (this module, see input_clear_control_binding.c), blam-cc: EAX -> binding
extern void console_out_printf(uint8_t unknown, const char *format, ...); // 0x004c6860

// Console/script 'unbind' command: parses <device_class_name, input_name> into a binding
// descriptor, clears whatever game control it was assigned to, and prints a device-appropriate
// 'unbound ...' confirmation. Does nothing if the descriptor fails to parse.
void hs_unbind_control(const char *device_class_name, const char *input_name)
{
    control_binding_descriptor binding;

    if (input_parse_device_binding_string(device_class_name, input_name, &binding) == 0) {
        return;
    }

    input_clear_control_binding(&binding);

    switch (binding.device_type) {
    case _control_device_keyboard:
        console_out_printf(0, "unbound %s key", input_name);
        break;

    case _control_device_mouse:
        // UNSURE: the retail binary prints this keyboard-shaped message unconditionally here
        // too, before the mouse-specific one below. Preserved as observed (objdump confirms
        // both branches push the identical string address 0x00669410).
        console_out_printf(0, "unbound %s key", input_name);
        if (binding.input_kind == _control_input_axis) {
            console_out_printf(0, "unbound mouse axis %s", input_name);
        } else {
            console_out_printf(0, "unbound %s mouse button", input_name);
        }
        break;

    case _control_device_gamepad:
        if (binding.input_kind == _control_input_axis) {
            console_out_printf(0, "unbound axis %s on gamepad %d", input_name, binding.device_index);
        } else {
            // Also covers input_kind == _control_input_pov, which shares this message in the
            // binary (only the axis case gets its own wording).
            console_out_printf(0, "unbound %s on gamepad %d", input_name, binding.device_index);
        }
        break;

    default:
        break;
    }
}

#if 0
Original Ghidra decompilation (0x48b8d0), from tools/pack.py 0x48b8d0:

void hs_unbind_control(void)

{
  char cVar1;
  short local_c;
  short local_8;

  cVar1 = input_parse_device_binding_string();
  if (cVar1 != '\0') {
    input_clear_control_binding();
    if (local_c == 1) {
      console_out_printf('\0',"unbound %s key");
    }
    else {
      if (local_c == 2) {
        console_out_printf('\0',"unbound %s key");
        if (local_8 != 1) {
          console_out_printf('\0',"unbound %s mouse button");
          return;
        }
        console_out_printf('\0',"unbound mouse axis %s");
        return;
      }
      if (local_c == 3) {
        if (local_8 != 1) {
          console_out_printf('\0',"unbound %s on gamepad %d");
          return;
        }
        console_out_printf('\0',"unbound axis %s on gamepad %d");
        return;
      }
    }
  }
  return;
}

objdump 0x48b8d0..0x48b9a0 (Intel syntax), the full control flow this rewrite is based on:

0048b8d0: sub    esp,0xc
0048b8d3: push   esi
0048b8d4: push   ebx
0048b8d5: lea    esi,[esp+0x8]                 ; esi = &binding
0048b8d9: call   0x48fea0                       ; input_parse_device_binding_string(edi, [ebx pushed]=input_name, esi)
0048b8de: add    esp,0x4
0048b8e1: test   al,al
0048b8e3: pop    esi
0048b8e4: je     0x48b99d                       ; parse failed -> return
0048b8ea: lea    eax,[esp]                      ; eax = &binding
0048b8ed: call   0x48b9b0                       ; input_clear_control_binding(eax)
0048b8f2: movsx  eax,WORD PTR [esp]             ; eax = binding.device_type
0048b8f6: dec    eax
0048b8f7: je     0x48b98d                       ; device_type == 1 (keyboard)
0048b8fd: dec    eax
0048b8fe: je     0x48b94e                       ; device_type == 2 (mouse)
0048b900: dec    eax
0048b901: jne    0x48b99d                       ; not gamepad -> return
0048b907: movsx  eax,WORD PTR [esp+0x4]         ; eax = binding.input_kind
0048b90c: dec    eax
0048b90d: je     0x48b934                       ; input_kind == 1 (axis)
0048b90f: dec    eax
0048b910: je     0x48b91a                       ; input_kind == 2 (pov)
0048b912: movsx  eax,WORD PTR [esp+0x2]         ; button: eax = device_index
0048b917: push   eax
0048b918: jmp    0x48b920
0048b91a: movsx  ecx,WORD PTR [esp+0x2]         ; pov: ecx = device_index
0048b91f: push   ecx
0048b920: push   ebx                            ; input_name
0048b921: push   0x6693a4                       ; "unbound %s on gamepad %d"
0048b926: push   0x0
0048b928: call   0x4c6860
0048b92d: add    esp,0x10
0048b930: add    esp,0xc
0048b933: ret
0048b934: movsx  edx,WORD PTR [esp+0x2]         ; axis: edx = device_index
0048b939: push   edx
0048b93a: push   ebx
0048b93b: push   0x6693c0                       ; "unbound axis %s on gamepad %d"
0048b940: push   0x0
0048b942: call   0x4c6860
0048b947: add    esp,0x10
0048b94a: add    esp,0xc
0048b94d: ret
0048b94e: push   ebx                            ; mouse: always prints the keyboard message first
0048b94f: push   0x669410                       ; "unbound %s key"
0048b954: push   0x0
0048b956: call   0x4c6860
0048b95b: movsx  eax,WORD PTR [esp+0x10]        ; eax = binding.input_kind (reload)
0048b960: add    esp,0xc
0048b963: dec    eax
0048b964: push   ebx
0048b965: je     0x48b97a                       ; input_kind == 1 (axis)
0048b967: push   0x6693e0                       ; "unbound %s mouse button"
0048b96c: push   0x0
0048b96e: call   0x4c6860
0048b973: add    esp,0xc
0048b976: add    esp,0xc
0048b979: ret
0048b97a: push   0x6693f8                       ; "unbound mouse axis %s"
0048b97f: push   0x0
0048b981: call   0x4c6860
0048b986: add    esp,0xc
0048b989: add    esp,0xc
0048b98c: ret
0048b98d: push   ebx                            ; keyboard
0048b98e: push   0x669410                       ; "unbound %s key"
0048b993: push   0x0
0048b995: call   0x4c6860
0048b99a: add    esp,0xc
0048b99d: add    esp,0xc
0048b9a0: ret
#endif
