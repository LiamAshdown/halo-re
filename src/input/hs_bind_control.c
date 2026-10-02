// hs_bind_control  (Ghidra: hs_bind_control, already named)
// address 0x48b750, size 90 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: objdump 0x48b750..0x48b7a9 traced against the three callees. input_parse_device_binding_string
//   0x48fea0's own prologue reads its device-class string straight out of EDI with no incoming
//   move (confirming EDI is its live-in register argument) and its single stack argument from
//   [esp+0x10] (the input-name text); input_action_name_to_index 0x48fe60's prologue likewise
//   uses EBX directly as its string argument with no incoming move. At the final printf call
//   site, ECX is reloaded straight from the same stack slot ([esp+0x20] == [esp+0x1c] earlier in
//   the frame after further pushes) that was passed to input_parse_device_binding_string,
//   confirming it is hs_bind_control's own first stack argument (the input-name text) rather
//   than a new value.
// register convention: EAX -> device_class_name, stack -> input_name, then action_name.
//   // blam-cc: EAX -> device_class_name (survives in EDI across both parses), stack ->
//   input_name (uint32_t/char*), stack -> action_name (uint32_t/char*)

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

extern uint8_t input_parse_device_binding_string(const char *device_class_name, const char *input_name,
                                                  control_binding_descriptor *out_binding);
    // 0x0048fea0, blam-cc: EDI -> device_class_name, stack -> input_name, ESI -> out_binding
extern int16_t input_action_name_to_index(const char *action_name);
    // 0x0048fe60, blam-cc: EBX -> action_name
extern uint8_t input_apply_control_binding(control_binding_descriptor *binding, int32_t action_index);
    // 0x0048b7b0 (this module, see input_apply_control_binding.c), blam-cc: ECX -> binding, EBX -> action_index
extern void console_out_printf(uint8_t unknown, const char *format, ...); // 0x004c6860

// Console/script 'bind' command: parses <device_class_name, input_name> into a binding
// descriptor, resolves <action_name> to a game control index, writes the binding, and echoes a
// confirmation line on success. Any failed step is silently ignored.
void hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name)
{
    control_binding_descriptor binding;
    int16_t action_index;

    if (input_parse_device_binding_string(device_class_name, input_name, &binding) != 0) {
        action_index = input_action_name_to_index(action_name);
        if (action_index != (int16_t)k_input_unbound) {
            if (input_apply_control_binding(&binding, action_index) != 0) {
                console_out_printf(0, "bound %s %s to game control %s", device_class_name, input_name, action_name);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x48b750), from tools/pack.py 0x48b750:

void hs_bind_control(undefined4 param_1)

{
  char cVar1;
  short sVar2;

  cVar1 = input_parse_device_binding_string(param_1);
  if (((cVar1 != '\0') && (sVar2 = input_action_name_to_index(), sVar2 != 0x7fff)) &&
     (cVar1 = input_apply_control_binding(), cVar1 != '\0')) {
    console_out_printf('\0',"bound %s %s to game control %s");
  }
  return;
}

objdump 0x48b750..0x48b7a9 (Intel syntax):

0048b750: sub    esp,0xc
0048b753: push   ebp
0048b754: mov    ebp,[esp+0x18]                ; ebp = action_name (2nd stack arg)
0048b758: push   esi
0048b759: push   edi
0048b75a: mov    edi,eax                        ; edi = device_class_name (EAX arg)
0048b75c: mov    eax,[esp+0x1c]                 ; eax = input_name (1st stack arg)
0048b760: push   eax
0048b761: lea    esi,[esp+0x10]                 ; esi = &binding (local, 0xc bytes)
0048b765: call   0x48fea0                       ; input_parse_device_binding_string(edi, [esp]=input_name, esi)
0048b76a: add    esp,0x4
0048b76d: test   al,al
0048b76f: je     0x48b7a3
0048b771: push   ebx
0048b772: mov    ebx,ebp                        ; ebx = action_name
0048b774: call   0x48fe60                       ; input_action_name_to_index(ebx)
0048b779: cmp    ax,0x7fff
0048b77d: je     0x48b7a2
0048b77f: mov    ebx,eax                        ; ebx = action_index
0048b781: mov    ecx,esi                        ; ecx = &binding
0048b783: call   0x48b7b0                       ; input_apply_control_binding(ecx, ebx)
0048b788: test   al,al
0048b78a: je     0x48b7a2
0048b78c: mov    ecx,[esp+0x20]                 ; ecx = input_name (reloaded, same stack slot)
0048b790: push   ebp                            ; action_name
0048b791: push   ecx                            ; input_name
0048b792: push   edi                            ; device_class_name
0048b793: push   0x669420                       ; "bound %s %s to game control %s"
0048b798: push   0x0
0048b79a: call   0x4c6860                       ; console_out_printf
0048b79f: add    esp,0x14
0048b7a2: pop    ebx
0048b7a3: pop    edi
0048b7a4: pop    esi
0048b7a5: pop    ebp
0048b7a6: add    esp,0xc
0048b7a9: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
