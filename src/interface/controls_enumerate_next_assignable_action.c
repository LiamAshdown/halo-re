// controls_enumerate_next_assignable_action  (Ghidra: FUN_004b43e0, named in phase 4)
// address 0x4b43e0, size 214 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4b43e0..0x4b44bb in the phase-4 review (the first rewrite
// had no action name and called 0x53aa20 without arguments). Finds the first control bound to
// the action named in EDI for one input device, filling the 12 byte binding record in ECX:
// {int16 device kind (1 keyboard, 2 mouse, 3 gamepad), int16 device index, int16 unused, int16
// control} (zeroed first). EAX 0 walks the keyboard controls from 0 while 0x53aa20 (ECX action
// name, ESI record) says the control is bound, skipping the reserved keys of 0x0065c15c (see
// controls_key_is_bindable); with the retry byte set, a second walk takes the first bound
// control even if it is reserved. EAX 1 tests mouse control 0 only, EAX 2.. gamepad EAX - 2
// control 0 only. Returns 1 when a control was found.
// register convention: EAX device, ECX record, EDI action name; one stack argument (a byte).
//   // blam-cc: device -> EAX, record -> ECX, action_name -> EDI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t controls_reserved_action_table[9]; // 0x0065c15c .. 0x0065c180

extern uint8_t control_profile_find_binding_for_action(const char *action_name, const int16_t *binding); // 0x53aa20, is the control bound to the action, blam-cc: ECX action_name, ESI binding

// blam-cc: device -> EAX, record -> ECX, action_name -> EDI
uint8_t controls_enumerate_next_assignable_action(int32_t device, int16_t *record, const char *action_name,
                                                   uint8_t accept_reserved_on_retry)
{
    uint8_t found = 0;

    ((int32_t *)record)[0] = 0;
    ((int32_t *)record)[1] = 0;
    ((int32_t *)record)[2] = 0;

    if (device == 0) {
        uint8_t accept_reserved = 0;

        record[0] = 1; // keyboard
        record[1] = 0;
        for (;;) {
            record[3] = 0;
            while (control_profile_find_binding_for_action(action_name, record) != 0) {
                int32_t i;
                uint8_t reserved = 0;

                if (accept_reserved) {
                    found = 1;
                    break;
                }
                for (i = 0; i < 9; i++) {
                    if (controls_reserved_action_table[i] == (int32_t)record[3]) {
                        reserved = 1;
                        break;
                    }
                }
                if (!reserved) {
                    found = 1;
                    break;
                }
                record[3]++;
            }
            if (accept_reserved_on_retry == 0 || found) {
                return found;
            }
            accept_reserved = 1;
            accept_reserved_on_retry = 0;
        }
    }

    record[3] = 0;
    if (device == 1) {
        record[0] = 2; // mouse
        record[1] = 0;
    } else {
        record[0] = 3; // gamepad
        record[1] = (int16_t)(device - 2);
    }
    if (control_profile_find_binding_for_action(action_name, record) != 0) {
        return 1;
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x4b43e0):

char FUN_004b43e0(char param_1)

{
  bool bVar1;
  char cVar2;
  char cVar3;
  int in_EAX;
  int *piVar4;
  undefined4 *in_ECX;

  *in_ECX = 0;
  in_ECX[1] = 0;
  cVar3 = '\0';
  in_ECX[2] = 0;
  if (in_EAX != 0) {
    *(undefined2 *)((int)in_ECX + 6) = 0;
    if (in_EAX == 1) {
      *(undefined2 *)in_ECX = 2;
      *(undefined2 *)((int)in_ECX + 2) = 0;
      cVar3 = FUN_0053aa20();
      if (cVar3 != '\0') {
        return '\x01';
      }
    }
    else {
      *(undefined2 *)in_ECX = 3;
      *(short *)((int)in_ECX + 2) = (short)in_EAX + -2;
      cVar3 = FUN_0053aa20();
      if (cVar3 != '\0') {
        return '\x01';
      }
    }
    return '\0';
  }
  bVar1 = false;
  *(undefined2 *)in_ECX = 1;
  *(undefined2 *)((int)in_ECX + 2) = 0;
  do {
    *(undefined2 *)((int)in_ECX + 6) = 0;
    cVar2 = FUN_0053aa20();
    while (cVar2 != '\0') {
      if (bVar1) {
LAB_004b443e:
        cVar3 = '\x01';
        break;
      }
      piVar4 = &DAT_0065c15c;
      while (*piVar4 != (int)*(short *)((int)in_ECX + 6)) {
        piVar4 = piVar4 + 1;
        if (0x65c17f < (int)piVar4) goto LAB_004b443e;
      }
      *(short *)((int)in_ECX + 6) = *(short *)((int)in_ECX + 6) + 1;
      cVar2 = FUN_0053aa20();
    }
    if (param_1 == '\0') {
      return cVar3;
    }
    bVar1 = true;
    param_1 = '\0';
    if (cVar3 != '\0') {
      return cVar3;
    }
  } while( true );
}
#endif
