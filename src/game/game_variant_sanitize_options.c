// game_variant_sanitize_options  (Ghidra: game_variant_sanitize_options, already named)
// address 0x466730, size 339 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/game.h game_variant documents every field this function touches as "sanitize
//   clamps/normalizes ..." (game_engine_index 1..5, teams 0/1, unknown_40 0/1,
//   respawn_time_growth/respawn_time/suicide_penalty/lives_per_round >= 0, speed_scale
//   0.25..4.0, starting_equipment 0..0xd, vehicle_set low nibble 0..8, ctf_option_7c..7f and
//   ctf_value_80 only touched when game_engine_index is 1, ctf_option_7c..7e also touched when
//   it is 2) -- this function is the evidence those comments cite.
// register convention: variant pointer in ECX (in_ECX).
//   // blam-cc: ECX -> variant

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// blam-cc: ECX -> variant
// Clamps and normalizes every numeric/boolean field of a game-variant options block in place,
// then re-normalizes the CTF-specific fields (0x7c..0x80) if the engine is CTF, or a subset of
// them if it is Slayer.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void game_variant_sanitize_options(game_variant *variant)
{
    int32_t engine_index;

    engine_index = variant->game_engine_index;
    if (engine_index < 1) {
        engine_index = 1;
    } else if (engine_index > 5) {
        engine_index = 5;
    }
    variant->game_engine_index = engine_index;

    variant->teams = (variant->teams != 0);
    variant->odd_man_out = (variant->odd_man_out != 0);

    if (variant->respawn_time_growth < 0) variant->respawn_time_growth = 0;
    if (variant->respawn_time < 0) variant->respawn_time = 0;
    if (variant->suicide_penalty < 0) variant->suicide_penalty = 0;
    if (variant->lives_per_round < 0) variant->lives_per_round = 0;

    if (variant->health < 0.25f) {
        variant->health = 0.25f;
    } else if (variant->health > 4.0f) {
        variant->health = 4.0f;
    }

    if (variant->weapon_set < 0) {
        variant->weapon_set = 0;
    } else if (variant->weapon_set > 0xd) {
        variant->weapon_set = 0xd;
    }

    {
        uint32_t low_nibble = variant->red_vehicle_set & 0xf;
        if (low_nibble > 8) {
            low_nibble = 8;
        }
        variant->red_vehicle_set = (variant->red_vehicle_set & ~(uint32_t)0xf) | low_nibble;
    }

    if (engine_index == _game_engine_ctf) {
        variant->engine.ctf.assault = (variant->engine.ctf.assault != 0);
        variant->engine.ctf.unknown_7d = (variant->engine.ctf.unknown_7d != 0);
        variant->engine.ctf.flag_must_reset = (variant->engine.ctf.flag_must_reset != 0);
        variant->engine.ctf.flag_at_home_to_score = (variant->engine.ctf.flag_at_home_to_score != 0);
        variant->teams = 1;
        if (variant->engine.ctf.single_flag_time < 0) {
            variant->engine.ctf.single_flag_time = 0;
        }
    } else if (engine_index == _game_engine_slayer) {
        variant->engine.slayer.death_bonus = (variant->engine.slayer.death_bonus != 0);
        variant->engine.slayer.kill_penalty = (variant->engine.slayer.kill_penalty != 0);
        variant->engine.slayer.kill_in_order = (variant->engine.slayer.kill_in_order != 0);
    }
}

#if 0
Original Ghidra decompilation (0x466730), from tools/pack.py 0x466730:

void game_variant_sanitize_options(void)

{
  undefined4 uVar1;
  uint uVar2;
  int iVar3;
  int in_ECX;
  uint uVar4;
  int iVar5;

  iVar5 = *(int *)(in_ECX + 0x30);
  if (iVar5 < 1) {
    iVar5 = 1;
  }
  else if (5 < iVar5) {
    iVar5 = 5;
  }
  *(bool *)(in_ECX + 0x34) = *(char *)(in_ECX + 0x34) != '\0';
  *(bool *)(in_ECX + 0x40) = *(char *)(in_ECX + 0x40) != '\0';
  *(int *)(in_ECX + 0x30) = iVar5;
  *(uint *)(in_ECX + 0x44) = *(uint *)(in_ECX + 0x44) & ((int)*(uint *)(in_ECX + 0x44) < 1) - 1;
  *(uint *)(in_ECX + 0x48) = *(uint *)(in_ECX + 0x48) & ((int)*(uint *)(in_ECX + 0x48) < 1) - 1;
  *(uint *)(in_ECX + 0x4c) = *(uint *)(in_ECX + 0x4c) & ((int)*(uint *)(in_ECX + 0x4c) < 1) - 1;
  *(uint *)(in_ECX + 0x50) = *(uint *)(in_ECX + 0x50) & ((int)*(uint *)(in_ECX + 0x50) < 1) - 1;
  if (0.25 <= *(float *)(in_ECX + 0x54)) {
    if (*(float *)(in_ECX + 0x54) <= 4.0) {
      uVar1 = *(undefined4 *)(in_ECX + 0x54);
    }
    else {
      uVar1 = 0x40800000;
    }
  }
  else {
    uVar1 = 0x3e800000;
  }
  iVar3 = *(int *)(in_ECX + 0x5c);
  *(undefined4 *)(in_ECX + 0x54) = uVar1;
  if (iVar3 < 0) {
    iVar3 = 0;
  }
  else if (0xd < iVar3) {
    iVar3 = 0xd;
  }
  *(int *)(in_ECX + 0x5c) = iVar3;
  uVar2 = *(uint *)(in_ECX + 0x60);
  uVar4 = uVar2 & 0xf;
  if (8 < uVar4) {
    uVar4 = 8;
  }
  *(uint *)(in_ECX + 0x60) = (uVar2 ^ uVar4) & 0xf ^ uVar2;
  if (iVar5 == 1) {
    *(bool *)(in_ECX + 0x7c) = *(char *)(in_ECX + 0x7c) != '\0';
    *(bool *)(in_ECX + 0x7d) = *(char *)(in_ECX + 0x7d) != '\0';
    *(bool *)(in_ECX + 0x7e) = *(char *)(in_ECX + 0x7e) != '\0';
    *(bool *)(in_ECX + 0x7f) = *(char *)(in_ECX + 0x7f) != '\0';
    *(undefined1 *)(in_ECX + 0x34) = 1;
    *(uint *)(in_ECX + 0x80) = *(uint *)(in_ECX + 0x80) & ((int)*(uint *)(in_ECX + 0x80) < 0) - 1;
  }
  else if (iVar5 == 2) {
    *(bool *)(in_ECX + 0x7c) = *(char *)(in_ECX + 0x7c) != '\0';
    *(bool *)(in_ECX + 0x7d) = *(char *)(in_ECX + 0x7d) != '\0';
    *(bool *)(in_ECX + 0x7e) = *(char *)(in_ECX + 0x7e) != '\0';
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
