// unit_state_allows_control  (Ghidra: unit_state_allows_control)
// address 0x565ca0, size 29 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.95 (VERIFIED against objdump (jump tables decoded))
// evidence: types/units.h unit_data.animation_state (0x2a3). Same ECX-offset-by-0xb pattern as
//   unit_animation_state_is_compatible (0x565be0); see that file's header for the ECX ->
//   unit_data identity argument.
// register convention: pointer in ECX.
//   // blam-cc: in_ECX -> unit (offset folded into the field access)
// UNSURE: the ECX-is-unit_data identity is inferred, not confirmed against raw assembly.

// reconciled: every caller passes ECX = object+0x298 (unit_data+0xa4, e.g. 0x5657a4 lea ecx,[esi+0x298]) and the
//   function reads the signed byte at +0xb (= unit_data.animation_state, +0xaf); the draft read +0xaf from ECX.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint8_t unit_state_allows_control(const uint8_t *animation_block) // blam-cc: ECX -> animation_block
{
    switch ((int8_t)animation_block[0xb]) {       // 0x565ca0: movsx ecx,BYTE PTR [ecx+0xb]
    case 1: case 2: case 3:
    case 0x17: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
    case 0x21: case 0x22: case 0x23: case 0x27: case 0x29:
        return 0;
    default:
        return 1;
    }
}

#if 0
Original Ghidra decompilation (0x565ca0):

undefined1 FUN_00565ca0(void)

{
  undefined1 uVar1;
  int in_ECX;

  uVar1 = 1;
  switch(*(undefined1 *)(in_ECX + 0xb)) {
  case 1:
  case 2:
  case 3:
  case 0x17:
  case 0x1a:
  case 0x1b:
  case 0x1c:
  case 0x1d:
  case 0x1e:
  case 0x1f:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x27:
  case 0x29:
    uVar1 = 0;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
