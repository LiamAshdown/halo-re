// unit_animation_state_is_compatible  (Ghidra: unit_animation_state_is_compatible)
// address 0x565be0, size 61 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.25
// evidence: types/units.h unit_data.animation_state (0x2a3). Ghidra shows this taking a
//   pointer in ECX and reading a byte at ECX+0xb with no traceable mov before either call site
//   in unit_update_animation_state_machine (0x565420); at both call sites ECX is not reloaded
//   between reading unit->animation_state (object+0x2a3) and this call, and 0x2a3 - 0xb =
//   0x298 = unit_data.animation_state_flags, so ECX is modelled here as pointing at
//   animation_state_flags and the byte it actually reads as unit->animation_state directly.
// register convention: state-flags-relative pointer in ECX (see note above, taken here as the
//   owning unit_data pointer), comparison state in DX.
//   // blam-cc: in_ECX -> unit (offset folded into the field access), in_DX -> requested_state
// UNSURE: the ECX-is-unit_data identity is inferred from the +0xb arithmetic lining up with
//   animation_state, not confirmed against the raw call-site assembly.

// reconciled: every caller passes ECX = object+0x298 (unit_data+0xa4, e.g. 0x5657a4 lea ecx,[esi+0x298]) and the
//   function reads the signed byte at +0xb (= unit_data.animation_state, +0xaf); the draft read +0xaf from ECX.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

uint8_t unit_animation_state_is_compatible(const uint8_t *animation_block, int16_t requested_state) // blam-cc: ECX -> animation_block, DX -> requested_state
{
    switch ((int8_t)animation_block[0xb]) {       // 0x565be0: movsx ecx,BYTE PTR [ecx+0xb]
    case 2:
    case 3:
    case 0x25:
    case 0x26:
        return requested_state != 0;
    case 0x17:
    case 0x1a:
    case 0x1b:
    case 0x1c:
        return 0;
    case 0x18:
    case 0x19:
        return (0x17 < requested_state) && (requested_state < 0x1a);
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x27:
    case 0x29:
        return requested_state == 0x17;
    default:
        return 1; // Ghidra's default case skips the "uVar1 = 0" fallthrough entirely
    }
}

#if 0
Original Ghidra decompilation (0x565be0):

undefined4 FUN_00565be0(void)

{
  undefined4 uVar1;
  int in_ECX;
  short in_DX;

  uVar1 = 1;
  switch(*(undefined1 *)(in_ECX + 0xb)) {
  case 2:
  case 3:
  case 0x25:
  case 0x26:
    if (in_DX != 0) {
      return 1;
    }
    break;
  default:
    goto switchD_00565bf5_caseD_4;
  case 0x17:
  case 0x1a:
  case 0x1b:
  case 0x1c:
    break;
  case 0x18:
  case 0x19:
    if ((0x17 < in_DX) && (in_DX < 0x1a)) {
      return 1;
    }
    break;
  case 0x1d:
  case 0x1e:
  case 0x1f:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x27:
  case 0x29:
    if (in_DX != 0x17) {
      return 0;
    }
    goto switchD_00565bf5_caseD_4;
  }
  uVar1 = 0;
switchD_00565bf5_caseD_4:
  return uVar1;
}
#endif
