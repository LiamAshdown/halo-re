// player_customization_slot_set  (Ghidra: FUN_004705f0; renamed, no established name)
// address 0x4705f0, size 49 bytes
// name confidence: 0.2   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Updates a per-player customization/allegiance slot
// value in a fixed 16-entry table, returning whether the value changed").
// register convention: fully reconstructed against
//   objdump -d -M intel --start-address=0x4705f0 --stop-address=0x470624 bin/halo.exe
// since Ghidra shows every operand as an unbound register (`in_EAX`, `in_ECX`, `unaff_BL`,
// `unaff_ESI`). ECX is the base of the caller's struct (the 16-entry, 0x20-stride table starts at
// base+0x1a2); ESI is the key byte to search for at each entry's +0x1f; BL is the new value to
// store at the matching entry's +0x1e.
//   // blam-cc: ECX -> base, EBX -> new_value, ESI -> key
// UNSURE: neither the containing struct (base) nor the meaning of the two per-entry bytes is
// attested in any header this module owns; modeled as a raw byte pointer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// VERIFIED against disassembly 0x4705f0..0x470623 (2026-09-30); fixed: key is the full ESI (zero-extended by the callers) and
//   the two byte comparisons sign-extend the stored bytes, so a stored byte >= 0x80 never matches / always reports changed.
// blam-cc: ECX -> base, EBX -> new_value, ESI -> key
// Scans the 16-entry, 0x20-stride table at base+0x1a2 for the entry whose key byte (+0x1f)
// equals `key`; if found, stores `new_value` at that entry's value byte (+0x1e) and returns
// whether it actually changed. Returns 0 if no entry matches.
uint8_t player_customization_slot_set(uint8_t *base, uint8_t new_value, uint32_t key)
{
    uint8_t *entry = base + 0x1a2;
    int32_t i;

    for (i = 0; i < 16; i++) {
        // 0x470600: movsx edi, byte [entry+0x1f]; cmp edi, esi -- the callers pass the key ZERO-extended (movzx esi, cl), so
        // a stored key byte >= 0x80 (sign-extended negative) never matches
        if ((int32_t)(int8_t)entry[0x1f] == (int32_t)key) {
            // 0x470613: movsx eax, byte [entry+0x1e]; movzx edx, bl; cmp; setne -- the stored byte is SIGN-extended
            uint8_t changed = (int32_t)(int8_t)entry[0x1e] != (int32_t)new_value;
            entry[0x1e] = new_value;
            return changed;
        }
        entry = entry + 0x20;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4705f0), from tools/pack.py 0x4705f0:

uint FUN_004705f0(void)

{
  char cVar1;
  uint in_EAX;
  int in_ECX;
  int iVar2;
  int iVar3;
  byte unaff_BL;
  int unaff_ESI;

  iVar3 = 0;
  iVar2 = in_ECX + 0x1a2;
  do {
    if (*(char *)(iVar2 + 0x1f) == unaff_ESI) {
      cVar1 = *(char *)(iVar2 + 0x1e);
      *(byte *)(iVar2 + 0x1e) = unaff_BL;
      return CONCAT31(cVar1 >> 7,(int)cVar1 != (uint)unaff_BL);
    }
    iVar3 = iVar3 + 1;
    iVar2 = iVar2 + 0x20;
  } while (iVar3 < 0x10);
  return in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
