// input_bind_scan_set_active  (Ghidra: FUN_0048b6b0; renamed per
//   out/phase4/input_types_notes.md correction: "this is the bind-scan begin/end (snapshot
//   joystick states into scan_baselines and set mode bit 3, or zero them and clear the bit). It
//   is not axis binding profile load")
// address 0x48b6b0, size 145 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: the AL == 0 path clears mode_flags bit 3 (_input_mode_bind_scan_bit) and zeroes the
//   whole 4-entry scan_baselines array in one 0xa0-dword sweep (0x00712544..0x007127c3, exactly
//   sizeof(joystick_state)*4 per types/input.h); the AL != 0 path sets the same bit and fills
//   each of the 4 scan_baselines slots either with zero (no device mapped to that slot,
//   joystick_slot_devices[slot] == -1), the live joystick_states[slot], or
//   joystick_neutral_state while input_suppressed is set. The Ghidra "if (&joystick_states[slot]
//   == 0) goto zero" check is a fixed static address compared to NULL, which can never be true;
//   it is dropped here as dead decompiler noise (objdump confirms the same unconditional
//   `lea`/copy at that site).
// register convention: AL -> enable_scan (bool), no return value.
//   // blam-cc: AL -> enable_scan

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
extern input_abstraction_globals input_globals; // 0x00710328
extern uint8_t input_suppressed;                               // 0x006b15f9
extern joystick_state joystick_states[4];                      // 0x006b2a68
extern joystick_state joystick_neutral_state;                  // 0x006b2cf8
extern int32_t joystick_slot_devices[4];                       // 0x006b2ce8

// Begins or ends the bind-capture axis scan. Starting the scan sets the bind-scan mode bit and
// snapshots each joystick slot's current axis/button/POV state (or the neutral state while
// input is suppressed, or zero for an unmapped slot) into scan_baselines, which
// input_scan_any_bound_input later compares against. Ending the scan clears the mode bit and
// zeroes every baseline.
void input_bind_scan_set_active(uint8_t enable_scan)
{
    uint8_t suppressed;
    int32_t slot;

    suppressed = input_suppressed;

    if (enable_scan == 0) {
        input_globals.mode_flags = input_globals.mode_flags & ~_input_mode_bind_scan_bit;
        memset(input_globals.scan_baselines, 0, sizeof(input_globals.scan_baselines));
        return;
    }

    input_globals.mode_flags = input_globals.mode_flags | _input_mode_bind_scan_bit;

    for (slot = 0; slot < 4; slot++) {
        if (joystick_slot_devices[slot] == -1) {
            memset(&input_globals.scan_baselines[slot], 0, sizeof(joystick_state));
        } else if (suppressed == 0) {
            input_globals.scan_baselines[slot] = joystick_states[slot];
        } else {
            input_globals.scan_baselines[slot] = joystick_neutral_state;
        }
    }
}

#if 0
Original Ghidra decompilation (0x48b6b0), from tools/pack.py 0x48b6b0:

void FUN_0048b6b0(void)

{
  char cVar1;
  char in_AL;
  int iVar2;
  undefined4 *puVar3;
  short sVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;

  cVar1 = DAT_006b15f9;
  if (in_AL == '\0') {
    DAT_00712542 = DAT_00712542 & 0xf7;
    puVar3 = &DAT_00712544;
    for (iVar2 = 0xa0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    return;
  }
  DAT_00712542 = DAT_00712542 | 8;
  sVar4 = 0;
  puVar3 = &DAT_00712544;
  do {
    if ((&DAT_006b2ce8)[sVar4] == -1) {
LAB_0048b70b:
      puVar5 = puVar3;
      for (iVar2 = 0x28; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar5 = 0;
        puVar5 = puVar5 + 1;
      }
    }
    else if (cVar1 == '\0') {
      if (&DAT_006b2a68 + sVar4 * 0x28 == (undefined4 *)0x0) goto LAB_0048b70b;
      puVar5 = &DAT_006b2a68 + sVar4 * 0x28;
      puVar6 = puVar3;
      for (iVar2 = 0x28; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
    }
    else {
      puVar5 = &DAT_006b2cf8;
      puVar6 = puVar3;
      for (iVar2 = 0x28; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
    }
    puVar3 = puVar3 + 0x28;
    sVar4 = sVar4 + 1;
    if (0x7127c3 < (int)puVar3) {
      return;
    }
  } while( true );
}

objdump call-site evidence (the only statically resolvable caller):
  004b531b: mov al,0x1
  004b531d: call 0x48b6b0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
