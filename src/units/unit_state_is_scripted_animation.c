// unit_state_is_scripted_animation  (Ghidra: unit_state_is_scripted_animation)
// address 0x565c60, size 31 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.95 (VERIFIED against objdump (jump tables decoded))
// evidence: types/units.h unit_data.animation_state (0x2a3). Same ECX-offset-by-0xb pattern as
//   unit_animation_state_is_compatible (0x565be0) and unit_state_allows_control (0x565ca0);
//   see that file's header for the ECX -> unit_data identity argument.
// register convention: pointer in ECX.
//   // blam-cc: in_ECX -> unit (offset folded into the field access)
// UNSURE: the ECX-is-unit_data identity is inferred, not confirmed against raw assembly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

uint8_t unit_state_is_scripted_animation(unit_data *unit) // blam-cc: in_ECX -> unit
{
    switch (unit->animation_state) {
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b:
    case 0x1d: case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23:
    case 0x27: case 0x29:
        return 1;
    default:
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x565c60):

undefined1 FUN_00565c60(void)

{
  undefined1 uVar1;
  int in_ECX;

  uVar1 = 0;
  switch(*(undefined1 *)(in_ECX + 0xb)) {
  case 0x17:
  case 0x18:
  case 0x19:
  case 0x1a:
  case 0x1b:
  case 0x1d:
  case 0x1e:
  case 0x1f:
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x27:
  case 0x29:
    uVar1 = 1;
  }
  return uVar1;
}
#endif
