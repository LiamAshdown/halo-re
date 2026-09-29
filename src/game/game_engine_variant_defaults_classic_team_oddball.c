// game_engine_variant_defaults_classic_team_oddball
// (Ghidra: game_engine_variant_defaults_classic_team_oddball, already named)
// address 0x464430, size 224 bytes
// name confidence: 0.75   rewrite confidence: 0.55
// evidence / method: see game_engine_variant_defaults_classic_slayer.c (this batch) for the
// general technique. This one differs in one respect worth flagging: Ghidra's own decompile of
// this specific function does NOT show the bulk zero-fill loop the other nine variants all have
// (`local_98`, its 0x30-byte name field, is never explicitly zeroed here) -- but every other
// field this function itself explicitly sets to 0 matches exactly what the zero-fill would have
// produced anyway, and every sibling variant does zero-fill. Treated as the same missing-from-
// the-decompile artifact rather than genuine uninitialized name data, and zeroed via memset here
// for consistency and to avoid reading uninitialized stack memory.
// register convention: __cdecl, one pointer argument (the output game_variant*).
// reconciled: R37 game_variant.unknown_94 -> uint16 variant_flags (bit 0 built-in, high byte default index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

// FIXED: the original returns its argument in EAX (mov eax,[ebp+8] ... rep movs; callers keep it)
game_variant * game_engine_variant_defaults_classic_team_oddball(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_oddball;
    out->teams = 1;
    out->flags = 0xa3;
    out->objective_indicator = 1;
    out->respawn_time = 300;
    out->health = 1.0f;
    out->score_limit = 2;
    out->suicide_penalty = 0x96;
    out->weapon_set = 0x0b;
    out->red_vehicle_set = 1;
    out->blue_vehicle_set = 1;
    out->friendly_fire = 1;
    out->engine.oddball.ball_count = 1;
    out->variant_flags = 1;
    return out;
}

#if 0
Original Ghidra decompilation (0x464430), from tools/pack.py 0x464430:

void __cdecl game_engine_variant_defaults_classic_team_oddball(void *variant_options)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_98 [12];
  undefined4 local_68;
  undefined1 local_64;
  uint local_60;
  undefined4 local_5c;
  undefined1 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined1 local_2c;
  undefined4 local_28;
  undefined1 local_24;
  undefined4 local_20;
  undefined1 local_1c;
  undefined1 local_1b;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined2 local_4;

  local_60 = local_60 & 0xfffffee3 | 0xa3;
  local_48 = 0;
  local_58 = 0;
  local_54 = 0;
  local_30 = 0;
  local_28 = 0;
  local_24 = 0;
  local_20 = 0;
  local_1b = 0;
  local_1c = 0;
  local_c = 0;
  local_14 = 0;
  local_10 = 0;
  local_18 = 0;
  local_68 = 3;
  local_5c = 1;
  local_44 = 0x3f800000;
  local_50 = 300;
  local_40 = 2;
  local_4c = 0x96;
  local_64 = 1;
  local_3c = 0xb;
  local_2c = 1;
  local_38 = 1;
  local_34 = 1;
  local_8 = 1;
  local_4 = 1;
  puVar2 = local_98;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(undefined4 *)variant_options = *puVar2;
    puVar2 = puVar2 + 1;
    variant_options = (undefined4 *)((int)variant_options + 4);
  }
  return;
}
#endif
