// game_effects_update  (Ghidra: game_effects_update, already named)
// address 0x45b4f0, size 148 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: types/game.h game_time_globals::ticks_this_frame (0x10); symbols/functions.txt
//   prototype `void __cdecl game_effects_update(float delta_time)`.
// register convention: __cdecl, delta_time on the stack (Ghidra's own recognized parameter).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern game_main_globals *main_game_globals; // 0x006b0b80
extern game_time_globals *game_time;   // 0x006f1d6c
extern real chimera_contrail_scale;    // 0x007c1208, TYPES-GAP

extern void contrail_update(real delta_time);      // 0x44cb50
extern void particle_systems_update(real delta_time);                    // UNSURE module
extern void particles_update(real tick_delta_time);                 // UNSURE module
extern void widgets_update_all(real tick_delta_time);             // 0x4ffd10
extern void weather_update(void);                                  // 0x53f5c0
extern void numeric_countdown_timer_update(void);                                 // 0x540240
extern void game_sound_update(void);                                  // 0x5445c0

// Per-tick update that advances contrail, weather, timer, sound and widget systems by a
// (possibly slow-motion-scaled) delta time.
void game_effects_update(real delta_time)
{
    real scale;
    int16_t ticks_this_frame;
    real tick_delta_time;

    scale = (main_game_globals->players_are_double_speed == 0) ? 1.0f : 0.5f;
    ticks_this_frame = game_time->ticks_this_frame;
    tick_delta_time = (real)ticks_this_frame * scale * 0.033333335f;
    delta_time = scale * delta_time;

    if (ticks_this_frame != 0) {
        particles_update(tick_delta_time);
    }
    contrail_update(delta_time);
    particle_systems_update(delta_time);
    if (ticks_this_frame != 0) {
        widgets_update_all(tick_delta_time);
    }
    game_sound_update();
    weather_update();
    chimera_contrail_scale = delta_time;
    numeric_countdown_timer_update();
}

#if 0
Original Ghidra decompilation (0x45b4f0), from tools/pack.py 0x45b4f0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl game_effects_update(float delta_time)

{
  float fVar1;
  float fVar2;
  int iVar3;

  if (*(char *)(DAT_006b0b80 + 2) == '\0') {
    fVar1 = 1.0;
  }
  else {
    fVar1 = 0.5;
  }
  iVar3 = (int)*(short *)(DAT_006f1d6c + 0x10);
  fVar2 = (float)iVar3 * fVar1 * 0.033333335;
  fVar1 = fVar1 * delta_time;
  if (iVar3 != 0) {
    FUN_00455b60(fVar2);
  }
  chimera__contrail_update(fVar1);
  FUN_00454000(fVar1);
  if (iVar3 != 0) {
    widgets_update_all(fVar2);
  }
  game_sound_update();
  weather_update();
  _DAT_007c1208 = fVar1;
  game_timer_update();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
