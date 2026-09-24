// cutscene_stop  (Ghidra: cutscene_stop, already named; hs cinematic_stop 0x47f800 calls it)
// address 0x449eb0, size 203 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/cutscene_types_notes.md cinematic_globals +0x08/+0x09 (cleared here,
// set by cutscene_start), player_globals +0x11 (cleared here, set by cutscene_start),
// ai_globals +0x10 communication_valid (set back to 1 here, cleared by cutscene_start),
// types/render.h cinematic_screen_effect_globals (0x78 bytes, reset through 0x0071cfc4) and
// rasterizer_model_ambient_reflection_tint (0x10 bytes, zeroed through 0x0071cfc0),
// types/interface.h ui_pending_error (the single record at ui_pending_errors[0], reported via
// display_error and then cleared).
// register convention: __cdecl, no parameters; every access is through named globals.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include "rasterizer.h"
#include "render.h"
#include "networking.h"
#include "cache.h"
#include "interface.h"
#include "cutscene.h"

extern void sound_set_music_gain(float gain); // 0x548680
extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error); // 0x498f20

extern float cinematic_saved_music_gain; // 0x00686b60, -1.0 when nothing is saved
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern player_globals *local_player_globals; // 0x0087a478
extern ai_globals *ai_globals_ptr; // 0x00880354
extern cinematic_screen_effect_globals *cinematic_screen_effect_state; // 0x0071cfc4
extern ColorARGB *rasterizer_model_ambient_reflection_tint; // 0x0071cfc0
extern ui_pending_error ui_pending_errors[4]; // 0x00718fb6

// Ends a cutscene: restores the previous music gain, clears the cinematic/letterbox state,
// resets the screen-effect and model-tint output blocks, and surfaces any pending cutscene
// error for local player 0 before clearing it.
void cutscene_stop(void)
{
    cinematic_screen_effect_globals *effects;
    ColorARGB *tint;
    int32_t i;

    if (cinematic_saved_music_gain != -1.0f) {
        sound_set_music_gain(cinematic_saved_music_gain);
    }

    cinematic_globals_ptr->show_letterbox = 0;
    local_player_globals->unknown_11 = 0;
    ai_globals_ptr->communication_valid = 1;

    effects = cinematic_screen_effect_state;
    cinematic_saved_music_gain = -1.0f;
    cinematic_globals_ptr->in_progress = 0;

    if (effects != 0) {
        // zero every dword of the 0x78 byte block, then restore the identity script values
        for (i = 0; i < (int32_t)(sizeof(cinematic_screen_effect_globals) / 4); i += 1) {
            ((uint32_t *)effects)[i] = 0;
        }
        effects->script_values[0] = 1.0f;
        effects->script_values[1] = 1.0f;
        effects->script_values[2] = 1.0f;
        effects->script_values[3] = 1.0f;
    }

    tint = rasterizer_model_ambient_reflection_tint;
    if (rasterizer_model_ambient_reflection_tint != 0) {
        tint->alpha = 0.0f;
        tint->red = 0.0f;
        tint->green = 0.0f;
        tint->blue = 0.0f;
    }

    if (effects != 0) {
        effects->near_clip_distance = 0.0f;
    }

    if (-1 < ui_pending_errors[0].error_string_index && ui_pending_errors[0].error_string_index < 0x3c) {
        display_error(ui_pending_errors[0].error_string_index, 0,
            ui_pending_errors[0].modal, ui_pending_errors[0].is_error);
    }
    ui_pending_errors[0].error_string_index = (int16_t)0xffff;
}

#if 0
Original Ghidra decompilation (0x449eb0):

void __cdecl cutscene_stop(void)

{
  undefined4 *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  bool bVar6;

  if (DAT_00686b60 != -1.0) {
    sound_set_music_gain(DAT_00686b60);
  }
  iVar3 = DAT_00880354;
  iVar2 = DAT_0087a478;
  iVar4 = DAT_006f187c;
  *(undefined1 *)(DAT_006f187c + 8) = 0;
  *(undefined1 *)(iVar2 + 0x11) = 0;
  *(undefined1 *)(iVar3 + 0x10) = 1;
  puVar1 = DAT_0071cfc4;
  bVar6 = DAT_0071cfc4 != (undefined4 *)0x0;
  DAT_00686b60 = -1.0;
  *(undefined1 *)(iVar4 + 9) = 0;
  if (bVar6) {
    puVar5 = puVar1;
    for (iVar4 = 0x1e; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    puVar1[0x19] = 0x3f800000;
    puVar1[0x1a] = 0x3f800000;
    puVar1[0x1b] = 0x3f800000;
    puVar1[0x1c] = 0x3f800000;
  }
  puVar5 = DAT_0071cfc0;
  if (DAT_0071cfc0 != (undefined4 *)0x0) {
    *DAT_0071cfc0 = 0;
    puVar5[1] = 0;
    puVar5[2] = 0;
    puVar5[3] = 0;
  }
  if (puVar1 != (undefined4 *)0x0) {
    puVar1[0x1d] = 0;
  }
  if ((-1 < DAT_00718fb6) && (DAT_00718fb6 < 0x3c)) {
    display_error(DAT_00718fb6,0,DAT_00718fb8,DAT_00718fb9);
  }
  DAT_00718fb6 = 0xffff;
  return;
}
#endif
