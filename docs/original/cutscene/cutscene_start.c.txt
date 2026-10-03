// cutscene_start  (Ghidra: cutscene_start, already named; hs cinematic_start 0x47f7f0 calls it)
// address 0x449720, size 92 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/cutscene_types_notes.md cinematic_globals +0x04/+0x08/+0x09 (seeded /
// set here, cleared by cutscene_stop), player_globals +0x11 (set here, cleared by
// cutscene_stop), ai_globals +0x10 communication_valid (cleared here, set back by
// cutscene_stop); objdump 0x449777 tail-jumps into game_engine_cleanup_stray_projectiles
// (0x467f70, already committed in src/game) instead of calling and returning.
// register convention: __cdecl, no parameters; every access is through named globals.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include "cutscene.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void sound_set_music_gain(float gain); // 0x548680
extern void game_engine_cleanup_stray_projectiles(void); // 0x467f70, this build's tail call target

extern float sound_music_gain;                      // 0x007252a8
extern float cinematic_saved_music_gain;             // 0x00686b60, -1.0 when nothing is saved
extern cinematic_globals *cinematic_globals_ptr;      // 0x006f187c
extern player_globals *local_player_globals;         // 0x0087a478
extern ai_globals *ai_globals_ptr;                    // 0x00880354
extern game_time_globals *game_time;                 // 0x006f1d6c

// Begins a cutscene: saves the current music gain (once, if nothing is already saved), forces
// music to full volume, marks the local player and ai communication state for cinematic mode,
// seeds the letterbox fade clock from the current game tick and marks it and the cinematic as
// active, then tail-calls the stray-projectile cleanup the hs cinematic layer also runs on
// start.
void cutscene_start(void)
{
    if (cinematic_saved_music_gain == -1.0f) {
        cinematic_saved_music_gain = sound_music_gain;
    }
    sound_set_music_gain(1.0f);

    local_player_globals->input_disabled = 1;
    ai_globals_ptr->dialogue_triggers_enabled = 0;

    cinematic_globals_ptr->show_letterbox = 1;
    cinematic_globals_ptr->letterbox_last_tick = game_time->game_time;
    cinematic_globals_ptr->in_progress = 1;

    game_engine_cleanup_stray_projectiles();
}

#if 0
Original Ghidra decompilation (0x449720):

void __cdecl cutscene_start(void)

{
  int iVar1;
  int iVar2;

  if (DAT_00686b60 == -1.0) {
    DAT_00686b60 = DAT_007252a8;
  }
  sound_set_music_gain(0x3f800000);
  iVar1 = DAT_00880354;
  *(undefined1 *)(DAT_0087a478 + 0x11) = 1;
  iVar2 = DAT_006f1d6c;
  *(undefined1 *)(iVar1 + 0x10) = 0;
  iVar1 = DAT_006f187c;
  *(undefined1 *)(DAT_006f187c + 8) = 1;
  *(undefined4 *)(iVar1 + 4) = *(undefined4 *)(iVar2 + 0xc);
  *(undefined1 *)(iVar1 + 9) = 1;
  FUN_00467f70();
  return;
}

objdump (the tail call Ghidra folded into a plain return):
  449777: e9 f4 e7 01 00        jmp    0x467f70
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
