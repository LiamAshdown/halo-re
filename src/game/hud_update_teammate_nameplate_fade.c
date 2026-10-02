// hud_update_teammate_nameplate_fade  (Ghidra: FUN_0045f220; named per
// out/phase4/game_functions.md)
// address 0x45f220, size 251 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Drives the fade-in/fade-out opacity of the teammate
// nameplate HUD element per screen/player"); types/game.h player_globals (local_players +0x04),
// game_engine_state (0x0087aa10).
// register convention: this function takes no recognized parameters; every input is a global.
// UNSURE: DAT_007c3108 (current local player index) and the per-player status byte table at
// 0x007124a4 (stride 0x28) belong to another, not-yet-attributed module (likely the HUD/widget
// layer); kept as raw externs. game_engine_unknown_aa14 in types/game.h is declared int32_t, but
// this function reads/writes it as a float fade value indexed by local player -- since
// k_maximum_local_players == 1 the indexing is a no-op either way, but the type in the header
// looks wrong for this use; declared here under a different name at the same address rather than
// silently reinterpreting the header's own extern.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t current_local_player_index;           // 0x007c3108, UNSURE owning module
extern uint8_t local_player_hud_status_table[];       // 0x007124a4, stride 0x28, UNSURE meaning
extern float game_engine_nameplate_fade_opacity_array[]; // 0x0087aa14, see header note above

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;            // 0x0087a480
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value; // 0x0087aa10, renamed to not collide with the
                                                   // game_engine_state enum tag in types/game.h

extern void hud_draw_teammate_nameplate(datum_index player_handle); // 0x45e520, this batch
extern void game_engine_rasterize_in_game_score(datum_index subject_player, float opacity); // 0x465690, blam-cc: stack -> subject_player, opacity
extern double pow(double base, double exponent); // C runtime (the retail copy is the CRT _CIpow at 0x6283c0)

// Fades the teammate-nameplate HUD element in or out for the current local player, then, once
// it is at least partly visible, draws the in-game scoreboard line for that player at the
// resulting opacity.
void hud_update_teammate_nameplate_fade(void)
{
    int16_t local_player = current_local_player_index;
    datum_index player_handle;
    float opacity;

    if (local_player == -1 || 0 < local_player) {
        player_handle = (datum_index)0xffffffff;
    } else {
        player_handle = ((datum_index *)((uint8_t *)local_player_globals + 4))[local_player];
    }

    if (current_game_engine != 0 &&
        (player_handle & 0xffff) * sizeof(player) + (uint32_t)player_data->data != 0) {
        hud_draw_teammate_nameplate(player_handle);
    }

    if (local_player_hud_status_table[local_player * 0x28] == 0 && game_engine_state_value != 1) {
        opacity = game_engine_nameplate_fade_opacity_array[local_player] - 0.06666667f;
    } else {
        opacity = game_engine_nameplate_fade_opacity_array[local_player] + 0.06666667f;
    }

    if (opacity < 0.0f) {
        game_engine_nameplate_fade_opacity_array[local_player] = 0.0f;
        return;
    }

    if (opacity <= 1.0f) {
        if (opacity <= 0.0f) {
            game_engine_nameplate_fade_opacity_array[local_player] = opacity;
            return;
        }
    } else {
        opacity = 1.0f;
    }

    // FIXED 2026-09-28: 0x45f2f0..0x45f304 passes (player, pow(opacity, 1.9)) -- two stack arguments.
    game_engine_rasterize_in_game_score(player_handle, (float)pow((double)opacity, (double)1.9f));
    game_engine_nameplate_fade_opacity_array[local_player] = opacity;
}

#if 0
Original Ghidra decompilation (0x45f220), from tools/pack.py 0x45f220:

void FUN_0045f220(void)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  float10 fVar4;
  float local_4;

  sVar1 = DAT_007c3108;
  iVar2 = (int)DAT_007c3108;
  if ((DAT_007c3108 == -1) || (0 < DAT_007c3108)) {
    uVar3 = 0xffffffff;
  }
  else {
    uVar3 = *(uint *)(DAT_0087a478 + 4 + DAT_007c3108 * 4);
  }
  if ((DAT_006f1d20 != 0) && ((uVar3 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) != 0)) {
    FUN_0045e520();
  }
  if (((&DAT_007124a4)[sVar1 * 0x28] == '\0') && (DAT_0087aa10 != 1)) {
    local_4 = *(float *)(&DAT_0087aa14 + iVar2 * 4) - 0.06666667;
  }
  else {
    local_4 = *(float *)(&DAT_0087aa14 + iVar2 * 4) + 0.06666667;
  }
  if (local_4 < 0.0) {
    *(undefined4 *)(&DAT_0087aa14 + iVar2 * 4) = 0;
    return;
  }
  if (local_4 <= 1.0) {
    if (local_4 <= 0.0) goto LAB_0045f30c;
  }
  else {
    local_4 = 1.0;
  }
  fVar4 = (float10)FUN_006283c0();
  game_engine_rasterize_in_game_score(uVar3,(float)fVar4);
LAB_0045f30c:
  *(float *)(&DAT_0087aa14 + iVar2 * 4) = local_4;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
