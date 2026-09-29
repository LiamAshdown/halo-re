// game_engine_variant_defaults_classic_snipers
// (Ghidra: game_engine_variant_defaults_classic_snipers, already named)
// address 0x464180, size 220 bytes
// name confidence: 0.75   rewrite confidence: 0.6
// evidence / method: see game_engine_variant_defaults_classic_slayer.c (this batch).
// register convention: __cdecl, one pointer argument (the output game_variant*).
// reconciled: R37 game_variant.unknown_94 -> uint16 variant_flags (bit 0 built-in, high byte default index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

// FIXED: the original returns its argument in EAX (mov eax,[ebp+8] ... rep movs; callers keep it)
game_variant * game_engine_variant_defaults_classic_snipers(game_variant *out)
{
    memset(out, 0, sizeof(game_variant));
    out->game_engine_index = _game_engine_slayer;
    out->flags = 0xa2;
    out->objective_indicator = 1;
    out->respawn_time_growth = 0x96;
    out->suicide_penalty = 300;
    out->speed_scale = 1.0f;
    out->score_limit = 0x0f;
    out->starting_equipment = 4;
    out->vehicle_set = 0x42;
    out->unknown_64 = 0x42;
    out->friendly_fire_mode = 1;
    out->variant_flags = 1;
    return out;
}

#if 0
Original Ghidra decompilation (0x464180), from tools/pack.py 0x464180:

void __cdecl game_engine_variant_defaults_classic_snipers(void *variant_options)

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
  undefined1 local_23;
  undefined1 local_22;
  undefined2 local_c;

  local_a0 = (undefined1  [2])0x0;
  puVar2 = (undefined4 *)(local_a0 + 2);
  for (iVar1 = 0x25; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0;
  local_68 = local_68 & 0xfffffee2 | 0xa2;
  local_40 = 0x42;
  local_3c = 0x42;
  local_64 = 1;
  local_34 = 1;
  local_c = 1;
  local_70 = 2;
  local_4c = 0x3f800000;
  local_50 = 0;
  local_60 = 0;
  local_58 = 0;
  local_5c = 0x96;
  local_48 = 0xf;
  local_54 = 300;
  local_6c = 0;
  local_44 = 4;
  local_38 = 0;
  local_30 = 0;
  local_2c = 0;
  local_28 = 0;
  local_24 = 0;
  local_23 = 0;
  local_22 = 0;
  puVar2 = (undefined4 *)local_a0;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *(undefined4 *)variant_options = *puVar2;
    puVar2 = puVar2 + 1;
    variant_options = (undefined4 *)((int)variant_options + 4);
  }
  return;
}
#endif
