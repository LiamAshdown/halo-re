// game_engine_reset_player_look_state  (Ghidra: game_engine_reset_player_look_state, already
// named)
// address 0x470de0, size 155 bytes, cc=__cdecl
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/game_functions.md ("Resets a player's look/aim state record to its
// defaults, including pitch clamp limits, and initializes global default turn-rate constants on
// first use"); types/game.h player_control_globals (header fields +0x00..+0x0c),
// local_player_control (unit +0x00, unknown_08, unknown_0a, desired_weapon_index +0x20,
// desired_grenade_index +0x22, desired_zoom_level +0x24, unknown_26, nameplate_target +0x28,
// pitch_minimum +0x38 == -1.4906585, pitch_maximum +0x3c == +1.4906585 -- both float bit
// patterns confirmed against those exact header comments); game.h's own empirical
// "+0x114 player_information" note on global_globals, which is itself wrong -- see below.
// UNSURE: global_globals+0x114 is read as a plain pointer (not `+ 4` into a TagReflexive), and
// its own +0x4c/+0x50 float fields (the two turn-rate defaults) are not otherwise named by any
// header this module owns.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>

extern player_control_globals *player_control_globals_ptr; // 0x006b145c
extern Globals *global_globals;                             // 0x00746fa0
extern real look_pitch_rate_setting[k_maximum_local_players]; // 0x006f1d78, degrees per second
extern real look_yaw_rate_setting[k_maximum_local_players];   // 0x006f1d74, degrees per second

// Zeroes the player_control_globals header (action flags) and local player 0's whole look-state
// record, then reinitializes its defaults (unit = none, desired weapon/grenade/zoom = none,
// nameplate_target = none, pitch clamped to +-1.4906585 rad), and, the first time this runs,
// seeds the two global default look-rate constants from the globals tag's player_control
// block.
void game_engine_reset_player_look_state(void)
{
    local_player_control *look = &player_control_globals_ptr->local_players[0];
    // CORRECTED (phase 4 review): Globals + 0x114 is player_control.pointer, not
    // player_information (that is + 0x174). +0x4c / +0x50 in it are
    // GlobalsPlayerControl::look_default_pitch_rate / look_default_yaw_rate, which is what
    // fixes 0x006f1d78 as the pitch setting and 0x006f1d74 as the yaw setting.
    GlobalsPlayerControl *player_control =
        (GlobalsPlayerControl *)global_globals->player_control.pointer;

    player_control_globals_ptr->action_flags = 0;
    player_control_globals_ptr->action_flags_latched = 0;
    player_control_globals_ptr->action_flags_edge = 0;
    player_control_globals_ptr->flags = 0;

    memset(look, 0, sizeof(*look));
    look->unit = k_datum_index_none;
    look->desired_weapon_index = -1;
    look->desired_grenade_index = -1;
    look->desired_zoom_level = -1;
    look->nameplate_target = k_datum_index_none;
    look->autolevelling_active = 0;
    look->pitch_maximum = 1.4906585f;
    look->pitch_minimum = -1.4906585f;
    look->suppressed_buttons = 0;
    look->suppressed_until_released = 0;

    if (look_pitch_rate_setting[0] == 0.0f) {
        look_pitch_rate_setting[0] = player_control->look_default_pitch_rate;
    }
    if (look_yaw_rate_setting[0] == 0.0f) {
        look_yaw_rate_setting[0] = player_control->look_default_yaw_rate;
    }
}

#if 0
Original Ghidra decompilation (0x470de0), from tools/pack.py 0x470de0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl game_engine_reset_player_look_state(void)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  undefined4 *puVar5;

  iVar2 = DAT_00746fa0;
  puVar3 = DAT_006b145c;
  puVar1 = DAT_006b145c + 4;
  DAT_006b145c[1] = 0;
  *puVar3 = 0;
  puVar3[2] = 0;
  puVar3[3] = 0;
  iVar2 = *(int *)(iVar2 + 0x114);
  puVar5 = puVar1;
  for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  }
  *puVar1 = 0xffffffff;
  *(undefined2 *)(puVar3 + 0xc) = 0xffff;
  *(undefined2 *)((int)puVar3 + 0x32) = 0xffff;
  *(undefined2 *)(puVar3 + 0xd) = 0xffff;
  puVar3[0xe] = 0xffffffff;
  *(undefined1 *)((int)puVar3 + 0x36) = 0;
  puVar3[0x13] = 0x3fbf0243;
  puVar3[0x12] = 0xbfbf0243;
  *(undefined2 *)(puVar3 + 6) = 0;
  *(undefined2 *)((int)puVar3 + 0x1a) = 0;
  if (_DAT_006f1d78 == 0.0) {
    _DAT_006f1d78 = *(float *)(iVar2 + 0x4c);
  }
  if (_DAT_006f1d74 == 0.0) {
    _DAT_006f1d74 = *(float *)(iVar2 + 0x50);
  }
  return;
}
#endif
