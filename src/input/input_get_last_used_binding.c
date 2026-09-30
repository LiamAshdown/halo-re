// input_get_last_used_binding  (Ghidra: FUN_0048bde0; renamed per its behavior)
// address 0x48bde0, size 100 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: objdump 0x48bde0..0x48be43 fully traced. Copies input_globals.last_used_bindings
//   [action] into *out whenever it is already cached (device_type != 0), or becomes known via
//   input_refresh_last_used_binding -- first asking the device class in last_input_device
//   (0x0087a460), then falling back to scanning device classes 0..4 in order. If none of those
//   report a binding, *out is left untouched (the binary's own dead `test al,al; je` at the loop
//   exit is always taken in that case, since it only follows a failed final iteration).
// register convention: EAX -> action, stack -> out.
//   // blam-cc: EAX -> action, stack -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

extern input_abstraction_globals input_globals; // 0x00710328
extern int32_t last_input_device;                // 0x0087a460


    // 0x0048bae0 (this module, see input_refresh_last_used_binding.c)
    // blam-cc: ECX -> device_class, ESI -> action

// Returns the input device/type most recently used to activate game control `action` by copying
// input_globals.last_used_bindings[action] into *out. If that record is not yet known
// (device_type == 0), first tries to resolve it on the last device class that produced any
// input (last_input_device), then falls back to trying every device class 0..4 in order.
// Leaves *out untouched if the control turns out not to be bound anywhere.
uint8_t input_get_last_used_binding(int16_t action, control_binding_descriptor *out)
{
    control_binding_descriptor *cached;
    uint8_t found;
    int32_t device_class;

    cached = &input_globals.last_used_bindings[action];
    found = (cached->device_type != 0);

    if (!found) {
        found = input_refresh_last_used_binding(last_input_device, action);
        if (!found) {
            for (device_class = 0; device_class < 5; device_class++) {
                found = input_refresh_last_used_binding(device_class, action);
                if (found) {
                    break;
                }
            }
        }
    }

    if (found) {
        *out = *cached;
    }
    return found; // AL survives the copy (0x48be2e..0x48be3c only touch ECX/EDX); the
                  // interface callers test it
}

#if 0
Original Ghidra decompilation (0x48bde0): not individually named by Ghidra (folded into the
surrounding batch); reconstructed here from objdump 0x48bde0..0x48be43:

0048bde0: push   ebx
0048bde1: push   ebp
0048bde2: mov    ebp,[esp+0xc]                  ; ebp = out (stack arg)
0048bde6: push   esi
0048bde7: mov    esi,eax                        ; esi = action
0048bde9: movsx  eax,si
0048bdec: lea    eax,[eax+eax*2]                ; eax = action*3
0048bdef: cmp    WORD PTR [eax*4+0x7127d4],0x0  ; last_used_bindings[action].device_type == 0 ?
0048bdf8: lea    ebx,[eax*4+0x7127d4]           ; ebx = &last_used_bindings[action]
0048bdff: push   edi
0048be00: je     0x48be06
0048be02: mov    al,0x1
0048be04: jmp    0x48be2e                        ; already cached -> copy out
0048be06: mov    ecx,DWORD PTR ds:0x87a460       ; ecx = last_input_device
0048be0c: call   0x48bae0                        ; input_refresh_last_used_binding(ecx, esi)
0048be11: xor    edi,edi
0048be13: test   al,al
0048be15: jne    0x48be2e                        ; found -> copy out
0048be17: cmp    edi,0x5
0048be1a: jge    0x48be2a
0048be1c: mov    ecx,edi
0048be1e: call   0x48bae0                        ; input_refresh_last_used_binding(edi, esi)
0048be23: inc    edi
0048be24: test   al,al
0048be26: je     0x48be17
0048be28: jmp    0x48be2e                        ; found -> copy out
0048be2a: test   al,al                           ; loop exhausted; al is always 0 here
0048be2c: je     0x48be3f                        ; -> skip copy
0048be2e: mov    ecx,[ebx]  ; mov [ebp+0],ecx
          mov    edx,[ebx+4]; mov [ebp+4],edx
          mov    ecx,[ebx+8]; mov [ebp+8],ecx
0048be3f: pop edi; pop esi; pop ebp; pop ebx; ret
#endif
