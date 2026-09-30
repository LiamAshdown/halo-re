// game_engine_apply_current_custom_variant  (Ghidra: game_engine_apply_current_custom_variant,
// already named)
// address 0x463b90, size 176 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Cycles to and loads the next entry of the custom
// game-variant cache into the currently active variant state"); types/game.h
// game_variant_history/_count/_current (0x00687b0c/0x10/0x18), game_engine_pending_variant
// (0x0087aa80, per src/game/game_engine_sync_variant_defaults.c), variant_defaults_source
// (0x0087aa40, 0x40 bytes per types/game.h's own global list).
// UNSURE: 0x0071c40c (network_game_mode-shaped, but a different address from the game module's
// own 0x00719720 network_game_mode), 0x0087aa7f, 0x0087aaec and 0x0087aaf8/0x00699608 are not
// attributed to this module anywhere in this batch's evidence; kept as raw externs with generic
// names.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <string.h>

extern uint32_t game_variant_history_count;    // 0x00687b10
extern int32_t game_variant_history_current;   // 0x00687b18
extern game_variant_history_entry *game_variant_history; // 0x00687b0c
extern char variant_defaults_source[0x40];      // 0x0087aa40
extern game_variant game_engine_pending_variant;         // 0x0087aa80
extern int32_t sv_friendly_fire_mode;   // 0x0071c40c, UNSURE identity
extern uint8_t unknown_0087aa7f;   // 0x0087aa7f, UNSURE identity
extern int32_t unknown_0087aaec;   // 0x0087aaec, UNSURE identity
extern int32_t sv_timelimit_minutes; // 0x00699608, UNSURE identity
extern int32_t unknown_0087aaf8;   // 0x0087aaf8, UNSURE identity


void game_engine_apply_current_custom_variant(void)
{
    game_variant_history_entry *entry;
    int32_t mode;
    uint8_t mode_is_1;

    if (game_variant_history_count == 0) {
        return;
    }

    game_variant_history_current = game_variant_history_current + 1;
    if (game_variant_history_count <= (uint32_t)game_variant_history_current) {
        game_variant_history_current = 0;
    }

    entry = &game_variant_history[game_variant_history_current];
    strncpy(variant_defaults_source, entry->name, 0x3f);

    mode = sv_friendly_fire_mode;
    mode_is_1 = (mode == 1);
    unknown_0087aa7f = 0;
    game_engine_pending_variant = entry->options;

    if (mode_is_1) {
        unknown_0087aaec = 0;
    } else if (mode == 2) {
        unknown_0087aaec = 2;
    } else if (mode == 3) {
        unknown_0087aaec = 1;
    }

    if (sv_timelimit_minutes != -1) {
        if (sv_timelimit_minutes != 0) {
            unknown_0087aaf8 = sv_timelimit_minutes * 0x708;
            return;
        }
        unknown_0087aaf8 = 0;
    }
}

#if 0
Original Ghidra decompilation (0x463b90), from tools/pack.py 0x463b90:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void game_engine_apply_current_custom_variant(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  bool bVar6;

  if (DAT_00687b10 != 0) {
    DAT_00687b18 = DAT_00687b18 + 1;
    if (DAT_00687b10 <= DAT_00687b18) {
      DAT_00687b18 = 0;
    }
    iVar3 = DAT_00687b18 * 0xa4;
    _strncpy(&DAT_0087aa40,*(char **)(iVar3 + DAT_00687b0c),0x3f);
    iVar1 = DAT_0071c40c;
    bVar6 = DAT_0071c40c == 1;
    DAT_0087aa7f = 0;
    puVar4 = (undefined4 *)(iVar3 + 0xc + DAT_00687b0c);
    puVar5 = &DAT_0087aa80;
    for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    if (bVar6) {
      DAT_0087aaec = 0;
    }
    else if (iVar1 == 2) {
      DAT_0087aaec = 2;
    }
    else if (iVar1 == 3) {
      DAT_0087aaec = 1;
    }
    if (DAT_00699608 != -1) {
      if (DAT_00699608 != 0) {
        _DAT_0087aaf8 = DAT_00699608 * 0x708;
        return;
      }
      _DAT_0087aaf8 = 0;
    }
  }
  return;
}
#endif
