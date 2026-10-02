// game_engine_ensure_variant_history_has_entry  (Ghidra: FUN_00463b20; renamed per its summary)
// address 0x463b20, size 108 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Ensures the recent/custom game-variant cache has at
// least one entry, restoring a previously saved default variant into it if it was empty");
// types/game.h game_variant_history_count (0x00687b10), game_variant_saved_default
// (0x00714de0), game_variant_saved_default_valid (0x00714e78); this batch's
// game_engine_free_custom_variant_cache (0x4638b0) and game_engine_variant_add_to_history
// (0x463980).
// UNSURE: 0x00719879 (the name passed to game_engine_variant_add_to_history) is not attributed
// to this module anywhere in this batch's evidence.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t game_variant_history_count;   // 0x00687b10
extern uint8_t game_variant_saved_default_valid; // 0x00714e78
extern game_variant game_variant_saved_default;  // 0x00714de0
extern char network_build_string[];        // 0x00719879, UNSURE identity

extern void game_engine_free_custom_variant_cache(void); // 0x4638b0, this batch
extern uint32_t game_engine_variant_add_to_history(char *name, game_variant *options, char *path); // 0x463980, this batch

uint32_t game_engine_ensure_variant_history_has_entry(void)
{
    game_engine_free_custom_variant_cache();
    if (game_variant_history_count != 0) {
        return 1;
    }
    if (game_variant_saved_default_valid != 0) {
        game_variant temp = game_variant_saved_default;

        game_engine_free_custom_variant_cache();
        game_engine_variant_add_to_history(network_build_string, &temp, 0);
        if (game_variant_history_count != 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x463b20), from tools/pack.py 0x463b20:

undefined4 FUN_00463b20(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_a0 [39];

  game_engine_free_custom_variant_cache();
  if (DAT_00687b10 != 0) {
    return 1;
  }
  if (DAT_00714e78 != '\0') {
    puVar2 = &DAT_00714de0;
    puVar3 = local_a0;
    for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
    game_engine_free_custom_variant_cache();
    game_engine_variant_add_to_history(&DAT_00719879,local_a0);
    if (DAT_00687b10 != 0) {
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
