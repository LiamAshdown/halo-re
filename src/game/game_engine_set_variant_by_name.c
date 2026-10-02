// game_engine_set_variant_by_name  (Ghidra: FUN_0045b920; renamed per symbols/review_queue.txt)
// address 0x45b920, size 112 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: types/game.h game_variant (0x98 bytes), the live copy at 0x0087ab20; symbols/
//   review_queue.txt 0x45b920 "Looks up a game variant by name and installs it as the active
//   variant, propagating the change to the network layer if needed."
// register convention: variant name in EAX (elided; this function takes no recognized
//   parameters at all in the decompile, so the name is inferred purely from its role as
//   game_engine_get_variant_by_name's argument).
//   // blam-cc: EAX -> name (UNSURE, not directly observed)
//
// UNSURE: Ghidra's `if (&stack0x00000000 != (undefined1 *)0x98)` is almost certainly its
// mis-rendering of a genuine "lookup succeeded" boolean (comparing a stack address to a literal
// is not meaningful source code); modelled as game_engine_get_variant_by_name returning a
// status byte instead. The network-session struct fields at +0x10c (variant) and +0x13c
// (game_engine_index) match types/game.h's own notes on network_server's layout.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_variant game_engine_active_variant; // 0x0087ab20 (NOT 0x006f1c88, which is the live copy)
extern uint8_t *network_server;             // 0x0071c2d4

extern uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out);
    // 0x4622d0, this module; blam-cc: ECX -> name, stack -> out. A NULL `out` only tests
    // whether the name is recognized.
extern void network_game_broadcast_player_set_changed(void *session); // UNSURE module, propagates a variant change over the network

// Looks up a game variant by name and installs it as the active variant, propagating the change
// to the network layer if it is hosting and the engine type actually changed.
void game_engine_set_variant_by_name(const char *name)
    // blam-cc: EAX -> name (UNSURE)
{
    game_variant looked_up;

    if (game_engine_get_variant_by_name(name, &looked_up) != 0) {
        game_engine_active_variant = looked_up;
        if (network_server != (void *)0 &&
            *(int32_t *)((uint8_t *)network_server + 0x13c) != looked_up.game_engine_index) {
            *(game_variant *)((uint8_t *)network_server + 0x10c) = looked_up;
            network_game_broadcast_player_set_changed(network_server);
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
Original Ghidra decompilation (0x45b920), from tools/pack.py 0x45b920:

void FUN_0045b920(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  bool bVar5;
  undefined4 local_98 [12];
  int local_68;

  game_engine_get_variant_by_name();
  iVar1 = DAT_0071c2d4;
  puVar4 = &DAT_0087ab20;
  iVar2 = 0x26;
  if (&stack0x00000000 != (undefined1 *)0x98) {
    bVar5 = DAT_0071c2d4 != 0;
    puVar3 = local_98;
    for (; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    if ((bVar5) && (*(int *)(iVar1 + 0x13c) != local_68)) {
      puVar4 = local_98;
      puVar3 = (undefined4 *)(iVar1 + 0x10c);
      for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar3 = puVar3 + 1;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
