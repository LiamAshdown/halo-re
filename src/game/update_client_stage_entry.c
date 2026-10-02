// update_client_stage_entry  (Ghidra: FUN_00473090; renamed, no established name)
// address 0x473090, size 59 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Builds a staged client update entry from raw input
// data plus extra state, ready to be queued for sending to the server").
// register convention: source data pointer in EAX (Ghidra's `in_EAX`).
//   // blam-cc: EAX -> source
// UNSURE: neither the 8-dword source record nor the 8-dword staging globals at 0x006f7ea4 are
// attested in any header this module owns; modeled as raw uint32_t[8] blocks. Dwords 1 and 2 of
// the staged copy are overwritten from two more raw globals (0x006f7ea8, 0x006f7eac) before the
// final copy, exactly as Ghidra shows.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t update_client_staged[8];  // 0x006f7ea4, UNSURE raw layout
extern uint32_t update_client_unknown_ea8; // 0x006f7ea8, UNSURE
extern uint32_t update_client_unknown_eac; // 0x006f7eac, UNSURE

// blam-cc: EAX -> source
// Copies 8 dwords from `source`, overwrites dwords 1 and 2 with the two extra-state globals, and
// stores the result into the staged client-update entry.
void update_client_stage_entry(uint32_t *source)
{
    uint32_t staged[8];
    int32_t i;

    for (i = 0; i < 8; i++) {
        staged[i] = source[i];
    }
    staged[1] = update_client_unknown_ea8;
    staged[2] = update_client_unknown_eac;
    for (i = 0; i < 8; i++) {
        update_client_staged[i] = staged[i];
    }
}

#if 0
Original Ghidra decompilation (0x473090), from tools/pack.py 0x473090:

void FUN_00473090(void)

{
  undefined4 *in_EAX;
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_20 [8];

  puVar2 = local_20;
  for (iVar1 = 8; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = *in_EAX;
    in_EAX = in_EAX + 1;
    puVar2 = puVar2 + 1;
  }
  local_20[2] = DAT_006f7eac;
  local_20[1] = DAT_006f7ea8;
  puVar2 = local_20;
  puVar3 = &DAT_006f7ea4;
  for (iVar1 = 8; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
