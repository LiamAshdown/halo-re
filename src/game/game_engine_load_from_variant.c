// game_engine_load_from_variant  (Ghidra: game_engine_load_from_variant, already named)
// address 0x45c2c0, size 112 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/game.h game_variant (0x98 bytes, game_engine_index at 0x30, i.e. dword
//   index 0xc, matching `variant[0xc]` here); global 0x006f1c88 game_engine_variant (the live
//   0x26-dword option block copy); global 0x00688308 game_engine_definitions[7]; global
//   0x006f1d20 current_game_engine; the game_engine_unknown_aa00.. block this function zeroes
//   is the same one game_engine_tick.c (unknown_0087aa00), game_engine_begin_end_game_sequence.c
//   (game_engine_end_game_timer) and game_engine_post_rasterize_post_game.c
//   (game_engine_post_game_fade) already declare; game_engine_state_value and
//   game_engine_dedicated_idle/_timer come from game_engine_update_end_game_sequence.c.
// register convention: variant pointer in EBX (unaff_EBX); no return value.
//   // blam-cc: EBX -> variant
//
// UNSURE: the two callers (Ghidra reports 2) are not examined here; this rewrite only covers
// this function's own body. game_engine_unknown_aa14 is written here as a plain 0, matching
// hud_update_teammate_nameplate_fade.c's finding that it is really a per-local-player float
// array with k_maximum_local_players == 1 (so index 0 is the whole array).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern uint32_t unknown_0087aa00;                          // 0x0087aa00
extern int32_t game_engine_auto_team_counter;              // 0x0087aa04
extern float game_engine_end_game_timer;                   // 0x0087aa08
extern float game_engine_post_game_fade;                   // 0x0087aa0c
extern game_engine_state game_engine_state_value;           // 0x0087aa10
extern float game_engine_nameplate_fade_opacity_array[];   // 0x0087aa14
extern uint8_t game_engine_dedicated_idle;                  // 0x0087aa18
extern float game_engine_dedicated_idle_timer;              // 0x0087aa1c
extern int32_t unknown_0087aa20;                            // 0x0087aa20
extern game_variant game_engine_variant;                    // 0x006f1c88
extern game_engine_definition *game_engine_definitions[7];  // 0x00688308
extern game_engine_definition *current_game_engine;          // 0x006f1d20

extern void game_variant_sanitize_options(game_variant *variant); // 0x466730, this module
extern void player_profile_cache_initialize(void);    // 0x466c20

// Resets the end-of-game / round state, and if `variant` selects a real gametype
// (game_engine_index != 0), copies it into the live variant, sanitizes it, and installs the
// matching engine from the 0x00688308 gametype table. Always (re)initializes the player
// profile cache afterward.
void game_engine_load_from_variant(const game_variant *variant)
    // blam-cc: EBX -> variant
{
    uint32_t *src;
    uint32_t *dst;
    int32_t i;

    unknown_0087aa00 = 0;
    game_engine_auto_team_counter = 0;
    game_engine_end_game_timer = 0.0f;
    game_engine_post_game_fade = 0.0f;
    game_engine_nameplate_fade_opacity_array[0] = 0.0f;
    game_engine_dedicated_idle = 0;
    game_engine_dedicated_idle_timer = 0.0f;
    unknown_0087aa20 = 0;
    game_engine_state_value = _game_engine_state_not_started;

    if (variant != (const game_variant *)0 && variant->game_engine_index != 0) {
        src = (uint32_t *)variant;
        dst = (uint32_t *)&game_engine_variant;
        for (i = 0x26; i != 0; i = i - 1) {
            *dst = *src;
            src = src + 1;
            dst = dst + 1;
        }
        game_variant_sanitize_options(&game_engine_variant); // objdump 0x45c30f: ECX = 0x006f1c88
        current_game_engine = game_engine_definitions[variant->game_engine_index];
    }
    player_profile_cache_initialize();
}

#if 0
Original Ghidra decompilation (0x45c2c0), from tools/pack.py 0x45c2c0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void game_engine_load_from_variant(void)

{
  int iVar1;
  undefined4 *unaff_EBX;
  undefined4 *puVar2;
  undefined4 *puVar3;

  _DAT_0087aa00 = 0;
  DAT_0087aa04 = 0;
  _DAT_0087aa08 = 0;
  DAT_0087aa0c = 0;
  _DAT_0087aa14 = 0;
  _DAT_0087aa18 = 0;
  _DAT_0087aa1c = 0;
  DAT_0087aa20 = 0;
  DAT_0087aa10 = 0;
  if ((unaff_EBX != (undefined4 *)0x0) && (unaff_EBX[0xc] != 0)) {
    puVar2 = unaff_EBX;
    puVar3 = &DAT_006f1c88;
    for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
    game_variant_sanitize_options();
    DAT_006f1d20 = *(undefined4 *)(&DAT_00688308 + unaff_EBX[0xc] * 4);
  }
  player_profile_cache_initialize();
  return;
}
#endif
