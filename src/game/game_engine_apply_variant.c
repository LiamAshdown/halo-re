// game_engine_apply_variant  (Ghidra: FUN_0045b990; renamed per symbols/review_queue.txt)
// address 0x45b990, size 73 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/game.h game_variant (0x98 bytes, game_engine_index at 0x30, i.e. dword index
//   0xc -- matches `in_EDX[0xc]` here); same network-session propagation as
//   game_engine_set_variant_by_name.c (0x45b920), which this function's own logic mirrors
//   almost exactly but takes the variant data directly instead of looking it up by name.
// register convention: variant pointer in EDX (in_EDX); NULL zeroes the live variant instead.
//   // blam-cc: EDX -> variant

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_variant game_engine_active_variant; // 0x0087ab20 (NOT 0x006f1c88, which is the live copy)
extern uint8_t *network_session;             // 0x0071c2d4

extern void FUN_004e1bf0(void *session); // UNSURE module, propagates a variant change over the network

// Installs a game variant struct as the active variant, propagating the change over the network
// if it is hosting and the engine type actually changed. A NULL variant zeroes the active one.
void game_engine_apply_variant(const game_variant *variant)
    // blam-cc: EDX -> variant
{
    if (variant != (const game_variant *)0) {
        game_engine_active_variant = *variant;
        if (network_session != (void *)0 &&
            *(int32_t *)((uint8_t *)network_session + 0x13c) != variant->game_engine_index) {
            *(game_variant *)((uint8_t *)network_session + 0x10c) = *variant;
            FUN_004e1bf0(network_session);
        }
    } else {
        uint8_t *dst = (uint8_t *)&game_engine_active_variant;
        uint32_t i;
        for (i = 0; i < sizeof(game_variant); i = i + 1) {
            dst[i] = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x45b990), from tools/pack.py 0x45b990:

void FUN_0045b990(void)

{
  int iVar1;
  int iVar2;
  undefined4 *in_EDX;
  undefined4 *puVar3;
  undefined4 *puVar4;
  bool bVar5;

  iVar1 = DAT_0071c2d4;
  puVar4 = &DAT_0087ab20;
  iVar2 = 0x26;
  if (in_EDX != (undefined4 *)0x0) {
    bVar5 = DAT_0071c2d4 != 0;
    puVar3 = in_EDX;
    for (; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    if ((bVar5) && (*(int *)(iVar1 + 0x13c) != in_EDX[0xc])) {
      puVar4 = (undefined4 *)(iVar1 + 0x10c);
      for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar4 = *in_EDX;
        in_EDX = in_EDX + 1;
        puVar4 = puVar4 + 1;
      }
      FUN_004e1bf0(iVar1);
    }
    return;
  }
  for (; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  return;
}
#endif
