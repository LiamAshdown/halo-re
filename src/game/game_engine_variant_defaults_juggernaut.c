// game_engine_variant_defaults_juggernaut  (Ghidra: already named)
// address 0x467540, size 259 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: types/game.h game_variant (0x98 bytes / 0x26 dwords); the function zeroes a
//   0x98-byte stack buffer, writes the fields below into it by fixed offset, then block-
//   copies all 0x26 dwords out to the caller's buffer -- the same pattern as every other
//   game_engine_variant_defaults_* function in this module.
// register convention: variant_options is the recognized cdecl stack parameter.
// reconciled: R37 game_variant.unknown_94 -> uint16 variant_flags (bit 0 built-in, high byte default index)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Fills a game-variant options block with this built-in multiplayer game type's
// hardcoded default settings. The block is zeroed first (which is also the default
// for every field not listed below) and then copied out whole.
// FIXED: the original returns its argument in EAX (mov eax,[ebp+8] ... rep movs; callers keep it)
game_variant * game_engine_variant_defaults_juggernaut(game_variant *variant_options)
{
    game_variant defaults;
    uint8_t *zero_cursor;
    uint32_t i;

    zero_cursor = (uint8_t *)&defaults;
    for (i = 0; i < sizeof(defaults); i = i + 1) {
        zero_cursor[i] = 0;
    }

    defaults.game_engine_index = 3;
    defaults.teams = 0;
    defaults.flags = 0x103;
    defaults.objective_indicator = 1;
    defaults.odd_man_out = 0;
    defaults.respawn_time_growth = 0;
    defaults.respawn_time = 0x96;
    defaults.suicide_penalty = 0x96;
    defaults.lives_per_round = 0;
    defaults.health = 1.0f; // 0x3f800000
    defaults.score_limit = 0xf;
    defaults.weapon_set = 0;
    defaults.red_vehicle_set = 0x249240;
    defaults.blue_vehicle_set = 0x249240;
    defaults.vehicle_respawn_time = 0x708;
    defaults.friendly_fire = 1;
    defaults.betrayal_penalty = 0;
    defaults.team_autobalance = 0;
    defaults.time_limit = 36000;
    defaults.engine.oddball.random_start = 0;
    defaults.engine.oddball.speed_with_ball = 1;
    defaults.engine.oddball.trait_with_ball = 2;
    defaults.engine.oddball.trait_without_ball = 0;
    defaults.engine.oddball.ball_type = 2;
    defaults.engine.oddball.ball_count = 1;
    defaults.variant_flags = 1;

    *variant_options = defaults;
    return variant_options;
}

#if 0
Original Ghidra decompilation (0x467540), from tools/pack.py 0x467540:


void __cdecl game_engine_variant_defaults_juggernaut(void *variant_options)

{
  int iVar1;
  undefined4 *puVar2;
  undefined1 local_a0 [2];
  undefined4 local_9e [11];
  undefined4 local_70;
  undefined1 local_6c;
  uint local_68;
  undefined4 local_64;
  undefined1 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined1 local_34;
  undefined4 local_30;
  undefined1 local_2c;
  undefined4 local_28;
  undefined1 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined2 local_c;
  
  local_a0 = (undefined1  [2])0x0;
  puVar2 = (undefined4 *)(local_a0 + 2);
  for (iVar1 = 0x25; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  local_40 = 0x249240;
  local_3c = 0x249240;
  local_58 = 0x96;
  local_54 = 0x96;
  local_68 = local_68 & 0xffffff03 | 0x103;
  local_1c = 2;
  local_14 = 2;
  local_64 = 1;
  local_34 = 1;
  local_20 = 1;
  local_10 = 1;
  local_c = 1;
  local_70 = 3;
  local_50 = 0;
  local_4c = 0x3f800000;
  local_5c = 0;
  local_60 = 0;
  local_44 = 0;
  local_38 = 0x708;
  local_30 = 0;
  local_2c = 0;
  local_18 = 0;
  local_24 = 0;
  local_48 = 0xf;
  local_6c = 0;
  local_28 = 36000;
  puVar2 = (undefined4 *)local_a0;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(undefined4 *)variant_options = *puVar2;
    puVar2 = puVar2 + 1;
    variant_options = (undefined4 *)((int)variant_options + 4);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
